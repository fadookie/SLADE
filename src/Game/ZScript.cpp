
#include "Main.h"
#include "ZScript.h"
#include "Archive/Archive.h"
#include "Utility/StringUtils.h"

using namespace ZScript;

void ZScript::logUnexpectedToken(Tokenizer& tz, string parsing, string expected, string got)
{
	LOG_MESSAGE(
		1,
		"Error parsing %s: Expected %s, got \"%s\" at line %d",
		parsing,
		expected,
		(got.empty()) ? tz.currentToken() : got,
		tz.lineNo()
	);
}


bool Enumerator::parse(Tokenizer& tz)
{
	// Name first
	string token = tz.getToken();
	if (tz.isSpecialCharacter(token[0]))
	{
		logUnexpectedToken(tz, "enum", "valid enumerator name", token);
		return false;
	}
	name = token;

	// Opening brace
	tz.getToken(&token);
	if (token != "{")
	{
		logUnexpectedToken(tz, "enum", "\"{\"", token);
		return false;
	}

	// Parse values
	Value val;
	long tmp;
	tz.getToken(&token);
	while (token != "}")
	{
		val.name = token;

		// Don't bother parsing values yet, needs a proper expression parser for stuff like eg.
		// enum { VALUE1 = 2, VALUE2 = VALUE1+1 }
		/*
		tz.getToken(&token);
		if (token == ",")
		{
			val.value++;
			values.push_back({val.name, val.value});
		}
		else if (token == "=")
		{
			tz.getToken(&token);
			if (StringUtils::isInteger(token))
			{
				if (token.ToLong(&tmp))
					val.value = tmp;
				else
				{
					// Unable to parse value, just increment it
					val.value++;
					LOG_MESSAGE(
						1,
						"Warning: Unable to parse value for enum member \"%s\", "
						"later enum values will likely be wrong",
						val.name
					);
				}
			}
			else // Not valid integer
			{
				// Check previous values
				bool is_prev = false;
				for (auto& pval : values)
					if (val.name.CmpNoCase(pval.name) == 0)
					{
						is_prev = true;
						val.value = pval.value;
					}

				if (!is_prev)
				{
					LOG_MESSAGE(
						1,
						"Error parsing enum: Expected valid integer or previous enum member, "
						"got \"%s\" at line %d",
						token,
						tz.lineNo()
					);
					return false;
				}
			}

			values.push_back({val.name, val.value});

			// Next token, skip ',' if found
			tz.getToken(token);
			if (token == ",")
				tz.getToken(token);
		}*/
		tz.getToken(&token);
		while (token != "," && token != "}")
			tz.getToken(&token);

		if (token == ",")
			tz.getToken(&token);

		values.push_back({val.name, 0});
		LOG_MESSAGE(3, "Parsed enum value %s", val.name);
	}

	LOG_MESSAGE(2, "Parsed enum %s successfully", name);
	return true;
}


bool Class::parse(Tokenizer& tz)
{
	// Name first
	string token = tz.getToken();
	if (tz.isSpecialCharacter(token[0]) || token.empty())
	{
		string ctype = (type == Type::Class) ? "class" : "struct";
		logUnexpectedToken(tz, ctype, S_FMT("%s name", ctype), token);
		return false;
	}
	name = token;

	// Read tokens until {
	tz.getToken(&token);
	while (token != "{")
	{
		token.MakeLower();

		// ':' (inheritance)
		if (token == ":")
		{
			inherits_class = tz.getToken();
			LOG_MESSAGE(2, "Inherits %s", inherits_class);
		}

		// Native class/struct
		else if (token == "native")
			native = true;

		tz.getToken(&token);
	}

	// Skip {
	tz.getToken(&token);

	// Parse
	while (token != "}")
	{
		token.MakeLower();

		// ';' (skip)
		if (token == ";")
		{
			tz.getToken(&token);
			continue;
		}

		// Enum
		else if (token == "enum")
		{
			Enumerator e;
			if (!e.parse(tz))
				return false;

			enumerators.push_back(e);
		}

		// Default block
		else if (token == "default")
		{
			// Check opening brace
			if (!tz.checkToken("{"))
			{
				logUnexpectedToken(tz, "class", "{");
				return false;
			}

			// Begin parsing
			tz.getToken(&token);
			vector<string> expression;
			while (token != "}")
			{
				// Flag
				if (token.StartsWith("+") || token.StartsWith("-"))
					default_properties.addFlag(token);
				
				// Property
				else
				{
					// Parse expression
					// For now ignore anything after the first whitespace/special character
					// so stuff like arithmetic expressions or comma separated lists won't
					// really work properly yet
					expression.clear();
					tz.getTokensUntil(expression, ";");
					if (expression.empty())
						default_properties.addFlag(token);
					else
						default_properties[token] = expression[0];
				}

				// Next property
				tz.getToken(&token);
			}
		}

		// States
		else if (token == "states")
		{
			// TODO
			tz.skipToken();
			tz.skipSection("{", "}");
		}

		// Unknown block (skip it)
		else if (token == "{")
		{
			LOG_MESSAGE(2, "Skip section at line %d", tz.lineNo());
			tz.skipSection("{", "}");
		}

		// Something else (variable or function)
		else
		{
			// Skip until ; or {} block
			while (!tz.atEnd())
			{
				tz.getToken(&token);
				if (token == ";")
					break;
				else if (token == "{")
				{
					// Skip block
					tz.skipSection("{", "}");
					break;
				}
			}
		}

		// Next token
		tz.getToken(&token);
	}

	LOG_MESSAGE(2, "Parsed class/struct %s successfully", name);
	LOG_MESSAGE(2, "Default properties:");
	LOG_MESSAGE(2, "%s", default_properties.toString());

	return true;
}


bool Definitions::parseZScript(ArchiveEntry* entry)
{
	Tokenizer tz;
	tz.openMem(&entry->getMCData(), "ZScript");

	LOG_MESSAGE(2, "Parsing ZScript entry \"%s\"", entry->getPath(true));

	string token;
	int unknown_line = -1;
	vector<string> unknown_tokens;
	while (!tz.atEnd())
	{
		tz.getToken(&token);
		token.MakeLower();

		// #include
		if (token == "#include")
		{
			ArchiveEntry* include = entry->getParent()->entryAtPath(tz.getToken());
			if (include)
			{
				if (!parseZScript(include))
					return false;
			}
			else
			{
				LOG_MESSAGE(
					1,
					"ZScript Warning: Unable to find #included entry \"%s\" at line %d, ignoring",
					tz.currentToken(),
					tz.lineNo()
				);
			}
		}

		// ; (skip)
		else if (token == ";")
			continue;

		else if (token == "struct")
		{
			Class ns(Class::Type::Struct);

			if (!ns.parse(tz))
				return false;

			classes.push_back(ns);
		}

		else if (token == "class")
		{
			Class nc(Class::Type::Class);

			if (!nc.parse(tz))
				return false;

			classes.push_back(nc);
		}

		else if (token == "enum")
		{
			Enumerator e;
			if (!e.parse(tz))
				return false;

			enumerators.push_back(e);
		}

		else if (token == "const")
		{
			tz.getToken(&token);
			variables.push_back({ token });

			LOG_MESSAGE(2, "%d: Add const variable \"%s\"", tz.lineNo(), token);

			while (token != ";")
				tz.getToken(&token);			
		}

		else if (!tz.atEnd())
		{
			if (unknown_line < 0)
				unknown_line = tz.lineNo();

			unknown_tokens.push_back(token);

			continue;
			/*LOG_MESSAGE(
				1,
				"ZScript: Unexpected token \"%s\" at line %d",
				token,
				tz.lineNo()
			);*/
			//return false;
		}

		if (!unknown_tokens.empty())
		{
			/*string message = S_FMT("ZScript: Unexpected tokens starting line %d: ", unknown_line);
			for (auto s : unknown_tokens)
				message += S_FMT("%s ", s);
			LOG_MESSAGE(1, message);*/
			unknown_tokens.clear();
			unknown_line = -1;
		}
	}

	return true;
}



// TEMP TESTING CONSOLE COMMANDS
#include "MainEditor/MainWindow.h"
#include "General/Console/Console.h"

CONSOLE_COMMAND(test_parse_zscript, 0, false)
{
	ArchiveEntry* entry = nullptr;
	if (!args.empty())
		entry = theMainWindow->getCurrentArchive()->entryAtPath(args[0]);
	else
		entry = theMainWindow->getCurrentEntry();

	if (entry)
	{
		Definitions test;
		if (test.parseZScript(entry))
			theConsole->logMessage("Parsed Successfully");
		else
			theConsole->logMessage("Parsing failed");
	}
	else
		theConsole->logMessage("Select an entry or enter a valid entry name/path");
}

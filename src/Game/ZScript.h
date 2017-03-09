#pragma once

#include "Utility/Tokenizer.h"
#include "Utility/PropertyList/PropertyList.h"

class ArchiveEntry;

namespace ZScript
{
	// Helpers
	void	logUnexpectedToken(Tokenizer& tz, string parsing, string expected, string got = "");

	class Enumerator
	{
	public:
		Enumerator(string name = "") : name{name} {}

		struct Value
		{
			string	name;
			int		value;
			//Value() : name{name}, value{0} {}
		};

		bool parse(Tokenizer& tz);
		
	private:
		string			name;
		vector<Value>	values;
	};

	class Identifier	// rename this
	{
	public:
		Identifier(string name = "") : name{name}, native{false}, deprecated{false} {}
		virtual ~Identifier() {}

		string	getName() { return name; }
		bool	isNative() { return native; }
		bool	isDeprecated() { return deprecated; }

	protected:
		string	name;
		bool	native;
		bool	deprecated;
	};

	class Variable : public Identifier
	{
	public:
		Variable(string name = "") : Identifier(name), type{"<unknown>"} {}
		virtual ~Variable() {}

	private:
		string	type;
	};

	class Function : public Identifier
	{
	public:
		Function(string name = "") : Identifier(name), return_type{"void"} {}
		virtual ~Function() {}

		struct Parameter
		{
			string name;
			string type;
			string default_value;
			Parameter() : name{"<unknown>"}, type{"<unknown>"}, default_value{""} {}
		};

	private:
		vector<Parameter>	parameters;
		string				return_type;
		bool				is_virtual;
		bool				is_static;
	};

	class Class : public Identifier
	{
	public:
		enum class Type
		{
			Class,
			Struct
		};

		Class(Type type, string name = "") : Identifier{name}, type{type} {}
		virtual ~Class() {}

		bool parse(Tokenizer& tz);

	private:
		Type				type;
		string				inherits_class;
		vector<Variable>	variables;
		vector<Function>	functions;
		vector<Enumerator>	enumerators;
		PropertyList		default_properties;
	};

	class Definitions	// rename this also
	{
	public:
		Definitions() {}
		~Definitions() {}

		bool	parseZScript(ArchiveEntry* entry);

	private:
		vector<Class>		classes;
		vector<Enumerator>	enumerators;
		vector<Variable>	variables;
		vector<Function>	functions;	// needed? dunno if global functions are a thing
	};
}


/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ActionSpecial.cpp
 * Description: ActionSpecial class, represents an action special
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "ActionSpecial.h"
#include "Utility/Parser.h"
#include "GameConfiguration.h"


/*******************************************************************
 * ACTIONSPECIAL CLASS FUNCTIONS
 *******************************************************************/

/* ActionSpecial::ActionSpecial
 * ActionSpecial class constructor
 *******************************************************************/
ActionSpecial::ActionSpecial(string name, string group)
{
	// Init variables
	this->name = name;
	this->group = group;
	this->tagged = 0;

	// Init args
	args.count = 0;
	args[0].name = "Arg1";
	args[1].name = "Arg2";
	args[2].name = "Arg3";
	args[3].name = "Arg4";
	args[4].name = "Arg5";
}

/* ActionSpecial::copy
 * Copies another ActionSpecial
 *******************************************************************/
void ActionSpecial::copy(ActionSpecial* copy)
{
	// Check AS to copy was given
	if (!copy) return;

	// Copy properties
	this->name = copy->name;
	this->group = copy->group;
	this->tagged = copy->tagged;
	this->args = copy->args;
}

/* ActionSpecial::reset
 * Resets all values to defaults
 *******************************************************************/
void ActionSpecial::reset()
{
	// Reset variables
	name = "Unknown";
	group = "";
	tagged = 0;

	// Reset args
	for (unsigned a = 0; a < 5; a++)
	{
		args[a].name = S_FMT("Arg%d", a+1);
		args[a].type = Arg::Type::Number;
		args[a].custom_flags.clear();
		args[a].custom_values.clear();
	}
}

/* ActionSpecial::parse
 * Reads an action special definition from a parsed tree [node]
 *******************************************************************/
void ActionSpecial::parse(ParseTreeNode* node, Arg::SpecialMap* shared_args)
{
	// Check for simple definition
	if (node->isLeaf())
	{
		name = node->getStringValue();
		return;
	}

	// Go through all child nodes/values
	ParseTreeNode* child = nullptr;
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		child = (ParseTreeNode*)node->getChild(a);
		string name = child->getName();
		int argn = -1;

		// Name
		if (S_CMPNOCASE(name, "name"))
			this->name = child->getStringValue();

		// Args
		else if (S_CMPNOCASE(name, "arg1"))
			argn = 0;
		else if (S_CMPNOCASE(name, "arg2"))
			argn = 1;
		else if (S_CMPNOCASE(name, "arg3"))
			argn = 2;
		else if (S_CMPNOCASE(name, "arg4"))
			argn = 3;
		else if (S_CMPNOCASE(name, "arg5"))
			argn = 4;

		// Tagged
		else if (S_CMPNOCASE(name, "tagged"))
			this->tagged = GameConfiguration::parseTagged(child);

		// Parse arg definition if it was one
		if (argn >= 0)
		{
			// Update arg count
			if (argn + 1 > args.count)
				args.count = argn + 1;

			args[argn].parse(child, shared_args);
		}
	}
}

/* ActionSpecial::getArgsString
 * Returns a string representation of the action special's args
 * given the values in [args]
 *******************************************************************/
string ActionSpecial::getArgsString(int args[5], string argstr[2])
{
	string ret;

	// Add each arg to the string
	for (unsigned a = 0; a < 5; a++)
	{
		// Skip if the arg name is undefined and the arg value is 0
		if (args[a] == 0 && this->args[a].name.StartsWith("Arg"))
			continue;

		ret += this->args[a].name;
		ret += ": ";
		if (a < 2 && args[a] == 0 && !argstr[a].IsEmpty())
			ret += argstr[a];
		else
			ret += this->args[a].valueString(args[a]);
		ret += ", ";
	}

	// Cut ending ", "
	if (!ret.IsEmpty())
		ret.RemoveLast(2);

	return ret;
}

/* ActionSpecial::stringDesc
 * Returns the action special info as a string
 *******************************************************************/
string ActionSpecial::stringDesc()
{
	// Init string
	string ret = S_FMT("\"%s\" in group \"%s\"", name, group);

	// Add tagged info
	if (tagged)
		ret += " (tagged)";
	else
		ret += " (not tagged)";

	// Add args
	ret += "\nArgs: ";
	for (unsigned a = 0; a < 5; a++)
	{
		ret += args[a].name + ": ";

		if (args[a].type == Arg::Type::Number)
			ret += "Number";
		else if (args[a].type == Arg::Type::YesNo)
			ret += "Yes/No";
		else if (args[a].type == Arg::Type::NoYes)
			ret += "No/Yes";
		else if (args[a].type == Arg::Type::Angle)
			ret += "Angle";
		else if (args[a].type == Arg::Type::Choice)
			ret += "Choice";
		else
			ret += "Unknown Type";

		ret += ", ";
	}

	return ret;
}

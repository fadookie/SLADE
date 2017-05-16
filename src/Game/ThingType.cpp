
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ThingType.cpp
// Description: ThingType class, represents a thing type
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "ThingType.h"
#include "Utility/Parser.h"
#include "Game/Configuration.h"

using namespace Game;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
ThingType ThingType::unknown_;


// ----------------------------------------------------------------------------
//
// ThingType Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingType::ThingType
//
// ThingType class constructor
// ----------------------------------------------------------------------------
ThingType::ThingType(const string& name, const string& group) :
	name_{ name },
	group_{ group },
	colour_{ 170, 170, 180, 255, 0 },
	radius_{ 20 },
	height_{ -1 },
	angled_{ true },
	hanging_{ false },
	shrink_{ false },
	fullbright_{ false },
	decoration_{ false },
	zeth_icon_{ -1 },
	decorate_{ false },
	solid_{ false },
	next_type_{ 0 },
	next_args_{ 0 },
	flags_{ 0 },
	tagged_{ TagType::None },
	number_{ -1 }
{
	// Init args
	args_.count = 0;
	args_[0].name = "Arg1";
	args_[1].name = "Arg2";
	args_[2].name = "Arg3";
	args_[3].name = "Arg4";
	args_[4].name = "Arg5";
}

// ----------------------------------------------------------------------------
// ThingType::copy
//
// Copies all properties from [copy]
// (excludes definition variables like name, number, etc.)
// ----------------------------------------------------------------------------
void ThingType::copy(const ThingType& copy)
{
	angled_ = copy.angled_;
	hanging_ = copy.hanging_;
	shrink_ = copy.shrink_;
	colour_ = copy.colour_;
	radius_ = copy.radius_;
	height_ = copy.height_;
	scale_ = copy.scale_;
	fullbright_ = copy.fullbright_;
	decoration_ = copy.decoration_;
	decorate_ = copy.decorate_;
	solid_ = copy.solid_;
	zeth_icon_ = copy.zeth_icon_;
	next_type_ = copy.next_type_;
	next_args_ = copy.next_args_;
	flags_ = copy.flags_;
	tagged_ = copy.tagged_;
	args_ = copy.args_;
}

// ----------------------------------------------------------------------------
// ThingType::define
//
// Defines this thing type's [number], [name] and [group]
// ----------------------------------------------------------------------------
void ThingType::define(int number, const string& name, const string& group)
{
	number_ = number;
	name_ = name;
	group_ = group;
}

// ----------------------------------------------------------------------------
// ThingType::reset
//
// Resets all values to defaults
// ----------------------------------------------------------------------------
void ThingType::reset()
{
	// Reset variables
	name_ = "Unknown";
	group_ = "";
	sprite_ = "";
	icon_ = "";
	translation_ = "";
	palette_ = "";
	angled_ = true;
	hanging_ = false;
	shrink_ = false;
	colour_ = COL_WHITE;
	radius_ = 20;
	height_ = -1;
	scale_ = { 1.0, 1.0 };
	fullbright_ = false;
	decoration_ = false;
	solid_ = false;
	zeth_icon_ = -1;
	next_type_ = 0;
	next_args_ = 0;
	flags_ = 0;
	tagged_ = TagType::None;

	// Reset args
	args_.count = 0;
	for (unsigned a = 0; a < 5; a++)
	{
		args_[a].name = S_FMT("Arg%d", a+1);
		args_[a].type = Arg::Type::Number;
		args_[a].custom_flags.clear();
		args_[a].custom_values.clear();
	}
}

// ----------------------------------------------------------------------------
// ThingType::parse
//
// Reads an thing type definition from a parsed tree [node]
// ----------------------------------------------------------------------------
void ThingType::parse(ParseTreeNode* node)
{
	// Go through all child nodes/values
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		string name = child->getName();
		int arg = -1;

		// Name
		if (S_CMPNOCASE(name, "name"))
			name_ = child->getStringValue();

		// Args
		else if (S_CMPNOCASE(name, "arg1"))
			arg = 0;
		else if (S_CMPNOCASE(name, "arg2"))
			arg = 1;
		else if (S_CMPNOCASE(name, "arg3"))
			arg = 2;
		else if (S_CMPNOCASE(name, "arg4"))
			arg = 3;
		else if (S_CMPNOCASE(name, "arg5"))
			arg = 4;

		// Sprite
		else if (S_CMPNOCASE(name, "sprite"))
			sprite_ = child->getStringValue();

		// Icon
		else if (S_CMPNOCASE(name, "icon"))
			icon_ = child->getStringValue();

		// Radius
		else if (S_CMPNOCASE(name, "radius"))
			radius_ = child->getIntValue();

		// Height
		else if (S_CMPNOCASE(name, "height"))
			height_ = child->getIntValue();

		// Scale
		else if (S_CMPNOCASE(name, "scale"))
		{
			float s = child->getFloatValue();
			scale_ = { s, s };
		}

		// ScaleX
		else if (S_CMPNOCASE(name, "scalex"))
			scale_.x = child->getFloatValue();

		// ScaleY
		else if (S_CMPNOCASE(name, "scaley"))
			scale_.y = child->getFloatValue();

		// Colour
		else if (S_CMPNOCASE(name, "colour"))
			colour_.set(child->getIntValue(0), child->getIntValue(1), child->getIntValue(2));

		// Show angle
		else if (S_CMPNOCASE(name, "angle"))
			angled_ = child->getBoolValue();

		// Hanging object
		else if (S_CMPNOCASE(name, "hanging"))
			hanging_ = child->getBoolValue();

		// Shrink on zoom
		else if (S_CMPNOCASE(name, "shrink"))
			shrink_ = child->getBoolValue();

		// Fullbright
		else if (S_CMPNOCASE(name, "fullbright"))
			fullbright_ = child->getBoolValue();

		// Decoration
		else if (S_CMPNOCASE(name, "decoration"))
			decoration_ = child->getBoolValue();

		// Solid
		else if (S_CMPNOCASE(name, "solid"))
			solid_ = child->getBoolValue();

		// Translation
		else if (S_CMPNOCASE(name, "translation"))
		{
			translation_ += "\"";
			size_t v = 0;
			do
			{
				translation_ += child->getStringValue(v++);
			}
			while ((v < child->nValues()) && ((translation_ += "\", \""), true));
			translation_ += "\"";
		}

		// Palette override
		else if (S_CMPNOCASE(name, "palette"))
			palette_ = child->getStringValue();

		// Zeth icon
		else if (S_CMPNOCASE(name, "zeth"))
			zeth_icon_ = child->getIntValue();

		// Pathed things stuff
		else if (S_CMPNOCASE(name, "nexttype"))
		{
			next_type_ = child->getIntValue();
			flags_ |= FLAG_PATHED;
		}
		else if (S_CMPNOCASE(name, "nextargs"))
		{
			next_args_ = child->getIntValue();
			flags_ |= FLAG_PATHED;
		}

		// Handle player starts
		else if (S_CMPNOCASE(name, "player_coop"))
			flags_ |= FLAG_COOPSTART;
		else if (S_CMPNOCASE(name, "player_dm"))
			flags_ |= FLAG_DMSTART;
		else if (S_CMPNOCASE(name, "player_team"))
			flags_ |= FLAG_TEAMSTART;

		// Hexen's critters are weird
		else if (S_CMPNOCASE(name, "dragon"))
			flags_ |= FLAG_DRAGON;
		else if (S_CMPNOCASE(name, "script"))
			flags_ |= FLAG_SCRIPT;

		// Some things tag other things directly
		else if (S_CMPNOCASE(name, "tagged"))
			tagged_ = Game::parseTagged(child);

		// Parse arg definition if it was one
		if (arg >= 0)
		{
			// Update arg count
			if (arg + 1 > args_.count)
				args_.count = arg + 1;

			// Check for simple definition
			if (child->isLeaf())
			{
				// Set name
				args_[arg].name = child->getStringValue();

				// Set description (if specified)
				if (child->nValues() > 1) args_[arg].desc = child->getStringValue(1);
			}
			else
			{
				// Extended arg definition

				// Name
				auto val = child->getChildPTN("name");
				if (val) args_[arg].name = val->getStringValue();

				// Description
				val = child->getChildPTN("desc");
				if (val) args_[arg].desc = val->getStringValue();

				// Type
				val = child->getChildPTN("type");
				string atype;
				if (val) atype = val->getStringValue();
				if (S_CMPNOCASE(atype, "yesno"))
					args_[arg].type = Arg::Type::YesNo;
				else if (S_CMPNOCASE(atype, "noyes"))
					args_[arg].type = Arg::Type::NoYes;
				else if (S_CMPNOCASE(atype, "angle"))
					args_[arg].type = Arg::Type::Angle;
				else
					args_[arg].type = Arg::Type::Number;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// ThingType::stringDesc
//
// Returns the thing type info as a string
// ----------------------------------------------------------------------------
string ThingType::stringDesc() const
{
	// Init return string
	string ret = S_FMT("\"%s\" in group \"%s\", colour %d,%d,%d, radius %d", name_, group_, colour_.r, colour_.g, colour_.b, radius_);

	// Add any extra info
	if (!sprite_.IsEmpty()) ret += S_FMT(", sprite \"%s\"", sprite_);
	if (!angled_) ret += ", angle hidden";
	if (hanging_) ret += ", hanging";
	if (fullbright_) ret += ", fullbright";
	if (decoration_) ret += ", decoration";
	if (decorate_) ret += ", defined in DECORATE";

	return ret;
}


// ----------------------------------------------------------------------------
//
// ThingType Class Static Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ThingType::initGlobal
//
// Initialises global (static) ThingType objects
// ----------------------------------------------------------------------------
void ThingType::initGlobal()
{
	unknown_.shrink_ = true;
	unknown_.icon_ = "unknown";
}

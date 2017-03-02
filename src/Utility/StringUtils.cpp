
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    StringUtils.cpp
 * Description: Various string utility functions
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
#include "StringUtils.h"
#include <wx/regex.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace StringUtils
{
	wxRegEx re_int1{ "^[+-]?[0-9]+[0-9]*$", wxRE_DEFAULT | wxRE_NOSUB };
	wxRegEx re_int2{ "^0[0-9]+$", wxRE_DEFAULT | wxRE_NOSUB };
	wxRegEx re_int3{ "^0x[0-9A-Fa-f]+$", wxRE_DEFAULT | wxRE_NOSUB };
	wxRegEx re_float{ "^[-+]?[0-9]*.?[0-9]+([eE][-+]?[0-9]+)?$", wxRE_DEFAULT | wxRE_NOSUB };
}


/*******************************************************************
 * STRINGUTILS NAMESPACE FUNCTIONS
 *******************************************************************/

/* StringUtils::escapedString
 * Returns a copy of [str] with escaped double quotes and
 * backslashes. If [swap_backslash] is true, instead of escaping it
 * will swap backslashes to forward slashes
 *******************************************************************/
string StringUtils::escapedString(const string& str, bool swap_backslash)
{
	string escaped = str;

	escaped.Replace(SLASH_BACK, swap_backslash ? SLASH_FORWARD : ESCAPED_SLASH_BACK);
	escaped.Replace(QUOTE_DOUBLE, ESCAPED_QUOTE_DOUBLE);

	return escaped;
}

/* StringUtils::isInteger
 * Returns true if [str] is a valid integer. If [allow_hex] is true,
 * can also be a valid hex string
 *******************************************************************/
bool StringUtils::isInteger(const string& str, bool allow_hex)
{
	return (re_int1.Matches(str) || re_int2.Matches(str) || (allow_hex && re_int3.Matches(str)));
}

/* StringUtils::isHex
 * Returns true if [str] is a valid hex string
 *******************************************************************/
bool StringUtils::isHex(const string& str)
{
	return re_int3.Matches(str);
}

/* StringUtils::isFloat
 * Returns true if [str] is a valid floating-point number
 *******************************************************************/
bool StringUtils::isFloat(const string& str)
{
	return (re_float.Matches(str));
}

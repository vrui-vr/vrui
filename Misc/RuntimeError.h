/***********************************************************************
RuntimeError - Child class of std::runtime_error for errors that have a
location string derived from __PRETTY_FUNCTION__ in addition to the
usual error message.
Copyright (c) 2023 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_RUNTIMEERROR_INCLUDED
#define MISC_RUNTIMEERROR_INCLUDED

#include <stddef.h>
#include <string>
#include <stdexcept>

namespace Misc {

class RuntimeError:public std::runtime_error
	{
	/* Elements: */
	private:
	size_t locationLength; // Length of the location prefix excluding the ": " separator
	
	/* Constructors and destructors: */
	public:
	RuntimeError(const char* what,size_t sLocationLength) // Creates a RuntimeError from the given combined location+error message and location length
		:std::runtime_error(what),
		 locationLength(sLocationLength)
		{
		}
	
	/* Methods: */
	size_t getLocationLength(void) const // Returns the length of the location prefix
		{
		return locationLength;
		}
	const char* getWhatBegin(void) const // Returns a pointer to the first character of the error message without the location prefix
		{
		return what()+(locationLength+2); // Skip the ": " separator
		}
	std::string getLocation(void) const // Returns the location prefix
		{
		return std::string(what(),what()+locationLength);
		}
	std::string getWhat(void) const // Returns the error message
		{
		return std::string(what()+(locationLength+2));
		}
	};

/* Namespace-global functions: */

RuntimeError runtimeError(const char* prettyFunction,const char* whatFormat,...); // Returns a RuntimeError object that can be thrown; first parameter is expected to be __PRETTY_FUNCTION__, second parameter is printf-style format string, rest are printf parameters

}

#endif

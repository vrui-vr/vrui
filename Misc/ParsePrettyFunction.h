/***********************************************************************
ParsePrettyFunction - Helper functions that turn the output of the
__PRETTY_FUNCTION__ macro into more concise locations that can be
printed as part of error messages.
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

#ifndef MISC_PARSEPRETTYFUNCTION_INCLUDED
#define MISC_PARSEPRETTYFUNCTION_INCLUDED

#include <string>

namespace Misc {

char* parsePrettyFunction(const char* prettyFunction,char* buffer,char* bufferEnd); // Writes a more concise version of the parameter, which is assumed to be an evaluation of the __PRETTY_FUNCTION__ macro, into the given buffer and returns a pointer to the end of the output
std::string parsePrettyFunction(const char* prettyFunction); // Returns a std::string containing a more concise version of the parameter, which is assumed to be an evaluation of the __PRETTY_FUNCTION__ macro

}

#endif

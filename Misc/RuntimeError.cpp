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

#include <Misc/RuntimeError.h>

#include <stdarg.h>
#include <stdio.h>
#include <Misc/ParsePrettyFunction.h>

namespace Misc {

/**************************
Namespace-global functions:
**************************/

RuntimeError runtimeError(const char* prettyFunction,const char* whatFormat,...)
	{
	/* Put everything into an on-stack buffer: */
	char buffer[1024]; // Hopefully long enough...
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Shorten the __PRETTY_FUNCTION__ string: */
	char* bufferPtr=parsePrettyFunction(prettyFunction,buffer,bufferEnd-3); // Leave room for ": " separator and NUL terminator
	size_t locationLength=bufferPtr-buffer;
	
	/* Append the location separator: */
	*(bufferPtr++)=':';
	*(bufferPtr++)=' ';
	
	/* Append the error message using printf: */
	va_list ap;
	va_start(ap,whatFormat);
	vsnprintf(bufferPtr,bufferEnd-bufferPtr,whatFormat,ap);
	va_end(ap);
	
	return RuntimeError(buffer,locationLength);
	}

}

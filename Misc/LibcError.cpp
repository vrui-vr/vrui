/***********************************************************************
LibcError - Child class of Misc::RuntimeError for errors reported by the
C library.
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

#include <Misc/LibcError.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <Misc/ParsePrettyFunction.h>

namespace Misc {

/**************************
Namespace-global functions:
**************************/

LibcError libcError(const char* prettyFunction,int error,const char* whatFormat,...)
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
	bufferPtr+=vsnprintf(bufferPtr,bufferEnd-bufferPtr,whatFormat,ap);
	va_end(ap);
	
	/* Append the libc error indicator if there is room in the buffer: */
	if(bufferPtr<bufferEnd)
		{
		/* Convert the error number into a string: */
		char strerrorBuffer[1024]; // Long enough according to spec
		char* errorString=strerror_r(error,strerrorBuffer,sizeof(strerrorBuffer));
		
		/* Append the libc error suffix using printf: */
		snprintf(bufferPtr,bufferEnd-bufferPtr," due to libc error %d (%s)",error,errorString);
		}
	
	return LibcError(buffer,locationLength,error);
	}

}

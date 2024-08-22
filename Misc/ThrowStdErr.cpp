/***********************************************************************
ThrowStdErr - Helper function to create std::runtime_error error
descriptions using the printf() interface. The function obviously never
returns...
Copyright (c) 2005-2023 Oliver Kreylos

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

#include <Misc/ThrowStdErr.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
#include <Misc/ParsePrettyFunction.h>

namespace Misc {

void throwStdErr(const char* formatString,...)
	{
	char msg[1024]; // Buffer for error messages - hopefully long enough...
	va_list ap;
	va_start(ap,formatString);
	vsnprintf(msg,sizeof(msg),formatString,ap);
	va_end(ap);
	throw std::runtime_error(msg);
	}

void stdErr(const char* prettyFunction,const char* formatString,...)
	{
	char msg[1024]; // Buffer for error messages - hopefully long enough...
	char* msgEnd=msg+sizeof(msg);
	
	/* Copy the source function name into the message buffer: */
	char* msgStart=parsePrettyFunction(prettyFunction,msg,msgEnd-3); // Leave room for ": " separator and NUL terminator
	
	/* Append the location separator: */
	*(msgStart++)=':';
	*(msgStart++)=' ';
	
	/* Print the given message into the rest of the message buffer: */
	va_list ap;
	va_start(ap,formatString);
	vsnprintf(msgStart,msgEnd-msgStart,formatString,ap);
	va_end(ap);
	
	/* Throw the exception: */
	throw std::runtime_error(msg);
	}

void libcErr(const char* prettyFunction,int libcError,const char* formatString,...)
	{
	char msg[1024]; // Buffer for error messages - hopefully long enough...
	char* msgEnd=msg+sizeof(msg);
	
	/* Copy the source function name into the message buffer: */
	char* msgStart=parsePrettyFunction(prettyFunction,msg,msgEnd-3); // Leave room for ": " separator and NUL terminator
	
	/* Append the location separator: */
	*(msgStart++)=':';
	*(msgStart++)=' ';
	
	/* Print the given message into the rest of the message buffer: */
	va_list ap;
	va_start(ap,formatString);
	msgStart+=vsnprintf(msgStart,msgEnd-msgStart,formatString,ap);
	va_end(ap);
	
	/* Append the libc error suffix if there is room in the buffer: */
	if(msgStart<msgEnd)
		{
		/* Convert the error number into a string: */
		char strerrorBuffer[1024]; // Long enough according to spec
		const char* libcErrorString=strerror_r(libcError,strerrorBuffer,sizeof(strerrorBuffer));
		
		/* Append the libc error suffix using printf: */
		snprintf(msgStart,msgEnd-msgStart," due to libc error %d (%s)",libcError,libcErrorString);
		}
	
	/* Throw the exception: */
	throw std::runtime_error(msg);
	}

const char* printStdErrMsg(const char* formatString,...)
	{
	static char msg[1024]; // Static buffer for error messages - hopefully long enough...
	va_list ap;
	va_start(ap,formatString);
	vsnprintf(msg,sizeof(msg),formatString,ap);
	va_end(ap);
	return msg;
	}

char* printStdErrMsgReentrant(char* buffer,size_t bufferSize,const char* formatString,...)
	{
	va_list ap;
	va_start(ap,formatString);
	vsnprintf(buffer,bufferSize,formatString,ap);
	va_end(ap);
	return buffer;
	}

std::string stdErrMsg(const char* prettyFunction,const char* formatString,...)
	{
	/* Create a fixed-size buffer for error messages: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Copy the source function name into the message buffer: */
	bufferStart=parsePrettyFunction(prettyFunction,bufferStart,bufferEnd-3); // Leave room for ": " separator and NUL terminator
	
	/* Append the location separator: */
	*(bufferStart++)=':';
	*(bufferStart++)=' ';
	
	/* Append the given message to the message buffer: */
	va_list ap;
	va_start(ap,formatString);
	vsnprintf(bufferStart,bufferEnd-bufferStart,formatString,ap);
	va_end(ap);
	
	/* Return the filled buffer as a std::string: */
	return std::string(buffer);
	}

std::string libcErrMsg(const char* prettyFunction,int libcError,const char* formatString,...)
	{
	/* Create a fixed-size buffer for error messages: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Copy the source function name into the message buffer: */
	bufferStart=parsePrettyFunction(prettyFunction,bufferStart,bufferEnd-3); // Leave room for ": " separator and NUL terminator
	
	/* Append the location separator: */
	*(bufferStart++)=':';
	*(bufferStart++)=' ';
	
	/* Append the given message to the message buffer: */
	va_list ap;
	va_start(ap,formatString);
	bufferStart+=vsnprintf(bufferStart,bufferEnd-bufferStart,formatString,ap);
	va_end(ap);
	
	/* Append the libc error suffix if there is room in the buffer: */
	if(bufferStart<bufferEnd)
		{
		/* Convert the error number into a string: */
		char strerrorBuffer[1024]; // Long enough according to spec
		const char* libcErrorString=strerror_r(libcError,strerrorBuffer,sizeof(strerrorBuffer));
		
		/* Append the libc error suffix using printf: */
		snprintf(bufferStart,bufferEnd-bufferStart," due to libc error %d (%s)",libcError,libcErrorString);
		}
	
	/* Return the filled buffer as a std::string: */
	return std::string(buffer);
	}

}

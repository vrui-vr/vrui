/***********************************************************************
StdError - Helper functions to create std::runtime_error objects with
formatted error messages using the printf() interface.
Copyright (c) 2005-2024 Oliver Kreylos

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

#include <Misc/StdError.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <Misc/ParsePrettyFunction.h>

namespace Misc {

namespace {

/****************
Helper functions:
****************/

char* makeStdErrMsg(char* bufferStart,char* bufferEnd,const char* prettyFunction,const char* formatString,va_list ap)
	{
	if(prettyFunction!=0)
		{
		/* Copy the source function name into the message buffer: */
		bufferStart=parsePrettyFunction(prettyFunction,bufferStart,bufferEnd-3); // Leave room for ": " separator and NUL terminator
		
		/* Append the location separator: */
		*(bufferStart++)=':';
		*(bufferStart++)=' ';
		}
	
	/* Append the given message to the message buffer: */
	bufferStart+=vsnprintf(bufferStart,bufferEnd-bufferStart,formatString,ap);
	
	return bufferStart;
	}

char* appendLibcErrMsg(char* bufferStart,char* bufferEnd,int libcError)
	{
	if(bufferStart<bufferEnd)
		{
		/* Convert the error number into a string: */
		char strerrorBuffer[1024]; // Long enough according to spec
		const char* libcErrorString=strerror_r(libcError,strerrorBuffer,sizeof(strerrorBuffer));
		
		/* Append the libc error suffix using printf: */
		bufferStart+=snprintf(bufferStart,bufferEnd-bufferStart," due to libc error %d (%s)",libcError,libcErrorString);
		}
	
	return bufferStart;
	}

}

std::string makeStdErrMsg(const char* prettyFunction,const char* formatString,...)
	{
	/* Create the error message in a fixed-size on-stack buffer: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Write the location prefix and the message: */
	va_list ap;
	va_start(ap,formatString);
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	va_end(ap);
	
	/* Return the filled buffer as a std::string: */
	return std::string(buffer);
	}

char* makeStdErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,const char* formatString,...)
	{
	/* Create the error message in the given buffer: */
	char* bufferStart=buffer;
	char* bufferEnd=buffer+bufferSize;
	
	/* Write the location prefix and the message: */
	va_list ap;
	va_start(ap,formatString);
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	va_end(ap);
	
	/* Return the start of the filled buffer: */
	return buffer;
	}

std::string makeStdErrMsg(const char* prettyFunction,const char* formatString,va_list ap)
	{
	/* Create the error message in a fixed-size on-stack buffer: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Write the location prefix and the message: */
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	
	/* Return the filled buffer as a std::string: */
	return std::string(buffer);
	}

char* makeStdErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,const char* formatString,va_list ap)
	{
	/* Create the error message in the given buffer: */
	char* bufferStart=buffer;
	char* bufferEnd=buffer+bufferSize;
	
	/* Write the location prefix and the message: */
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	
	/* Return the start of the filled buffer: */
	return buffer;
	}

std::string makeLibcErrMsg(const char* prettyFunction,int libcError,const char* formatString,...)
	{
	/* Create the error message in a fixed-size on-stack buffer: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Write the location prefix and the message: */
	va_list ap;
	va_start(ap,formatString);
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	va_end(ap);
	
	/* Append the libc error code: */
	bufferStart=appendLibcErrMsg(bufferStart,bufferEnd,libcError);
	
	/* Return the filled buffer as a std::string: */
	return std::string(buffer);
	}

char* makeLibcErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,int libcError,const char* formatString,...)
	{
	/* Create the error message in the given buffer: */
	char* bufferStart=buffer;
	char* bufferEnd=buffer+bufferSize;
	
	/* Write the location prefix and the message: */
	va_list ap;
	va_start(ap,formatString);
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	va_end(ap);
	
	/* Append the libc error code: */
	bufferStart=appendLibcErrMsg(bufferStart,bufferEnd,libcError);
	
	/* Return the start of the filled buffer: */
	return buffer;
	}

std::string makeLibcErrMsg(const char* prettyFunction,int libcError,const char* formatString,va_list ap)
	{
	/* Create the error message in a fixed-size on-stack buffer: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Write the location prefix and the message: */
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	
	/* Append the libc error code: */
	bufferStart=appendLibcErrMsg(bufferStart,bufferEnd,libcError);
	
	/* Return the filled buffer as a std::string: */
	return std::string(buffer);
	}

char* makeLibcErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,int libcError,const char* formatString,va_list ap)
	{
	/* Create the error message in the given buffer: */
	char* bufferStart=buffer;
	char* bufferEnd=buffer+bufferSize;
	
	/* Write the location prefix and the message: */
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	
	/* Append the libc error code: */
	bufferStart=appendLibcErrMsg(bufferStart,bufferEnd,libcError);
	
	/* Return the start of the filled buffer: */
	return buffer;
	}

std::runtime_error makeStdErr(const char* prettyFunction,const char* formatString,...)
	{
	/* Create the error message in a fixed-size on-stack buffer: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Write the location prefix and the message: */
	va_list ap;
	va_start(ap,formatString);
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	va_end(ap);
	
	/* Create and return the exception object: */
	return std::runtime_error(buffer);
	}

std::runtime_error makeLibcErr(const char* prettyFunction,int libcError,const char* formatString,...)
	{
	/* Create the error message in a fixed-size on-stack buffer: */
	char buffer[1024];
	char* bufferStart=buffer;
	char* bufferEnd=buffer+sizeof(buffer);
	
	/* Write the location prefix and the message: */
	va_list ap;
	va_start(ap,formatString);
	bufferStart=makeStdErrMsg(bufferStart,bufferEnd,prettyFunction,formatString,ap);
	va_end(ap);
	
	/* Append the libc error code: */
	bufferStart=appendLibcErrMsg(bufferStart,bufferEnd,libcError);
	
	/* Create and return the exception object: */
	return std::runtime_error(buffer);
	}

}

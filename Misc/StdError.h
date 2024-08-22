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

#ifndef MISC_STDERROR_INCLUDED
#define MISC_STDERROR_INCLUDED

#include <stddef.h>
#include <stdarg.h>
#include <string>
#include <stdexcept>

namespace Misc {

std::string makeStdErrMsg(const char* prettyFunction,const char* formatString,...); // Returns formatted error message prefixed with source location, expected to be evaluation of __PRETTY_FUNCTION__ macro
char* makeStdErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,const char* formatString,...); // Ditto; creates message in provided buffer; returns start of buffer
std::string makeStdErrMsg(const char* prettyFunction,const char* formatString,va_list ap); // Ditto, with variable argument list
char* makeStdErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,const char* formatString,va_list ap); // Ditto; creates message in provided buffer; returns start of buffer

std::string makeLibcErrMsg(const char* prettyFunction,int libcError,const char* formatString,...); // Ditto; postfixes error message with libc error message
char* makeLibcErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,int libcError,const char* formatString,...); // Ditto; creates message in provided buffer; returns start of buffer
std::string makeLibcErrMsg(const char* prettyFunction,int libcError,const char* formatString,va_list ap); // Ditto, with variable argument list
char* makeLibcErrMsg(char* buffer,size_t bufferSize,const char* prettyFunction,int libcError,const char* formatString,va_list ap); // Ditto; creates message in provided buffer; returns start of buffer

std::runtime_error makeStdErr(const char* prettyFunction,const char* formatString,...); // Creates a std::runtime_error object with a formatted error message prefixed with with a source location, expected to be evaluation of __PRETTY_FUNCTION__ macro
std::runtime_error makeLibcErr(const char* prettyFunction,int libcError,const char* formatString,...); // Ditto; postfixes error message with libc error code and message

}

#endif

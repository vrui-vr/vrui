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

#include <Misc/ParsePrettyFunction.h>

#include <string.h>

namespace Misc {

char* parsePrettyFunction(const char* prettyFunction,char* buffer,char* bufferEnd)
	{
	/* Find the start and end of the function name by looking for the argument list's opening parenthesis, but ignore "(*" indicating a function-type result: */
	const char* fnStart=prettyFunction;
	const char* fnEnd;
	for(fnEnd=prettyFunction;*fnEnd!='\0'&&(*fnEnd!='('||fnEnd[1]=='*');++fnEnd)
		if(*fnEnd==' ')
			fnStart=fnEnd+1;
	
	/* Copy the function name into the buffer while skipping any template parameters: */
	char* bufPtr=buffer;
	unsigned int templateLevel=0;
	while(fnStart!=fnEnd&&bufPtr!=bufferEnd)
		{
		if(*fnStart=='<')
			++templateLevel;
		if(templateLevel==0)
			*(bufPtr++)=*fnStart;
		if(*fnStart=='>')
			--templateLevel;
		++fnStart;
		}
	
	/* Return a pointer to the beginning of the unused part of the buffer: */
	return bufPtr;
	}

std::string parsePrettyFunction(const char* prettyFunction)
	{
	/* Find the start and end of the function name by looking for the argument list's opening parenthesis, but ignore "(*" indicating a function-type result: */
	const char* fnStart=prettyFunction;
	const char* fnEnd;
	for(fnEnd=prettyFunction;*fnEnd!='\0'&&(*fnEnd!='('||fnEnd[1]=='*');++fnEnd)
		if(*fnEnd==' ')
			fnStart=fnEnd+1;
	
	/* Copy the function name into a new string while skipping any template parameters: */
	std::string result;
	unsigned int templateLevel=0;
	while(fnStart!=fnEnd)
		{
		if(*fnStart=='<')
			++templateLevel;
		if(templateLevel==0)
			result.push_back(*fnStart);
		if(*fnStart=='>')
			--templateLevel;
		++fnStart;
		}
	
	return result;
	}

}

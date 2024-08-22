/***********************************************************************
ValueCoder - Generic class to encode/decode values into/from human-
readable strings.
Copyright (c) 2004-2022 Oliver Kreylos

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

#include <Misc/ValueCoder.h>

#include <ctype.h>
#include <stdio.h>

namespace Misc {

/****************
Helper functions:
****************/

const char* skipWhitespace(const char* start,const char* end)
	{
	/* Skip whitespace characters: */
	while(start!=end&&isspace(*start))
		++start;
	
	/* Return pointer to end of whitespace: */
	return start;
	}

const char* checkSeparator(char separator,const char* start,const char* end)
	{
	const char* cPtr=start;
	
	/* Check for separator character: */
	if(cPtr==end||*cPtr!=separator)
		{
		char errorMsg[64];
		snprintf(errorMsg,sizeof(errorMsg),"missing %c separator",separator);
		throw DecodingError(errorMsg);
		}
	++cPtr;
	
	/* Skip whitespace characters: */
	while(cPtr!=end&&isspace(*cPtr))
		++cPtr;
	
	/* Return pointer to end of whitespace: */
	return cPtr;
	}

const char* checkOpening(char opener,const char* start,const char* end)
	{
	const char* cPtr=start;
	
	/* Check for opening character: */
	if(cPtr==end||*cPtr!=opener)
		{
		char errorMsg[64];
		snprintf(errorMsg,sizeof(errorMsg),"missing opening %c",opener);
		throw DecodingError(errorMsg);
		}
	++cPtr;
	
	/* Skip whitespace characters: */
	while(cPtr!=end&&isspace(*cPtr))
		++cPtr;
	
	/* Return pointer to end of whitespace: */
	return cPtr;
	}

const char* checkClosing(char closer,const char* start,const char* end)
	{
	const char* cPtr=start;
	
	/* Check for closing character: */
	if(cPtr==end||*cPtr!=closer)
		{
		char errorMsg[64];
		snprintf(errorMsg,sizeof(errorMsg),"missing closing %c",closer);
		throw DecodingError(errorMsg);
		}
	++cPtr;
	
	/* Return pointer to behind closing character: */
	return cPtr;
	}

}

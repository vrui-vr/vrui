/***********************************************************************
HttpPostRequest - Helper class to parse HTTP 1.1 POST requests.
Copyright (c) 2025 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Comm/HttpPostRequest.h>

#include <ctype.h>
#include <stdexcept>
#include <Misc/StdError.h>
#include <IO/File.h>
#include <IO/ValueSource.h>

namespace Comm {

/********************************
Methods of class HttpPostRequest:
********************************/

namespace {

/****************
Helper functions:
****************/

int fromHex(int c) // Converts a hexadecimal character into its integer value
	{
	if(c>='0'&&c<'9')
		return c-'0';
	else if(c>='A'&&c<='F')
		return c-'A'+10;
	else if(c>='a'&&c<='f')
		return c-'a'+10;
	else
		throw std::runtime_error("Invalid hex digit");
	}

int decodeUrl(IO::File& file,size_t& contentLength,std::string& string) // Reads and URL-decodes a string from the given file; returns the string-terminating character
	{
	int result=0;
	
	int state=0;
	int percentCode;
	while(contentLength>0&&state<3&&!file.eof())
		{
		/* Read the next character from the file: */
		int c=file.getChar();
		--contentLength;
		
		/* Process the character based on the current decoding state: */
		switch(state)
			{
			case 0: // Regular character
				if(c=='&'||c=='=')
					{
					/* Stop decoding and return the terminating character: */
					result=c;
					state=3;
					}
				else if(c=='+')
					string.push_back(' ');
				else if(c=='%')
					{
					/* Start reading a percent encoding: */
					percentCode=0x0;
					state=1;
					}
				else
					string.push_back(c);
				break;
			
			case 1: // First character of percent encoding
				percentCode=fromHex(c);
				state=2;
				break;
			
			case 2: // Second character of percent encoding
				percentCode=(percentCode<<4)|fromHex(c);
				string.push_back(percentCode);
				state=0;
				break;
			}
		}
	
	/* Check for errors: */
	if(state==1||state==2)
		throw std::runtime_error("Truncated percent encoding");
	
	return result;
	}

}

HttpPostRequest::HttpPostRequest(IO::File& file)
	{
	try
		{
		/* Parse the request header: */
		size_t contentLength=0;
		
		{
		/* Attach a value source to the connection to parse the client's request: */
		IO::ValueSource request(&file);
		request.setPunctuation("\n");
		request.setWhitespace(" \r");
		request.skipWs();
		
		/* Check for the POST keyword: */
		if(!request.isString("POST"))
			throw std::runtime_error("Missing POST keyword");
		
		/* Extract the action URL: */
		actionUrl=request.readString();
		
		/* Check for the protocol identifier: */
		if(!(request.isString("HTTP/1.1")&&request.isLiteral('\n')))
			throw std::runtime_error("Wrong HTTP specifier");
		
		/* Parse request data fields: */
		request.setPunctuation(":\n");
		bool haveContentType=false;
		while(!request.eof())
			{
			/* Bail out if the line is empty: */
			if(request.peekc()=='\n')
				break;
			
			/* Read a data field: */
			std::string fieldName=request.readString();
			if(!request.isLiteral(':'))
				throw std::runtime_error("Missing ':' in request header field");
			
			std::string fieldValue=request.readLine();
			request.skipWs();
			
			/* Strip trailing whitespace from the field value: */
			while(isspace(fieldValue.back()))
				fieldValue.pop_back();
			
			/* Parse the data field: */
			if(fieldName=="Content-Type")
				{
				if(fieldValue!="application/x-www-form-urlencoded")
					throw std::runtime_error("Wrong content type");
				haveContentType=true;
				}
			else if(fieldName=="Content-Length")
				{
				const char* vStart=fieldValue.c_str();
				char* vEnd=0;
				contentLength=size_t(strtoul(vStart,&vEnd,10));
				if(vEnd==vStart||*vEnd!='\0')
					throw std::runtime_error("Invalid content length");
				}
			}
		if(!request.isLiteral('\n'))
			throw std::runtime_error("Truncated header");
		if(!haveContentType)
			throw std::runtime_error("Unspecified content type");
		if(contentLength==0)
			throw std::runtime_error("Unspecified content length");
		}
		
		/* Parse the request's name=value pairs: */
		while(contentLength>0&&!file.eof())
			{
			NameValue nv;
			
			/* Parse the next name: */
			int separator=decodeUrl(file,contentLength,nv.name);
			if(separator!='=')
				throw std::runtime_error("Missing '=' in name=value pair");
			
			/* Parse the next value: */
			separator=decodeUrl(file,contentLength,nv.value);
			if(separator=='=')
				throw std::runtime_error("Extra '=' in name=value pair");
			
			/* Store the name=value pair: */
			nameValueList.push_back(nv);
			}
		if(contentLength>0)
			throw std::runtime_error("Truncated body");
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot parse POST request due to exception %s",err.what());
		}
	}

}

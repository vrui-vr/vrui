/***********************************************************************
HttpRequestHeader - Helper class to parse general HTTP 1.1 request
headers in a non-blocking fashion.
Copyright (c) 2025-2026 Oliver Kreylos

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

#include <Comm/HttpRequestHeader.h>

#include <ctype.h>
#include <stdlib.h>
#include <Misc/StdError.h>
#include <IO/File.h>

namespace Comm {

/**********************************
Methods of class HttpRequestHeader:
**********************************/

HttpRequestHeader::HttpRequestHeader(void)
	:parserState(RequestTypeString),
	 requestType(Invalid),
	 keepAlive(true), // keep-alive is default behavior in HTTP/1.1
	 contentLength(~size_t(0))
	{
	}

bool HttpRequestHeader::read(IO::File& file)
	{
	/* Bail out if the file's read buffer is empty: */
	if(!file.canReadImmediately())
		return parserState==Finished;
	
	/* Access the file's read buffer: */
	void* buffer;
	size_t bufferSize=file.readInBuffer(buffer);
	char* rPtr=static_cast<char*>(buffer);
	char* rEnd=rPtr+bufferSize;
	
	/* Run the parser state machine: */
	do
		{
		switch(parserState)
			{
			case RequestTypeString:
				/* Keep reading the request type string: */
				while(rPtr!=rEnd&&*rPtr!=' ')
					string.push_back(*(rPtr++));
				
				/* Check if the request type string is over: */
				if(rPtr!=rEnd)
					{
					/* Determine the request type: */
					if(string=="GET")
						requestType=Get;
					else if(string=="PUT")
						requestType=Put;
					else if(string=="POST")
						requestType=Post;
					else if(string=="OPTIONS")
						requestType=Options;
					else
						{
						requestType=Invalid;
						throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid HTTP request type %s",string.c_str());
						}
					
					/* Skip the separating space and start parsing the request URI: */
					++rPtr;
					string.clear();
					parserState=RequestURI;
					}
				
				break;
			
			case RequestURI:
				/* Keep reading the request URI string: */
				while(rPtr!=rEnd&&*rPtr!=' ')
					requestUri.push_back(*(rPtr++));
				
				/* Check if the request URI string is over: */
				if(rPtr!=rEnd)
					{
					/* Skip the separating space and start parsing the HTTP protocol version: */
					++rPtr;
					parserState=HTTPVersion;
					}
				
				break;
			
			case HTTPVersion:
				/* Keep reading the HTTP protocol version string: */
				while(rPtr!=rEnd&&*rPtr!='\r')
					string.push_back(*(rPtr++));
				
				/* Check if the HTTP protocol version string is over: */
				if(rPtr!=rEnd)
					{
					if(string!="HTTP/1.1")
						throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid HTTP protocol version %s",string.c_str());
					
					/* Skip the terminating carriage return and parse a request header start line end: */
					++rPtr;
					string.clear();
					parserState=StartLineEnd;
					}
				
				break;
			
			case StartLineEnd:
				/* Check for and skip a line feed, then read the first header line: */
				if(*rPtr!='\n')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid line end in request header's start line");
				++rPtr;
				parserState=HeaderLine;
				
				break;
			
			case HeaderLine:
				/* Check for an empty line: */
				if(*rPtr=='\r')
					{
					/* Skip the carriage return and parse a request header end: */
					++rPtr;
					parserState=HeaderEnd;
					}
				else
					{
					/* Start parsing a request header field name: */
					parserState=HeaderFieldName;
					}
				
				break;
			
			case HeaderFieldName:
				/* Keep reading the request header field name: */
				while(rPtr!=rEnd&&*rPtr!=':')
					nameValue.name.push_back(tolower(*(rPtr++))); // Convert header field names to lowercase
				
				/* Check if the request header field name is over: */
				if(rPtr!=rEnd)
					{
					/* Skip the separating colon and start parsing header request field value leading whitespace: */
					++rPtr;
					parserState=HeaderFieldValueLeadingSpace;
					}
				
				break;
			
			case HeaderFieldValueLeadingSpace:
				/* Keep skipping whitespace: */
				while(rPtr!=rEnd&&*rPtr!='\r'&&isspace(*rPtr))
					++rPtr;
				
				/* Check if whitespace is over: */
				if(rPtr!=rEnd)
					{
					/* Check if we're at the line end: */
					if(*rPtr=='\r')
						{
						/* Skip the carriage return and parse a request header line end: */
						++rPtr;
						parserState=HeaderLineEnd;
						}
					else
						{
						/* Start parsing a request header field value: */
						parserState=HeaderFieldValue;
						}
					}
				break;
			
			case HeaderFieldValue:
				/* Keep reading the request header field value: */
				while(rPtr!=rEnd&&*rPtr!='\r')
					nameValue.value.push_back(*(rPtr++));
				
				/* Check if the request header field value is over: */
				if(rPtr!=rEnd)
					{
					/* Skip the terminating carriage return and parse a request header line end: */
					++rPtr;
					parserState=HeaderLineEnd;
					}
				
				break;
			
			case HeaderLineEnd:
				/* Check for and skip a line feed: */
				if(*rPtr!='\n')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid line end in request header");
				++rPtr;
				
				/* Remove trailing whitespace from the just-parsed request header field value: */
				while(!nameValue.value.empty()&&isspace(nameValue.value.back()))
					nameValue.value.pop_back();
				
				/* Check for well-known request header fields: */
				if(nameValue.name=="connection")
					{
					if(nameValue.value=="keep-alive")
						keepAlive=true;
					else if(nameValue.value=="close")
						keepAlive=false;
					else
						throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid value \"%s\" in Connection header field",nameValue.value.c_str());
					}
				else if(nameValue.name=="content-length")
					{
					/* Try converting the field value field to an integer: */
					const char* vStart=nameValue.value.c_str();
					char* vEnd=0;
					contentLength=size_t(strtoul(vStart,&vEnd,10));
					if(vEnd==vStart||*vEnd!='\0')
						throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid value \"%s\" in Content-Length header field",nameValue.value.c_str());
					}
				
				/* Save the just-parsed request header field: */
				headerFields.push_back(nameValue);
				nameValue.name.clear();
				nameValue.value.clear();
				
				/* Parse the next header line: */
				parserState=HeaderLine;
				
				break;
			
			case HeaderEnd:
				/* Check for and skip a line feed, then mark the header as finished: */
				if(*rPtr!='\n')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid line end in request header");
				++rPtr;
				parserState=Finished;
				
				/* Mark the remainder of the file's read buffer as unread: */
				file.putBackInBuffer(rEnd-rPtr);
				rPtr=rEnd;
				
				break;
			}
		}
	while(rPtr!=rEnd);
	
	return parserState==Finished;
	}

bool HttpRequestHeader::hasHeaderField(const char* headerFieldName) const
	{
	/* Find the header field of the given name: */
	for(NameValueList::const_iterator hfIt=headerFields.begin();hfIt!=headerFields.end();++hfIt)
		if(hfIt->name==headerFieldName)
			{
			/* Found it: */
			return true;
			}
	
	/* Didn't find it: */
	return false;
	}

const std::string& HttpRequestHeader::getHeaderFieldValue(const char* headerFieldName) const
	{
	/* Find the header field of the given name: */
	for(NameValueList::const_iterator hfIt=headerFields.begin();hfIt!=headerFields.end();++hfIt)
		if(hfIt->name==headerFieldName)
			{
			/* Return the header field's value: */
			return hfIt->value;
			}
	
	/* The header field wasn't found; throw an exception: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Header field %s not found",headerFieldName);
	}

}

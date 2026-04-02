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

#ifndef COMM_HTTPREQUEST_INCLUDED
#define COMM_HTTPREQUEST_INCLUDED

#include <stddef.h>
#include <string>
#include <vector>

/* Forward declarations: */
namespace IO {
class File;
}

namespace Comm {

class HttpRequestHeader
	{
	/* Embedded classes: */
	public:
	enum RequestType // Enumerated type for supported HTTP request types
		{
		Get,Put,Post,Options,Invalid
		};
	
	struct NameValue // Structure to hold a name=value pair
		{
		/* Elements: */
		public:
		std::string name;
		std::string value;
		};
	
	typedef std::vector<NameValue> NameValueList; // Type for lists of name=value or name: value pairs
	
	private:
	enum ParserState // Enumerated type for request parsing states
		{
		RequestTypeString,
		RequestURI,
		HTTPVersion,
		StartLineEnd,
		HeaderLine,HeaderFieldName,HeaderFieldValueLeadingSpace,HeaderFieldValue,HeaderLineEnd,
		HeaderEnd,
		Finished
		};
	
	/* Elements: */
	ParserState parserState; // Current request parser state
	std::string string; // Buffer for the currently-parsed string
	NameValue nameValue; // Buffer for a currently-parsed name/value pair
	RequestType requestType; // The type of this request
	std::string requestUri; // The request's URI
	NameValueList headerFields; // List of request header field names and their values; header field names are converted to lowercase during parsing
	bool keepAlive; // Flag if the client wants to keep the underlying TCP connection alive beyond this request
	size_t contentLength; // Numerical value of the Content-Length header field if one was present; ~size_t(0) otherwise
	
	/* Constructors and destructors: */
	public:
	HttpRequestHeader(void); // Creates an invalid HTTP request header
	
	/* Methods: */
	bool read(IO::File& file); // Continues parsing the HTTP request header from the given file's read buffer; does not block; returns true if the request header has been completely read; throws exceptions on parsing errors
	bool isComplete(void) const // Returns true if the request header has been completely read
		{
		return parserState==Finished;
		}
	RequestType getRequestType(void) const // Returns the request header's type
		{
		return requestType;
		}
	const std::string& getRequestUri(void) const // Returns the request URI
		{
		return requestUri;
		}
	const NameValueList& getHeaderFields(void) const // Returns the list of request header fields; all field names are lowercase
		{
		return headerFields;
		}
	bool hasHeaderField(const char* headerFieldName) const; // Returns true if the request has a header field of the given name
	const std::string& getHeaderFieldValue(const char* headerFieldName) const; // Returns the value of the header field of the given name, which is in lowercase; throws exception if header field doesn't exist
	bool getKeepAlive(void) const // Returns true if the client wants to keep the underlying TCP connection alive beyond this request
		{
		return keepAlive;
		}
	bool hasContentLength(void) const // Returns true if the header included a Content-Length field
		{
		return contentLength!=~size_t(0);
		}
	size_t getContentLength(void) const // Returns the numerical value of the Content-Length header field if one was present, or ~size_t(0) otherwise
		{
		return contentLength;
		}
	};

}

#endif

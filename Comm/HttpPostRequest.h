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

#ifndef COMM_HTTPPOSTREQUEST_INCLUDED
#define COMM_HTTPPOSTREQUEST_INCLUDED

#include <string>
#include <vector>

/* Forward declarations: */
namespace IO {
class File;
}

namespace Comm {

class HttpPostRequest
	{
	/* Embedded classes: */
	public:
	struct NameValue // Structure to hold a name=value pair
		{
		/* Elements: */
		public:
		std::string name;
		std::string value;
		};
	
	typedef std::vector<NameValue> NameValueList; // Type for lists of name=value pairs
	
	/* Elements: */
	private:
	std::string actionUrl; // The action URL provided in the POST request
	NameValueList nameValueList; // The list of name=value pairs parsed from the POST request
	
	/* Constructors and destructors: */
	public:
	HttpPostRequest(IO::File& file); // Parses an HTTP post request from the given readable stream
	
	/* Methods: */
	const std::string& getActionUrl(void) const // Returns the action URL
		{
		return actionUrl;
		}
	const NameValueList& getNameValueList(void) const // Returns the list of name=value pairs
		{
		return nameValueList;
		}
	};

}

#endif

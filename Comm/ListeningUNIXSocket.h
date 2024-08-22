/***********************************************************************
ListeningUNIXSocket - Class for UNIX-domain half-sockets that can accept
incoming connections.
Copyright (c) 2022 Oliver Kreylos

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

#ifndef COMM_LISTENINGUNIXSOCKET_INCLUDED
#define COMM_LISTENINGUNIXSOCKET_INCLUDED

#include <Comm/ListeningSocket.h>

namespace Comm {

class ListeningUNIXSocket:public ListeningSocket
	{
	/* Constructors and destructors: */
	public:
	ListeningUNIXSocket(const char* socketName,int backlog,bool abstract); // Creates a listening socket with the given name; creates socket in the abstract name space if given flag is true
	private:
	ListeningUNIXSocket(const ListeningUNIXSocket& source); // Prohibit copy constructor
	ListeningUNIXSocket& operator=(const ListeningUNIXSocket& source); // Prohibit assignment operator
	
	/* Methods from class ListeningSocket: */
	public:
	virtual PipePtr accept(void);
	
	/* New methods: */
	std::string getAddress(void) const; // Returns the UNIX domain address to which the socket is bound
	};

}

#endif

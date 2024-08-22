/***********************************************************************
ListeningSocket - Abstract base class for half-sockets that can accept
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

#ifndef COMM_LISTENINGSOCKET_INCLUDED
#define COMM_LISTENINGSOCKET_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/RefCounted.h>
#include <Comm/Pipe.h>

/* Forward declarations: */
namespace Misc {
class Time;
}

namespace Comm {

class ListeningSocket:public Threads::RefCounted
	{
	/* Elements: */
	protected:
	int fd; // File descriptor of the listening half-socket
	
	/* Constructors and destructors: */
	public:
	ListeningSocket(void); // Dummy constructor; creates invalid file descriptor
	private:
	ListeningSocket(const ListeningSocket& source); // Prohibit copy constructor
	ListeningSocket& operator=(const ListeningSocket& source); // Prohibit assignment operator
	public:
	virtual ~ListeningSocket(void); // Closes the half-socket
	
	/* Methods: */
	bool isValid(void) const // Returns true if the listening socket is valid
		{
		return fd>=0;
		}
	int getFd(void) const // Returns this half-socket's file descriptor
		{
		return fd;
		}
	bool isBlocking(void) const; // Returns true if the listening socket is in non-blocking mode
	void setBlocking(bool newBlocking); // Sets the listening socket to blocking or non-blocking mode
	bool waitForConnection(const Misc::Time& timeout) const; // Waits for an incoming connection until timeout; returns true if a connection is waiting to be accepted
	virtual PipePtr accept(void) =0; // Returns a new pipe object for an incoming connection
	};

typedef Misc::Autopointer<ListeningSocket> ListeningSocketPtr; // Type for pointers to reference-counted listening socket objects

}

#endif

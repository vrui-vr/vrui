/***********************************************************************
ListeningSocket - Abstract base class for half-sockets that can accept
incoming connections.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <Comm/ListeningSocket.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <Misc/StdError.h>
#include <Misc/Time.h>
#include <Misc/FdSet.h>

namespace Comm {

/********************************
Methods of class ListeningSocket:
********************************/

ListeningSocket::ListeningSocket(void)
	:fd(-1)
	{
	}

ListeningSocket::~ListeningSocket(void)
	{
	/* Close the socket if it is valid: */
	if(fd>=0)
		close(fd);
	}

bool ListeningSocket::isBlocking(void) const
	{
	/* Query the file descriptor flags: */
	int flags=fcntl(fd,F_GETFL,0);
	if(flags<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve blocking flag");
	
	return (flags&O_NONBLOCK)!=0x0;
	}

void ListeningSocket::setBlocking(bool newBlocking)
	{
	/* Query the current file descriptor flags: */
	int flags=fcntl(fd,F_GETFL,0);
	if(flags<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve blocking flag");
	
	/* Set or reset the blocking flag: */
	int newFlags=flags;
	if(newBlocking)
		newFlags&=~O_NONBLOCK;
	else
		newFlags|=O_NONBLOCK;
	if(newFlags!=flags&&fcntl(fd,F_SETFL,newFlags)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to set blocking flag");
	}

bool ListeningSocket::waitForConnection(const Misc::Time& timeout) const
	{
	/* Wait for a connection (socket ready for reading) and return whether one is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	}

}

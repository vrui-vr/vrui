/***********************************************************************
UNIXPipe - Class for high-performance reading/writing from/to connected
UNIX domain sockets.
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

#ifndef COMM_UNIXPIPE_INCLUDED
#define COMM_UNIXPIPE_INCLUDED

#include <Comm/Pipe.h>

/* Forward declarations: */
namespace Comm {
class ListeningUNIXSocket;
}

namespace Comm {

class UNIXPipe:public Pipe
	{
	/* Elements: */
	private:
	int fd; // File descriptor of the underlying TCP socket
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	virtual size_t writeDataUpTo(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	UNIXPipe(const char* socketName,bool abstract); // Opens a UNIX pipe connected to the given UNIX domain socket with "DontCare" endianness setting
	UNIXPipe(ListeningUNIXSocket& listenSocket); // Opens a UNIX pipe connected by accepting the first pending connection on the given listening socket with "DontCare" endianness setting
	private:
	UNIXPipe(const UNIXPipe& source); // Prohibit copy constructor
	UNIXPipe& operator=(const UNIXPipe& source); // Prohibit assignment operator
	public:
	virtual ~UNIXPipe(void);
	
	/* Methods from IO::File: */
	virtual int getFd(void) const;
	
	/* Methods from Pipe: */
	virtual bool waitForData(void) const;
	virtual bool waitForData(const Misc::Time& timeout) const;
	virtual void shutdown(bool read,bool write);
	
	/* New methods: */
	int readFd(void); // Reads a file descriptor from the pipe
	void writeFd(int wFd); // Writes a file descriptor to the pipe
	std::string getAddress(void) const; // Returns the UNIX domain name to which the local end of the pipe is bound
	std::string getPeerAddress(void) const; // Returns the UNIX domain name to which the other end of the pipe is bound
	};

}

#endif

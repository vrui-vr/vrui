/***********************************************************************
ListeningUNIXSocket - Class for UNIX-domain half-sockets that can accept
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

#include <Comm/ListeningUNIXSocket.h>

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <Misc/StdError.h>
#include <Comm/UNIXPipe.h>

namespace Comm {

/************************************
Methods of class ListeningUNIXSocket:
************************************/

ListeningUNIXSocket::ListeningUNIXSocket(const char* socketName,int backlog,bool abstract)
	{
	/* Open the socket: */
	fd=socket(AF_UNIX,SOCK_STREAM,0);
	if(fd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create UNIX domain socket");
	
	/* Set the socket address: */
	sockaddr_un socketAddress;
	memset(&socketAddress,0,sizeof(socketAddress));
	socketAddress.sun_family=AF_UNIX;
	if(abstract)
		{
		/* Mark the socket path in the abstract namespace: */
		socketAddress.sun_path[0]='\0';
		strncpy(socketAddress.sun_path+1,socketName,sizeof(socketAddress.sun_path)-2);
		}
	else
		{
		/* Set the regular socket path and unlink a potentially existing socket of the same name: */
		strncpy(socketAddress.sun_path,socketName,sizeof(socketAddress.sun_path)-1);
		unlink(socketName);
		}
	
	/* Bind the socket to the socket address: */
	if(bind(fd,reinterpret_cast<sockaddr*>(&socketAddress),sizeof(socketAddress))<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot bind UNIX domain socket to address %s",socketName);
	
	/* Start listening on the socket: */
	if(listen(fd,backlog)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot listen on UNIX domain address %s",socketName);
	}

PipePtr ListeningUNIXSocket::accept(void)
	{
	/* Return a new UNIX domain pipe: */
	return new UNIXPipe(*this);
	}

std::string ListeningUNIXSocket::getAddress(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_un socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot query socket address");
	
	/* Extract the socket path: */
	if(socketAddressLen>sizeof(socketAddress))
		socketAddressLen=sizeof(socketAddress);
	const char* sunPath=socketAddress.sun_path;
	size_t sunPathLen=socketAddressLen-offsetof(struct sockaddr_un,sun_path);
	
	/* Check if the socket is in the abstract namespace: */
	if(sunPathLen>0&&*sunPath=='\0')
		{
		++sunPath;
		--sunPathLen;
		}
	return std::string(sunPath,sunPath+sunPathLen);
	}

}

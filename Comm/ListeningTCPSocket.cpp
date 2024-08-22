/***********************************************************************
ListeningTCPSocket - Class for TCP half-sockets that can accept incoming
connections.
Copyright (c) 2011-2024 Oliver Kreylos

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

#include <Comm/ListeningTCPSocket.h>

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <Misc/PrintInteger.h>
#include <Misc/StdError.h>
#include <Comm/TCPPipe.h>

namespace Comm {

/***********************************
Methods of class ListeningTCPSocket:
***********************************/

ListeningTCPSocket::ListeningTCPSocket(int portId,int backlog,ListeningTCPSocket::AddressFamily addressFamily)
	{
	/* Convert port ID to string for getaddrinfo (awkward!): */
	if(portId<0||portId>65535)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid port %d",portId);
	char portIdBuffer[6];
	char* portIdString=Misc::print(portId,portIdBuffer+5);
	
	/* Create a local any-IP address: */
	struct addrinfo hints;
	memset(&hints,0,sizeof(struct addrinfo));
	switch(addressFamily)
		{
		case Any:
			hints.ai_family=AF_UNSPEC;
			break;
			
		case IPv4:
			hints.ai_family=AF_INET;
			break;
		
		case IPv6:
			hints.ai_family=AF_INET6;
			break;
		}
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_NUMERICSERV|AI_PASSIVE|AI_ADDRCONFIG;
	hints.ai_protocol=0;
	struct addrinfo* addresses;
	int aiResult=getaddrinfo(0,portIdString,&hints,&addresses);
	if(aiResult!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create listening address on port %d due to error %d (%s)",portId,aiResult,gai_strerror(aiResult));
	
	/* Try all returned addresses in order until one successfully binds: */
	for(struct addrinfo* aiPtr=addresses;aiPtr!=0;aiPtr=aiPtr->ai_next)
		{
		/* Open a socket: */
		fd=socket(aiPtr->ai_family,aiPtr->ai_socktype,aiPtr->ai_protocol);
		if(fd<0)
			continue;
		
		/* Bind the local address and bail out if successful: */
		if(bind(fd,reinterpret_cast<struct sockaddr*>(aiPtr->ai_addr),aiPtr->ai_addrlen)==0)
			break;
		
		/* Close the socket again and try the next address: */
		close(fd);
		fd=-1;
		}
	
	/* Release the returned addresses: */
	freeaddrinfo(addresses);
	
	/* Check if socket setup failed: */
	if(fd<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create listening socket on port %d",portId);
	
	/* Start listening on the socket: */
	if(listen(fd,backlog)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot start listening on port %d",portId);
	}

PipePtr ListeningTCPSocket::accept(void)
	{
	/* Return a new TCP pipe: */
	return new TCPPipe(*this);
	}

int ListeningTCPSocket::getPortId(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot query socket address");
	
	/* Extract a numeric port ID from the socket's address: */
	char portIdBuffer[NI_MAXSERV];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,0,0,portIdBuffer,sizeof(portIdBuffer),NI_NUMERICSERV);
	if(niResult!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve port ID due to error %d (%s)",niResult,gai_strerror(niResult));
	
	/* Convert the port ID string to a number: */
	int result=0;
	for(int i=0;i<NI_MAXSERV&&portIdBuffer[i]!='\0';++i)
		result=result*10+int(portIdBuffer[i]-'0');
	
	return result;
	}

std::string ListeningTCPSocket::getAddress(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot query socket address");
	
	/* Extract a numeric address from the socket's address: */
	char addressBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,NI_NUMERICHOST);
	if(niResult!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve interface address due to error %d (%s)",niResult,gai_strerror(niResult));
	
	return addressBuffer;
	}

std::string ListeningTCPSocket::getInterfaceName(bool throwException) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot query socket address");
	
	/* Extract a host name from the socket's address: */
	char hostNameBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,hostNameBuffer,sizeof(hostNameBuffer),0,0,0);
	if(niResult!=0)
		{
		if(throwException)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve interface name due to error %d (%s)",niResult,gai_strerror(niResult));
		else
			{
			/* Fall back to returning the socket's numeric address: */
			niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,hostNameBuffer,sizeof(hostNameBuffer),0,0,NI_NUMERICHOST);
			if(niResult!=0)
				{
				/* Oh well, now we do have to throw an exception anyway: */
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve interface address due to error %d (%s)",niResult,gai_strerror(niResult));
				}
			}
		}
	
	return hostNameBuffer;
	}

}

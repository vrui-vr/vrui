/***********************************************************************
UDPSocket - Wrapper class for UDP sockets ensuring exception safety.
Copyright (c) 2004-2024 Oliver Kreylos

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

#include <Comm/UDPSocket.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <Misc/StdError.h>
#include <Misc/Time.h>
#include <Misc/FdSet.h>
#include <Comm/IPv4Address.h>
#include <Comm/IPv4SocketAddress.h>

namespace Comm {

/**************************
Methods of class UPDSocket:
**************************/

UDPSocket::UDPSocket(int localPortId,int)
	{
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_DGRAM,0);
	if(socketFd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to create socket");
	
	/* Bind the socket file descriptor to the local port ID: */
	IPv4SocketAddress socketAddress(localPortId>=0?(unsigned int)(localPortId):0U);
	if(bind(socketFd,(struct sockaddr*)&socketAddress,sizeof(struct IPv4SocketAddress))==-1)
		{
		int myerrno=errno;
		close(socketFd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,myerrno,"Unable to bind socket to port %d",localPortId);
		}
	}

UDPSocket::UDPSocket(int localPortId,const std::string& hostname,int hostPortId)
	{
	/* Lookup host's IP address: */
	IPv4SocketAddress hostSocketAddress(hostPortId,IPv4Address(hostname.c_str()));
	
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_DGRAM,0);
	if(socketFd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to create socket");
	
	/* Bind the socket file descriptor to the local port ID: */
	IPv4SocketAddress mySocketAddress(localPortId>=0?(unsigned int)(localPortId):0U);
	if(bind(socketFd,(struct sockaddr*)&mySocketAddress,sizeof(IPv4SocketAddress))==-1)
		{
		int myerrno=errno;
		close(socketFd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,myerrno,"Unable to bind socket to port %d",localPortId);
		}
	
	/* Connect to the remote host: */
	if(::connect(socketFd,(const struct sockaddr*)&hostSocketAddress,sizeof(IPv4SocketAddress))==-1)
		{
		int myerrno=errno;
		close(socketFd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,myerrno,"Unable to connect to host %s on port %d",hostname.c_str(),hostPortId);
		}
	}

UDPSocket::UDPSocket(int localPortId,const IPv4SocketAddress& hostAddress)
	{
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_DGRAM,0);
	if(socketFd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to create socket");
	
	/* Bind the socket file descriptor to the local port ID: */
	IPv4SocketAddress mySocketAddress(localPortId>=0?(unsigned int)(localPortId):0U);
	if(bind(socketFd,(struct sockaddr*)&mySocketAddress,sizeof(IPv4SocketAddress))==-1)
		{
		int myerrno=errno;
		close(socketFd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,myerrno,"Unable to bind socket to port %d",localPortId);
		}
	
	/* Connect to the remote host: */
	if(::connect(socketFd,(const struct sockaddr*)(&hostAddress),sizeof(IPv4SocketAddress))==-1)
		{
		int myerrno=errno;
		close(socketFd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,myerrno,"Unable to connect to host %s on port %d",hostAddress.getAddress().getHostname().c_str(),hostAddress.getPort());
		}
	}

UDPSocket::UDPSocket(const UDPSocket& source)
	:socketFd(dup(source.socketFd))
	{
	}

UDPSocket& UDPSocket::operator=(const UDPSocket& source)
	{
	if(this!=&source)
		{
		close(socketFd);
		socketFd=dup(source.socketFd);
		}
	return *this;
	}

UDPSocket::~UDPSocket(void)
	{
	close(socketFd);
	}

IPv4SocketAddress UDPSocket::getAddress(void) const
	{
	IPv4SocketAddress socketAddress;
	#ifdef __SGI_IRIX__
	int socketAddressLen=sizeof(IPv4SocketAddress);
	#else
	socklen_t socketAddressLen=sizeof(IPv4SocketAddress);
	#endif
	if(getsockname(socketFd,(struct sockaddr*)&socketAddress,&socketAddressLen)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to query socket address");
	if(socketAddressLen<sizeof(IPv4SocketAddress))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Returned address has wrong size; %u bytes instead of %u bytes",(unsigned int)socketAddressLen,(unsigned int)sizeof(IPv4SocketAddress));
	
	return socketAddress;
	}

int UDPSocket::getPortId(void) const
	{
	/* Retrieve the socket address and return only the port number: */
	return getAddress().getPort();
	}

void UDPSocket::setMulticastLoopback(bool multicastLoopback)
	{
	/* Set the loop option: */
	unsigned char loop=multicastLoopback?1U:0U;
	if(setsockopt(socketFd,IPPROTO_IP,IP_MULTICAST_LOOP,&loop,sizeof(unsigned char))<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to %s multicast loopback",multicastLoopback?"enable":"disable");
	}

void UDPSocket::setMulticastTTL(unsigned int multicastTTL)
	{
	/* Set the TTL option: */
	unsigned char ttl=multicastTTL;
	if(setsockopt(socketFd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(unsigned char))<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to set multicast TTL");
	}

void UDPSocket::setMulticastInterface(const IPv4Address& interfaceAddress)
	{
	/* Set the given interface address: */
	if(setsockopt(socketFd,IPPROTO_IP,IP_MULTICAST_IF,&interfaceAddress,sizeof(struct in_addr))<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to set outgoing multicast interface to %s",interfaceAddress.getHostname().c_str());
	}

void UDPSocket::joinMulticastGroup(const IPv4Address& groupAddress,const IPv4Address& interfaceAddress)
	{
	/* Issue a multicast request: */
	struct ip_mreq joinRequest;
	joinRequest.imr_multiaddr.s_addr=groupAddress.s_addr;
	joinRequest.imr_interface.s_addr=interfaceAddress.s_addr;
	if(setsockopt(socketFd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&joinRequest,sizeof(struct ip_mreq))<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to join multicast group %s on interface %s",groupAddress.getHostname().c_str(),interfaceAddress.getHostname().c_str());
	}

void UDPSocket::leaveMulticastGroup(const IPv4Address& groupAddress,const IPv4Address& interfaceAddress)
	{
	/* Issue a multicast request: */
	struct ip_mreq joinRequest;
	joinRequest.imr_multiaddr.s_addr=groupAddress.s_addr;
	joinRequest.imr_interface.s_addr=interfaceAddress.s_addr;
	if(setsockopt(socketFd,IPPROTO_IP,IP_DROP_MEMBERSHIP,&joinRequest,sizeof(struct ip_mreq))<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to leave multicast group %s on interface %s",groupAddress.getHostname().c_str(),interfaceAddress.getHostname().c_str());
	}

void UDPSocket::connect(const std::string& hostname,int hostPortId)
	{
	/* Lookup host's IP address: */
	IPv4SocketAddress hostSocketAddress(hostPortId,IPv4Address(hostname.c_str()));
	
	/* Connect to the remote host: */
	if(::connect(socketFd,(const struct sockaddr*)&hostSocketAddress,sizeof(IPv4SocketAddress))==-1)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to connect to host %s on port %d",hostname.c_str(),hostPortId);
	}

void UDPSocket::connect(const IPv4SocketAddress& hostAddress)
	{
	/* Connect to the remote host: */
	if(::connect(socketFd,(const struct sockaddr*)&hostAddress,sizeof(IPv4SocketAddress))==-1)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to connect to host %s on port %d",hostAddress.getAddress().getHostname().c_str(),hostAddress.getPort());
	}

void UDPSocket::accept(void)
	{
	/* Wait for an incoming message: */
	char buffer[256];
	struct sockaddr_in senderAddress;
	socklen_t senderAddressLen=sizeof(struct sockaddr_in);
	ssize_t numBytesReceived=recvfrom(socketFd,buffer,sizeof(buffer),0,(struct sockaddr*)&senderAddress,&senderAddressLen);
	if(numBytesReceived<0||size_t(numBytesReceived)>sizeof(buffer))
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to read connection request");
	
	/* Connect to the sender: */
	if(::connect(socketFd,(const struct sockaddr*)&senderAddress,sizeof(struct sockaddr_in))==-1)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to connect to requester");
	}

bool UDPSocket::waitForMessage(const Misc::Time& timeout) const
	{
	Misc::FdSet readFds(socketFd);
	return Misc::pselect(&readFds,0,0,&timeout)>=0&&readFds.isSet(socketFd);
	}

size_t UDPSocket::receiveMessage(void* messageBuffer,size_t messageSize,IPv4SocketAddress& senderAddress)
	{
	/* Receive a message: */
	ssize_t recvResult;
	socklen_t senderAddressSize;
	do
		{
		senderAddressSize=sizeof(IPv4SocketAddress);
		recvResult=recvfrom(socketFd,messageBuffer,messageSize,0,(struct sockaddr*)&senderAddress,&senderAddressSize);
		}
	while(recvResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	
	/* Handle the result from the recv call: */
	if(recvResult<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to receive message");
	
	/* If the sender address had a mismatching size, null it out: */
	if(senderAddressSize!=sizeof(IPv4SocketAddress))
		senderAddress=IPv4SocketAddress();
	
	return size_t(recvResult);
	}

size_t UDPSocket::receiveMessage(void* messageBuffer,size_t messageSize)
	{
	/* Receive a message: */
	ssize_t recvResult;
	do
		{
		recvResult=recv(socketFd,messageBuffer,messageSize,0);
		}
	while(recvResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	
	/* Handle the result from the recv call: */
	if(recvResult<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to receive message");
	
	return size_t(recvResult);
	}

void UDPSocket::sendMessage(const void* messageBuffer,size_t messageSize,const IPv4SocketAddress& recipientAddress)
	{
	ssize_t sendResult;
	do
		{
		sendResult=sendto(socketFd,messageBuffer,messageSize,0,(const struct sockaddr*)&recipientAddress,sizeof(IPv4SocketAddress));
		}
	while(sendResult<0&&errno==EINTR);
	if(sendResult<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to send message");
	else if(size_t(sendResult)!=messageSize)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Message truncation from %u to %u",(unsigned int)messageSize,(unsigned int)sendResult);
	}

void UDPSocket::sendMessage(const void* messageBuffer,size_t messageSize)
	{
	ssize_t sendResult;
	do
		{
		sendResult=send(socketFd,messageBuffer,messageSize,0);
		}
	while(sendResult<0&&errno==EINTR);
	if(sendResult<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to send message");
	else if(size_t(sendResult)!=messageSize)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Message truncation from %u to %u",(unsigned int)messageSize,(unsigned int)sendResult);
	}

}

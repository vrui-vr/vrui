/***********************************************************************
UNIXPipe - Class for high-performance reading/writing from/to connected
UNIX domain sockets.
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

#include <Comm/UNIXPipe.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Misc/FdSet.h>
#include <Comm/ListeningUNIXSocket.h>

namespace Comm {

/*************************
Methods of class UNIXPipe:
*************************/

size_t UNIXPipe::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Read more data from source: */
	ssize_t readResult;
	do
		{
		readResult=::read(fd,buffer,bufferSize);
		}
	while(readResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	
	/* Handle the result from the read call: */
	if(readResult<0)
		{
		/* Unknown error; probably a bad thing: */
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot read from pipe"));
		}
	
	return size_t(readResult);
	}

void UNIXPipe::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	while(bufferSize>0)
		{
		/* Write data to the sink: */
		ssize_t writeResult=::write(fd,buffer,bufferSize);
		if(writeResult>0)
			{
			/* Prepare to write more data: */
			buffer+=writeResult;
			bufferSize-=writeResult;
			}
		else if(writeResult==0)
			{
			/* Sink has reached end-of-file: */
			throw WriteError(__PRETTY_FUNCTION__,bufferSize);
			}
		else if(errno==EPIPE)
			{
			/* Other side hung up: */
			throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Connection terminated by peer"));
			}
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
			{
			/* Unknown error; probably a bad thing: */
			throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot write to pipe"));
			}
		}
	}

size_t UNIXPipe::writeDataUpTo(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Write data to the sink: */
	ssize_t writeResult;
	do
		{
		writeResult=::write(fd,buffer,bufferSize);
		}
	while(writeResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	if(writeResult>0)
		return size_t(writeResult);
	else if(writeResult==0)
		{
		/* Sink has reached end-of-file: */
		throw WriteError(__PRETTY_FUNCTION__,bufferSize);
		}
	else if(errno==EPIPE)
		{
		/* Other side hung up: */
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Connection terminated by peer"));
		}
	else
		{
		/* Unknown error; probably a bad thing: */
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot write to pipe"));
		}
	}

UNIXPipe::UNIXPipe(const char* socketName,bool abstract)
	:Comm::Pipe(ReadWrite),
	 fd(-1)
	{
	/* Create the socket: */
	fd=socket(AF_UNIX,SOCK_STREAM,0);
	if(fd<0)
		throw OpenError(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot create socket"));
	
	/* Connect the socket to the requested UNIX domain socket in the abstract or regular namespace: */
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
		/* Set the regular socket path: */
		strncpy(socketAddress.sun_path,socketName,sizeof(socketAddress.sun_path)-1);
		}
	if(connect(fd,reinterpret_cast<sockaddr*>(&socketAddress),sizeof(socketAddress))<0)
		{
		/* Destroy the socket and throw an exception: */
		int myerrno=errno;
		close(fd);
		throw OpenError(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,myerrno,"Cannot connect to socket %s",socketName));
		}
	}

UNIXPipe::UNIXPipe(ListeningUNIXSocket& listenSocket)
	:Comm::Pipe(ReadWrite),
	 fd(-1)
	{
	/* Wait for a connection attempt on the listening socket: */
	fd=accept(listenSocket.getFd(),0,0);
	if(fd<0)
		throw OpenError(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot accept connection from listening socket"));
	}

UNIXPipe::~UNIXPipe(void)
	{
	try
		{
		/* Flush the write buffer: */
		flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Print an error message and carry on: */
		Misc::userError(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Caught exception \"%s\" while closing pipe",err.what()).c_str());
		}
	
	/* Close the socket: */
	if(fd>=0)
		close(fd);
	}

int UNIXPipe::getFd(void) const
	{
	return fd;
	}

bool UNIXPipe::waitForData(void) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,0)>=0&&readFds.isSet(fd);
	}

bool UNIXPipe::waitForData(const Misc::Time& timeout) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	}

void UNIXPipe::shutdown(bool read,bool write)
	{
	/* Flush the write buffer: */
	flush();
	
	/* Shut down the socket: */
	if(read&&write)
		::shutdown(fd,SHUT_RDWR);
	else if(read)
		::shutdown(fd,SHUT_RD);
	else if(write)
		::shutdown(fd,SHUT_WR);
	}

namespace {

/***********
Helper data:
***********/

static const Misc::UInt32 fileDescriptorMessageTag=0x53454446U; // "FDES" in little-endian ASCII

}

int UNIXPipe::readFd(void)
	{
	/* Write a bit of data to the sender to synchronize: */
	write(fileDescriptorMessageTag);
	flush();
	
	/* Create an I/O structure to read a message alongside a file descriptor: */
	Misc::UInt32 data=0x0U;
	iovec iov[1];
	iov[0].iov_base=&data;
	iov[0].iov_len=sizeof(data);
	msghdr msg;
	msg.msg_name=0;
	msg.msg_namelen=0;
	msg.msg_iov=iov;
	msg.msg_iovlen=1;
	union
		{
		cmsghdr cMsgHdr;
		char control[CMSG_SPACE(sizeof(int))];
		} cMsgU;
	msg.msg_control=cMsgU.control;
	msg.msg_controllen=sizeof(cMsgU.control);
	
	/* Read the next message: */
	ssize_t recvResult=recvmsg(fd,&msg,0);
	if(recvResult<0)
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot read file descriptor"));
	if(size_t(recvResult)!=sizeof(data)||data!=fileDescriptorMessageTag)
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Cannot read file descriptor due to mismatching message data"));
	cmsghdr* cmsg=CMSG_FIRSTHDR(&msg);
	if(cmsg==0||cmsg->cmsg_len!=CMSG_LEN(sizeof(int))||cmsg->cmsg_level!=SOL_SOCKET||cmsg->cmsg_type!=SCM_RIGHTS)
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Cannot read file descriptor due to malformed control message"));
	
	return *reinterpret_cast<int*>(CMSG_DATA(cmsg));
	}

void UNIXPipe::writeFd(int wFd)
	{
	/* Flush the current write buffer: */
	flush();
	
	/* Read a bit of data from the receiver to synchronize: */
	if(read<Misc::UInt32>()!=fileDescriptorMessageTag)
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Cannot read file descriptor request due to mismatching message data"));
	
	/* Create an I/O structure to write a message alongside a file descriptor: */
	Misc::UInt32 data=fileDescriptorMessageTag;
	iovec iov[1];
	iov[0].iov_base=&data;
	iov[0].iov_len=sizeof(data);
	msghdr msg;
	msg.msg_name=0;
	msg.msg_namelen=0;
	msg.msg_iov=iov;
	msg.msg_iovlen=1;
	union
		{
		cmsghdr cMsgHdr;
		char control[CMSG_SPACE(sizeof(int))];
		} cMsgU;
	msg.msg_control=cMsgU.control;
	msg.msg_controllen=sizeof(cMsgU.control);
	cmsghdr* cmsg=CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len=CMSG_LEN(sizeof(int));
	cmsg->cmsg_level=SOL_SOCKET;
	cmsg->cmsg_type=SCM_RIGHTS;
	*reinterpret_cast<int*>(CMSG_DATA(cmsg))=wFd;
	
	/* Send the message: */
	ssize_t sendResult=sendmsg(fd,&msg,0);
	if(sendResult<0)
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot write file descriptor"));
	if(size_t(sendResult)!=sizeof(data))
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Cannot write file descriptor due to mismatching message data size"));
	}

std::string UNIXPipe::getAddress(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_un socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot query socket address"));
	
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

std::string UNIXPipe::getPeerAddress(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_un socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getpeername(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot query peer's socket address"));
	
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

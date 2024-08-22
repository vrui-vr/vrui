/***********************************************************************
SerialPort - Class for high-performance reading/writing from/to serial
ports.
Copyright (c) 2001-2024 Oliver Kreylos

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

#include <Comm/SerialPort.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <Misc/StdError.h>
#include <Misc/FdSet.h>

/* Check if ioctl calls are undefined (on BSD-likes), then redefine: */
#ifndef TIOCMGET
#define TIOCMGET TIOCMODG
#endif
#ifndef TIOCMSET
#define TIOCMSET TIOCMODS
#endif

namespace Comm {

/***************************
Methods of class SerialPort:
***************************/

size_t SerialPort::readData(IO::File::Byte* buffer,size_t bufferSize)
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
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Cannot read from port"));
		}
	
	return size_t(readResult);
	}

void SerialPort::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	while(bufferSize>0)
		{
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
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
			{
			/* Unknown error; probably a bad thing: */
			throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Cannot write to port"));
			}
		}
	}

size_t SerialPort::writeDataUpTo(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Write data from the given buffer: */
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
	else
		{
		/* Unknown error; probably a bad thing: */
		throw Error(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Cannot write to port"));
		}
	}

SerialPort::SerialPort(const char* deviceName,bool nonBlocking)
	:Pipe(ReadWrite),
	 fd(-1)
	{
	/* Open the device file: */
	int openFlags=O_RDWR|O_NOCTTY;
	if(nonBlocking)
		openFlags|=O_NONBLOCK;
	fd=open(deviceName,openFlags);
	if(fd<0)
		throw OpenError(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot open device %s",deviceName));
	
	/* Configure as "raw" port: */
	struct termios term;
	tcgetattr(fd,&term);
	cfmakeraw(&term);
	term.c_iflag|=IGNBRK; // Don't generate signals
	term.c_cflag|=CREAD|CLOCAL; // Enable receiver; no modem line control
	term.c_cc[VMIN]=1; // Block read() until at least a single byte is read
	term.c_cc[VTIME]=0; // No timeout on read()
	if(tcsetattr(fd,TCSANOW,&term)!=0)
		throw OpenError(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot configure device %s",deviceName));
	
	/* Flush both queues: */
	tcflush(fd,TCIFLUSH);
	tcflush(fd,TCOFLUSH);
	}

SerialPort::~SerialPort(void)
	{
	if(fd>=0)
		close(fd);
	}

int SerialPort::getFd(void) const
	{
	return fd;
	}

bool SerialPort::waitForData(void) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,0)>=0&&readFds.isSet(fd);
	}

bool SerialPort::waitForData(const Misc::Time& timeout) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	}

void SerialPort::shutdown(bool read,bool write)
	{
	/* Flush the write buffer: */
	flush();
	
	if(write)
		{
		/* Drain the port's buffer: */
		tcdrain(fd);
		}
	}

void SerialPort::setPortSettings(int portSettingsMask)
	{
	/* Retrieve current flags: */
	int fileFlags=fcntl(fd,F_GETFL);
	if(fileFlags<0)
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot read device configuration"));
	
	/* Change flags according to given parameter: */
	if(portSettingsMask&NonBlocking)
		fileFlags|=FNDELAY|FNONBLOCK;
	else
		fileFlags&=~(FNDELAY|FNONBLOCK);
	
	/* Set new flags: */
	if(fcntl(fd,F_SETFL,fileFlags)!=0)
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot configure device"));
	}

namespace {

/****************
Helper functions:
****************/

struct termios tcgetattr(int fd,const char* source)
	{
	struct termios term;
	if(tcgetattr(fd,&term)!=0)
		throw IO::File::Error(Misc::makeLibcErrMsg(source,errno,"Cannot get terminal settings"));
	return term;
	}

void tcsetattr(int fd,int optional_actions,const struct termios& term,const char* source)
	{
	if(tcsetattr(fd,optional_actions,&term)!=0)
		throw IO::File::Error(Misc::makeLibcErrMsg(source,errno,"Cannot set terminal settings"));
	}

}

void SerialPort::setSerialSettings(int bitRate,int charLength,SerialPort::Parity parity,int numStopbits,bool enableHandshake)
	{
	/* Initialize a termios structure: */
	struct termios term=tcgetattr(fd,__PRETTY_FUNCTION__);
	
	/* Set rate of bits per second: */
	#ifdef __SGI_IRIX__
	term.c_ospeed=bitRate;
	term.c_ispeed=bitRate;
	#else
	/* Find the closest existing bitrate to the given one: */
	static speed_t bitRates[][2]={{0,B0},{50,B50},{75,B75},{110,B110},{134,B134},{150,B150},
	                              {200,B200},{300,B300},{600,B600},{1200,B1200},{1800,B1800},
	                              {2400,B2400},{4800,B4800},{9600,B9600},{19200,B19200},
	                              {38400,B38400},{57600,B57600},{115200,B115200},{230400,B230400}};
	int l=0;
	int r=19;
	while(r-l>1)
		{
		int m=(l+r)>>1;
		if(speed_t(bitRate)>=bitRates[m][0])
			l=m;
		else
			r=m;
		}
	cfsetspeed(&term,bitRates[l][1]);
	#endif
	
	/* Set character size: */
	term.c_cflag&=~CSIZE;
	switch(charLength)
		{
		case 5:
			term.c_cflag|=CS5;
			break;
		
		case 6:
			term.c_cflag|=CS6;
			break;
		
		case 7:
			term.c_cflag|=CS7;
			break;
		
		case 8:
			term.c_cflag|=CS8;
			break;
		}
	
	/* Set parity settings: */
	term.c_cflag&=~(PARENB|PARODD);
	switch(parity)
		{
		case OddParity:
			term.c_cflag|=PARENB|PARODD;
			break;
		
		case EvenParity:
			term.c_cflag|=PARENB;
			break;
		
		default:
			;
		}
	
	/* Set stop bit settings: */
	term.c_cflag&=~CSTOPB;
	if(numStopbits==2)
		term.c_cflag|=CSTOPB;
	
	/* Set handshake settings: */
	#ifdef __SGI_IRIX__
	term.c_cflag&=~CNEW_RTSCTS;
	#else
	term.c_cflag&=~CRTSCTS;
	#endif
	if(enableHandshake)
		{
		#ifdef __SGI_IRIX__
		term.c_cflag|=CNEW_RTSCTS;
		#else
		term.c_cflag|=CRTSCTS;
		#endif
		}
		
	/* Configure the port: */
	tcsetattr(fd,TCSADRAIN,term,__PRETTY_FUNCTION__);
	}

void SerialPort::setRawMode(int minNumBytes,int timeOut)
	{
	/* Read the current port settings: */
	struct termios term=tcgetattr(fd,__PRETTY_FUNCTION__);
	
	/* Disable canonical mode: */
	term.c_lflag&=~ICANON;
	
	/* Set the min/time parameters: */
	term.c_cc[VMIN]=cc_t(minNumBytes);
	term.c_cc[VTIME]=cc_t(timeOut);
	
	/* Configure the port: */
	tcsetattr(fd,TCSANOW,term,__PRETTY_FUNCTION__);
	}

void SerialPort::setCanonicalMode(void)
	{
	/* Read the current port settings: */
	struct termios term=tcgetattr(fd,__PRETTY_FUNCTION__);
	
	/* Enable canonical mode: */
	term.c_lflag|=ICANON;
	
	/* Configure the port: */
	tcsetattr(fd,TCSANOW,term,__PRETTY_FUNCTION__);
	}

void SerialPort::setLineControl(bool respectModemLines,bool hangupOnClose)
	{
	/* Read the current port settings: */
	struct termios term=tcgetattr(fd,__PRETTY_FUNCTION__);
	
	if(respectModemLines)
		term.c_cflag&=~CLOCAL;
	else
		term.c_cflag|=CLOCAL;
	if(hangupOnClose)
		term.c_cflag|=HUPCL;
	else
		term.c_cflag&=~HUPCL;
	
	/* Configure the port: */
	tcsetattr(fd,TCSANOW,term,__PRETTY_FUNCTION__);
	}

namespace {

/****************
Helper functions:
****************/

int getControlBits(int fd,const char* source)
	{
	/* Read the current state of the control bits: */
	int controlBits;
	if(ioctl(fd,TIOCMGET,&controlBits)<0)
		throw IO::File::Error(Misc::makeLibcErrMsg(source,errno,"Cannot get device control bits"));
	
	return controlBits;
	}

void setControlBits(int fd,int controlBits,const char* source)
	{
	/* Set the state of the control bits: */
	if(ioctl(fd,TIOCMSET,&controlBits)<0)
		throw IO::File::Error(Misc::makeLibcErrMsg(source,errno,"Cannot set device control bits"));
	}

}

bool SerialPort::getRTS(void)
	{
	/* Read the current state of the control bits: */
	int controlBits=getControlBits(fd,__PRETTY_FUNCTION__);
	
	/* Return the RTS bit's state: */
	return (controlBits&TIOCM_RTS)!=0;
	}

bool SerialPort::setRTS(bool newRTS)
	{
	/* Read the current state of the control bits: */
	int controlBits=getControlBits(fd,__PRETTY_FUNCTION__);
	bool result=(controlBits&TIOCM_RTS)!=0;
	
	/* Set the RTS bit: */
	if(newRTS)
		controlBits|=TIOCM_RTS;
	else
		controlBits&=~TIOCM_RTS;
	
	/* Set the state of the control bits: */
	setControlBits(fd,controlBits,__PRETTY_FUNCTION__);
	
	/* Return the RTS bit's previous state: */
	return result;
	}

bool SerialPort::getCTS(void)
	{
	/* Read the current state of the control bits: */
	int controlBits=getControlBits(fd,__PRETTY_FUNCTION__);
	
	/* Return the CTS bit's state: */
	return (controlBits&TIOCM_CTS)!=0;
	}

bool SerialPort::setCTS(bool newCTS)
	{
	/* Read the current state of the control bits: */
	int controlBits=getControlBits(fd,__PRETTY_FUNCTION__);
	bool result=(controlBits&TIOCM_CTS)!=0;
	
	/* Set the CTS bit: */
	if(newCTS)
		controlBits|=TIOCM_CTS;
	else
		controlBits&=~TIOCM_CTS;
	
	/* Set the state of the control bits: */
	setControlBits(fd,controlBits,__PRETTY_FUNCTION__);
	
	/* Return the CTS bit's previous state: */
	return result;
	}

}

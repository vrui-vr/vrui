/***********************************************************************
File - Base class for high-performance buffered binary read/write access
to file-like objects.
Copyright (c) 2010-2024 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#define DEBUGGING 0

#include <IO/File.h>

#include <string.h>
#if DEBUGGING
#include <iostream>
#endif
#include <Misc/StdError.h>

namespace IO {

/********************************
Methods of class File::ReadError:
********************************/

File::ReadError::ReadError(const char* source,size_t sNumMissingBytes)
	:Error(Misc::makeStdErrMsg(source,"Short read by %u bytes",(unsigned int)sNumMissingBytes)),
	 numMissingBytes(sNumMissingBytes)
	{
	}

/*************************************
Methods of class File::UngetCharError:
*************************************/

File::UngetCharError::UngetCharError(const char* source)
	:Error(Misc::makeStdErrMsg(source,"Unget buffer is full"))
	{
	}

/*********************************
Methods of class File::WriteError:
*********************************/

File::WriteError::WriteError(const char* source,size_t sNumMissingBytes)
	:Error(Misc::makeStdErrMsg(source,"Short write by %u bytes",(unsigned int)sNumMissingBytes)),
	 numMissingBytes(sNumMissingBytes)
	{
	}

/*********************
Methods of class File:
*********************/

File::AccessMode File::disableRead(File::AccessMode accessMode)
	{
	switch(accessMode)
		{
		case WriteOnly:
		case ReadWrite:
			return WriteOnly;
		
		default:
			return NoAccess;
		}
	}

File::AccessMode File::disableWrite(File::AccessMode accessMode)
	{
	switch(accessMode)
		{
		case ReadOnly:
		case ReadWrite:
			return ReadOnly;
		
		default:
			return NoAccess;
		}
	}

const char* File::getAccessModeName(File::AccessMode accessMode)
	{
	const char* result=0;
	switch(accessMode)
		{
		case NoAccess:
			result="nothing";
			break;
		
		case ReadOnly:
			result="reading";
			break;

		case WriteOnly:
			result="writing";
			break;

		case ReadWrite:
			result="reading/writing";
			break;
		}
	
	return result;
	}

void File::setReadBuffer(size_t newReadBufferSize,File::Byte* newReadBuffer,bool deleteOldBuffer)
	{
	/* Delete the previous buffer if requested: */
	if(deleteOldBuffer)
		delete[] readBuffer;
	
	/* Install the new read buffer: */
	readBufferSize=newReadBufferSize;
	readBuffer=newReadBuffer;
	readDataEnd=readBuffer;
	readPtr=readBuffer;
	}

void File::setWriteBuffer(size_t newWriteBufferSize,File::Byte* newWriteBuffer,bool deleteOldBuffer)
	{
	/* Delete the previous buffer if requested: */
	if(deleteOldBuffer)
		delete[] writeBuffer;
	
	/* Install the new write buffer: */
	writeBufferSize=newWriteBufferSize;
	writeBuffer=newWriteBuffer;
	writeBufferEnd=writeBuffer+writeBufferSize;
	writePtr=writeBuffer;
	}

size_t File::readData(File::Byte* buffer,size_t bufferSize)
	{
	/* Return end-of-file indicator: */
	return 0;
	}

void File::writeData(const File::Byte* buffer,size_t bufferSize)
	{
	/* Throw write error: */
	throw WriteError(__PRETTY_FUNCTION__,bufferSize);
	}

size_t File::writeDataUpTo(const File::Byte* buffer,size_t bufferSize)
	{
	/* Throw write error: */
	throw WriteError(__PRETTY_FUNCTION__,bufferSize);
	}

void File::bufferedRead(void* buffer,size_t bufferSize)
	{
	#if DEBUGGING
	std::cout<<"Reading from "<<this<<std::endl;
	#endif
	
	/* Copy the first bit of data from the read buffer: */
	Byte* bufPtr=static_cast<Byte*>(buffer);
	size_t copySize=readDataEnd-readPtr;
	if(copySize>0)
		{
		memcpy(bufPtr,readPtr,copySize);
		bufPtr+=copySize;
		bufferSize-=copySize;
		readPtr=readDataEnd;
		}
	
	/* Bypass the read buffer if supported and if there is a lot of data left to read: */
	if(canReadThrough&&bufferSize>=readBufferSize/2)
		{
		/* Read directly from the source: */
		while(bufferSize>0)
			{
			/* Read directly into the provided buffer: */
			size_t readSize=readData(bufPtr,bufferSize);
			
			/* Check for premature end-of-file: */
			if(readSize==0)
				{
				haveEof=true;
				throw ReadError(__PRETTY_FUNCTION__,bufferSize);
				}
			
			/* Prepare to read more data: */
			bufPtr+=readSize;
			bufferSize-=readSize;
			}
		}
	else
		{
		/* Read through the read buffer: */
		while(bufferSize>0)
			{
			/* Read more data: */
			fillReadBuffer();
			
			/* Check for premature end-of-file: */
			if(haveEof&&readPtr==readDataEnd)
				throw ReadError(__PRETTY_FUNCTION__,bufferSize);
			
			/* Copy data from the buffer: */
			size_t copySize=readDataEnd-readPtr;
			if(copySize>bufferSize)
				copySize=bufferSize;
			memcpy(bufPtr,readPtr,copySize);
			bufPtr+=copySize;
			bufferSize-=copySize;
			
			/* Advance the read position: */
			readPtr+=copySize;
			}
		}
	}

void File::bufferedSkip(size_t skipSize)
	{
	/* Skip the first bit of data in the read buffer: */
	skipSize-=readDataEnd-readPtr;
	
	/* Read through the read buffer: */
	while(skipSize>0)
		{
		/* Read more data: */
		fillReadBuffer();
		
		/* Check for premature end-of-file: */
		if(haveEof&&readPtr==readDataEnd)
			throw ReadError(__PRETTY_FUNCTION__,skipSize);
		
		/* Skip data from the buffer: */
		size_t copySize=readDataEnd-readPtr;
		if(copySize>skipSize)
			copySize=skipSize;
		skipSize-=copySize;
		
		/* Advance the read position: */
		readPtr+=copySize;
		}
	}

void File::bufferedWrite(const void* buffer,size_t bufferSize)
	{
	#if DEBUGGING
	std::cout<<"Writing to "<<this<<std::endl;
	#endif
	
	const Byte* bufPtr=static_cast<const Byte*>(buffer);
	
	/* Copy the first chunk of data into the write buffer: */
	size_t copySize=writeBufferEnd-writePtr;
	memcpy(writePtr,bufPtr,copySize);
	bufPtr+=copySize;
	bufferSize-=copySize;
	
	/* Write the full write buffer: */
	writeData(writeBuffer,writeBufferSize);
	writePtr=writeBuffer;
	
	/* Bypass the write buffer if supported and if there is a lot of data left to write: */
	if(bufferSize<writeBufferSize/2)
		{
		/* Copy the rest of the data into the write buffer: */
		memcpy(writePtr,bufPtr,bufferSize);
		writePtr+=bufferSize;
		}
	else if(canWriteThrough)
		{
		/* Write the rest of the data directly to the sink: */
		writeData(bufPtr,bufferSize);
		}
	else
		{
		/* Copy the rest of the data into the write buffer in multiple steps: */
		while(bufferSize>0)
			{
			/* Write the write buffer to sink if it is full: */
			if(writePtr==writeBufferEnd)
				{
				writeData(writeBuffer,writeBufferSize);
				writePtr=writeBuffer;
				}
			
			/* Copy the next chunk of data: */
			size_t copySize=writeBufferEnd-writePtr;
			if(copySize>bufferSize)
				copySize=bufferSize;
			memcpy(writePtr,bufPtr,copySize);
			bufPtr+=copySize;
			bufferSize-=copySize;
			writePtr+=copySize;
			}
		}
	}

File::File(void)
	:readBufferSize(0),readBuffer(0),readDataEnd(0),haveEof(false),readPtr(0),
	 canReadThrough(true),readMustSwapEndianness(false),
	 writeBufferSize(0),writeBuffer(0),writeBufferEnd(0),writePtr(0),
	 canWriteThrough(true),writeMustSwapEndianness(false)
	{
	#if DEBUGGING
	std::cout<<"Created File object at "<<this<<std::endl;
	#endif
	}

File::File(File::AccessMode sAccessMode)
	:readBufferSize(0),readBuffer(0),readDataEnd(0),haveEof(false),readPtr(0),
	 canReadThrough(true),readMustSwapEndianness(false),
	 writeBufferSize(0),writeBuffer(0),writeBufferEnd(0),writePtr(0),
	 canWriteThrough(true),writeMustSwapEndianness(false)
	{
	if(sAccessMode==ReadOnly||sAccessMode==ReadWrite)
		{
		/* Create a default read buffer: */
		readBufferSize=8192;
		readBuffer=new Byte[readBufferSize];
		readDataEnd=readBuffer;
		readPtr=readBuffer;
		}
	
	if(sAccessMode==WriteOnly||sAccessMode==ReadWrite)
		{
		/* Create a default write buffer: */
		writeBufferSize=8192;
		writeBuffer=new Byte[writeBufferSize];
		writeBufferEnd=writeBuffer+writeBufferSize;
		writePtr=writeBuffer;
		}
	#if DEBUGGING
	std::cout<<"Created File object at "<<this<<std::endl;
	#endif
	}

File::~File(void)
	{
	/* Delete the read and write buffers: */
	delete[] readBuffer;
	delete[] writeBuffer;
	#if DEBUGGING
	std::cout<<"Destroyed File object at "<<this<<std::endl;
	#endif
	}

int File::getFd(void) const
	{
	/* Default behavior is not supported: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"File does not have file descriptor");
	
	/* Just to make compiler happy: */
	return -1;
	}

size_t File::getReadBufferSize(void) const
	{
	return readBufferSize;
	}

size_t File::getWriteBufferSize(void) const
	{
	return writeBufferSize;
	}

size_t File::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Ensure that the new read buffer can hold currently unread data: */
	size_t unreadDataSize=readDataEnd-readPtr;
	if(newReadBufferSize<unreadDataSize)
		newReadBufferSize=unreadDataSize;
	
	/* Allocate the new read buffer: */
	readBufferSize=newReadBufferSize;
	Byte* newReadBuffer=new Byte[readBufferSize];
	
	/* Copy unread data from the previous read buffer: */
	if(unreadDataSize>0)
		memcpy(newReadBuffer,readPtr,unreadDataSize);
	delete[] readBuffer;
	readBuffer=newReadBuffer;
	
	/* Reset the buffer pointers: */
	readDataEnd=readBuffer+unreadDataSize;
	readPtr=readBuffer;
	
	return readBufferSize;
	}

void File::resizeWriteBuffer(size_t newWriteBufferSize)
	{
	/* Write the entire write buffer if there is any data in it: */
	if(writePtr!=writeBuffer)
		writeData(writeBuffer,writePtr-writeBuffer);
	
	/* Re-allocate the write buffer: */
	delete[] writeBuffer;
	writeBufferSize=newWriteBufferSize;
	writeBuffer=new Byte[writeBufferSize];
	writeBufferEnd=writeBuffer+writeBufferSize;
	writePtr=writeBuffer;
	}

size_t File::readSomeData(void)
	{
	/* Calculate size of unread data currently in buffer and bail out if buffer is already full: */
	size_t unreadDataSize=readDataEnd-readPtr;
	if(unreadDataSize==readBufferSize)
		return unreadDataSize;
	
	/* Move unread data to the beginning of the read buffer if that opens a lot of space: */
	if(unreadDataSize>0)
		{
		/* Only move when there's more room in the front than in the back: */
		size_t roomFront=readPtr-readBuffer;
		size_t roomEnd=(readBuffer+readBufferSize)-readDataEnd;
		if(roomFront>roomEnd)
			{
			/* Move unread data to beginning of read buffer (needs to use memmove as data blocks might overlap): */
			memmove(readBuffer,readPtr,unreadDataSize);
			
			/* Update the read buffer pointers: */
			readPtr=readBuffer;
			readDataEnd=readBuffer+unreadDataSize;
			}
		}
	else
		{
		/* Buffer is empty; reset pointers: */
		readPtr=readBuffer;
		readDataEnd=readBuffer;
		}
	
	/* Read more data: */
	size_t readSize=readData(readDataEnd,readBufferSize-unreadDataSize);
	
	/* Update the read buffer pointers and check for end-of-file: */
	readDataEnd+=readSize;
	haveEof=readSize==0;
	
	/* Return new amount of unread data in read buffer: */
	return unreadDataSize+readSize;
	}

size_t File::writeSomeData(void)
	{
	/* Calculate available space in buffer and bail out if buffer is already empty: */
	size_t bufferSpace=writeBufferEnd-writePtr;
	if(bufferSpace==writeBufferSize)
		return bufferSpace;
	
	/* Write some data from the beginning of the buffer: */
	size_t writeSize=writeDataUpTo(writeBuffer,writePtr-writeBuffer);
	
	/* Move the remaining unwritten data to the beginning of the write buffer: */
	memmove(writeBuffer,writeBuffer+writeSize,writePtr-(writeBuffer+writeSize));
	
	/* Update the write buffer pointers: */
	writePtr-=writeSize;
	
	/* Return the new amount of free space in the buffer: */
	return bufferSpace+writeSize;
	}

void File::setEndianness(Misc::Endianness newEndianness)
	{
	/* Remember whether reading and writing require endianness swaps: */
	readMustSwapEndianness=writeMustSwapEndianness=Misc::mustSwapEndianness(newEndianness);
	}

void File::setSwapOnRead(bool newSwapOnRead)
	{
	readMustSwapEndianness=newSwapOnRead;
	}

void File::setSwapOnWrite(bool newSwapOnWrite)
	{
	writeMustSwapEndianness=newSwapOnWrite;
	}

}

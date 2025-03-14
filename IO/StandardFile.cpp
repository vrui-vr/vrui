/***********************************************************************
StandardFile - Class for high-performance reading/writing from/to
standard operating system files.
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

#include <IO/StandardFile.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <Misc/StdError.h>

#ifdef __APPLE__
#define lseek64 lseek
#endif

namespace IO {

/*****************************
Methods of class StandardFile:
*****************************/

size_t StandardFile::readData(File::Byte* buffer,size_t bufferSize)
	{
	/* Check if file needs to be repositioned: */
	if(filePos!=readPos)
		if(lseek64(fd,readPos,SEEK_SET)<0)
			throw SeekError(__PRETTY_FUNCTION__,errno,readPos);
	
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
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot read from file"));
		}
	
	/* Advance the read pointer: */
	readPos+=readResult;
	filePos=readPos;
	
	return size_t(readResult);
	}

void StandardFile::writeData(const File::Byte* buffer,size_t bufferSize)
	{
	/* Check if file needs to be repositioned: */
	if(filePos!=writePos)
		if(lseek64(fd,writePos,SEEK_SET)<0)
			throw SeekError(__PRETTY_FUNCTION__,errno,writePos);
	
	/* Invalidate the read buffer to prevent reading stale data: */
	flushReadBuffer();
	
	/* Write all data in the given buffer: */
	while(bufferSize>0)
		{
		ssize_t writeResult=::write(fd,buffer,bufferSize);
		if(writeResult>0)
			{
			/* Prepare to write more data: */
			buffer+=writeResult;
			bufferSize-=writeResult;
			
			/* Advance the write pointer: */
			writePos+=writeResult;
			filePos=writePos;
			}
		else if(writeResult==0)
			{
			/* Sink has reached end-of-file: */
			throw WriteError(__PRETTY_FUNCTION__,bufferSize);
			}
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
			{
			/* Unknown error; probably a bad thing: */
			throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot write to file"));
			}
		}
	}

size_t StandardFile::writeDataUpTo(const File::Byte* buffer,size_t bufferSize)
	{
	/* Check if file needs to be repositioned: */
	if(filePos!=writePos)
		if(lseek64(fd,writePos,SEEK_SET)<0)
			throw SeekError(__PRETTY_FUNCTION__,errno,writePos);
	
	/* Invalidate the read buffer to prevent reading stale data: */
	flushReadBuffer();
	
	/* Write data from the given buffer: */
	ssize_t writeResult;
	do
		{
		writeResult=::write(fd,buffer,bufferSize);
		}
	while(writeResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	if(writeResult>0)
		{
		/* Advance the write pointer: */
		writePos+=writeResult;
		filePos=writePos;
		
		return size_t(writeResult);
		}
	else if(writeResult==0)
		{
		/* Sink has reached end-of-file: */
		throw WriteError(__PRETTY_FUNCTION__,bufferSize);
		}
	else
		{
		/* Unknown error; probably a bad thing: */
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot write to file"));
		}
	}

void StandardFile::openFile(const char* fileName,File::AccessMode accessMode,int flags,int mode)
	{
	/* Adjust flags according to access mode: */
	switch(accessMode)
		{
		case NoAccess:
			flags&=~(O_RDONLY|O_WRONLY|O_RDWR|O_CREAT|O_TRUNC|O_APPEND);
			break;
		
		case ReadOnly:
			flags&=~(O_WRONLY|O_RDWR|O_CREAT|O_TRUNC|O_APPEND);
			flags|=O_RDONLY;
			break;
		
		case WriteOnly:
			flags&=~(O_RDONLY|O_RDWR);
			flags|=O_WRONLY;
			break;
		
		case ReadWrite:
			flags&=~(O_RDONLY|O_WRONLY);
			flags|=O_RDWR;
			break;
		}
	
	/* Open the file: */
	fd=open(fileName,flags,mode);
	
	/* Check for errors and throw an exception: */
	if(fd<0)
		throw OpenError(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot open file %s for %s",fileName,getAccessModeName(accessMode)));
	}

StandardFile::StandardFile(const char* fileName,File::AccessMode accessMode)
	:SeekableFile(accessMode),
	 fd(-1),
	 filePos(0)
	{
	/* Create flags and mode to open the file: */
	int flags=O_CREAT;
	if(accessMode==WriteOnly)
		flags|=O_TRUNC;
	mode_t mode=S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	
	/* Open the file: */
	openFile(fileName,accessMode,flags,mode);
	}

StandardFile::StandardFile(const char* fileName,File::AccessMode accessMode,int flags,int mode)
	:SeekableFile(accessMode),
	 fd(-1),
	 filePos(0)
	{
	/* Open the file: */
	openFile(fileName,accessMode,flags,mode);
	}

StandardFile::StandardFile(int sFd,File::AccessMode accessMode)
	:SeekableFile(accessMode),
	 fd(sFd),
	 filePos(0)
	{
	}

StandardFile::~StandardFile(void)
	{
	/* Flush the write buffer, and then close the file: */
	flush();
	if(fd>=0)
		close(fd);
	}

int StandardFile::getFd(void) const
	{
	return fd;
	}

SeekableFile::Offset StandardFile::getSize(void) const
	{
	/* Get the file's total size: */
	struct stat statBuffer;
	if(fstat(fd,&statBuffer)<0)
		throw Error(Misc::makeLibcErrMsg(__PRETTY_FUNCTION__,errno,"Cannot query file size"));
	
	/* Return the file size: */
	return statBuffer.st_size;
	}

}

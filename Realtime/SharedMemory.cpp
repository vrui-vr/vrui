/***********************************************************************
SharedMemory - Class representing a block of POSIX shared memory. This
class is in the Realtime Processing Library because the required POSIX
shared memory functions are -- for some reason! -- included in the librt
library.
Copyright (c) 2015-2024 Oliver Kreylos

This file is part of the Realtime Processing Library (Realtime).

The Realtime Processing Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Realtime Processing Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Realtime Processing Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Realtime/SharedMemory.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <Misc/StdError.h>

namespace Realtime {

/*****************************
Methods of class SharedMemory:
*****************************/

SharedMemory::SharedMemory(const char* sName,size_t sSize)
	:name(sName),owner(true),
	 fd(shm_open(sName,O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)),
	 memory(0),size(sSize)
	{
	/* Throw an exception if the shared memory object could not be opened: */
	if(fd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create shared memory object %s",sName);
	
	/* Set the shared memory block's size: */
	if(ftruncate(fd,size)<0)
		{
		/* Destroy the shared memory block and throw an exception: */
		int error=errno;
		close(fd);
		shm_unlink(sName);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,error,"Cannot set size of shared memory object %s to %lu bytes",sName,(unsigned long)(size));
		}
	
	/* Map the shared memory segment into the process's address space: */
	void* address=mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(address==MAP_FAILED)
		{
		/* Destroy the shared memory block and throw an exception: */
		int error=errno;
		close(fd);
		shm_unlink(sName);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,error,"Cannot memory-map shared memory object %s of size %lu bytes",sName,(unsigned long)(size));
		}
	
	/* Get a byte-sized pointer to the shared memory segment: */
	memory=static_cast<char*>(address);
	}

SharedMemory::SharedMemory(const char* sName,bool write)
	:name(sName),owner(false),
	 fd(shm_open(sName,write?O_RDWR:O_RDONLY,0x0)),
	 memory(0),size(0)
	{
	/* Throw an exception if the shared memory object could not be opened: */
	if(fd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot open shared memory object %s",sName);
	
	/* Query the shared memory block's size: */
	struct stat sharedMemoryStats;
	if(fstat(fd,&sharedMemoryStats)<0)
		{
		/* Release the shared memory block and throw an exception: */
		int error=errno;
		close(fd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,error,"Cannot query size of shared memory object %s",sName);
		}
	size=size_t(sharedMemoryStats.st_size);
	
	/* Map the shared memory segment into the process's address space: */
	int prot=PROT_READ;
	if(write)
		prot|=PROT_WRITE;
	void* address=mmap(0,size,prot,MAP_SHARED,fd,0);
	if(address==MAP_FAILED)
		{
		/* Release the shared memory block and throw an exception: */
		int error=errno;
		close(fd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,error,"Cannot memory-map shared memory object %s of size %lu bytes",sName,(unsigned long)(size));
		}
	
	/* Get a byte-sized pointer to the shared memory segment: */
	memory=static_cast<char*>(address);
	}

SharedMemory::SharedMemory(int sFd,bool write)
	:owner(false),
	 fd(sFd),
	 memory(0),size(0)
	{
	/* Query the shared memory block's size: */
	struct stat sharedMemoryStats;
	if(fstat(fd,&sharedMemoryStats)<0)
		{
		/* Release the shared memory block and throw an exception: */
		int error=errno;
		close(fd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,error,"Cannot query size of anonymous shared memory object");
		}
	size=size_t(sharedMemoryStats.st_size);
	
	/* Map the shared memory segment into the process's address space: */
	int prot=PROT_READ;
	if(write)
		prot|=PROT_WRITE;
	void* address=mmap(0,size,prot,MAP_SHARED,fd,0);
	if(address==MAP_FAILED)
		{
		/* Release the shared memory block and throw an exception: */
		int error=errno;
		close(fd);
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,error,"Cannot memory-map anonymous shared memory object of size %lu bytes",(unsigned long)(size));
		}
	
	/* Get a byte-sized pointer to the shared memory segment: */
	memory=static_cast<char*>(address);
	}

SharedMemory::~SharedMemory(void)
	{
	/* Unmap the shared memory block: */
	munmap(memory,size);
	
	/* Close the underlying shared memory object: */
	close(fd);
	
	if(owner)
		{
		/* Destroy the shared memory block: */
		shm_unlink(name.c_str());
		}
	}

}

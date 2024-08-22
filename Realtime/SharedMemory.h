/***********************************************************************
SharedMemory - Class representing a block of POSIX shared memory. This
class is in the Realtime Processing Library because the required POSIX
shared memory functions are -- for some reason! -- included in the librt
library.
Copyright (c) 2015-2022 Oliver Kreylos

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

#ifndef REALTIME_SHAREDMEMORY_INCLUDED
#define REALTIME_SHAREDMEMORY_INCLUDED

#include <stddef.h>
#include <string>

namespace Realtime {

class SharedMemory
	{
	/* Elements: */
	private:
	std::string name; // Name of the shared memory block
	bool owner; // Flag if this object is the owner of the shared memory block, i.e., created it
	int fd; // File descriptor of the shared memory object backing the memory block
	char* memory; // Pointer to the memory block's first byte in the owning process's address space
	size_t size; // Memory block's size in bytes
	
	/* Constructors and destructors: */
	public:
	SharedMemory(const char* sName,size_t sSize); // Creates a shared memory block of the given name and size
	SharedMemory(const char* sName,bool write); // Opens an existing shared memory block for reading, or reading and writing if flag is true
	SharedMemory(int sFd,bool write); // Opens an existing shared memory block backed by a shared memory object of the given file descriptor for reading, or reading and writing if flag is true
	private:
	SharedMemory(const SharedMemory& source); // Prohibit copy constructor
	SharedMemory& operator=(const SharedMemory& source); // Prohibit assignment operator
	public:
	~SharedMemory(void); // Releases the shared memory block
	
	/* Methods: */
	int getFd(void) const // Returns the file descriptor of the underlying shared memory object
		{
		return fd;
		}
	size_t getSize(void) const // Returns the size of the shared memory segment
		{
		return size;
		}
	static ptrdiff_t align(ptrdiff_t offset) // Aligns a byte-based shared memory offset to place arbitrary objects
		{
		/* Round the offset up to the next multiple of the pointer type's size: */
		return ((offset+sizeof(ptrdiff_t)-1)/sizeof(ptrdiff_t))*sizeof(ptrdiff_t);
		}
	template <class ValueParam>
	const ValueParam* getValue(ptrdiff_t offset) const // Accesses a variable at a byte-based offset in shared memory, aligned to a pointer type
		{
		return reinterpret_cast<const ValueParam*>(memory+align(offset));
		}
	template <class ValueParam>
	ValueParam* getValue(ptrdiff_t offset) // Ditto
		{
		return reinterpret_cast<ValueParam*>(memory+align(offset));
		}
	};

}

#endif

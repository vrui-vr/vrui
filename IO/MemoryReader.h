/***********************************************************************
MemoryReader - Class to read from fixed-size memory blocks using a File
abstraction.
Copyright (c) 2021 Oliver Kreylos

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

#ifndef IO_MEMORYREADER_INCLUDED
#define IO_MEMORYREADER_INCLUDED

#include <sys/types.h>
#include <IO/SeekableFile.h>

namespace IO {

class MemoryReader:public SeekableFile
	{
	/* Elements: */
	private:
	size_t memSize; // Size of file's memory block
	const Byte* memBlock; // Pointer to file's memory block
	
	/* Constructors and destructors: */
	public:
	MemoryReader(const void* sMemBlock,size_t sMemSize); // Creates a file interface for the given memory block
	virtual ~MemoryReader(void);
	
	/* Methods from File: */
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	virtual void resizeWriteBuffer(size_t newWriteBufferSize);
	
	/* Methods from SeekableFile: */
	virtual Offset getSize(void) const;
	};

}

#endif

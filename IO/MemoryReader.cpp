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

#include <IO/MemoryReader.h>

namespace IO {

/*****************************
Methods of class MemoryReader:
*****************************/

MemoryReader::MemoryReader(const void* sMemBlock,size_t sMemSize)
	:SeekableFile(),
	 memSize(sMemSize),memBlock(static_cast<const Byte*>(sMemBlock))
	{
	/* Re-allocate the buffered file's buffers: */
	setReadBuffer(memSize,const_cast<Byte*>(memBlock),false); // I need to do something about this ugliness
	canReadThrough=false;
	
	/* Assume that the memory block has already been filled by the caller: */
	appendReadBufferData(memSize);
	readPos=memSize;
	}

MemoryReader::~MemoryReader(void)
	{
	/* Release the buffered file's buffers: */
	setReadBuffer(0,0,false);
	}

size_t MemoryReader::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Ignore it and return the full memory size: */
	return memSize;
	}

void MemoryReader::resizeWriteBuffer(size_t newWriteBufferSize)
	{
	/* Ignore it */
	}

SeekableFile::Offset MemoryReader::getSize(void) const
	{
	/* Return the file size: */
	return getReadBufferDataSize();
	}


}

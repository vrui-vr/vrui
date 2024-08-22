/***********************************************************************
Buffer - A simple low-overhead File-like interface for fixed-sized
buffers for low-level data exchange with kernel devices, without error
checking.
Copyright (c) 2024 Oliver Kreylos

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

#ifndef IO_BUFFER_INCLUDED
#define IO_BUFFER_INCLUDED

#include <stddef.h>
#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/Endianness.h>

namespace IO {

class Buffer
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt8 Byte; // Type for unsigned bytes
	
	/* Elements: */
	private:
	Byte* buffer; // Pointer to the buffer
	Byte* bufferEnd; // Pointer to the end of the buffer
	Byte* pos; // Current reading/writing position in the buffer
	bool mustSwapEndianness; // Flag whether the current endianness of the buffer differs from machine endianness
	
	/* Constructors and destructors: */
	public:
	Buffer(void* sBuffer,size_t sBufferSize) // Creates an IO buffer for the given memory block
		:buffer(static_cast<Byte*>(sBuffer)),
		 bufferEnd(buffer+sBufferSize),
		 pos(buffer),
		 mustSwapEndianness(false)
		{
		}
	
	/* Methods: */
	Byte* getBuffer(void) // Returns a pointer to the buffer
		{
		return buffer;
		}
	size_t getSize(void) const // Returns the size of the buffer
		{
		return size_t(bufferEnd-buffer);
		}
	Byte* getPtr(void) // Returns the current reading/writing position as a pointer
		{
		return pos;
		}
	ptrdiff_t getPos(void) const // Returns the current reading/writing position
		{
		return pos-buffer;
		}
	void zero(void) // Zeroes out the buffer from the current reading/writing position to the end
		{
		memset(pos,0,bufferEnd-pos);
		}
	void setPos(ptrdiff_t newPos) // Sets the reading/writing position to the given byte in the buffer
		{
		pos=buffer+newPos;
		}
	void setEndianness(Misc::Endianness newEndianness) // Sets the buffer's endianness for subsequent reading/writing
		{
		/* Remember whether endianness swapping is required: */
		mustSwapEndianness=Misc::mustSwapEndianness(newEndianness);
		}
	
	/* Reading interface: */
	void readRaw(void* data,size_t dataSize) // Reads a chunk of data from the buffer
		{
		/* Read data from the buffer: */
		memcpy(data,pos,dataSize);
		pos+=dataSize;
		}
	template <class DataParam>
	DataParam read(void) // Reads single value
		{
		/* Read data from the buffer: */
		DataParam result;
		memcpy(&result,pos,sizeof(DataParam));
		pos+=sizeof(DataParam);
		
		/* Endianness-correct the read data if required: */
		if(mustSwapEndianness)
			Misc::swapEndianness(result);
		
		return result;
		}
	template <class DataParam>
	DataParam& read(DataParam& data) // Reads single value through reference
		{
		/* Read data from the buffer: */
		memcpy(&data,pos,sizeof(DataParam));
		pos+=sizeof(DataParam);
		
		/* Endianness-correct the read data if required: */
		if(mustSwapEndianness)
			swapEndianness(data);
		
		return data;
		}
	template <class DataParam>
	void read(DataParam* data,size_t numItems) // Reads array of values
		{
		/* Read data from the buffer: */
		memcpy(data,pos,numItems*sizeof(DataParam));
		pos+=numItems*sizeof(DataParam);
		
		/* Endianness-correct the read data if required: */
		if(mustSwapEndianness)
			swapEndianness(data,numItems);
		}
	
	/* Writing interface: */
	void writeRaw(void* data,size_t dataSize) // Writes a chunk of data to the buffer
		{
		/* Write data to the buffer: */
		memcpy(pos,data,dataSize);
		pos+=dataSize;
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes single value
		{
		/* Check if data must be endianness-corrected: */
		if(mustSwapEndianness)
			{
			/* Endianness-correct the data in a temporary copy: */
			DataParam temp(data);
			Misc::swapEndianness(temp);
			
			/* Write endianness-corrected data to the buffer: */
			memcpy(pos,&data,sizeof(DataParam));
			}
		else
			{
			/* Write data to the buffer: */
			memcpy(pos,&data,sizeof(DataParam));
			}
		pos+=sizeof(DataParam);
		}
	template <class DataParam>
	void write(const DataParam* data,size_t numItems) // Writes array of values
		{
		/* Check if data must be endianness-corrected: */
		if(mustSwapEndianness)
			{
			/* Endianness-correct and write the data one item at a time: */
			for(size_t i=0;i<numItems;++i)
				{
				/* Endianness-correct the data in a temporary copy: */
				DataParam temp(data[i]);
				Misc::swapEndianness(temp);
				
				/* Write endianness-corrected data to the buffer: */
				memcpy(pos,&data,sizeof(DataParam));
				pos+=sizeof(DataParam);
				}
			}
		else
			{
			/* Write data to the buffer: */
			memcpy(pos,data,numItems*sizeof(DataParam));
			pos+=numItems*sizeof(DataParam);
			}
		}
	};

}

#endif

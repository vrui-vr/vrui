/***********************************************************************
RingBufferLF - Class for simple fixed-size lock-free ring buffers to
stream data from a single producer to a single consumer.
Copyright (c) 2026 Oliver Kreylos

This file is part of the Portable Threading Library (Threads).

The Portable Threading Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Portable Threading Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Threading Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef THREADS_RINGBUFFERLF_INCLUDED
#define THREADS_RINGBUFFERLF_INCLUDED

#include <stddef.h>
#include <string.h>

namespace Threads {

template <class ValueParam>
class RingBufferLF
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value;
	
	/* Elements: */
	private:
	Value* buffer; // Pointer to the beginning of the circular buffer
	Value* bufferEnd; // Pointer to the end of the circular buffer
	Value* volatile readPtr; // Current read position; only ever changed by reader-side interface
	Value* volatile writePtr; // Current write position; only ever changed by writer-side interface
	
	/* Constructors and destructors: */
	public:
	RingBufferLF(size_t bufferSize) // Creates a buffer that can contain up to the given number of elements; the actually allocated buffer is one element larger
		:buffer(new Value[bufferSize+1]),
		 bufferEnd(buffer+(bufferSize+1)),
		 readPtr(buffer),writePtr(buffer)
		{
		}
	~RingBufferLF(void) // Destroys the buffer
		{
		delete[] buffer;
		}
	
	/* Methods: */
	
	/* Read methods: */
	size_t getReadSize(void) const // Returns the amount of data readable from the buffer
		{
		/* Retrieve the current values of the read and write pointers: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		/* Determine whether the current buffer content is contiguous or wrapped around: */
		if(wp>=rp)
			{
			/* Return the data size of the contiguous buffer: */
			return wp-rp;
			}
		else
			{
			/* Return the data size of the wrapped-around buffer: */
			return (bufferEnd-rp)+(wp-buffer);
			}
		}
	void readData(Value* dest,size_t destSize) // Reads the given amount of data into the given destination; assumes that destSize<=getReadSize()
		{
		/* Retrieve the current values of the read and write pointers: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		/* Determine whether the current buffer content is contiguous or wrapped around: */
		if(wp>=rp)
			{
			/* Copy the requested amount of data from the contiguous buffer in one go: */
			memcpy(dest,rp,destSize*sizeof(Value));
			rp+=destSize; // The read pointer cannot wrap around here
			}
		else
			{
			/* Copy an initial chunk of data from the first part of the wrapped-around buffer: */
			size_t firstSize=bufferEnd-rp;
			if(firstSize>destSize)
				firstSize=destSize;
			memcpy(dest,rp,firstSize*sizeof(Value));
			if((rp+=firstSize)==bufferEnd)
				rp=buffer;
			
			/* Check if more data needs to be copied: */
			destSize-=firstSize;
			if(destSize>0U)
				{
				/* Copy a second chunk of data from the second part of the wrapped-around buffer: */
				memcpy(dest+firstSize,rp,destSize*sizeof(Value));
				rp+=destSize; // The read pointer cannot wrap around here
				}
			}
		
		/* Update the read pointer: */
		__atomic_store_n(&readPtr,rp,__ATOMIC_RELEASE);
		}
	size_t getContiguousReadSize(void) const // Returns the amount of data that can be read from the buffer in a contiguous fashion
		{
		/* Retrieve the current values of the read and write pointers: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		/* Determine whether the current buffer content is contiguous or wrapped around: */
		if(wp>=rp)
			{
			/* Return the data size of the contiguous buffer: */
			return wp-rp;
			}
		else
			{
			/* Return the data size of the first half of the wrapped-around buffer: */
			return (bufferEnd-rp);
			}
		}
	const Value* getContiguousReadPtr(void) // Returns a pointer to read data directly from the buffer; size of data read must be <=getContiguousReadSize()
		{
		/* Retrieve the current value of the read pointer: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		
		return rp;
		}
	void finishContiguousRead(size_t destSize) // Updates the buffer after data has been read directly from the buffer; assumes that destSize<=getContiguousReadSize()
		{
		/* Retrieve the current value of the read pointer: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		
		/* Mark the data as read: */
		if((rp+=destSize)==bufferEnd)
			rp=buffer;
		
		/* Update the read pointer: */
		__atomic_store_n(&readPtr,rp,__ATOMIC_RELEASE);
		}
	
	/* Write methods: */
	size_t getWriteSize(void) const // Returns the total amount of data that can be written to the buffer
		{
		/* Retrieve the current values of the read and write pointers: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		/* Determine whether the current buffer content is contiguous or wrapped around: */
		if(wp>=rp)
			{
			/* Return the size of the wrapped-around empty buffer: */
			return (bufferEnd-wp)+(rp-buffer)-1; // Subtract one to distinguish a completely full buffer from an empty buffer
			}
		else
			{
			/* Return the size of the continuous empty buffer: */
			return (rp-wp)-1; // Subtract one to distinguish a completely full buffer from an empty buffer
			}
		}
	void writeData(const Value* source,size_t sourceSize) // Writes the given amount of data from the given source; assumes that sourceSize<=getWriteSize()
		{
		/* Retrieve the current values of the read and write pointers: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		/* Determine whether the current buffer content is contiguous or wrapped around: */
		if(wp>=rp)
			{
			/* Copy an initial chunk of data into the first part of the wrapped-around empty buffer: */
			size_t firstSize=bufferEnd-wp;
			if(firstSize>sourceSize)
				firstSize=sourceSize;
			memcpy(wp,source,firstSize*sizeof(Value));
			if((wp+=firstSize)==bufferEnd)
				wp=buffer;
			
			/* Check if more data needs to be copied: */
			sourceSize-=firstSize;
			if(sourceSize>0U)
				{
				/* Copy a second chunk of data into the second part of the wrapped-around empty buffer: */
				memcpy(wp,source+firstSize,sourceSize*sizeof(Value));
				wp+=sourceSize; // The write pointer cannot wrap around here
				}
			}
		else
			{
			/* Copy the requested amount of data into the contiguous empty buffer in one go: */
			memcpy(wp,source,sourceSize*sizeof(Value));
			wp+=sourceSize; // The write pointer cannot wrap around here
			}
		
		/* Update the write pointer: */
		__atomic_store_n(&writePtr,wp,__ATOMIC_RELEASE);
		}
	size_t getContiguousWriteSize(void) const // Returns the amount of data that can be written to the buffer in a contiguous fashion
		{
		/* Retrieve the current values of the read and write pointers: */
		Value* rp=__atomic_load_n(&readPtr,__ATOMIC_ACQUIRE);
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		/* Determine whether the current buffer content is contiguous or wrapped around: */
		if(wp>=rp)
			{
			/* Return the size of the first half of the wrap-around empty buffer: */
			size_t result=bufferEnd-wp;
			if(rp==buffer)
				--result; // Subtract one to distinguish a completely full buffer from an empty buffer
			return result;
			}
		else
			{
			/* Return the size of the continuous empty buffer: */
			return (rp-wp)-1; // Subtract one to distinguish a completely full buffer from an empty buffer
			}
		}
	Value* getContiguousWritePtr(void) // Returns a pointer to write data directly into the buffer; size of data written must be <=getContiguousWriteSize()
		{
		/* Retrieve the current value of the write pointer: */
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		return wp;
		}
	void finishContiguousWrite(size_t sourceSize) // Updates the buffer after data has been written directly into the buffer; assumes that sourceSize<=getContiguousWriteSize()
		{
		/* Retrieve the current value of the write pointer: */
		Value* wp=__atomic_load_n(&writePtr,__ATOMIC_ACQUIRE);
		
		/* Mark the data as written: */
		if((wp+=sourceSize)==bufferEnd)
			wp=buffer;
		
		/* Update the write pointer: */
		__atomic_store_n(&writePtr,wp,__ATOMIC_RELEASE);
		}
	};

}

#endif

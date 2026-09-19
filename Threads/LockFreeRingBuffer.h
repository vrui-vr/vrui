/***********************************************************************
LockFreeRingBuffer - Class for high-performance and low-latency data
streaming from a single producer to a single consumer using a fixed-size
ring buffer.
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

#ifndef THREADS_LOCKFREERINGBUFFER_INCLUDED
#define THREADS_LOCKFREERINGBUFFER_INCLUDED

#include <string.h>
#include <atomic>
#include <Misc/StdError.h>

namespace Threads {

template <class ValueParam>
class LockFreeRingBuffer
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value; // Type of streamed data
	
	struct ReadState // Structure encapsulating the current read state of the buffer to minimize the number of atomic accesses
		{
		friend class LockFreeRingBuffer;
		
		/* Elements: */
		public:
		const Value* const readPtr; // The start of the buffer data available to read
		const size_t dataSize; // The total amount of readable data in the buffer
		const size_t contiguousDataSize; // The amount of data in the buffer that can be read contiguously
		
		/* Constructors and destructors: */
		private:
		ReadState(const Value* sReadPtr,size_t sDataSize,size_t sContiguousDataSize)
			:readPtr(sReadPtr),dataSize(sDataSize),contiguousDataSize(sContiguousDataSize)
			{
			}
		};
	
	struct WriteState // Structure encapsulating the current write state of the buffer to minimize the number of atomic accesses
		{
		friend class LockFreeRingBuffer;
		
		/* Elements: */
		public:
		Value* const writePtr; // The start of the buffer space available to write into
		const size_t dataSize; // The total amount of writable space in the buffer
		const size_t contiguousDataSize; // The amount of space in the buffer that can be written into contiguously
		
		/* Constructors and destructors: */
		private:
		WriteState(Value* sWritePtr,size_t sDataSize,size_t sContiguousDataSize)
			:writePtr(sWritePtr),dataSize(sDataSize),contiguousDataSize(sContiguousDataSize)
			{
			}
		};
	
	/* Elements: */
	private:
	Value* buffer; // Pointer to the beginning of the circular buffer
	Value* bufferEnd; // Pointer to the end of the circular buffer
	std::atomic<const Value*> readPtr; // Current read position; only ever changed by reader-side interface
	std::atomic<Value*> writePtr; // Current write position; only ever changed by writer-side interface
	
	/* Constructors and destructors: */
	public:
	LockFreeRingBuffer(size_t bufferSize) // Creates a buffer of the given size; the buffer can at most hold one fewer than the given number of values to distinguish an empty from a full buffer
		:buffer(new Value[bufferSize]), // Allocate one extra buffer element to distinguish a completely empty buffer from a completely full buffer
		 bufferEnd(buffer+(bufferSize)),
		 readPtr(buffer),writePtr(buffer)
		{
		}
	LockFreeRingBuffer(LockFreeRingBuffer&& source) // Move constructor
		:buffer(source.buffer),bufferEnd(source.bufferEnd),
		 readPtr(source.readPtr.load(std::memory_order_relaxed)),
		 writePtr(source.writePtr.load(std::memory_order_relaxed))
		{
		/* Invalidate the source: */
		source.bufferEnd=source.buffer=0;
		source.readPtr.store(0,std::memory_order_relaxed);
		source.writePtr.store(0,std::memory_order_relaxed);
		}
	~LockFreeRingBuffer(void) // Destroys the buffer
		{
		delete[] buffer;
		}
	
	/* Methods: */
	
	/* Read methods: */
	ReadState getReadState(void) const // Returns the current read state of the buffer
		{
		/* Retrieve the current values of the read and write pointers: */
		const Value* rp=readPtr.load(std::memory_order_acquire);
		Value* wp=writePtr.load(std::memory_order_acquire);
		
		/* Determine whether the current buffer content is contiguous or wrapped around: */
		if(wp>=rp)
			{
			/* Return the size of the contiguous data: */
			return ReadState(rp,wp-rp,wp-rp);
			}
		else
			{
			/* Return the size of the entire wrapped-around data and the initial contiguous chunk: */
			return ReadState(rp,(bufferEnd-rp)+(wp-buffer),bufferEnd-rp);
			}
		}
	void readData(const ReadState& readState,Value* dest,size_t destSize) // Reads the given amount of data into the given destination
		{
		/* Determine whether the requested data can be read contiguously: */
		if(destSize<=readState.contiguousDataSize)
			{
			/* Read the requested amount of data in one go: */
			const Value* rp=readState.readPtr;
			memcpy(dest,rp,destSize*sizeof(Value));
			
			/* Update the read pointer, wrapping it around if it reaches the end of the buffer: */
			if((rp+=destSize)==bufferEnd)
				rp=buffer;
			readPtr.store(rp,std::memory_order_release);
			}
		else if(destSize<=readState.dataSize)
			{
			/* Copy an initial chunk of data from the first part of the wrapped-around buffer: */
			memcpy(dest,readState.readPtr,readState.contiguousDataSize*sizeof(Value));
			dest+=readState.contiguousDataSize;
			destSize-=readState.contiguousDataSize;
			
			/* Copy a second chunk of data from the second part of the wrapped-around buffer: */
			memcpy(dest,buffer,destSize*sizeof(Value));
			
			/* Update the read pointer, which cannot wrap around a second time here: */
			readPtr.store(buffer+destSize,std::memory_order_release);
			}
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Buffer underflow by %lu items",destSize-readState.dataSize);
		}
	void markDataRead(const ReadState& readState,size_t destSize) // Updates the buffer state after the client read data contiguously directly from the buffer
		{
		/* Update the read pointer, wrapping it around if it reaches the end of the buffer: */
		const Value* rp=readState.readPtr;
		if((rp+=destSize)==bufferEnd)
			rp=buffer;
		readPtr.store(rp,std::memory_order_release);
		}
	
	/* Write methods: */
	WriteState getWriteState(void) // Returns the current write state of the buffer
		{
		/* Retrieve the current values of the read and write pointers: */
		const Value* rp=readPtr.load(std::memory_order_acquire);
		Value* wp=writePtr.load(std::memory_order_acquire);
		
		/* Determine whether the current buffer empty space is wrapped around or contiguous: */
		if(wp>=rp)
			{
			/* Return the size of the entire wrapped-around empty space and the initial contiguous chunk: */
			return WriteState(wp,(bufferEnd-wp)+(rp-buffer)-1,rp!=buffer?bufferEnd-wp:(bufferEnd-wp)-1); // Subtract one to distinguish a completely full buffer from an empty buffer
			}
		else
			{
			/* Return the size of the contiguous empty buffer: */
			return WriteState(wp,(rp-wp)-1,(rp-wp)-1); // Subtract one to distinguish a completely full buffer from an empty buffer
			}
		}
	void writeData(const WriteState& writeState,const Value* source,size_t sourceSize) // Writes the given amount of data from the given source
		{
		/* Determine whether the given data can be written contiguously: */
		if(sourceSize<=writeState.contiguousDataSize)
			{
			/* Write the given amount of data in one go: */
			Value* wp=writeState.writePtr;
			memcpy(wp,source,sourceSize*sizeof(Value));
			
			/* Update the write pointer, wrapping it around if it reaches the end of the buffer: */
			if((wp+=sourceSize)==bufferEnd)
				wp=buffer;
			writePtr.store(wp,std::memory_order_release);
			}
		else if(sourceSize<=writeState.dataSize)
			{
			/* Copy an initial chunk of data into the first part of the wrapped-around buffer: */
			memcpy(writeState.writePtr,source,writeState.contiguousDataSize*sizeof(Value));
			source+=writeState.contiguousDataSize;
			sourceSize-=writeState.contiguousDataSize;
			
			/* Copy a second chunk of data into the second part of the wrapped-around buffer: */
			memcpy(buffer,source,sourceSize*sizeof(Value));
			
			/* Update the write pointer, which cannot wrap around a second time here: */
			writePtr.store(buffer+sourceSize,std::memory_order_release);
			}
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Buffer overflow by %lu items",sourceSize-writeState.dataSize);
		}
	void markDataWritten(const WriteState& writeState,size_t sourceSize) // Updates the buffer state after the client wrote data contiguously directly into the buffer
		{
		/* Update the write pointer, wrapping it around if it reaches the end of the buffer: */
		Value* wp=writeState.writePtr;
		if((wp+=sourceSize)==bufferEnd)
			wp=buffer;
		writePtr.store(wp,std::memory_order_release);
		}
	};

}

#endif

/***********************************************************************
RingBuffer - Class to allow one-way synchronous communication between a
producer and a consumer.
Copyright (c) 2009-2022 Oliver Kreylos

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

#ifndef THREADS_RINGBUFFER_INCLUDED
#define THREADS_RINGBUFFER_INCLUDED

#include <stdexcept>
#include <Threads/MutexCond.h>

namespace Threads {

template <class ValueParam>
class RingBuffer
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value; // Type of communicated data
	
	class Shutdown:public std::runtime_error // Class to signal buffer shutdown while a reader/writer are blocked
		{
		/* Constructors and destructors: */
		public:
		Shutdown(void)
			:std::runtime_error("Threads::RingBuffer: Buffer shut down while blocking")
			{
			}
		};
	
	class ReadLock // Helper class to lock a region in the ring buffer for reading
		{
		friend class RingBuffer;
		
		/* Elements: */
		private:
		const Value* values; // Base pointer in ring buffer
		size_t numValues; // Number of locked values
		
		/* Constructors and destructors: */
		public:
		ReadLock(void) // Creates invalid lock
			:values(0),numValues(0)
			{
			}
		private:
		ReadLock(const Value* sValues,size_t sNumValues) // Creates valid lock for the given buffer and value region
			:values(sValues),numValues(sNumValues)
			{
			}
		
		/* Methods: */
		public:
		const Value* getValues(void) const
			{
			return values;
			}
		size_t getNumValues(void) const
			{
			return numValues;
			}
		};
	
	class WriteLock // Helper class to lock a region in the ring buffer for writing
		{
		friend class RingBuffer;
		
		/* Elements: */
		private:
		Value* values; // Base pointer in ring buffer
		size_t numValues; // Number of locked values
		
		/* Constructors and destructors: */
		public:
		WriteLock(void) // Creates invalid lock
			:values(0),numValues(0)
			{
			}
		private:
		WriteLock(Value* sValues,size_t sNumValues) // Creates valid lock for the given buffer and value region
			:values(sValues),numValues(sNumValues)
			{
			}
		
		/* Methods: */
		public:
		Value* getValues(void) const
			{
			return values;
			}
		size_t getNumValues(void) const
			{
			return numValues;
			}
		};
	
	/* Elements: */
	private:
	size_t bufferSize; // Size of the ring buffer
	Value* buffer; // The ring buffer
	Value* bufferEnd; // The end of the ring buffer
	MutexCond bufferCond; // Condition variable protecting the buffer's read and write positions and letting readers or writers block
	const Value* readPtr; // Current reading position in buffer
	Value* writePtr; // Current writing position in buffer
	size_t bufferUsed; // Current actual number of values in buffer
	bool keepRunning; // Flag to notify blocked readers or writers that the buffer is being destroyed
	
	/* Private methods: */
	void blockOnRead(MutexCond::Lock& bufferLock) // Blocks for reading on an empty buffer; throws exception if buffer is destroyed
		{
		/* Block until data becomes available or the buffer is destroyed: */
		while(bufferUsed==0&&keepRunning)
			bufferCond.wait(bufferLock);
		
		/* Check for buffer destruction: */
		if(bufferUsed==0)
			throw Shutdown();
		}
	void blockOnWrite(MutexCond::Lock& bufferLock) // Blocks for writing on a full buffer; throws exception if buffer is destroyed
		{
		/* Block until space becomes available or the buffer is destroyed: */
		while(bufferUsed==bufferSize&&keepRunning)
			bufferCond.wait(bufferLock);
		
		/* Check for buffer destruction: */
		if(bufferUsed==bufferSize)
			throw Shutdown();
		}
	
	/* Constructors and destructors: */
	public:
	RingBuffer(size_t sBufferSize) // Creates empty ring buffer of given size
		:bufferSize(sBufferSize),buffer(bufferSize>0?new Value[bufferSize]:0),
		 bufferEnd(buffer+bufferSize),
		 readPtr(buffer),writePtr(buffer),bufferUsed(0),
		 keepRunning(true)
		{
		}
	private:
	RingBuffer(const RingBuffer& source); // Prohibit copy constructor
	RingBuffer& operator=(const RingBuffer& source); // Prohibit assignment operator
	public:
	~RingBuffer(void) // Destroys the ring buffer
		{
		delete[] buffer;
		}
	
	/* Methods: */
	MutexCond& acquireLock(void) // Returns the mutex/condition variable for the sole purpose of acquiring a lock
		{
		return bufferCond;
		}
	void resize(size_t newBufferSize) // Resizes the buffer, discarding all data; must only be called when no one is reading or writing
		{
		/* Reallocate the buffer: */
		delete[] buffer;
		bufferSize=newBufferSize;
		buffer=new Value[bufferSize];
		bufferEnd=buffer+bufferSize;
		readPtr=writePtr=buffer;
		bufferUsed=0;
		}
	void shutdown(void) // Shuts down the buffer prior to destruction; wakes up all blocked readers or writers by throwing exceptions
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Signal shutdown and wake up all blocked threads: */
		keepRunning=false;
		bufferCond.broadcast();
		}
	bool empty(void) const // Returns true if there is no data to be read in the ring buffer
		{
		/* No need to lock: */
		return bufferUsed==0;
		}
	bool full(void) const // Returns true if there is no room to write data in the ring buffer
		{
		/* No need to lock: */
		return bufferUsed==bufferSize;
		}
	ReadLock acquireReadLock(size_t maxNumValues) // Blocks until at least one value can be read from the buffer; returns lock on number of values
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Block if the buffer is empty: */
		if(bufferUsed==0)
			blockOnRead(bufferLock);
		
		/* Adjust the number of values that can be read for ring buffer wrap-around and requested number of values: */
		size_t numValues=bufferUsed;
		size_t bufferEnd=bufferSize-(readPtr-buffer);
		if(numValues>bufferEnd)
			numValues=bufferEnd;
		if(numValues>maxNumValues)
			numValues=maxNumValues;
		
		/* Return a read lock: */
		return ReadLock(readPtr,numValues);
		}
	void releaseReadLock(const ReadLock& readLock) // Releases a read lock; assumes that all data in the locked region has been read
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Advance the read pointer: */
		if((readPtr+=readLock.numValues)==bufferEnd)
			readPtr=buffer;
		
		/* Wake up blocked writers if the buffer was full: */
		if(bufferUsed==bufferSize)
			bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple writers are waiting
		bufferUsed-=readLock.numValues;
		}
	WriteLock getWriteLock(size_t maxNumValues) // Blocks until at least one value can be written to the buffer; returns lock on number of values
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Block if the buffer is full: */
		if(bufferUsed==bufferSize)
			blockOnWrite(bufferLock);
		
		/* Adjust the number of values that can be written for ring buffer wrap-around and requested number of values: */
		size_t numValues=bufferSize-bufferUsed;
		size_t bufferEnd=bufferSize-(writePtr-buffer);
		if(numValues>bufferEnd)
			numValues=bufferEnd;
		if(numValues>maxNumValues)
			numValues=maxNumValues;
		
		/* Return a write lock: */
		return WriteLock(writePtr,numValues);
		}
	void releaseWriteLock(const WriteLock& writeLock) // Releases a write lock; assumes that all data in the locked region has been written
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Advance the write pointer: */
		if((writePtr+=writeLock.numValues)==bufferEnd)
			writePtr=buffer;
		
		/* Wake up blocked readers if the buffer was empty: */
		if(bufferUsed==0)
			bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple readers are waiting
		bufferUsed+=writeLock.numValues;
		}
	Value read(void) // Reads a single value from the buffer; blocks if no data is available
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Block if the buffer is empty: */
		if(bufferUsed==0)
			blockOnRead(bufferLock);
		
		/* Read from the buffer: */
		Value result=*readPtr;
		if(++readPtr==bufferEnd)
			readPtr=buffer;
		
		/* Wake up blocked writers if the buffer was full: */
		if(bufferUsed==bufferSize)
			bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple writers are waiting
		--bufferUsed;
		
		return result;
		}
	Value read(MutexCond::Lock& bufferLock) // Ditto; takes existing buffer lock
		{
		/* Block if the buffer is empty: */
		if(bufferUsed==0)
			blockOnRead(bufferLock);
		
		/* Read from the buffer: */
		Value result=*readPtr;
		if(++readPtr==bufferEnd)
			readPtr=buffer;
		
		/* Wake up blocked writers if the buffer was full: */
		if(bufferUsed==bufferSize)
			bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple writers are waiting
		--bufferUsed;
		
		return result;
		}
	size_t read(Value* values,size_t numValues) // Reads between one and numValues from buffer; returns number read; blocks if no data is available
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Block if the buffer is empty: */
		if(bufferUsed==0)
			blockOnRead(bufferLock);
		
		/* Read data from the buffer: */
		size_t readSize=bufferUsed;
		if(readSize>numValues)
			readSize=numValues;
		for(size_t i=0;i<readSize;++i,++values)
			{
			/* Read a value: */
			*values=*readPtr;
			if(++readPtr==bufferEnd)
				readPtr=buffer;
			}
		
		/* Wake up blocked writers if the buffer was full: */
		if(bufferUsed==bufferSize)
			bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple writers are waiting
		bufferUsed-=readSize;
		
		return readSize;
		}
	void blockingRead(Value* values,size_t numValues) // Reads the given array from buffer; blocks until everything is read
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Read chunks of data until all requested values have been read: */
		while(numValues>0)
			{
			/* Block if the buffer is empty: */
			if(bufferUsed==0)
				blockOnRead(bufferLock);
			
			/* Read data from the buffer: */
			size_t readSize=bufferUsed;
			size_t bufferEndSize=bufferSize-(readPtr-buffer);
			if(readSize>bufferEndSize)
				readSize=bufferEndSize;
			if(readSize>numValues)
				readSize=numValues;
			for(Value* readEnd=readPtr+readSize;readPtr!=readEnd;++values,++readPtr)
				*values=*readPtr;
			if(readPtr==bufferEnd)
				readPtr=buffer;
			
			/* Wake up blocked writers if the buffer was full: */
			if(bufferUsed==bufferSize)
				bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple writers are waiting
			bufferUsed-=readSize;
			
			numValues-=readSize;
			}
		}
	void write(const Value& value) // Writes a single value into the buffer; blocks if there is no room in the buffer
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Block if the buffer is full: */
		if(bufferUsed==bufferSize)
			blockOnWrite(bufferLock);
		
		/* Write into the buffer: */
		*writePtr=value;
		if(++writePtr==bufferEnd)
			writePtr=buffer;
		
		/* Wake up blocked readers if the buffer was empty: */
		if(bufferUsed==0)
			bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple readers are waiting
		++bufferUsed;
		}
	void write(const Value& value,MutexCond::Lock& bufferLock) // Ditto; takes existing buffer lock
		{
		/* Block if the buffer is full: */
		if(bufferUsed==bufferSize)
			blockOnWrite(bufferLock);
		
		/* Write into the buffer: */
		*writePtr=value;
		if(++writePtr==bufferEnd)
			writePtr=buffer;
		
		/* Wake up blocked readers if the buffer was empty: */
		if(bufferUsed==0)
			bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple readers are waiting
		++bufferUsed;
		}
	void blockingWrite(const Value* values,size_t numValues) // Writes the given array into buffer; blocks until everything is written
		{
		/* Lock the buffer: */
		MutexCond::Lock bufferLock(bufferCond);
		
		/* Write chunks of data until all requested values have been written: */
		while(numValues>0)
			{
			/* Block if the buffer is full: */
			if(bufferUsed==bufferSize)
				blockOnWrite(bufferLock);
			
			/* Write data into the buffer: */
			size_t writeSize=bufferSize-bufferUsed;
			size_t bufferEndSize=bufferSize-(writePtr-buffer);
			if(writeSize>bufferEndSize)
				writeSize=bufferEndSize;
			if(writeSize>numValues)
				writeSize=numValues;
			for(Value* writeEnd=writePtr+writeSize;writePtr!=writeEnd;++writePtr,++values)
				*writePtr=*values;
			if(writePtr==bufferEnd)
				writePtr=buffer;
			
			/* Wake up blocked readers if the buffer was empty: */
			if(bufferUsed==0)
				bufferCond.broadcast(); // Need to use broadcast here or the buffer might stall if multiple readers are waiting
			
			bufferUsed+=writeSize;
			numValues-=writeSize;
			}
		}
	};

}

#endif

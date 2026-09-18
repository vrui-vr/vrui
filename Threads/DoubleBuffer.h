/***********************************************************************
DoubleBuffer - Class for lock-free atomic transmission of data between a
single writer and any number of readers through a double buffer.
Copyright (c) 2014-2026 Oliver Kreylos

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

#ifndef THREADS_DOUBLEBUFFER_INCLUDED
#define THREADS_DOUBLEBUFFER_INCLUDED

#include <atomic>

namespace Threads {

template <class DataParam>
class DoubleBuffer
	{
	/* Embedded classes: */
	public:
	typedef DataParam Data; // The type of data being shared
	
	/* Elements: */
	private:
	std::atomic<unsigned int> counter; // A counter to protect non-atomic updates of the shared data
	Data data[2]; // A double buffer of shared data values to prevent long read times when writes are slow or interrupted; the counter's LSB determines which buffer half contains readable data
	
	/* Constructors and destructors: */
	public:
	DoubleBuffer(void) // Creates a double buffer with uninitialized data
		:counter(0)
		{
		}
	DoubleBuffer(const Data& sData) // Creates a double buffer with the given initial data
		:counter(0)
		{
		data[0]=sData;
		}
	
	/* Writer-side methods: */
	const Data& readBack(void) const // Allows writer to read back the most recently written data
		{
		return data[counter.load(std::memory_order_relaxed)&0x1U];
		}
	Data& startWrite(void) // Returns the buffer half not currently used by readers
		{
		return data[(~counter.load(std::memory_order_acquire))&0x1U];
		}
	void finishWrite(void) // Updates the double buffer after the writer has written into the buffer half not currently used by readers
		{
		counter.fetch_add(1,std::memory_order_release);
		}
	void write(const Data& newData) // Atomically writes the given data into the double buffer
		{
		/* Write the given data into the buffer half not currently used by readers: */
		data[(~counter.load(std::memory_order_relaxed))&0x1U]=newData;
		
		/* Increment the counter to invalidate previous data and flip the readable buffer half: */
		counter.fetch_add(1,std::memory_order_release);
		}
	
	/* Reader-side methods: */
	Data& read(Data& readData) const // Atomically reads the double buffer's current data
		{
		/* Keep reading from the shared object until successful: */
		unsigned int counter0,counter1;
		do
			{
			/* Read the counter to determine which buffer half contains readable data: */
			counter0=counter.load(std::memory_order_acquire);
			
			/* Copy the readable buffer half into the result: */
			readData=data[counter0&0x1U];
			
			/* Read the counter again to detect changes during the read: */
			counter1=counter.load(std::memory_order_acquire);
			}
		while(counter0!=counter1);
		
		return readData;
		}
	Data read(void) const // Ditto, returning the read data as a result
		{
		/* Delegate to the other method using a temporary object: */
		Data result;
		read(result);
		return result;
		}
	};

}

#endif

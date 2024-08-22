/***********************************************************************
MemoryBacked - Class representing Vulkan objects that are backed by GPU
or CPU memory.
Copyright (c) 2022-2024 Oliver Kreylos

This file is part of the Vulkan C++ Wrapper Library (Vulkan).

The Vulkan C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vulkan C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vulkan C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef VULKAN_MEMORYBACKED_INCLUDED
#define VULKAN_MEMORYBACKED_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>
#include <Vulkan/MemoryAllocator.h>

namespace Vulkan {

class MemoryBacked:public DeviceAttached
	{
	/* Elements: */
	protected:
	MemoryAllocator::Allocation allocation; // Allocation object representing the memory chunk backing this object
	
	/* Constructors and destructors: */
	public:
	MemoryBacked(Device& sDevice); // Creates object without memory backing
	MemoryBacked(MemoryBacked&& source); // Move constructor
	virtual ~MemoryBacked(void); // Releases allocated memory
	
	/* Methods: */
	MemoryBacked& operator=(MemoryBacked&& source); // Move assignment operator
	const MemoryAllocator::Allocation& getAllocation(void) const // Returns the allocation object representing the object's backing memory
		{
		return allocation;
		}
	VkDeviceSize getSize(void) const // Returns the size of the allocated memory chunk
		{
		return allocation.getSize();
		}
	VkDeviceSize getOffset(void) const // Returns the offset of the allocated memory chunk
		{
		return allocation.getOffset();
		}
	void* map(VkMemoryMapFlags flags,bool willBeRead =false); // Maps the object's allocated memory chunk into CPU-accessible memory and returns a pointer; flag indicates whether mapped memory will be read from
	void unmap(bool wasWritten =true); // Unmaps currently mapped device memory region; flag indicates whether mapping was written to
	int getExportFd(void) const; // Returns a file descriptor to import memory allocated as exportable into another process
	};

}

#endif

/***********************************************************************
Buffer - Class representing Vulkan buffers and their associated memory
blocks.
Copyright (c) 2022-2023 Oliver Kreylos

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

#ifndef VULKAN_BUFFER_INCLUDED
#define VULKAN_BUFFER_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/MemoryBacked.h>

/* Forward declarations: */
namespace Vulkan {
class CommandPool;
}

namespace Vulkan {

class Buffer:public MemoryBacked
	{
	/* Elements: */
	private:
	VkBuffer buffer; // Vulkan buffer handle
	
	/* Constructors and destructors: */
	public:
	Buffer(Device& sDevice,VkDeviceSize size,VkBufferUsageFlags usage,MemoryAllocator& allocator,VkMemoryPropertyFlags properties,bool exportable); // Creates a buffer of the given size with the given usage and memory property flags
	Buffer(Buffer&& source); // Move constructor
	virtual ~Buffer(void); // Destroys the buffer and releases its associated memory block
	
	/* Methods: */
	Buffer& operator=(Buffer&& source); // Move assignment operator
	VkBuffer getHandle(void) const // Returns the Vulkan buffer handle
		{
		return buffer;
		}
	void copy(VkDeviceSize destOffset,const Buffer& source,VkDeviceSize sourceOffset,VkDeviceSize size,CommandPool& commandPool); // Copies the contents of the given source buffer using the given command pool
	};

}

#endif

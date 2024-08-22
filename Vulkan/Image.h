/***********************************************************************
Image - Class representing Vulkan images and their associated memory
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

#ifndef VULKAN_IMAGE_INCLUDED
#define VULKAN_IMAGE_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/Types.h>
#include <Vulkan/MemoryBacked.h>

/* Forward declarations: */
namespace Vulkan {
class Buffer;
class CommandPool;
}

namespace Vulkan {

class Image:public MemoryBacked
	{
	/* Elements: */
	private:
	VkImage image; // Vulkan image handle
	VkFormat format; // Current image format
	VkImageLayout layout; // Current image layout
	VkExtent3D extent; // Image extents
	
	/* Constructors and destructors: */
	public:
	Image(Device& sDevice,VkImageType type,VkFormat initialFormat,const VkExtent3D& sExtent,VkImageTiling tiling,VkImageUsageFlags usage,bool keepPixels,MemoryAllocator& allocator,VkMemoryPropertyFlags properties,bool exportable); // Creates an image with the given parameters and usage
	Image(Image&& source); // Move constructor
	virtual ~Image(void); // Destroys the image and releases its associated memory block
	
	/* Methods: */
	VkImage getHandle(void) const // Returns the Vulkan image handle
		{
		return image;
		}
	const VkExtent3D& getExtent(void) const // Returns the image's extents
		{
		return extent;
		}
	void transitionLayout(VkImageLayout newLayout,CommandPool& commandPool); // Transitions the image layout to the new layout using transient command buffers from the given command pool
	void copyFromBuffer(Buffer& buffer,const Rect& rect,CommandPool& commandPool); // Copies the contents of the given buffer into the sub-image defined by the given rectangle using transient command buffers from the given command pool
	};

}

#endif

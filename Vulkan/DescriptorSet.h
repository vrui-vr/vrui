/***********************************************************************
DescriptorSet - Class representing Vulkan descriptor sets.
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

#ifndef VULKAN_DESCRIPTORSET_INCLUDED
#define VULKAN_DESCRIPTORSET_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>

/* Forward declarations: */
namespace Vulkan {
class DescriptorPool;
class ImageView;
class Sampler;
}

namespace Vulkan {

class DescriptorSet
	{
	/* Elements: */
	private:
	DescriptorPool& descriptorPool; // Descriptor pool from which the descriptor set was allocated
	VkDescriptorSet descriptorSet; // Vulkan descriptor set handle
	
	/* Constructors and destructors: */
	public:
	DescriptorSet(DescriptorPool& sDescriptorPool,VkDescriptorSet sDescriptorSet); // Construction from Vulkan descriptor set handle
	private:
	DescriptorSet(const DescriptorSet& source); // Prohibit copy constructor
	DescriptorSet& operator=(const DescriptorSet& source); // Prohibit assignment operator
	public:
	DescriptorSet(DescriptorSet&& source); // Move constructor
	~DescriptorSet(void); // Returns the descriptor set to its originating descriptor pool
	
	/* Methods: */
	VkDescriptorSet getHandle(void) const // Returns the Vulkan descriptor set handle
		{
		return descriptorSet;
		}
	void setCombinedImageSampler(uint32_t binding,uint32_t arrayElement,VkImageLayout imageLayout,const ImageView& imageView,const Sampler& sampler); // Sets one of the descriptor's combined image samplers
	};

}

#endif

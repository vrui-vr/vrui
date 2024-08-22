/***********************************************************************
DescriptorPool - Class representing Vulkan descriptor set pools.
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

#ifndef VULKAN_DESCRIPTORPOOL_INCLUDED
#define VULKAN_DESCRIPTORPOOL_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

/* Forward declarations: */
namespace Vulkan {
class DescriptorSetLayout;
class DescriptorSet;
}

namespace Vulkan {

class DescriptorPool:public DeviceAttached
	{
	/* Embedded classes: */
	public:
	class Constructor // Helper class to create descriptor pool objects
		{
		friend class DescriptorPool;
		
		/* Elements: */
		private:
		std::vector<VkDescriptorPoolSize> descriptorPoolSizes; // List of pool sizes for different descriptor types
		
		/* Methods: */
		public:
		void addDescriptorPoolSize(VkDescriptorType type,uint32_t descriptorCount); // Adds a pool of the given size for the given descriptor type
		};
	
	/* Elements: */
	private:
	VkDescriptorPool descriptorPool; // Vulkan descriptor set pool handle
	bool freeEnabled; // Flag whether descriptor sets can be freed
	
	/* Constructors and destructors: */
	public:
	DescriptorPool(Device& sDevice,VkDescriptorPoolCreateFlags flags,uint32_t maxSets,const Constructor& constructor); // Creates a descriptor set pool for the given device and constructor
	DescriptorPool(DescriptorPool&& source); // Move constructor
	virtual ~DescriptorPool(void); // Destroys the descriptor set pool
	
	/* Methods: */
	VkDescriptorPool getHandle(void) const // Returns the Vulkan descriptor set pool handle
		{
		return descriptorPool;
		}
	DescriptorSet allocateDescriptorSet(const DescriptorSetLayout& descriptorSetLayout); // Allocates a single descriptor set with the given layout
	void freeDescriptorSet(DescriptorSet& descriptorSet); // Frees a single descriptor set
	};

}

#endif

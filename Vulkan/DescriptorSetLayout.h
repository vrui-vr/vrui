/***********************************************************************
DescriptorSetLayout - Class representing Vulkan descriptor set layouts.
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

#ifndef VULKAN_DESCRIPTORSETLAYOUT_INCLUDED
#define VULKAN_DESCRIPTORSETLAYOUT_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

namespace Vulkan {

class DescriptorSetLayout:public DeviceAttached
	{
	/* Embedded classes: */
	public:
	class Constructor // Helper class to create descriptor set layout objects
		{
		friend class DescriptorSetLayout;
		
		/* Elements: */
		private:
		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings; // List of descriptor bindings composing the descriptor set layout
		
		/* Methods: */
		public:
		void addBinding(const VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding); // Adds the given descriptor binding to the list
		void addBinding(uint32_t binding,VkDescriptorType descriptorType,uint32_t descriptorCount,VkShaderStageFlags stageFlags); // Adds the descriptor binding defined by the given components to the list
		};
	
	/* Elements: */
	private:
	VkDescriptorSetLayout descriptorSetLayout; // Vulkan descriptor set layout handle
	
	/* Constructors and destructors: */
	public:
	DescriptorSetLayout(Device& sDevice,const Constructor& constructor); // Creates a descriptor set layout for the given device, using information from the given constructor
	DescriptorSetLayout(DescriptorSetLayout&& source); // Move constructor
	virtual ~DescriptorSetLayout(void); // Destroys the descriptor set layout
	
	/* Methods; */
	VkDescriptorSetLayout getHandle(void) const // Returns the Vulkan descriptor set layout handle
		{
		return descriptorSetLayout;
		}
	};

}

#endif

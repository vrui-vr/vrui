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

#include <Vulkan/DescriptorSetLayout.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/*************************************************
Methods of class DescriptorSetLayout::Constructor:
*************************************************/

void DescriptorSetLayout::Constructor::addBinding(const VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding)
	{
	/* Add the given descriptor set layout binding structure to the list: */
	descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
	}

void DescriptorSetLayout::Constructor::addBinding(uint32_t binding,VkDescriptorType descriptorType,uint32_t descriptorCount,VkShaderStageFlags stageFlags)
	{
	/* Construct a descriptor set layout binding structure: */
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
	descriptorSetLayoutBinding.binding=binding;
	descriptorSetLayoutBinding.descriptorType=descriptorType;
	descriptorSetLayoutBinding.descriptorCount=descriptorCount;
	descriptorSetLayoutBinding.stageFlags=stageFlags;
	descriptorSetLayoutBinding.pImmutableSamplers=0;
	
	/* Add the descriptor set layout binding structure to the list: */
	descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
	}

/************************************
Methods of class DescriptorSetLayout:
************************************/

DescriptorSetLayout::DescriptorSetLayout(Device& sDevice,const DescriptorSetLayout::Constructor& constructor)
	:DeviceAttached(sDevice),
	 descriptorSetLayout(VK_NULL_HANDLE)
	{
	/* Set up a descriptor set layout creation structure: */
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext=0;
	descriptorSetLayoutCreateInfo.flags=0x0;
	descriptorSetLayoutCreateInfo.bindingCount=constructor.descriptorSetLayoutBindings.size();
	descriptorSetLayoutCreateInfo.pBindings=descriptorSetLayoutCreateInfo.bindingCount>0?constructor.descriptorSetLayoutBindings.data():0;
	
	/* Create the descriptor set layout: */
	throwOnError(vkCreateDescriptorSetLayout(device.getHandle(),&descriptorSetLayoutCreateInfo,0,&descriptorSetLayout),
	             __PRETTY_FUNCTION__,"create Vulkan device descriptor set layout");
	}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& source)
	:DeviceAttached(std::move(source)),
	 descriptorSetLayout(source.descriptorSetLayout)
	{
	/* Invalidate the source: */
	source.descriptorSetLayout=VK_NULL_HANDLE;
	}

DescriptorSetLayout::~DescriptorSetLayout(void)
	{
	/* Destroy the device descriptor set layout: */
	vkDestroyDescriptorSetLayout(device.getHandle(),descriptorSetLayout,0);
	}

}

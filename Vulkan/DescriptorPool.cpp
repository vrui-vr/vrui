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

#include <Vulkan/DescriptorPool.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/DescriptorSetLayout.h>
#include <Vulkan/DescriptorSet.h>

namespace Vulkan {

/********************************************
Methods of class DescriptorPool::Constructor:
********************************************/

void DescriptorPool::Constructor::addDescriptorPoolSize(VkDescriptorType type,uint32_t descriptorCount)
	{
	/* Create a descriptor pool size structure: */
	VkDescriptorPoolSize descriptorPoolSize;
	descriptorPoolSize.type=type;
	descriptorPoolSize.descriptorCount=descriptorCount;
	
	/* Add the structure to the descriptor pool size list: */
	descriptorPoolSizes.push_back(descriptorPoolSize);
	}

/*******************************
Methods of class DescriptorPool:
*******************************/

DescriptorPool::DescriptorPool(Device& sDevice,VkDescriptorPoolCreateFlags flags,uint32_t maxSets,const DescriptorPool::Constructor& constructor)
	:DeviceAttached(sDevice),
	 descriptorPool(VK_NULL_HANDLE),
	 freeEnabled((flags&VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)!=0x0)
	{
	/* Set up a descriptor pool creation structure: */
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	descriptorPoolCreateInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext=0;
	descriptorPoolCreateInfo.flags=flags;
	descriptorPoolCreateInfo.maxSets=maxSets;
	descriptorPoolCreateInfo.poolSizeCount=constructor.descriptorPoolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes=descriptorPoolCreateInfo.poolSizeCount>0?constructor.descriptorPoolSizes.data():0;
	
	/* Create the descriptor pool: */
	throwOnError(vkCreateDescriptorPool(device.getHandle(),&descriptorPoolCreateInfo,0,&descriptorPool),
	             __PRETTY_FUNCTION__,"create Vulkan descriptor pool");
	}

DescriptorPool::DescriptorPool(DescriptorPool&& source)
	:DeviceAttached(std::move(source)),
	 descriptorPool(source.descriptorPool)
	{
	/* Invalidate the source: */
	source.descriptorPool=VK_NULL_HANDLE;
	}

DescriptorPool::~DescriptorPool(void)
	{
	/* Destroy the descriptor pool: */
	vkDestroyDescriptorPool(device.getHandle(),descriptorPool,0);
	}

DescriptorSet DescriptorPool::allocateDescriptorSet(const DescriptorSetLayout& descriptorSetLayout)
	{
	/* Set up the descriptor set allocation structure: */
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
	descriptorSetAllocateInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext=0;
	descriptorSetAllocateInfo.descriptorPool=descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount=1;
	VkDescriptorSetLayout descriptorSetLayoutHandle=descriptorSetLayout.getHandle();
	descriptorSetAllocateInfo.pSetLayouts=&descriptorSetLayoutHandle;
	
	/* Allocate the descriptor set: */
	VkDescriptorSet result;
	throwOnError(vkAllocateDescriptorSets(device.getHandle(),&descriptorSetAllocateInfo,&result),
	             __PRETTY_FUNCTION__,"allocate Vulkan descriptor set");
	
	return DescriptorSet(*this,result);
	}

void DescriptorPool::freeDescriptorSet(DescriptorSet& descriptorSet)
	{
	if(freeEnabled)
		{
		/* Free the descriptor set: */
		VkDescriptorSet descriptorSetHandle=descriptorSet.getHandle();
		vkFreeDescriptorSets(device.getHandle(),descriptorPool,1,&descriptorSetHandle);
		}
	}

}

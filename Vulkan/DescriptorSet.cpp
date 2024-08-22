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

#include <Vulkan/DescriptorSet.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/DescriptorPool.h>
#include <Vulkan/ImageView.h>
#include <Vulkan/Sampler.h>

namespace Vulkan {

/******************************
Methods of class DescriptorSet:
******************************/

DescriptorSet::DescriptorSet(DescriptorPool& sDescriptorPool,VkDescriptorSet sDescriptorSet)
	:descriptorPool(sDescriptorPool),descriptorSet(sDescriptorSet)
	{
	}

DescriptorSet::DescriptorSet(DescriptorSet&& source)
	:descriptorPool(source.descriptorPool),descriptorSet(source.descriptorSet)
	{
	/* Invalidate the source: */
	source.descriptorSet=VK_NULL_HANDLE;
	}

DescriptorSet::~DescriptorSet(void)
	{
	/* Free the descriptor set: */
	descriptorPool.freeDescriptorSet(*this);
	}

void DescriptorSet::setCombinedImageSampler(uint32_t binding,uint32_t arrayElement,VkImageLayout imageLayout,const ImageView& imageView,const Sampler& sampler)
	{
	/* Set up the descriptor image info structure: */
	VkDescriptorImageInfo descriptorImageInfo;
	descriptorImageInfo.imageLayout=imageLayout;
	descriptorImageInfo.imageView=imageView.getHandle();
	descriptorImageInfo.sampler=sampler.getHandle();
	
	/* Set up the write descriptor set structure: */
	VkWriteDescriptorSet writeDescriptorSet;
	writeDescriptorSet.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext=0;
	writeDescriptorSet.dstSet=descriptorSet;
	writeDescriptorSet.dstBinding=binding;
	writeDescriptorSet.dstArrayElement=arrayElement;
	writeDescriptorSet.descriptorCount=1;
	writeDescriptorSet.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.pImageInfo=&descriptorImageInfo;
	writeDescriptorSet.pBufferInfo=0;
	writeDescriptorSet.pTexelBufferView=0;
	vkUpdateDescriptorSets(descriptorPool.getDevice().getHandle(),1,&writeDescriptorSet,0,0);
	}

}

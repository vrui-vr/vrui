/***********************************************************************
PipelineLayout - Class representing Vulkan pipeline layouts.
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

#include <Vulkan/PipelineLayout.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/*******************************
Methods of class PipelineLayout:
*******************************/

PipelineLayout::PipelineLayout(Device& sDevice,const Constructor& constructor)
	:DeviceAttached(sDevice),
	 pipelineLayout(VK_NULL_HANDLE)
	{
	/* Setup a pipeline layout creation structure: */
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext=0;
	pipelineLayoutCreateInfo.flags=0x0;
	pipelineLayoutCreateInfo.setLayoutCount=constructor.descriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts=pipelineLayoutCreateInfo.setLayoutCount>0?constructor.descriptorSetLayouts.data():0;
	pipelineLayoutCreateInfo.pushConstantRangeCount=constructor.pushConstantRanges.size();
	pipelineLayoutCreateInfo.pPushConstantRanges=pipelineLayoutCreateInfo.pushConstantRangeCount>0?constructor.pushConstantRanges.data():0;
	
	/* Create the pipeline layout object: */
	throwOnError(vkCreatePipelineLayout(device.getHandle(),&pipelineLayoutCreateInfo,0,&pipelineLayout),
	             __PRETTY_FUNCTION__,"create Vulkan pipeline layout");
	}

PipelineLayout::PipelineLayout(PipelineLayout&& source)
	:DeviceAttached(std::move(source)),
	 pipelineLayout(source.pipelineLayout)
	{
	/* Invalidate the source: */
	source.pipelineLayout=VK_NULL_HANDLE;
	}

PipelineLayout::~PipelineLayout(void)
	{
	/* Destroy the pipeline layout: */
	vkDestroyPipelineLayout(device.getHandle(),pipelineLayout,0);
	}

}

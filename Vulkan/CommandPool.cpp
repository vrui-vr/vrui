/***********************************************************************
CommandPool - Class representing Vulkan command pools.
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

#include <Vulkan/CommandPool.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/CommandBuffer.h>

namespace Vulkan {

/****************************
Methods of class CommandPool:
****************************/

CommandPool::CommandPool(Device& sDevice,uint32_t queueFamilyIndex,VkCommandPoolCreateFlags flags)
	:DeviceAttached(sDevice),
	 commandPool(VK_NULL_HANDLE)
	{
	/* Set up the command pool creation structure: */
	VkCommandPoolCreateInfo commandPoolCreateInfo;
	commandPoolCreateInfo.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext=0;
	commandPoolCreateInfo.flags=flags;
	commandPoolCreateInfo.queueFamilyIndex=queueFamilyIndex;
	
	throwOnError(vkCreateCommandPool(device.getHandle(),&commandPoolCreateInfo,0,&commandPool),
	             __PRETTY_FUNCTION__,"create Vulkan command pool object");
	}

CommandPool::~CommandPool(void)
	{
	/* Destroy the command pool object: */
	vkDestroyCommandPool(device.getHandle(),commandPool,0);
	}

CommandBuffer CommandPool::allocateCommandBuffer(void)
	{
	/* Set up the command buffer allocation structure: */
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext=0;
	commandBufferAllocateInfo.commandPool=commandPool;
	commandBufferAllocateInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount=1;
	
	/* Allocate the command buffer: */
	VkCommandBuffer result;
	throwOnError(vkAllocateCommandBuffers(device.getHandle(),&commandBufferAllocateInfo,&result),
	             __PRETTY_FUNCTION__,"allocate Vulkan command buffer");
	
	return CommandBuffer(*this,result);
	}

void CommandPool::freeCommandBuffer(CommandBuffer& commandBuffer)
	{
	/* Free the command buffer: */
	VkCommandBuffer commandBufferHandle=commandBuffer.getHandle();
	vkFreeCommandBuffers(device.getHandle(),commandPool,1,&commandBufferHandle);
	}

CommandBuffer CommandPool::beginOneshotCommand(void)
	{
	/* Allocate a command buffer and begin recording immediately: */
	CommandBuffer result=allocateCommandBuffer();
	result.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	return result;
	}

void CommandPool::executeOneshotCommand(CommandBuffer& commandBuffer)
	{
	/* End recording into the given command buffer: */
	commandBuffer.end();
	
	/* Submit the command buffer to the device's rendering queue and wait until it is executed: */
	device.submitRenderingCommand(commandBuffer);
	device.waitRenderingQueue();
	}

}

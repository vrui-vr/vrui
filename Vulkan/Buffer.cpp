/***********************************************************************
Buffer - Class representing Vulkan buffers and their associated memory
blocks.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <Vulkan/Buffer.h>

#include <Misc/StdError.h>
#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/MemoryAllocator.h>
#include <Vulkan/CommandPool.h>
#include <Vulkan/CommandBuffer.h>

namespace Vulkan {

/***********************
Methods of class Buffer:
***********************/

Buffer::Buffer(Device& sDevice,VkDeviceSize size,VkBufferUsageFlags usage,MemoryAllocator& allocator,VkMemoryPropertyFlags properties,bool exportable)
	:MemoryBacked(sDevice),
	 buffer(VK_NULL_HANDLE)
	{
	/* Set up a buffer creation structure: */
	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext=0;
	bufferCreateInfo.flags=0x0;
	bufferCreateInfo.size=size;
	bufferCreateInfo.usage=usage;
	bufferCreateInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount=0;
	bufferCreateInfo.pQueueFamilyIndices=0;
	
	/* Check if the buffer is supposed to be externally visible: */
	VkExternalMemoryBufferCreateInfoKHR externalMemoryBufferCreateInfo;
	if(exportable)
		{
		/* Set up an external buffer creation structure: */
		externalMemoryBufferCreateInfo.sType=VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
		externalMemoryBufferCreateInfo.pNext=0;
		externalMemoryBufferCreateInfo.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
		
		/* Link the structure to the base structure: */
		bufferCreateInfo.pNext=&externalMemoryBufferCreateInfo;
		}
	
	/* Create the buffer: */
	throwOnError(vkCreateBuffer(device.getHandle(),&bufferCreateInfo,0,&buffer),
	             __PRETTY_FUNCTION__,"create Vulkan buffer");
	
	/* Query the buffer's memory requirements: */
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device.getHandle(),buffer,&memoryRequirements);
	
	/* Allocate device memory: */
	allocation=allocator.allocate(memoryRequirements,properties,exportable);
	
	/* Associate the allocated memory with the buffer: */
	VkResult bindResult=vkBindBufferMemory(device.getHandle(),buffer,allocation.getHandle(),allocation.getOffset());
	if(bindResult!=VK_SUCCESS)
		{
		/* Destroy the buffer and throw an exception: */
		vkDestroyBuffer(device.getHandle(),buffer,0);
		throwOnError(bindResult,__PRETTY_FUNCTION__,"bind device memory to buffer");
		}
	}

Buffer::Buffer(Buffer&& source)
	:MemoryBacked(std::move(source)),
	 buffer(source.buffer)
	{
	/* Invalidate the source object: */
	source.buffer=VK_NULL_HANDLE;
	}

Buffer::~Buffer(void)
	{
	/* Destroy the buffer: */
	vkDestroyBuffer(device.getHandle(),buffer,0);
	}

Buffer& Buffer::operator=(Buffer&& source)
	{
	if(this!=&source)
		{
		/* Destroy the current buffer: */
		vkDestroyBuffer(device.getHandle(),buffer,0);
		
		MemoryBacked::operator=(std::move(source));
		buffer=source.buffer;
		
		/* Invalidate the source object: */
		source.buffer=VK_NULL_HANDLE;
		}
	
	return *this;
	}

void Buffer::copy(VkDeviceSize destOffset,const Buffer& source,VkDeviceSize sourceOffset,VkDeviceSize size,CommandPool& commandPool)
	{
	/* Check parameters for validity: */
	if(destOffset+size>getSize())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Destination buffer range is out of bounds");
	if(sourceOffset+size>source.getSize())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source buffer range is out of bounds");
	
	/* Begin recording into a transient command buffer: */
	CommandBuffer commandBuffer=commandPool.beginOneshotCommand();
	
	/* Execute a buffer copy command to copy the source buffer's contents into this buffer: */
	VkBufferCopy bufferCopy;
	bufferCopy.srcOffset=sourceOffset;
	bufferCopy.dstOffset=destOffset;
	bufferCopy.size=size;
	vkCmdCopyBuffer(commandBuffer.getHandle(),source.getHandle(),buffer,1,&bufferCopy);
	
	/* Finish the command buffer and execute the command: */
	commandPool.executeOneshotCommand(commandBuffer);
	}

}

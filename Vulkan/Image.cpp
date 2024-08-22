/***********************************************************************
Image - Class representing Vulkan images and their associated memory
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

#include <Vulkan/Image.h>

#include <Misc/StdError.h>
#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/Buffer.h>
#include <Vulkan/CommandPool.h>
#include <Vulkan/CommandBuffer.h>

namespace Vulkan {

/***********************
Methods of class Buffer:
***********************/

Image::Image(Device& sDevice,VkImageType type,VkFormat initialFormat,const VkExtent3D& sExtent,VkImageTiling tiling,VkImageUsageFlags usage,bool keepPixels,MemoryAllocator& allocator,VkMemoryPropertyFlags properties,bool exportable)
	:MemoryBacked(sDevice),
	 image(VK_NULL_HANDLE),
	 format(initialFormat),layout(keepPixels?VK_IMAGE_LAYOUT_PREINITIALIZED:VK_IMAGE_LAYOUT_UNDEFINED),extent(sExtent)
	{
	/* Set up an image creation structure: */
	VkImageCreateInfo imageCreateInfo;
	imageCreateInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext=0;
	imageCreateInfo.flags=0x0;
	imageCreateInfo.imageType=type;
	imageCreateInfo.format=format;
	imageCreateInfo.extent=extent;
	imageCreateInfo.mipLevels=1;
	imageCreateInfo.arrayLayers=1;
	imageCreateInfo.samples=VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling=tiling;
	imageCreateInfo.usage=usage;
	imageCreateInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount=0;
	imageCreateInfo.pQueueFamilyIndices=0;
	imageCreateInfo.initialLayout=layout;
	
	/* Check if the image is supposed to be externally visible: */
	VkExternalMemoryImageCreateInfoKHR externalMemoryImageCreateInfo;
	if(exportable)
		{
		/* Set up an external image creation structure: */
		externalMemoryImageCreateInfo.sType=VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
		externalMemoryImageCreateInfo.pNext=0;
		externalMemoryImageCreateInfo.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
		
		/* Link the structure to the base structure: */
		imageCreateInfo.pNext=&externalMemoryImageCreateInfo;
		}
	
	/* Create the image: */
	throwOnError(vkCreateImage(device.getHandle(),&imageCreateInfo,0,&image),
	             __PRETTY_FUNCTION__,"create Vulkan image");
	
	/* Query the image's memory requirements: */
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device.getHandle(),image,&memoryRequirements);
	
	/* Allocate a chunk of memory to back the image: */
	allocation=allocator.allocate(memoryRequirements,properties,exportable);
	
	/* Associate the given memory with the image: */
	throwOnError(vkBindImageMemory(device.getHandle(),image,allocation.getHandle(),allocation.getOffset()),
	             __PRETTY_FUNCTION__,"bind device memory to image");
	}

Image::Image(Image&& source)
	:MemoryBacked(std::move(source)),
	 image(source.image),
	 format(source.format),layout(source.layout),extent(source.extent)
	{
	/* Invalidate the source object: */
	source.image=VK_NULL_HANDLE;
	}

Image::~Image(void)
	{
	/* Destroy the image: */
	vkDestroyImage(device.getHandle(),image,0);
	}

void Image::transitionLayout(VkImageLayout newLayout,CommandPool& commandPool)
	{
	/* Begin recording into a transient command buffer: */
	CommandBuffer commandBuffer=commandPool.beginOneshotCommand();
	
	/* Execute an image memory barrier command to transition the image layout: */
	VkImageMemoryBarrier barrier;
	barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext=0;
	
	/* Determine barrier stages and access masks based on the current and new image layout: */
	VkPipelineStageFlags srcStage=0x0;
	VkPipelineStageFlags dstStage=0x0;
	if(layout==VK_IMAGE_LAYOUT_UNDEFINED)
		{
		if(newLayout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
			/* Transition image from undefined to buffer upload: */
			barrier.srcAccessMask=0x0;
			srcStage=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
			dstStage=VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
		else if(newLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
			/* Transition image from undefined to optimal shader access: */
			barrier.srcAccessMask=0x0;
			srcStage=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
			dstStage=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported layout transition");
		}
	else if(layout==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL&&newLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
		/* Transition image from buffer upload to optimal shader access: */
		barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage=VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
		dstStage=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported layout transition");
	
	barrier.oldLayout=layout;
	barrier.newLayout=newLayout;
	barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	barrier.image=image;
	barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel=0;
	barrier.subresourceRange.levelCount=1;
	barrier.subresourceRange.baseArrayLayer=0;
	barrier.subresourceRange.layerCount=1;
	
	vkCmdPipelineBarrier(commandBuffer.getHandle(),srcStage,dstStage,0,0,0,0,0,1,&barrier);
	
	/* Finish the command buffer and execute the command: */
	commandPool.executeOneshotCommand(commandBuffer);
	
	/* Remember the new image layout: */
	layout=newLayout;
	}

void Image::copyFromBuffer(Buffer& buffer,const Rect& rect,CommandPool& commandPool)
	{
	/* Begin recording into a transient command buffer: */
	CommandBuffer commandBuffer=commandPool.beginOneshotCommand();
	
	/* Execute a buffer image copy command to copy buffer contents into the image: */
	VkBufferImageCopy bufferImageCopy;
	bufferImageCopy.bufferOffset=0;
	bufferImageCopy.bufferRowLength=0;
	bufferImageCopy.bufferImageHeight=0;
	bufferImageCopy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	bufferImageCopy.imageSubresource.mipLevel=0;
	bufferImageCopy.imageSubresource.baseArrayLayer=0;
	bufferImageCopy.imageSubresource.layerCount=1;
	bufferImageCopy.imageOffset.x=rect.offset[0];
	bufferImageCopy.imageOffset.y=rect.offset[1];
	bufferImageCopy.imageOffset.z=0;
	bufferImageCopy.imageExtent.width=rect.size[0];
	bufferImageCopy.imageExtent.height=rect.size[1];
	bufferImageCopy.imageExtent.depth=1;
	
	vkCmdCopyBufferToImage(commandBuffer.getHandle(),buffer.getHandle(),image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&bufferImageCopy);
	
	/* Finish the command buffer and execute the command: */
	commandPool.executeOneshotCommand(commandBuffer);
	}

}

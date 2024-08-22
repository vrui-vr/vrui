/***********************************************************************
CommandBuffer - Class representing Vulkan command buffers.
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

#include <Vulkan/CommandBuffer.h>

#include <Vulkan/Common.h>
#include <Vulkan/Semaphore.h>
#include <Vulkan/Fence.h>
#include <Vulkan/CommandPool.h>
#include <Vulkan/PipelineLayout.h>
#include <Vulkan/RenderPass.h>
#include <Vulkan/GraphicsPipeline.h>
#include <Vulkan/DescriptorSet.h>
#include <Vulkan/Framebuffer.h>

namespace Vulkan {

/******************************
Methods of class CommandBuffer:
******************************/

CommandBuffer::CommandBuffer(CommandPool& sCommandPool,VkCommandBuffer sCommandBuffer)
	:commandPool(sCommandPool),
	 commandBuffer(sCommandBuffer)
	{
	}

CommandBuffer::CommandBuffer(CommandBuffer&& source)
	:commandPool(source.commandPool),
	 commandBuffer(source.commandBuffer)
	{
	/* Invalidate the source: */
	source.commandBuffer=VK_NULL_HANDLE;
	}

CommandBuffer::~CommandBuffer(void)
	{
	/* Free the command buffer: */
	commandPool.freeCommandBuffer(*this);
	}
void CommandBuffer::reset(VkFlags flags)
	{
	/* Execute the command: */
	vkResetCommandBuffer(commandBuffer,flags);
	}

void CommandBuffer::begin(VkFlags flags)
	{
	/* Set up the begin buffer command structure: */
	VkCommandBufferBeginInfo commandBufferBeginInfo;
	commandBufferBeginInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext=0;
	commandBufferBeginInfo.flags=flags;
	commandBufferBeginInfo.pInheritanceInfo=0;
	
	/* Begin the command buffer: */
	throwOnError(vkBeginCommandBuffer(commandBuffer,&commandBufferBeginInfo),
	             __PRETTY_FUNCTION__,"execute Vulkan command");
	}

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask,VkPipelineStageFlags dstStageMask,VkDependencyFlags dependencyFlags,const VkMemoryBarrier& memoryBarrier)
	{
	/* Execute the command: */
	vkCmdPipelineBarrier(commandBuffer,srcStageMask,dstStageMask,dependencyFlags,1,&memoryBarrier,0,0,0,0);
	}

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask,VkPipelineStageFlags dstStageMask,VkDependencyFlags dependencyFlags,const VkBufferMemoryBarrier& bufferMemoryBarrier)
	{
	/* Execute the command: */
	vkCmdPipelineBarrier(commandBuffer,srcStageMask,dstStageMask,dependencyFlags,0,0,1,&bufferMemoryBarrier,0,0);
	}

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask,VkPipelineStageFlags dstStageMask,VkDependencyFlags dependencyFlags,const VkImageMemoryBarrier& imageMemoryBarrier)
	{
	/* Execute the command: */
	vkCmdPipelineBarrier(commandBuffer,srcStageMask,dstStageMask,dependencyFlags,0,0,0,0,1,&imageMemoryBarrier);
	}

void CommandBuffer::beginRenderPass(const RenderPass& renderPass,const Framebuffer& framebuffer,const VkRect2D& renderArea,const std::vector<VkClearValue>& clearValues,bool subPassInline)
	{
	/* Set up the begin render pass command structure: */
	VkRenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext=0;
	renderPassBeginInfo.renderPass=renderPass.getHandle();
	renderPassBeginInfo.framebuffer=framebuffer.getHandle();
	renderPassBeginInfo.renderArea=renderArea;
	renderPassBeginInfo.clearValueCount=clearValues.size();
	renderPassBeginInfo.pClearValues=renderPassBeginInfo.clearValueCount>0?clearValues.data():0;
	
	/* Execute the command: */
	vkCmdBeginRenderPass(commandBuffer,&renderPassBeginInfo,subPassInline?VK_SUBPASS_CONTENTS_INLINE:VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	}

void CommandBuffer::bindPipeline(const GraphicsPipeline& pipeline)
	{
	/* Execute the command: */
	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline.getHandle());
	}

void CommandBuffer::setViewport(uint32_t viewportIndex,const VkViewport& viewport)
	{
	/* Execute the command: */
	vkCmdSetViewport(commandBuffer,viewportIndex,1,&viewport);
	}

void CommandBuffer::setViewport(uint32_t viewportIndex,const Misc::Rect<2>& viewportRect,float minDepth,float maxDepth)
	{
	/* Create a Vulkan viewport structure: */
	VkViewport viewport;
	viewport.x=float(viewportRect.offset[0]);
	viewport.y=float(viewportRect.offset[1]);
	viewport.width=float(viewportRect.size[0]);
	viewport.height=float(viewportRect.size[1]);
	viewport.minDepth=minDepth;
	viewport.maxDepth=maxDepth;
	
	/* Execute the command: */
	vkCmdSetViewport(commandBuffer,viewportIndex,1,&viewport);
	}

void CommandBuffer::setScissor(uint32_t scissorIndex,const VkRect2D& scissor)
	{
	/* Execute the command: */
	vkCmdSetScissor(commandBuffer,scissorIndex,1,&scissor);
	}

void CommandBuffer::setScissor(uint32_t scissorIndex,const Misc::Rect<2>& scissorRect)
	{
	/* Create a Vulkan rectangle: */
	VkRect2D scissor;
	scissor.offset.x=scissorRect.offset[0];
	scissor.offset.y=scissorRect.offset[1];
	scissor.extent.width=scissorRect.size[0];
	scissor.extent.height=scissorRect.size[1];
	
	/* Execute the command: */
	vkCmdSetScissor(commandBuffer,scissorIndex,1,&scissor);
	}

void CommandBuffer::bindVertexBuffers(uint32_t firstBinding,const std::vector<CommandBuffer::BufferBinding>& bufferBindings)
	{
	/* Create lists of buffer handles and buffer offsets: */
	std::vector<VkBuffer> bufferHandles;
	bufferHandles.reserve(bufferBindings.size());
	std::vector<VkDeviceSize> offsets;
	offsets.reserve(bufferBindings.size());
	for(std::vector<BufferBinding>::const_iterator bbIt=bufferBindings.begin();bbIt!=bufferBindings.end();++bbIt)
		{
		bufferHandles.push_back(bbIt->buffer);
		offsets.push_back(bbIt->offset);
		}
	
	/* Execute the command: */
	vkCmdBindVertexBuffers(commandBuffer,firstBinding,bufferBindings.size(),bufferHandles.data(),offsets.data());
	}

void CommandBuffer::bindVertexBuffers(uint32_t firstBinding,const Buffer& buffer,VkDeviceSize offset)
	{
	/* Execute the command: */
	VkBuffer bufferHandle=buffer.getHandle();
	vkCmdBindVertexBuffers(commandBuffer,firstBinding,1,&bufferHandle,&offset);
	}

void CommandBuffer::bindIndexBuffer(const Buffer& buffer,VkDeviceSize offset,VkIndexType indexType)
	{
	/* Execute the command: */
	vkCmdBindIndexBuffer(commandBuffer,buffer.getHandle(),offset,indexType);
	}

void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const std::vector<DescriptorSet>& descriptorSets,const std::vector<uint32_t>& dynamicOffsets)
	{
	/* Create a list of descriptor set handles: */
	std::vector<VkDescriptorSet> descriptorSetHandles;
	descriptorSetHandles.reserve(descriptorSets.size());
	for(std::vector<DescriptorSet>::const_iterator dsIt=descriptorSets.begin();dsIt!=descriptorSets.end();++dsIt)
		descriptorSetHandles.push_back(dsIt->getHandle());
	
	/* Execute the command: */
	vkCmdBindDescriptorSets(commandBuffer,pipelineBindPoint,pipelineLayout.getHandle(),firstSet,descriptorSetHandles.size(),descriptorSetHandles.data(),dynamicOffsets.size(),dynamicOffsets.data());
	}

void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const std::vector<DescriptorSet>& descriptorSets)
	{
	/* Create a list of descriptor set handles: */
	std::vector<VkDescriptorSet> descriptorSetHandles;
	descriptorSetHandles.reserve(descriptorSets.size());
	for(std::vector<DescriptorSet>::const_iterator dsIt=descriptorSets.begin();dsIt!=descriptorSets.end();++dsIt)
		descriptorSetHandles.push_back(dsIt->getHandle());
	
	/* Execute the command: */
	vkCmdBindDescriptorSets(commandBuffer,pipelineBindPoint,pipelineLayout.getHandle(),firstSet,descriptorSetHandles.size(),descriptorSetHandles.data(),0,0);
	}

void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const DescriptorSet& descriptorSet,const std::vector<uint32_t>& dynamicOffsets)
	{
	/* Execute the command: */
	VkDescriptorSet descriptorSetHandle=descriptorSet.getHandle();
	vkCmdBindDescriptorSets(commandBuffer,pipelineBindPoint,pipelineLayout.getHandle(),firstSet,1,&descriptorSetHandle,dynamicOffsets.size(),dynamicOffsets.data());
	}

void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const DescriptorSet& descriptorSet)
	{
	/* Execute the command: */
	VkDescriptorSet descriptorSetHandle=descriptorSet.getHandle();
	vkCmdBindDescriptorSets(commandBuffer,pipelineBindPoint,pipelineLayout.getHandle(),firstSet,1,&descriptorSetHandle,0,0);
	}

void CommandBuffer::pushConstants(const PipelineLayout& pipelineLayout,VkShaderStageFlags shaderStageFlags,uint32_t offset,uint32_t size,const void* values)
	{
	/* Execute the command: */
	vkCmdPushConstants(commandBuffer,pipelineLayout.getHandle(),shaderStageFlags,offset,size,values);
	}

void CommandBuffer::draw(uint32_t vertexCount,uint32_t instanceCount,uint32_t firstVertex,uint32_t firstInstance)
	{
	/* Execute the command: */
	vkCmdDraw(commandBuffer,vertexCount,instanceCount,firstVertex,firstInstance);
	}

void CommandBuffer::drawIndexed(uint32_t indexCount,uint32_t instanceCount,uint32_t firstIndex,int32_t vertexOffset,uint32_t firstInstance)
	{
	/* Execute the command: */
	vkCmdDrawIndexed(commandBuffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
	}

void CommandBuffer::endRenderPass(void)
	{
	/* Execute the command: */
	vkCmdEndRenderPass(commandBuffer);
	}

void CommandBuffer::end(void)
	{
	/* End the command buffer: */
	throwOnError(vkEndCommandBuffer(commandBuffer),
	             __PRETTY_FUNCTION__,"execute Vulkan command");
	}

}

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

#ifndef VULKAN_COMMANDBUFFER_INCLUDED
#define VULKAN_COMMANDBUFFER_INCLUDED

#include <vector>
#include <Misc/Rect.h>
#include <vulkan/vulkan.h>
#include <Vulkan/Buffer.h>

/* Forward declarations: */
namespace Vulkan {
class Semaphore;
class Fence;
class CommandPool;
class PipelineLayout;
class RenderPass;
class GraphicsPipeline;
class DescriptorSet;
class Framebuffer;
}

namespace Vulkan {

class CommandBuffer
	{
	/* Embedded classes: */
	public:
	struct BufferBinding // Structure to bind a part of a buffer to a pipeline
		{
		/* Elements: */
		public:
		VkBuffer buffer; // Buffer to be bound
		VkDeviceSize offset; // Offset in the bound buffer where buffer data starts
		
		/* Constructors and destructors: */
		BufferBinding(const Buffer& sBuffer,VkDeviceSize sOffset) // Elementwise constructor
			:buffer(sBuffer.getHandle()),offset(sOffset)
			{
			}
		};
	
	/* Elements: */
	private:
	CommandPool& commandPool; // Command pool from which the command buffer was allocated
	VkCommandBuffer commandBuffer; // Vulkan command buffer handle
	
	/* Constructors and destructors: */
	public:
	CommandBuffer(CommandPool& sCommandPool,VkCommandBuffer sCommandBuffer); // Construction from Vulkan command buffer handle
	private:
	CommandBuffer(const CommandBuffer& source); // Prohibit copy constructor
	CommandBuffer& operator=(const CommandBuffer& source); // Prohibit assignment operator
	public:
	CommandBuffer(CommandBuffer&& source); // Move constructor
	~CommandBuffer(void); // Returns the command buffer to its originating command pool
	
	/* Methods: */
	VkCommandBuffer getHandle(void) const // Returns the Vulkan command buffer handle
		{
		return commandBuffer;
		}
	void reset(VkFlags flags); // Resets the command buffer for new recording
	void begin(VkFlags flags); // Begins recording into the buffer
	void pipelineBarrier(VkPipelineStageFlags srcStageMask,VkPipelineStageFlags dstStageMask,VkDependencyFlags dependencyFlags,const VkMemoryBarrier& memoryBarrier); // Inserts a memory pipeline barrier
	void pipelineBarrier(VkPipelineStageFlags srcStageMask,VkPipelineStageFlags dstStageMask,VkDependencyFlags dependencyFlags,const VkBufferMemoryBarrier& bufferMemoryBarrier); // Inserts a buffer memory pipeline barrier
	void pipelineBarrier(VkPipelineStageFlags srcStageMask,VkPipelineStageFlags dstStageMask,VkDependencyFlags dependencyFlags,const VkImageMemoryBarrier& imageMemoryBarrier); // Inserts an image memory pipeline barrier
	void beginRenderPass(const RenderPass& renderPass,const Framebuffer& framebuffer,const VkRect2D& renderArea,const std::vector<VkClearValue>& clearValues,bool subPassInline =true); // Begins a render pass into the given framebuffer
	void bindPipeline(const GraphicsPipeline& pipeline); // Binds the given graphics pipeline
	void setViewport(uint32_t viewportIndex,const VkViewport& viewport); // Sets a dynamic viewport
	void setViewport(uint32_t viewportIndex,const Misc::Rect<2>& viewportRect,float minDepth,float maxDepth); // Ditto
	void setScissor(uint32_t scissorIndex,const VkRect2D& scissor); // Sets a dynamic scissor rectangle
	void setScissor(uint32_t scissorIndex,const Misc::Rect<2>& scissorRect); // Ditto
	void bindVertexBuffers(uint32_t firstBinding,const std::vector<BufferBinding>& bufferBindings); // Binds a set of vertex buffers to the pipeline
	void bindVertexBuffers(uint32_t firstBinding,const Buffer& buffer,VkDeviceSize offset); // Ditto, with a single buffer
	void bindIndexBuffer(const Buffer& buffer,VkDeviceSize offset,VkIndexType indexType); // Binds an index buffer to the pipeline
	void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const std::vector<DescriptorSet>& descriptorSets,const std::vector<uint32_t>& dynamicOffsets); // Binds a set of descriptor sets to the given pipeline
	void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const std::vector<DescriptorSet>& descriptorSets); // Ditto, without dynamic offsets
	void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const DescriptorSet& descriptorSet,const std::vector<uint32_t>& dynamicOffsets); // Ditto, with a single descriptor set
	void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,const PipelineLayout& pipelineLayout,uint32_t firstSet,const DescriptorSet& descriptorSet); // Ditto, with a single descriptor set and without dynamic offsets
	void pushConstants(const PipelineLayout& pipelineLayout,VkShaderStageFlags shaderStageFlags,uint32_t offset,uint32_t size,const void* values); // Pushes a set of constants to the shader
	void draw(uint32_t vertexCount,uint32_t instanceCount,uint32_t firstVertex,uint32_t firstInstance); // Draws a set of vertices and/or instances
	void drawIndexed(uint32_t indexCount,uint32_t instanceCount,uint32_t firstIndex,int32_t vertexOffset,uint32_t firstInstance); // Draws a set of indexed vertices and/or instances
	void endRenderPass(void); // Ends the current render pass
	void end(void); // Ends recording into the buffer
	};

}

#endif

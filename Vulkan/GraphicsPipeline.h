/***********************************************************************
GraphicsPipeline - Class representing Vulkan graphics pipelines.
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

#ifndef VULKAN_GRAPHICSPIPELINE_INCLUDED
#define VULKAN_GRAPHICSPIPELINE_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/Pipeline.h>

/* Forward declarations: */
namespace Vulkan {
class ShaderModule;
class PipelineLayout;
class RenderPass;
}

namespace Vulkan {

class GraphicsPipeline:public Pipeline
	{
	/* Embedded classes: */
	public:
	class Constructor // Helper class to construct graphics pipeline objects
		{
		friend class GraphicsPipeline;
		
		/* Elements: */
		private:
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages; // List of shader stages
		VkPipelineVertexInputStateCreateFlags vertexInputFlags; // Vertex input flags
		std::vector<VkVertexInputBindingDescription> vertexInputBindings; // List of vertex input bindings
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes; // List of vertex input attributes
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
		std::vector<VkViewport> viewports; // List of viewports
		std::vector<VkRect2D> scissors; // List of scissor rectangles
		VkPipelineRasterizationStateCreateInfo rasterizationState;
		VkPipelineMultisampleStateCreateInfo multisampleState;
		VkPipelineDepthStencilStateCreateInfo depthStencilState;
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments; // List of per-framebuffer color blend attachments
		VkPipelineColorBlendStateCreateInfo colorBlendState;
		std::vector<VkDynamicState> dynamicStates; // List of dynamic pipeline states
		bool dynamicViewport,dynamicScissor; // Flag if viewports and scissor rectangles are part of dynamic state, respectively
		unsigned int numDynamicViewports,numDynamicScissors; // Number of dynamic viewports and scissor rectangles, respectively
		VkPipelineDynamicStateCreateInfo dynamicState;
		
		/* Constructors and destructors: */
		public:
		Constructor(void); // Creates an "empty" graphics pipeline setup
		
		/* Methods: */
		void addShaderStage(const ShaderModule& shaderModule); // Adds the given shader module as a new shader stage of the graphics pipeline; assumes that stage has not been attached yet
		void setVertexInputFlags(VkPipelineVertexInputStateCreateFlags newVertexInputFlags); // Sets the vertex input flags
		void addVertexInputBinding(const VkVertexInputBindingDescription& newBindingDescription); // Adds a vertex input binding
		void addVertexInputBinding(uint32_t binding,uint32_t stride,VkVertexInputRate inputRate); // Ditto, element-wise
		void addVertexInputAttribute(const VkVertexInputAttributeDescription& newAttributeDescription); // Adds a vertex input attribute
		void addVertexInputAttribute(uint32_t location,uint32_t binding,VkFormat format,uint32_t offset); // Ditto, element-wise
		VkPipelineInputAssemblyStateCreateInfo& getInputAssemblyState(void)
			{
			return inputAssemblyState;
			}
		void setInputAssemblyPrimitiveTopology(VkPrimitiveTopology topology); // Sets the input assembly primitive topology
		void setInputAssemblyPrimitiveRestart(bool primitiveRestart); // Sets the primitive restart enable flag
		void addViewport(const VkViewport& viewport); // Adds the given viewport
		void addScissor(const VkRect2D& scissor); // Adds the given scissor rectangle
		VkPipelineRasterizationStateCreateInfo& getRasterizationState(void)
			{
			return rasterizationState;
			}
		VkPipelineMultisampleStateCreateInfo& getMultisampleState(void)
			{
			return multisampleState;
			}
		VkPipelineDepthStencilStateCreateInfo& getDepthStencilState(void)
			{
			return depthStencilState;
			}
		void addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& colorBlendAttachment); // Adds the given color blend attachment
		VkPipelineColorBlendStateCreateInfo& getColorBlendState(void)
			{
			return colorBlendState;
			}
		void addDynamicState(VkDynamicState state); // Adds a dynamic state
		void addDynamicViewports(unsigned int newNumViewports); // Adds the given number of viewports to dynamic state
		void addDynamicScissors(unsigned int newNumScissors); // Adds the given number of scissor rectangles to dynamic state
		VkPipelineDynamicStateCreateInfo& getDynamicState(void)
			{
			return dynamicState;
			}
		};
	
	/* Constructors and destructors: */
	public:
	GraphicsPipeline(Device& sDevice,const Constructor& constructor,const PipelineLayout& layout,const RenderPass& renderPass,uint32_t subpass,const GraphicsPipeline* basePipeline =0,int32_t basePipelineIndex =-1); // Creates a graphics pipeline for the given logical device
	GraphicsPipeline(GraphicsPipeline&& source); // Move constructor
	};

}

#endif

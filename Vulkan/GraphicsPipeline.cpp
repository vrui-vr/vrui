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

#include "GraphicsPipeline.h"

#include <string.h>
#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/ShaderModule.h>
#include <Vulkan/PipelineLayout.h>
#include <Vulkan/RenderPass.h>

namespace Vulkan {

/**********************************************
Methods of class GraphicsPipeline::Constructor:
**********************************************/

GraphicsPipeline::Constructor::Constructor(void)
	:vertexInputFlags(0x0U),
	 dynamicViewport(false),dynamicScissor(false),
	 numDynamicViewports(0),numDynamicScissors(0)
	{
	/* Initialize fixed-function setup structures: */
	memset(&inputAssemblyState,0,sizeof(inputAssemblyState));
	inputAssemblyState.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	
	memset(&rasterizationState,0,sizeof(rasterizationState));
	rasterizationState.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	
	memset(&multisampleState,0,sizeof(multisampleState));
	multisampleState.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	
	memset(&depthStencilState,0,sizeof(depthStencilState));
	depthStencilState.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	
	memset(&colorBlendState,0,sizeof(colorBlendState));
	colorBlendState.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	
	memset(&dynamicState,0,sizeof(dynamicState));
	dynamicState.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	}

void GraphicsPipeline::Constructor::addShaderStage(const ShaderModule& shaderModule)
	{
	/* Create a shader stage setup structure: */
	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
	pipelineShaderStageCreateInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineShaderStageCreateInfo.pNext=0;
	pipelineShaderStageCreateInfo.flags=0x0;
	switch(shaderModule.getStage())
		{
		case ShaderModule::Vertex:
			pipelineShaderStageCreateInfo.stage=VK_SHADER_STAGE_VERTEX_BIT;
			break;
		
		case ShaderModule::TessellationControl:
			pipelineShaderStageCreateInfo.stage=VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			break;
		
		case ShaderModule::TessellationEvaluation:
			pipelineShaderStageCreateInfo.stage=VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			break;
		
		case ShaderModule::Geometry:
			pipelineShaderStageCreateInfo.stage=VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		
		case ShaderModule::Fragment:
			pipelineShaderStageCreateInfo.stage=VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		
		case ShaderModule::Compute:
			pipelineShaderStageCreateInfo.stage=VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		
		default:
			; // We'll never get here; just to make compiler happy
		}
	pipelineShaderStageCreateInfo.module=shaderModule.getHandle();
	pipelineShaderStageCreateInfo.pName=shaderModule.getEntryPoint().c_str();
	pipelineShaderStageCreateInfo.pSpecializationInfo=0;
	
	/* Add the setup structure to the list: */
	shaderStages.push_back(pipelineShaderStageCreateInfo);
	}

void GraphicsPipeline::Constructor::setVertexInputFlags(VkPipelineVertexInputStateCreateFlags newVertexInputFlags)
	{
	vertexInputFlags=newVertexInputFlags;
	}

void GraphicsPipeline::Constructor::addVertexInputBinding(const VkVertexInputBindingDescription& newBindingDescription)
	{
	/* Add the binding description to the list: */
	vertexInputBindings.push_back(newBindingDescription);
	}

void GraphicsPipeline::Constructor::addVertexInputBinding(uint32_t binding,uint32_t stride,VkVertexInputRate inputRate)
	{
	/* Create a vertex input binding description: */
	VkVertexInputBindingDescription vertexInputBindingDescription;
	vertexInputBindingDescription.binding=binding;
	vertexInputBindingDescription.stride=stride;
	vertexInputBindingDescription.inputRate=inputRate;
	
	/* Add the binding description to the list: */
	vertexInputBindings.push_back(vertexInputBindingDescription);
	}

void GraphicsPipeline::Constructor::addVertexInputAttribute(const VkVertexInputAttributeDescription& newAttributeDescription)
	{
	/* Add the attribute description to the list: */
	vertexInputAttributes.push_back(newAttributeDescription);
	}

void GraphicsPipeline::Constructor::addVertexInputAttribute(uint32_t location,uint32_t binding,VkFormat format,uint32_t offset)
	{
	/* Create a vertex input attribute description: */
	VkVertexInputAttributeDescription vertexInputAttributeDescription;
	vertexInputAttributeDescription.location=location;
	vertexInputAttributeDescription.binding=binding;
	vertexInputAttributeDescription.format=format;
	vertexInputAttributeDescription.offset=offset;
	
	/* Add the attribute description to the list: */
	vertexInputAttributes.push_back(vertexInputAttributeDescription);
	}

void GraphicsPipeline::Constructor::setInputAssemblyPrimitiveTopology(VkPrimitiveTopology topology)
	{
	inputAssemblyState.topology=topology;
	}

void GraphicsPipeline::Constructor::setInputAssemblyPrimitiveRestart(bool primitiveRestart)
	{
	inputAssemblyState.primitiveRestartEnable=primitiveRestart;
	}

void GraphicsPipeline::Constructor::addViewport(const VkViewport& viewport)
	{
	viewports.push_back(viewport);
	}

void GraphicsPipeline::Constructor::addScissor(const VkRect2D& scissor)
	{
	scissors.push_back(scissor);
	}

void GraphicsPipeline::Constructor::addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
	{
	colorBlendAttachments.push_back(colorBlendAttachment);
	
	/* Update the color blend state creation structure: */
	colorBlendState.attachmentCount=colorBlendAttachments.size();
	colorBlendState.pAttachments=colorBlendAttachments.data();
	}

void GraphicsPipeline::Constructor::addDynamicState(VkDynamicState state)
	{
	dynamicStates.push_back(state);
	
	/* Update the dynamic state creation structure: */
	dynamicState.dynamicStateCount=dynamicStates.size();
	dynamicState.pDynamicStates=dynamicStates.data();
	}

void GraphicsPipeline::Constructor::addDynamicViewports(unsigned int newNumDynamicViewports)
	{
	/* Add viewports to dynamic state if not already done: */
	if(!dynamicViewport)
		addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicViewport=true;
	
	/* Remember the number of requested viewports: */
	numDynamicViewports=newNumDynamicViewports;
	}

void GraphicsPipeline::Constructor::addDynamicScissors(unsigned int newNumDynamicScissors)
	{
	/* Add scissor rectangles to dynamic state if not already done: */
	if(!dynamicScissor)
		addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	dynamicScissor=true;
	
	/* Remember the number of requested scissor rectangles: */
	numDynamicScissors=newNumDynamicScissors;
	}

/*********************************
Methods of class GraphicsPipeline:
*********************************/

GraphicsPipeline::GraphicsPipeline(Device& sDevice,const Constructor& constructor,const PipelineLayout& layout,const RenderPass& renderPass,uint32_t subpass,const GraphicsPipeline* basePipeline,int32_t basePipelineIndex)
	:Pipeline(sDevice)
	{
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
	vertexInputStateCreateInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pNext=0;
	vertexInputStateCreateInfo.flags=constructor.vertexInputFlags;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount=constructor.vertexInputBindings.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions=constructor.vertexInputBindings.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount=constructor.vertexInputAttributes.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions=constructor.vertexInputAttributes.data();
	
	/* Set up a viewport state creation structure: */
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
	pipelineViewportStateCreateInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.pNext=0;
	pipelineViewportStateCreateInfo.flags=0x0;
	if(constructor.dynamicViewport)
		{
		pipelineViewportStateCreateInfo.viewportCount=uint32_t(constructor.numDynamicViewports);
		pipelineViewportStateCreateInfo.pViewports=0;
		}
	else
		{
		pipelineViewportStateCreateInfo.viewportCount=constructor.viewports.size();
		pipelineViewportStateCreateInfo.pViewports=pipelineViewportStateCreateInfo.viewportCount>0?constructor.viewports.data():0;
		}
	if(constructor.dynamicScissor)
		{
		pipelineViewportStateCreateInfo.scissorCount=constructor.numDynamicScissors;
		pipelineViewportStateCreateInfo.pScissors=0;
		}
	else
		{
		pipelineViewportStateCreateInfo.scissorCount=constructor.scissors.size();
		pipelineViewportStateCreateInfo.pScissors=pipelineViewportStateCreateInfo.scissorCount>0?constructor.scissors.data():0;
		}
	
	/* Set up the graphics pipeline creation structure: */
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
	graphicsPipelineCreateInfo.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pNext=0;
	graphicsPipelineCreateInfo.flags=0x0;
	graphicsPipelineCreateInfo.stageCount=constructor.shaderStages.size();
	graphicsPipelineCreateInfo.pStages=graphicsPipelineCreateInfo.stageCount>0?constructor.shaderStages.data():0;
	graphicsPipelineCreateInfo.pVertexInputState=&vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState=&constructor.inputAssemblyState;
	graphicsPipelineCreateInfo.pTessellationState=0;
	graphicsPipelineCreateInfo.pViewportState=&pipelineViewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState=&constructor.rasterizationState;
	graphicsPipelineCreateInfo.pMultisampleState=&constructor.multisampleState;
	graphicsPipelineCreateInfo.pDepthStencilState=&constructor.depthStencilState;
	graphicsPipelineCreateInfo.pColorBlendState=&constructor.colorBlendState;
	graphicsPipelineCreateInfo.pDynamicState=&constructor.dynamicState;
	graphicsPipelineCreateInfo.layout=layout.getHandle();
	graphicsPipelineCreateInfo.renderPass=renderPass.getHandle();
	graphicsPipelineCreateInfo.subpass=subpass;
	graphicsPipelineCreateInfo.basePipelineHandle=basePipeline!=0?basePipeline->getHandle():VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex=basePipelineIndex;
	
	/* Create the graphics pipeline object: */
	throwOnError(vkCreateGraphicsPipelines(device.getHandle(),VK_NULL_HANDLE,1,&graphicsPipelineCreateInfo,0,&pipeline),
	             "Vulkan::GraphicsPipeline","creating Vulkan graphics pipeline");
	}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& source)
	:Pipeline(std::move(source))
	{
	}

}

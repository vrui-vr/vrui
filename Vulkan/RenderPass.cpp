/***********************************************************************
RenderPass - Class representing Vulkan render passes.
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

#include <Vulkan/RenderPass.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/***************************
Methods of class RenderPass:
***************************/

RenderPass::RenderPass(Device& sDevice,const Constructor& constructor)
	:DeviceAttached(sDevice),
	 renderPass(VK_NULL_HANDLE)
	{
	/* Set up the render pass creation structure: */
	VkRenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext=0;
	renderPassCreateInfo.flags=0x0;
	renderPassCreateInfo.attachmentCount=constructor.attachments.size();
	renderPassCreateInfo.pAttachments=renderPassCreateInfo.attachmentCount>0?constructor.attachments.data():0;
	renderPassCreateInfo.subpassCount=constructor.subpasses.size();
	renderPassCreateInfo.pSubpasses=renderPassCreateInfo.subpassCount>0?constructor.subpasses.data():0;
	renderPassCreateInfo.dependencyCount=constructor.subpassDependencies.size();
	renderPassCreateInfo.pDependencies=renderPassCreateInfo.dependencyCount>0?constructor.subpassDependencies.data():0;
	
	/* Create the render pass object: */
	throwOnError(vkCreateRenderPass(device.getHandle(),&renderPassCreateInfo,0,&renderPass),
	             __PRETTY_FUNCTION__,"create Vulkan render pass object");
	}

RenderPass::RenderPass(RenderPass&& source)
	:DeviceAttached(std::move(source)),
	 renderPass(source.renderPass)
	{
	/* Invalidate the source: */
	source.renderPass=VK_NULL_HANDLE;
	}

RenderPass::~RenderPass(void)
	{
	/* Destroy the render pass object: */
	vkDestroyRenderPass(device.getHandle(),renderPass,0);
	}

}

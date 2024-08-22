/***********************************************************************
Framebuffer - Class representing Vulkan framebuffers.
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

#include <Vulkan/Framebuffer.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/RenderPass.h>

namespace Vulkan {

/****************************
Methods of class Framebuffer:
****************************/

Framebuffer::Framebuffer(Device& sDevice,const RenderPass& renderPass,const std::vector<VkImageView>& attachments,const VkExtent2D& size,uint32_t layers)
	:DeviceAttached(sDevice),
	 framebuffer(VK_NULL_HANDLE)
	{
	/* Set up the framebuffer creation structure: */
	VkFramebufferCreateInfo framebufferCreateInfo;
	framebufferCreateInfo.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext=0;
	framebufferCreateInfo.flags=0x0;
	framebufferCreateInfo.renderPass=renderPass.getHandle();
	framebufferCreateInfo.attachmentCount=attachments.size();
	framebufferCreateInfo.pAttachments=framebufferCreateInfo.attachmentCount>0?attachments.data():0;
	framebufferCreateInfo.width=size.width;
	framebufferCreateInfo.height=size.height;
	framebufferCreateInfo.layers=layers;
	
	/* Create the framebuffer object: */
	throwOnError(vkCreateFramebuffer(device.getHandle(),&framebufferCreateInfo,0,&framebuffer),
	             __PRETTY_FUNCTION__,"create Vulkan framebuffer object");
	}

Framebuffer::Framebuffer(Framebuffer&& source)
	:DeviceAttached(std::move(source)),
	 framebuffer(source.framebuffer)
	{
	/* Invalidate the source: */
	source.framebuffer=VK_NULL_HANDLE;
	}

Framebuffer::~Framebuffer(void)
	{
	/* Destroy the framebuffer: */
	vkDestroyFramebuffer(device.getHandle(),framebuffer,0);
	}

}

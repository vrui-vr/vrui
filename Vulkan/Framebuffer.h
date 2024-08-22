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

#ifndef VULKAN_FRAMEBUFFER_INCLUDED
#define VULKAN_FRAMEBUFFER_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>

#include <Vulkan/DeviceAttached.h>

/* Forward declarations: */
namespace Vulkan {
class RenderPass;
}

namespace Vulkan {

class Framebuffer:public DeviceAttached
	{
	/* Elements: */
	private:
	VkFramebuffer framebuffer; // Vulkan framebuffer handle
	
	/* Constructors and destructors: */
	public:
	Framebuffer(Device& sDevice,const RenderPass& renderPass,const std::vector<VkImageView>& attachments,const VkExtent2D& size,uint32_t layers); // Creates a framebuffer for the given device, render pass, and attachment list
	Framebuffer(Framebuffer&& source); // Move constructor
	virtual ~Framebuffer(void); // Destroys the framebuffer
	
	/* Methods: */
	VkFramebuffer getHandle(void) const // Returns the Vulkan framebuffer handle
		{
		return framebuffer;
		}
	};

}

#endif

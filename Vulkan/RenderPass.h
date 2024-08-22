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

#ifndef VULKAN_RENDERPASS_INCLUDED
#define VULKAN_RENDERPASS_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

namespace Vulkan {

class RenderPass:public DeviceAttached
	{
	/* Embedded classes: */
	public:
	class Constructor // Helper class to construct render pass objects
		{
		friend class RenderPass;
		
		/* Elements: */
		private:
		std::vector<VkAttachmentDescription> attachments; // List of attachments
		std::vector<VkSubpassDescription> subpasses; // List of subpasses
		std::vector<VkSubpassDependency> subpassDependencies; // List of subpass dependencies
		
		/* Methods: */
		public:
		void addAttachment(const VkAttachmentDescription& attachment) // Adds an attachment
			{
			attachments.push_back(attachment);
			}
		void addSubpass(const VkSubpassDescription& subpass) // Adds a subpass
			{
			subpasses.push_back(subpass);
			}
		void addSubpassDependency(const VkSubpassDependency& subpassDependency) // Adds a subpass dependency
			{
			subpassDependencies.push_back(subpassDependency);
			}
		};
	
	/* Elements: */
	private:
	VkRenderPass renderPass; // Vulkan render pass handle
	
	/* Constructors and destructors: */
	public:
	RenderPass(Device& sDevice,const Constructor& constructor); // Creates a render pass for the given device using the given constructor
	RenderPass(RenderPass&& source); // Move constructor
	virtual ~RenderPass(void); // Destroys the render pass
	
	/* Methods: */
	VkRenderPass getHandle(void) const // Returns the Vulkan render pass handle
		{
		return renderPass;
		}
	};

}

#endif

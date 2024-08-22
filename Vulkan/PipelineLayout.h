/***********************************************************************
PipelineLayout - Class representing Vulkan pipeline layouts.
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

#ifndef VULKAN_PIPELINELAYOUT_INCLUDED
#define VULKAN_PIPELINELAYOUT_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

namespace Vulkan {

class PipelineLayout:public DeviceAttached
	{
	/* Embedded classes: */
	public:
	class Constructor // Helper class to create pipeline layout objects
		{
		friend class PipelineLayout;
		
		/* Elements: */
		private:
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		std::vector<VkPushConstantRange> pushConstantRanges;
		
		/* Methods: */
		public:
		void addDescriptorSetLayout(const VkDescriptorSetLayout& descriptorSetLayout)
			{
			descriptorSetLayouts.push_back(descriptorSetLayout);
			}
		void addPushConstantRange(const VkPushConstantRange& pushConstantRange)
			{
			pushConstantRanges.push_back(pushConstantRange);
			}
		};
	
	/* Elements: */
	private:
	VkPipelineLayout pipelineLayout; // Vulkan pipeline layout handle
	
	/* Constructors and destructors: */
	public:
	PipelineLayout(Device& sDevice,const Constructor& constructor); // Creates a pipeline layout attached to the given logical device
	PipelineLayout(PipelineLayout&& source); // Move constructor
	virtual ~PipelineLayout(void); // Destroys the pipeline layout
	
	/* Methods: */
	VkPipelineLayout getHandle(void) const // Returns the Vulkan pipeline layout handle
		{
		return pipelineLayout;
		}
	};

}

#endif

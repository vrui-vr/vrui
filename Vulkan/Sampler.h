/***********************************************************************
Sampler - Class representing Vulkan texture samplers.
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

#ifndef VULKAN_SAMPLER_INCLUDED
#define VULKAN_SAMPLER_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

namespace Vulkan {

class Sampler:public DeviceAttached
	{
	/* Elements: */
	private:
	VkSampler sampler; // Vulkan sampler handle
	
	/* Constructors and destructors: */
	public:
	Sampler(Device& sDevice); // Creates a default sampler for the given logical device
	Sampler(Device& sDevice,const VkSamplerCreateInfo& samplerCreateInfo); // Creates a sampler for the given logical device with the given creation structure
	Sampler(Sampler&& source); // Move constructor
	virtual ~Sampler(void); // Destroys the sampler
	
	/* Methods: */
	VkSampler getHandle(void) const // Returns the Vulkan sampler handle
		{
		return sampler;
		}
	};

}

#endif

/***********************************************************************
Sampler - Class representing Vulkan texture samplers.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <Vulkan/Sampler.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/************************
Methods of class Sampler:
************************/

Sampler::Sampler(Device& sDevice)
	:DeviceAttached(sDevice),
	 sampler(VK_NULL_HANDLE)
	{
	/* Set up a sampler creation structure: */
	VkSamplerCreateInfo samplerCreateInfo;
	samplerCreateInfo.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext=0;
	samplerCreateInfo.flags=0x0;
	samplerCreateInfo.magFilter=VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter=VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias=0.0f;
	samplerCreateInfo.anisotropyEnable=VK_FALSE;
	samplerCreateInfo.maxAnisotropy=0.0f;
	samplerCreateInfo.compareEnable=VK_FALSE;
	samplerCreateInfo.compareOp=VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod=0.0f;
	samplerCreateInfo.maxLod=0.0f;
	samplerCreateInfo.borderColor=VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates=VK_FALSE;
	
	/* Create the sampler: */
	throwOnError(vkCreateSampler(device.getHandle(),&samplerCreateInfo,0,&sampler),
	             __PRETTY_FUNCTION__,"create Vulkan sampler");
	}

Sampler::Sampler(Device& sDevice,const VkSamplerCreateInfo& samplerCreateInfo)
	:DeviceAttached(sDevice),
	 sampler(VK_NULL_HANDLE)
	{
	/* Create the sampler: */
	throwOnError(vkCreateSampler(device.getHandle(),&samplerCreateInfo,0,&sampler),
	             __PRETTY_FUNCTION__,"create Vulkan sampler");
	}

Sampler::Sampler(Sampler&& source)
	:DeviceAttached(std::move(source)),
	 sampler(source.sampler)
	{
	/* Invalidate the source: */
	source.sampler=VK_NULL_HANDLE;
	}

Sampler::~Sampler(void)
	{
	/* Destroy the sampler: */
	vkDestroySampler(device.getHandle(),sampler,0);
	}

}

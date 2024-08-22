/***********************************************************************
Fence - Class representing Vulkan fences for CPU-GPU synchronization.
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

#include <Vulkan/Fence.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/**********************
Methods of class Fence:
**********************/

Fence::Fence(Device& sDevice,VkFence sFence)
	:DeviceAttached(sDevice),
	 fence(sFence)
	{
	}

Fence::Fence(Device& sDevice,bool createSignaled)
	:DeviceAttached(sDevice),
	 fence(VK_NULL_HANDLE)
	{
	/* Set up the semaphore creation structure: */
	VkFenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext=0;
	fenceCreateInfo.flags=0x0;
	if(createSignaled)
		fenceCreateInfo.flags|=VK_FENCE_CREATE_SIGNALED_BIT;
	
	/* Create the semaphore: */
	throwOnError(vkCreateFence(device.getHandle(),&fenceCreateInfo,0,&fence),
	             __PRETTY_FUNCTION__,"create Vulkan fence");
	}

Fence::Fence(Fence&& source)
	:DeviceAttached(std::move(source)),
	 fence(source.fence)
	{
	/* Invalidate the source object: */
	source.fence=VK_NULL_HANDLE;
	}

Fence::~Fence(void)
	{
	/* Destroy the fence: */
	vkDestroyFence(device.getHandle(),fence,0);
	}

void Fence::wait(bool reset)
	{
	/* Wait for the fence: */
	vkWaitForFences(device.getHandle(),1,&fence,VK_TRUE,UINT64_MAX);
	
	/* Reset the fence if requested: */
	if(reset)
		vkResetFences(device.getHandle(),1,&fence);
	}

void Fence::reset(void)
	{
	/* Reset the fence: */
	vkResetFences(device.getHandle(),1,&fence);
	}

}

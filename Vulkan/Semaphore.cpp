/***********************************************************************
Semaphore - Class representing Vulkan semaphores for GPU-GPU
synchronization.
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

#include <Vulkan/Semaphore.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>

namespace Vulkan {

/**************************
Methods of class Semaphore:
**************************/

Semaphore::Semaphore(Device& sDevice)
	:DeviceAttached(sDevice),
	 semaphore(VK_NULL_HANDLE)
	{
	/* Set up the semaphore creation structure: */
	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext=0;
	semaphoreCreateInfo.flags=0x0;
	
	/* Create the semaphore: */
	throwOnError(vkCreateSemaphore(device.getHandle(),&semaphoreCreateInfo,0,&semaphore),
	             __PRETTY_FUNCTION__,"create Vulkan semaphore");
	}

Semaphore::Semaphore(Semaphore&& source)
	:DeviceAttached(std::move(source)),
	 semaphore(source.semaphore)
	{
	/* Invalidate the source object: */
	source.semaphore=VK_NULL_HANDLE;
	}

Semaphore::~Semaphore(void)
	{
	/* Destroy the semaphore: */
	vkDestroySemaphore(device.getHandle(),semaphore,0);
	}

}

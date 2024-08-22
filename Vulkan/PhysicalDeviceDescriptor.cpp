/***********************************************************************
PhysicalDeviceDescriptor - Class to communicate the rendering setup of a
physical device between a Vulkan instance and a Vulkan logical device.
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

#include <Vulkan/PhysicalDeviceDescriptor.h>

#include <string.h>
#include <Misc/StdError.h>
#include <Vulkan/Surface.h>

namespace Vulkan {

/*****************************************
Methods of class PhysicalDeviceDescriptor:
*****************************************/

bool PhysicalDeviceDescriptor::findQueueFamilies(void)
	{
	/* Query the command queue families offered by the device: */
	uint32_t numQueueFamilies=0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,&numQueueFamilies,0);
	std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,&numQueueFamilies,queueFamilies.data());
	
	/* Find a command queue family that supports graphics and memory transfers: */
	renderingQueueFamilyIndex=~0U;
	for(uint32_t queueFamilyIndex=0;queueFamilyIndex<numQueueFamilies&&renderingQueueFamilyIndex==~0U;++queueFamilyIndex)
		{
		/* Check if the queue family supports rendering: */
		VkFlags flags=queueFamilies[queueFamilyIndex].queueFlags;
		if((flags&VK_QUEUE_GRAPHICS_BIT)&&(flags&VK_QUEUE_TRANSFER_BIT))
			renderingQueueFamilyIndex=queueFamilyIndex;
		}
	
	if(surface!=0)
		{
		/* Find a command queue family that supports presentation to the given surface: */
		presentationQueueFamilyIndex=~0U;
		for(uint32_t queueFamilyIndex=0;queueFamilyIndex<numQueueFamilies&&presentationQueueFamilyIndex==~0U;++queueFamilyIndex)
			{
			/* Check if the queue family supports presentation to the given surface: */
			VkBool32 canPresent=false;
			throwOnError(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice,queueFamilyIndex,surface->getHandle(),&canPresent),
			             __PRETTY_FUNCTION__,"query queue family surface support");
			if(canPresent)
				presentationQueueFamilyIndex=queueFamilyIndex;
			}
		
		return renderingQueueFamilyIndex!=~0U&&presentationQueueFamilyIndex!=~0U;
		}
	else
		return renderingQueueFamilyIndex!=~0U;
	}

PhysicalDeviceDescriptor::PhysicalDeviceDescriptor(Surface* sSurface)
	:physicalDevice(VK_NULL_HANDLE),
	 renderingQueueFamilyIndex(~0U),
	 surface(sSurface),presentationQueueFamilyIndex(~0U)
	{
	memset(&deviceFeatures,0,sizeof(deviceFeatures));
	}

void PhysicalDeviceDescriptor::setPhysicalDevice(VkPhysicalDevice newPhysicalDevice)
	{
	/* Update the physical device: */
	physicalDevice=newPhysicalDevice;
	
	/* Update the device queue families: */
	if(!findQueueFamilies())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No queue families found on new physical device");
	}

}

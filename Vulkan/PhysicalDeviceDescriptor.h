/***********************************************************************
PhysicalDeviceDescriptor - Class to communicate the rendering setup of a
physical device between a Vulkan instance and a Vulkan logical device.
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

#ifndef VULKAN_PHYSICALDEVICEDESCRIPTOR_INCLUDED
#define VULKAN_PHYSICALDEVICEDESCRIPTOR_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/Common.h>

/* Forward declarations: */
namespace Vulkan {
class Surface;
}

namespace Vulkan {

class PhysicalDeviceDescriptor
	{
	friend class Instance;
	friend class Device;
	
	/* Elements: */
	private:
	VkPhysicalDevice physicalDevice; // Vulkan physical device handle
	VkPhysicalDeviceFeatures deviceFeatures; // Required physical device features
	CStringList deviceExtensions; // Required device extensions
	CStringList validationLayers; // Required validation layers
	uint32_t renderingQueueFamilyIndex; // Index of a command queue to which to submit graphics and memory transfer commands
	Surface* surface; // Optional pointer to a surface to which to present rendering results
	uint32_t presentationQueueFamilyIndex; // Index of a command queue to which to submit presentation commands if a surface is defined
	
	/* Private methods: */
	bool findQueueFamilies(void); // Finds the physical device's rendering and optionally presentation queues; returns true if all required families were found
	
	/* Constructors and destructors: */
	public:
	PhysicalDeviceDescriptor(Surface* sSurface =0); // Creates invalid device descriptor with the given optional surface
	private:
	PhysicalDeviceDescriptor(const PhysicalDeviceDescriptor& source); // Prohibit copy constructor
	PhysicalDeviceDescriptor& operator=(const PhysicalDeviceDescriptor& source); // Prohibit assignment operator
	
	/* Methods: */
	public:
	void setPhysicalDevice(VkPhysicalDevice newPhysicalDevice); // Explicitly sets a physical device
	CStringList& getDeviceExtensions(void) // Returns the list of required device extensions
		{
		return deviceExtensions;
		}
	void addDeviceExtension(const char* extension) // Adds a device extension to the required list
		{
		deviceExtensions.push_back(extension);
		}
	VkPhysicalDeviceFeatures& getFeatures(void) // Returns the set of required device features
		{
		return deviceFeatures;
		}
	bool isValid(void) const // Returns true if the descriptor is valid
		{
		return physicalDevice!=VK_NULL_HANDLE;
		}
	};

}

#endif

/***********************************************************************
PhysicalDevice - Class representing Vulkan physical devices.
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

#ifndef VULKAN_PHYSICALDEVICE_INCLUDED
#define VULKAN_PHYSICALDEVICE_INCLUDED

#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/Common.h>

namespace Vulkan {

class PhysicalDevice
	{
	/* Elements: */
	private:
	VkPhysicalDevice physicalDevice; // Vulkan physical device handle
	
	/* Constructors and destructors: */
	public:
	PhysicalDevice(void) // Creates an invalid physical device
		:physicalDevice(VK_NULL_HANDLE)
		{
		}
	PhysicalDevice(VkPhysicalDevice sPhysicalDevice) // Creates a physical device object for the given Vulkan physical device handle
		:physicalDevice(sPhysicalDevice)
		{
		}
	
	/* Methods: */
	bool isValid(void) const // Returns true if the physical device is valid
		{
		return physicalDevice!=VK_NULL_HANDLE;
		}
	VkPhysicalDevice getHandle(void) const // Returns the Vulkan physical device handle
		{
		return physicalDevice;
		}
	std::string getDeviceName(void) const; // Returns the physical device's name
	VkPhysicalDeviceLimits getDeviceLimits(void) const; // Returns the physical device's limits
	CStringList getExtensions(void) const; // Returns the list of extensions supported by the physical device
	bool hasExtension(const char* extensionName) const; // Returns true if the physical device supports the given Vulkan extension
	
	/* Methods requiring VK_KHR_display extension: */
	std::vector<VkDisplayPropertiesKHR> getDisplayProperties(void) const; // Returns a list of properties for displays connected to the physical device
	std::vector<VkDisplayModePropertiesKHR> getDisplayModeProperties(VkDisplayKHR display) const; // Returns a list of mode properties for the given display
	std::vector<VkDisplayPlanePropertiesKHR> getDisplayPlaneProperties(void) const; // Returns a list of properties for display planes on the physical device
	std::vector<VkDisplayKHR> getDisplayPlaneSupportedDisplays(uint32_t displayPlaneIndex) const; // Returns the list of displays supported by the display plane of the given index
	VkDisplayPlaneCapabilitiesKHR getDisplayPlaneCapabilities(VkDisplayModeKHR displayMode,uint32_t displayPlaneIndex) const; // Returns capabilities of the display plane of the given index for the given display mode
	void dumpInfo(void) const; // Prints detailed information about the physical device to stdout
	};

}

#endif

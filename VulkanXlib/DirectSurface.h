/***********************************************************************
DirectSurface - Class representing Vulkan presentation surfaces
associated with a direct-mode X display using the Xlib API.
Copyright (c) 2022-2023 Oliver Kreylos

This file is part of the Vulkan Xlib Bindings Library (VulkanXlib).

The Vulkan Xlib Bindings Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vulkan Xlib Bindings Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vulkan Xlib Bindings Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
MA 02111-1307 USA
***********************************************************************/

#ifndef VULKANXLIB_DIRECTSURFACE_INCLUDED
#define VULKANXLIB_DIRECTSURFACE_INCLUDED

#include <string>
#include <X11/Xlib.h>
#include <vulkan/vulkan.h>
#include <Vulkan/Common.h>
#include <Vulkan/Surface.h>
#include <Vulkan/PhysicalDevice.h>

/* Forward declarations: */
namespace Vulkan {
class PhysicalDeviceDescriptor;
}
namespace VulkanXlib {
class XlibDisplay;
}

namespace VulkanXlib {

class DirectSurface:public Vulkan::Surface
	{
	/* Elements: */
	private:
	Vulkan::PhysicalDevice directDevice; // Physical device to which the direct display is connected
	VkDisplayKHR directDisplay; // Vulkan handle for the direct display
	uint32_t directDisplayPlaneIndex; // Index of the display plane to which the direct display is bound
	VkDisplayModeKHR directDisplayMode; // Mode for the direct display
	VkDisplayModeParametersKHR directDisplayModeParameters; // Parameters for the direct display mode
	
	/* Constructors and destructors: */
	public:
	DirectSurface(const Vulkan::Instance& sInstance,const XlibDisplay& xlibDisplay,const std::string& displayName,double targetRefreshRate); // Creates a surface for the given Vulkan instance and the given Vulkan display name on the given X display connection
	virtual ~DirectSurface(void);
	
	/* Methods: */
	static Vulkan::CStringList& addRequiredInstanceExtensions(Vulkan::CStringList& extensions); // Adds the list of instance extensions required to create surfaces to the given extension list
	static Vulkan::CStringList& addRequiredDeviceExtensions(Vulkan::CStringList& extensions); // Adds the list of device extensions required to create surfaces to the given extension list
	const Vulkan::PhysicalDevice& getDirectDevice(void) const // Returns the physical device to which the direct display is connected
		{
		return directDevice;
		}
	VkDisplayKHR getDirectDisplay(void) const // Returns the direct display
		{
		return directDisplay;
		}
	uint32_t getDirectDisplayPlaneIndex(void) const // Returns the index of the display plane to which the direct display is bound
		{
		return directDisplayPlaneIndex;
		}
	VkDisplayModeKHR getDirectDisplayMode(void) const // Returns the mode used by the direct display
		{
		return directDisplayMode;
		}
	const VkDisplayModeParametersKHR& getDirectDisplayModeParameters(void) const // Returns parameters for the direct display mode
		{
		return directDisplayModeParameters;
		}
	Vulkan::PhysicalDeviceDescriptor& setPhysicalDevice(Vulkan::PhysicalDeviceDescriptor& physicalDeviceDescriptor) const; // Sets the direct device in the given physical device descriptor
	};

}

#endif

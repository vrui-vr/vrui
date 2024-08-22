/***********************************************************************
Swapchain - Class representing Vulkan swapchains.
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

#ifndef VULKAN_SWAPCHAIN_INCLUDED
#define VULKAN_SWAPCHAIN_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>
#include <Vulkan/ImageView.h>

/* Forward declarations: */
namespace Vulkan {
class Surface;
class Semaphore;
class Fence;
}

namespace Vulkan {

class Swapchain:public DeviceAttached
	{
	/* Elements: */
	private:
	const Surface& surface; // Reference to the surface on which the swapchain operates
	VkSwapchainKHR swapchain; // Vulkan swapchain handle
	VkFormat imageFormat; // Format of images in the swapchain
	VkExtent2D imageExtent; // Size of images in the swapchain
	std::vector<VkImage> images; // List of images in the swapchain
	std::vector<ImageView> imageViews; // List of image views for images in the swapchain
	PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXTFunc; // Function pointer for the vkGetSwapchainCounterEXT function
	
	/* Constructors and destructors: */
	public:
	Swapchain(Device& sDevice,const Surface& sSurface,bool immediateMode,unsigned int numExtraImages); // Creates a default swapchain for the given logical device and surface; sets swapchain to immediate display mode if flag is true; allocates the given number of additional images beyond the minimum
	private:
	Swapchain(const Swapchain& source); // Prohibit copy constructor
	Swapchain& operator=(const Swapchain& source); // Prohibit assignment operator
	public:
	~Swapchain(void); // Destroys the swapchain
	
	/* Methods: */
	VkSwapchainKHR getHandle(void) const // Returns the Vulkan swapchain handle
		{
		return swapchain;
		}
	VkFormat getImageFormat(void) const // Returns the swapchain's image format
		{
		return imageFormat;
		}
	const VkExtent2D getImageExtent(void) const // Returns the swapchain's image size
		{
		return imageExtent;
		}
	const std::vector<ImageView>& getImageViews(void) const // Returns the swapchain's image views
		{
		return imageViews;
		}
	uint32_t acquireImage(const Semaphore& imageAvailableSemaphore); // Acquires the next image from the swapchain and synchronizes with the given semaphore
	uint32_t acquireImage(const Fence& imageAvailableFence); // Acquires the next image from the swapchain and synchronizes with the given fence
	uint32_t acquireImage(const Semaphore& imageAvailableSemaphore,const Fence& imageAvailableFence); // Acquires the next image from the swapchain and synchronizes with the given semaphore and fence
	bool vblankCounterSupported(void) const // Return true if the swapchain supports the vblank surface counter
		{
		return vkGetSwapchainCounterEXTFunc!=0;
		}
	uint64_t getVblankCounter(void); // Returns the current value of the vblank surface counter
	};

}

#endif

/***********************************************************************
Device - Class representing Vulkan logical devices.
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

#ifndef VULKAN_DEVICE_INCLUDED
#define VULKAN_DEVICE_INCLUDED

#include <vulkan/vulkan.h>

/* Forward declarations: */
namespace Vulkan {
class Instance;
class PhysicalDeviceDescriptor;
class Semaphore;
class Fence;
class Swapchain;
class CommandBuffer;
}

namespace Vulkan {

class Device
	{
	/* Elements: */
	private:
	Instance& instance; // Vulkan instance to which the physical and logical devices belong
	VkPhysicalDevice physicalDevice; // Vulkan physical device handle
	VkDevice device; // Vulkan logical device handle
	uint32_t renderingQueueFamilyIndex,presentationQueueFamilyIndex; // Indices of the physical device's rendering and presentation queue families
	VkQueue renderingQueue; // Vulkan queue handle for the logical device's rendering command queue
	VkQueue presentationQueue; // Vulkan queue handle for the logical device's presentation command queue, if the device is associated with a surface
	PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXTFunc; // Function pointer for the vkRegisterDisplayEventEXT function
	PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXTFunc; // Function pointer for the vkDisplayPowerControlEXT function
	
	/* Private methods: */
	PFN_vkVoidFunction getFunctionPointer(const char* functionName,bool throwOnError) const; // Low-level method to retrieve function pointers
	
	/* Constructors and destructors: */
	public:
	Device(Instance& sInstance,const PhysicalDeviceDescriptor& descriptor); // Creates a logical device for the given physical device descriptor
	private:
	Device(const Device& source); // Prohibit copy constructor
	Device& operator=(const Device& source); // Prohibit assignment operator
	public:
	Device(Device&& source); // Move constructor
	~Device(void);
	
	/* Methods: */
	Instance& getInstance(void) // Returns the instance to which the physical and logical devices belong
		{
		return instance;
		}
	VkPhysicalDevice getPhysicalHandle(void) const // Returns the Vulkan physical device handle
		{
		return physicalDevice;
		}
	VkDevice getHandle(void) const // Returns the Vulkan logical device handle
		{
		return device;
		}
	template <class FunctionTypeParam>
	FunctionTypeParam getFunction(const char* functionName,bool throwOnError =true) const // Returns a function pointer for the extension function of the given name
		{
		return (FunctionTypeParam)getFunctionPointer(functionName,throwOnError);
		}
	uint32_t getRenderingQueueFamilyIndex(void) const // Returns the index of the physical device's rendering queue family
		{
		return renderingQueueFamilyIndex;
		}
	uint32_t getPresentationQueueFamilyIndex(void) const // Returns the index of the physical device's presentation queue family
		{
		return presentationQueueFamilyIndex;
		}
	void submitRenderingCommand(const CommandBuffer& commandBuffer); // Submits the given command buffer to the rendering queue
	void submitRenderingCommand(const Semaphore& waitSemaphore,VkPipelineStageFlags waitStage,const CommandBuffer& commandBuffer,const Semaphore& signalSemaphore); // Submits a command buffer to the rendering queue, bracketed by the given semaphores
	void submitRenderingCommand(const Semaphore& waitSemaphore,VkPipelineStageFlags waitStage,const CommandBuffer& commandBuffer,const Semaphore& signalSemaphore,const Fence& signalFence); // Submits a command buffer to the rendering queue, bracketed by the given semaphores, signals completion on the given fence
	void waitRenderingQueue(void); // Waits until the rendering queue is idle
	void present(const Swapchain& swapchain,uint32_t imageIndex); // Presents the image of the given index to the given swap chain
	void present(const Semaphore& waitSemaphore,const Swapchain& swapchain,uint32_t imageIndex); // Presents the image of the given index to the given swap chain, bracketed by the given semaphore
	void waitPresentationQueue(void); // Waits until the presentation queue is idle
	void waitIdle(void); // Waits until the device finishes all pending operations
	bool displayEventsSupported(void) const // Returns true if the device supports display events
		{
		return vkRegisterDisplayEventEXTFunc!=0;
		}
	Fence registerVblankEvent(VkDisplayKHR display); // Returns a fence that is triggered whenever a new frame begins to scan out on the given display
	bool displayPowerControlSupported(void) const // Returns true if the device supports display power control
		{
		return vkDisplayPowerControlEXTFunc!=0;
		}
	void setDisplayPowerState(VkDisplayKHR display,VkDisplayPowerStateEXT powerState); // Sets the power state of the given display
	};

}

#endif

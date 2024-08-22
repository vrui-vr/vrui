/***********************************************************************
HMD - Class to representing a head-mounte display in direct display
mode.
Copyright (c) 2022-2023 Oliver Kreylos

This file is part of the Vrui VR Compositing Server (VRCompositor).

The Vrui VR Compositing Server is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Compositing Server is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Compositing Server; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef HMD_INCLUDED
#define HMD_INCLUDED

#include <string>
#include <vector>
#include <Realtime/Time.h>

#include <Vulkan/Common.h>
#include <Vulkan/Device.h>
#include <Vulkan/Swapchain.h>
#include <Vulkan/RenderPass.h>
#include <Vulkan/Framebuffer.h>
#include <Vulkan/Fence.h>
#include <VulkanXlib/XlibDisplay.h>
#include <VulkanXlib/DirectSurface.h>

#include "VblankEstimator.h"

/* Forward declarations: */
namespace Vulkan {
class Instance;
class Semaphore;
}

class HMD
	{
	/* Embedded classes: */
	public:
	typedef VblankEstimator::TimePoint TimePoint; // Type for points in time
	typedef VblankEstimator::TimeVector TimeVector; // Type for time intervals
	
	/* Elements: */
	private:
	VulkanXlib::XlibDisplay display; // Xlib connection to the X server
	VulkanXlib::DirectSurface surface; // Vulkan rendering surface associated with a display in direct mode
	Vulkan::Device device; // Vulkan logical device representing the GPU to which the HMD is connected
	Vulkan::Swapchain swapchain; // A swap chain to show rendering results on the HMD's display
	Vulkan::RenderPass renderPass; // A render pass to render geometry to a surface
	std::vector<Vulkan::Framebuffer> framebuffers; // Array of one framebuffer per image in the swap chain
	uint32_t currentSwapchainImage; // Index of the currently acquired swapchain image
	
	/* State to estimate the HMD's actual vertical retrace period and predict vertical retrace times: */
	uint64_t vblankCounter; // Value of the swapchain's vblank counter on the last call to vsync()
	TimePoint vblankSample; // The most recent vblank event sample, which might be a few frames outdated
	VblankEstimator vblankEstimator; // Kalman filter to estimate the next vblank event and the vblank period
	
	/* Constructors and destructors: */
	public:
	HMD(Vulkan::Instance& instance,const std::string& hmdName,double targetRefreshRate,const Vulkan::CStringList& deviceExtensions); // Connects to an HMD of the given name on any display output port
	virtual ~HMD(void);
	
	/* Methods: */
	static Vulkan::CStringList& addRequiredInstanceExtensions(Vulkan::CStringList& extensions); // Adds the list of instance extensions required to drive an HMD in direct mode to the given extension list
	const Vulkan::PhysicalDevice& getDirectDevice(void) const // Returns the physical device representing the HMD's direct display surface
		{
		return surface.getDirectDevice();
		}
	const Vulkan::Device& getDevice(void) const // Returns the logical device representing the GPU to which the HMD is connected
		{
		return device;
		}
	Vulkan::Device& getDevice(void) // Ditto
		{
		return device;
		}
	VulkanXlib::DirectSurface& getSurface(void) // Returns the HMD's direct display surface
		{
		return surface;
		}
	const VkExtent2D& getVisibleRegion(void) const // Returns the extent of the visible region of the HMD's display
		{
		return surface.getDirectDisplayModeParameters().visibleRegion;
		}
	uint32_t getRefreshRate(void) const // Returns the refresh rate of the direct display mode in 1/1000 Hz
		{
		return surface.getDirectDisplayModeParameters().refreshRate;
		}
	Vulkan::Swapchain& getSwapchain(void) // Returns the swap chain for the HMD's display
		{
		return swapchain;
		}
	Vulkan::RenderPass& getRenderPass(void) // Returns the render pass object to render to the HMD's frame buffers
		{
		return renderPass;
		}
	Vulkan::Framebuffer& getFramebuffer(unsigned int imageIndex) // Returns the framebuffer associated with the swapchain image of the given index
		{
		return framebuffers[imageIndex];
		}
	
	/* Render pass methods: */
	void acquireSwapchainImage(const Vulkan::Semaphore& imageReady); // Acquires the next image from the HMD's swapchain; signals the given semaphore when the image is ready
	const Vulkan::Framebuffer& getAcquiredFramebuffer(void) const // Returns the current framebuffer in the HMD's swapchain
		{
		return framebuffers[currentSwapchainImage];
		}
	void present(const Vulkan::Semaphore& renderingFinished); // Presents the current framebuffer to the HMD's swapchain; waits for the given semaphore to signal
	
	/* Synchronization methods: */
	bool displayEventsSupported(void) const // Returns true if the device supports display events
		{
		return device.displayEventsSupported();
		}
	Vulkan::Fence registerVblankEvent(void) // Returns a fence that is triggered whenever a new frame begins to scan out on the given display
		{
		return device.registerVblankEvent(surface.getDirectDisplay());
		}
	void startVblankEstimator(void); // Starts tracking vertical retrace events and initializes the retrace time and period estimators
	uint64_t vsync(void); // Waits for the next vertical retrace and returns the number of missed vertical retraces since the previous call
	uint64_t getVblankCounter(void) const // Returns the current vblank counter
		{
		return vblankCounter;
		}
	const TimePoint& getVblankSample(void) const // Returns the most recently taken vertical retraces sample
		{
		return vblankSample;
		}
	const VblankEstimator& getVblankEstimator(void) const // Returns the vblank estimator object
		{
		return vblankEstimator;
		}
	const TimePoint& getVblankTime(void) const // Returns the current vblank time estimate
		{
		return vblankEstimator.getVblankTime();
		}
	const TimeVector& getVblankPeriod(void) const // Returns the current vblank period estimate
		{
		return vblankEstimator.getFrameInterval();
		}
	TimePoint predictNextVblank(void) const // Predicts the time at which the next vblank will occur
		{
		return vblankEstimator.predictNextVblankTime();
		}
	
	/* Device control methods: */
	bool displayPowerControlSupported(void) const // Returns true if the device supports display power control
		{
		return device.displayPowerControlSupported();
		}
	void setDisplayPowerState(VkDisplayPowerStateEXT powerState) // Sets the power state of the given display
		{
		device.setDisplayPowerState(surface.getDirectDisplay(),powerState);
		}
	};

#endif

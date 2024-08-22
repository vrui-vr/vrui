/***********************************************************************
Swapchain - Class representing Vulkan swapchains.
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

#include <Vulkan/Swapchain.h>

#include <Misc/StdError.h>
#include <Vulkan/Common.h>
#include <Vulkan/Instance.h>
#include <Vulkan/Surface.h>
#include <Vulkan/Device.h>
#include <Vulkan/Semaphore.h>
#include <Vulkan/Fence.h>

#define DEBUGGING 0

#if DEBUGGING
#include <iostream>
#endif

namespace Vulkan {

/**************************
Methods of class Swapchain:
**************************/

Swapchain::Swapchain(Device& sDevice,const Surface& sSurface,bool immediateMode,unsigned int numExtraImages)
	:DeviceAttached(sDevice),
	 surface(sSurface),
	 swapchain(VK_NULL_HANDLE),
	 vkGetSwapchainCounterEXTFunc(0)
	{
	/* Query the list of supported surface formats: */
	uint32_t numSurfaceFormats;
	throwOnError(vkGetPhysicalDeviceSurfaceFormatsKHR(device.getPhysicalHandle(),surface.getHandle(),&numSurfaceFormats,0),
	             __PRETTY_FUNCTION__,"query surface formats");
	std::vector<VkSurfaceFormatKHR> surfaceFormats(numSurfaceFormats);
	throwOnError(vkGetPhysicalDeviceSurfaceFormatsKHR(device.getPhysicalHandle(),surface.getHandle(),&numSurfaceFormats,surfaceFormats.data()),
	             __PRETTY_FUNCTION__,"query surface formats");
	
	/* Find the preferred format (32-bit non-linear sRGB(A)): */
	std::vector<VkSurfaceFormatKHR>::iterator surfaceFormat=surfaceFormats.end();
	for(std::vector<VkSurfaceFormatKHR>::iterator sfIt=surfaceFormats.begin();sfIt!=surfaceFormats.end()&&surfaceFormat==surfaceFormats.end();++sfIt)
		if(sfIt->colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		   &&(sfIt->format==VK_FORMAT_R8G8B8_SRGB
		      ||sfIt->format==VK_FORMAT_B8G8R8_SRGB
		      ||sfIt->format==VK_FORMAT_R8G8B8A8_SRGB
		      ||sfIt->format==VK_FORMAT_B8G8R8A8_SRGB
		      ||sfIt->format==VK_FORMAT_A8B8G8R8_SRGB_PACK32))
			surfaceFormat=sfIt;
	
	/* Fall back to the first available format if no sRGB(A) formats were found: */
	if(surfaceFormat==surfaceFormats.end())
		surfaceFormat=surfaceFormats.begin();
	
	/* Query the list of supported present modes: */
	uint32_t numPresentModes;
	throwOnError(vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalHandle(),surface.getHandle(),&numPresentModes,0),
	             __PRETTY_FUNCTION__,"query present modes");
	std::vector<VkPresentModeKHR> presentModes(numPresentModes);
	throwOnError(vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalHandle(),surface.getHandle(),&numPresentModes,presentModes.data()),
	             __PRETTY_FUNCTION__,"query present modes");
	
	#if DEBUGGING
	std::cout<<"Vulkan::SwapChain: Available presentation modes:";
	for(std::vector<VkPresentModeKHR>::iterator pmIt=presentModes.begin();pmIt!=presentModes.end();++pmIt)
		{
		switch(*pmIt)
			{
			case VK_PRESENT_MODE_IMMEDIATE_KHR:
				std::cout<<" Immediate";
				break;
			
			case VK_PRESENT_MODE_MAILBOX_KHR:
				std::cout<<" Mailbox";
				break;
			
			case VK_PRESENT_MODE_FIFO_KHR:
				std::cout<<" FIFO";
				break;
			
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
				std::cout<<" FIFO_Relaxed";
				break;
			
			case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
				std::cout<<" Shared_demand_refresh";
				break;
			
			case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
				std::cout<<" Shared_continuous_refresh";
				break;
			
			default:
				std::cout<<" <unknown>";
			}
		}
	std::cout<<std::endl;
	#endif
	
	/* Find the preferred presentation mode (double-buffering): */
	VkPresentModeKHR preferredPresentationMode=immediateMode?VK_PRESENT_MODE_IMMEDIATE_KHR:VK_PRESENT_MODE_FIFO_KHR;
	std::vector<VkPresentModeKHR>::iterator presentMode=presentModes.end();
	for(std::vector<VkPresentModeKHR>::iterator pmIt=presentModes.begin();pmIt!=presentModes.end()&&presentMode==presentModes.end();++pmIt)
		if(*pmIt==preferredPresentationMode)
			presentMode=pmIt;
	if(presentMode==presentModes.end())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Selected device/surface combination does not support %s-buffering",immediateMode?"single":"double");
	
	/* Query the device's capabilities vis-a-vis the requested surface: */
	VkExtent2D swapExtent;
	uint32_t minImageCount;
	VkSurfaceTransformFlagBitsKHR currentTransform;
	VkSurfaceCounterFlagsEXT supportedSurfaceCounters=0x0U;
	PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXTFunc=device.getInstance().getFunction<PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT>("vkGetPhysicalDeviceSurfaceCapabilities2EXT",false);
	if(vkGetPhysicalDeviceSurfaceCapabilities2EXTFunc!=0)
		{
		/* Query extended surface capabilities: */
		VkSurfaceCapabilities2EXT surfaceCapabilities;
		surfaceCapabilities.sType=VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT;
		surfaceCapabilities.pNext=0;
		throwOnError(vkGetPhysicalDeviceSurfaceCapabilities2EXTFunc(device.getPhysicalHandle(),surface.getHandle(),&surfaceCapabilities),
		             __PRETTY_FUNCTION__,"query device surface capabilities");
		swapExtent=surfaceCapabilities.currentExtent;
		minImageCount=surfaceCapabilities.minImageCount;
		currentTransform=surfaceCapabilities.currentTransform;
		supportedSurfaceCounters=surfaceCapabilities.supportedSurfaceCounters;
		}
	else
		{
		/* Query standard surface capabilities: */
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		throwOnError(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalHandle(),surface.getHandle(),&surfaceCapabilities),
		             __PRETTY_FUNCTION__,"query device surface capabilities");
		swapExtent=surfaceCapabilities.currentExtent;
		minImageCount=surfaceCapabilities.minImageCount;
		currentTransform=surfaceCapabilities.currentTransform;
		}
	
	/* Set up the swapchain creation structure: */
	VkSwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext=0;
	
	/* Check if the device/surface combo supports vsync counters: */
	VkSwapchainCounterCreateInfoEXT swapchainCounterCreateInfo;
	if((supportedSurfaceCounters&VK_SURFACE_COUNTER_VBLANK_EXT)!=0x0)
		{
		/* Set up the swapchain counter creation structure: */
		swapchainCounterCreateInfo.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT;
		swapchainCounterCreateInfo.pNext=0;
		swapchainCounterCreateInfo.surfaceCounters=VK_SURFACE_COUNTER_VBLANK_EXT;
		
		/* Register the swapchain counter creation structure: */
		swapchainCreateInfo.pNext=&swapchainCounterCreateInfo;
		
		/* Retrieve extension functions pointers: */
		vkGetSwapchainCounterEXTFunc=device.getFunction<PFN_vkGetSwapchainCounterEXT>("vkGetSwapchainCounterEXT",false);
		}
	
	/* Create the swapchain create info structure: */
	swapchainCreateInfo.flags=0x0;
	swapchainCreateInfo.surface=surface.getHandle();
	swapchainCreateInfo.minImageCount=minImageCount+numExtraImages;
	swapchainCreateInfo.imageFormat=surfaceFormat->format;
	swapchainCreateInfo.imageColorSpace=surfaceFormat->colorSpace;
	swapchainCreateInfo.imageExtent=swapExtent;
	swapchainCreateInfo.imageArrayLayers=1;
	swapchainCreateInfo.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	uint32_t queueFamilyIndices[2]={device.getRenderingQueueFamilyIndex(),device.getPresentationQueueFamilyIndex()};
	if(queueFamilyIndices[0]!=queueFamilyIndices[1])
		{
		/* Share the swapchain between two queues: */
		swapchainCreateInfo.imageSharingMode=VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount=2;
		swapchainCreateInfo.pQueueFamilyIndices=queueFamilyIndices;
		}
	else
		{
		/* Access the swapchain from a single queue: */
		swapchainCreateInfo.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount=0;
		swapchainCreateInfo.pQueueFamilyIndices=0;
		}
	swapchainCreateInfo.preTransform=currentTransform;
	swapchainCreateInfo.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode=*presentMode;
	swapchainCreateInfo.clipped=VK_TRUE;
	swapchainCreateInfo.oldSwapchain=VK_NULL_HANDLE;
	
	/* Create the swapchain: */
	throwOnError(vkCreateSwapchainKHR(device.getHandle(),&swapchainCreateInfo,0,&swapchain),
	             __PRETTY_FUNCTION__,"create Vulkan swapchain");
	
	/* Retrieve the list of images: */
	uint32_t numImages;
	VkResult getSwapchainImagesResult=vkGetSwapchainImagesKHR(device.getHandle(),swapchain,&numImages,0);
	if(getSwapchainImagesResult==VK_SUCCESS)
		{
		images.resize(numImages);
		getSwapchainImagesResult=vkGetSwapchainImagesKHR(device.getHandle(),swapchain,&numImages,images.data());
		}
	if(getSwapchainImagesResult!=VK_SUCCESS)
		{
		/* Destroy the swapchain and throw an exception: */
		vkDestroySwapchainKHR(device.getHandle(),swapchain,0);
		throwOnError(getSwapchainImagesResult,__PRETTY_FUNCTION__,"query swapchain images");
		}
	
	/* Create an image view for each image: */
	imageViews.reserve(images.size());
	for(std::vector<VkImage>::iterator iIt=images.begin();iIt!=images.end();++iIt)
		imageViews.push_back(ImageView(device,*iIt,surfaceFormat->format));
	
	/* Store the image format and extent: */
	imageFormat=surfaceFormat->format;
	imageExtent=swapExtent;
	}

Swapchain::~Swapchain(void)
	{
	/* Destroy the swapchain: */
	vkDestroySwapchainKHR(device.getHandle(),swapchain,0);
	}

uint32_t Swapchain::acquireImage(const Semaphore& imageAvailableSemaphor)
	{
	uint32_t result;
	throwOnError(vkAcquireNextImageKHR(device.getHandle(),swapchain,UINT64_MAX,imageAvailableSemaphor.getHandle(),VK_NULL_HANDLE,&result),
	             __PRETTY_FUNCTION__,"acquire Vulkan image");
	
	return result;
	}

uint32_t Swapchain::acquireImage(const Fence& imageAvailableFence)
	{
	uint32_t result;
	throwOnError(vkAcquireNextImageKHR(device.getHandle(),swapchain,UINT64_MAX,VK_NULL_HANDLE,imageAvailableFence.getHandle(),&result),
	             __PRETTY_FUNCTION__,"acquire Vulkan image");
	
	return result;
	}

uint32_t Swapchain::acquireImage(const Semaphore& imageAvailableSemaphor,const Fence& imageAvailableFence)
	{
	uint32_t result;
	throwOnError(vkAcquireNextImageKHR(device.getHandle(),swapchain,UINT64_MAX,imageAvailableSemaphor.getHandle(),imageAvailableFence.getHandle(),&result),
	             __PRETTY_FUNCTION__,"acquire Vulkan image");
	
	return result;
	}

uint64_t Swapchain::getVblankCounter(void)
	{
	if(vkGetSwapchainCounterEXTFunc==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Swapchain does not support swapchain counters");
	
	uint64_t result=0U;
	throwOnError(vkGetSwapchainCounterEXTFunc(device.getHandle(),swapchain,VK_SURFACE_COUNTER_VBLANK_EXT,&result),
							__PRETTY_FUNCTION__,"query vblank surface counter");
	
	return result;
	}

}

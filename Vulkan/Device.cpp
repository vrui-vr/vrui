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

#include <Vulkan/Device.h>

#include <string.h>
#include <vector>
#include <Misc/StdError.h>
#include <Vulkan/Common.h>
#include <Vulkan/Surface.h>
#include <Vulkan/PhysicalDeviceDescriptor.h>
#include <Vulkan/Semaphore.h>
#include <Vulkan/Fence.h>
#include <Vulkan/Swapchain.h>
#include <Vulkan/CommandBuffer.h>

namespace Vulkan {

/***********************
Methods of class Device:
***********************/

PFN_vkVoidFunction Device::getFunctionPointer(const char* functionName,bool throwOnError) const
	{
	PFN_vkVoidFunction result=vkGetDeviceProcAddr(device,functionName);
	if(throwOnError&&result==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot resolve function %s",functionName);
	
	return result;
	}

Device::Device(Instance& sInstance,const PhysicalDeviceDescriptor& descriptor)
	:instance(sInstance),
	 physicalDevice(descriptor.physicalDevice),
	 device(VK_NULL_HANDLE),
	 renderingQueueFamilyIndex(descriptor.renderingQueueFamilyIndex),presentationQueueFamilyIndex(descriptor.presentationQueueFamilyIndex),
	 renderingQueue(VK_NULL_HANDLE),presentationQueue(VK_NULL_HANDLE)
	{
	/* Create queue creation structures for all required queue families: */
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	float queuePriority=1.0f;
	
	/* Set up a queue creation structure for the rendering queue: */
	VkDeviceQueueCreateInfo renderingDeviceQueueCreateInfo;
	renderingDeviceQueueCreateInfo.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	renderingDeviceQueueCreateInfo.pNext=0;
	renderingDeviceQueueCreateInfo.flags=0x0;
	renderingDeviceQueueCreateInfo.queueFamilyIndex=descriptor.renderingQueueFamilyIndex;
	renderingDeviceQueueCreateInfo.queueCount=1;
	renderingDeviceQueueCreateInfo.pQueuePriorities=&queuePriority;
	deviceQueueCreateInfos.push_back(renderingDeviceQueueCreateInfo);
	
	if(descriptor.surface!=0&&descriptor.presentationQueueFamilyIndex!=descriptor.renderingQueueFamilyIndex)
		{
		/* Set up a queue creation structure for the presentation queue: */
		VkDeviceQueueCreateInfo presentationDeviceQueueCreateInfo;
		presentationDeviceQueueCreateInfo.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		presentationDeviceQueueCreateInfo.pNext=0;
		presentationDeviceQueueCreateInfo.flags=0x0;
		presentationDeviceQueueCreateInfo.queueFamilyIndex=descriptor.presentationQueueFamilyIndex;
		presentationDeviceQueueCreateInfo.queueCount=1;
		presentationDeviceQueueCreateInfo.pQueuePriorities=&queuePriority;
		deviceQueueCreateInfos.push_back(presentationDeviceQueueCreateInfo);
		}
	
	/* Set up the device creation structure: */
	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext=0;
	deviceCreateInfo.flags=0x0;
	deviceCreateInfo.queueCreateInfoCount=deviceQueueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos=deviceQueueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures=&descriptor.deviceFeatures;
	deviceCreateInfo.enabledExtensionCount=descriptor.deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames=deviceCreateInfo.enabledExtensionCount>0?descriptor.deviceExtensions.data():0;
	deviceCreateInfo.enabledLayerCount=descriptor.validationLayers.size();
	deviceCreateInfo.ppEnabledLayerNames=deviceCreateInfo.enabledLayerCount>0?descriptor.validationLayers.data():0;
	
	/* Create the logical device: */
	throwOnError(vkCreateDevice(descriptor.physicalDevice,&deviceCreateInfo,0,&device),
	             __PRETTY_FUNCTION__,"create logical device");
	
	/* Retrieve the render queue family's handle: */
	vkGetDeviceQueue(device,descriptor.renderingQueueFamilyIndex,0,&renderingQueue);
	
	if(descriptor.surface!=0)
		{
		/* Retrieve the presentation queue family's handle: */
		vkGetDeviceQueue(device,descriptor.presentationQueueFamilyIndex,0,&presentationQueue);
		}
	
	/* Retrieve extension function pointers: */
	vkRegisterDisplayEventEXTFunc=getFunction<PFN_vkRegisterDisplayEventEXT>("vkRegisterDisplayEventEXT",false);
	vkDisplayPowerControlEXTFunc=getFunction<PFN_vkDisplayPowerControlEXT>("vkDisplayPowerControlEXT",false);
	}

Device::Device(Device&& source)
	:instance(source.instance),
	 physicalDevice(source.physicalDevice),
	 device(source.device),
	 renderingQueueFamilyIndex(source.renderingQueueFamilyIndex),presentationQueueFamilyIndex(source.presentationQueueFamilyIndex),
	 renderingQueue(source.renderingQueue),
	 presentationQueue(source.presentationQueue),
	 vkRegisterDisplayEventEXTFunc(source.vkRegisterDisplayEventEXTFunc),
	 vkDisplayPowerControlEXTFunc(source.vkDisplayPowerControlEXTFunc)
	{
	/* Invalidate the source object: */
	physicalDevice=VK_NULL_HANDLE;
	device=VK_NULL_HANDLE;
	renderingQueue=VK_NULL_HANDLE;
	presentationQueue=VK_NULL_HANDLE;
	}

Device::~Device(void)
	{
	/* Destroy the logical device: */
	vkDestroyDevice(device,0);
	}

void Device::submitRenderingCommand(const CommandBuffer& commandBuffer)
	{
	/* Set up the submission setup structure: */
	VkSubmitInfo submitInfo;
	submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext=0;
	submitInfo.waitSemaphoreCount=0;
	submitInfo.pWaitSemaphores=0;
	submitInfo.pWaitDstStageMask=0;
	submitInfo.commandBufferCount=1;
	VkCommandBuffer cb=commandBuffer.getHandle();
	submitInfo.pCommandBuffers=&cb;
	submitInfo.signalSemaphoreCount=0;
	submitInfo.pSignalSemaphores=0;
	
	/* Submit the command buffer to the rendering queue: */
	throwOnError(vkQueueSubmit(renderingQueue,1,&submitInfo,VK_NULL_HANDLE),
	             __PRETTY_FUNCTION__,"submit command buffer to rendering queue");
	}

void Device::submitRenderingCommand(const Semaphore& waitSemaphore,VkPipelineStageFlags waitStage,const CommandBuffer& commandBuffer,const Semaphore& signalSemaphore)
	{
	/* Set up the submission setup structure: */
	VkSubmitInfo submitInfo;
	submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext=0;
	submitInfo.waitSemaphoreCount=1;
	VkSemaphore ws=waitSemaphore.getHandle();
	submitInfo.pWaitSemaphores=&ws;
	submitInfo.pWaitDstStageMask=&waitStage;
	submitInfo.commandBufferCount=1;
	VkCommandBuffer cb=commandBuffer.getHandle();
	submitInfo.pCommandBuffers=&cb;
	submitInfo.signalSemaphoreCount=1;
	VkSemaphore ss=signalSemaphore.getHandle();
	submitInfo.pSignalSemaphores=&ss;
	
	/* Submit the command buffer to the rendering queue: */
	throwOnError(vkQueueSubmit(renderingQueue,1,&submitInfo,VK_NULL_HANDLE),
	             __PRETTY_FUNCTION__,"submit command buffer to rendering queue");
	}

void Device::submitRenderingCommand(const Semaphore& waitSemaphore,VkPipelineStageFlags waitStage,const CommandBuffer& commandBuffer,const Semaphore& signalSemaphore,const Fence& completeFence)
	{
	/* Set up the submission setup structure: */
	VkSubmitInfo submitInfo;
	submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext=0;
	submitInfo.waitSemaphoreCount=1;
	VkSemaphore ws=waitSemaphore.getHandle();
	submitInfo.pWaitSemaphores=&ws;
	submitInfo.pWaitDstStageMask=&waitStage;
	submitInfo.commandBufferCount=1;
	VkCommandBuffer cb=commandBuffer.getHandle();
	submitInfo.pCommandBuffers=&cb;
	submitInfo.signalSemaphoreCount=1;
	VkSemaphore ss=signalSemaphore.getHandle();
	submitInfo.pSignalSemaphores=&ss;
	
	/* Submit the command buffer to the rendering queue: */
	throwOnError(vkQueueSubmit(renderingQueue,1,&submitInfo,completeFence.getHandle()),
	             __PRETTY_FUNCTION__,"submit command buffer to rendering queue");
	}

void Device::waitRenderingQueue(void)
	{
	throwOnError(vkQueueWaitIdle(renderingQueue),
	             __PRETTY_FUNCTION__,"wait on Vulkan rendering queue");
	}

void Device::present(const Swapchain& swapchain,uint32_t imageIndex)
	{
	/* Set up the presentation setup structure: */
	VkPresentInfoKHR presentInfo;
	presentInfo.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext=0;
	presentInfo.waitSemaphoreCount=0;
	presentInfo.pWaitSemaphores=0;
	presentInfo.swapchainCount=1;
	VkSwapchainKHR sc=swapchain.getHandle();
	presentInfo.pSwapchains=&sc;
	presentInfo.pImageIndices=&imageIndex;
	presentInfo.pResults=0;
	
	/* Submit the command to the presentation queue: */
	throwOnError(vkQueuePresentKHR(presentationQueue,&presentInfo),
	             __PRETTY_FUNCTION__,"submit command to presentation queue");
	}

void Device::present(const Semaphore& waitSemaphore,const Swapchain& swapchain,uint32_t imageIndex)
	{
	/* Set up the presentation setup structure: */
	VkPresentInfoKHR presentInfo;
	presentInfo.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext=0;
	presentInfo.waitSemaphoreCount=1;
	VkSemaphore ws=waitSemaphore.getHandle();
	presentInfo.pWaitSemaphores=&ws;
	presentInfo.swapchainCount=1;
	VkSwapchainKHR sc=swapchain.getHandle();
	presentInfo.pSwapchains=&sc;
	presentInfo.pImageIndices=&imageIndex;
	presentInfo.pResults=0;
	
	/* Submit the command to the presentation queue: */
	throwOnError(vkQueuePresentKHR(presentationQueue,&presentInfo),
	             __PRETTY_FUNCTION__,"submit command to presentation queue");
	}

void Device::waitPresentationQueue(void)
	{
	throwOnError(vkQueueWaitIdle(presentationQueue),
	             __PRETTY_FUNCTION__,"wait on Vulkan presentation queue");
	}

void Device::waitIdle(void)
	{
	/* Execute the command: */
	throwOnError(vkDeviceWaitIdle(device),
	             __PRETTY_FUNCTION__,"wait on Vulkan device");
	}

Fence Device::registerVblankEvent(VkDisplayKHR display)
	{
	if(vkRegisterDisplayEventEXTFunc==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device does not support display events");
	
	/* Set up a display event registration structure: */
	VkDisplayEventInfoEXT displayEventInfo;
	displayEventInfo.sType=VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT;
	displayEventInfo.pNext=0;
	displayEventInfo.displayEvent=VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT;
	
	/* Register the display event: */
	VkFence fence;
	throwOnError(vkRegisterDisplayEventEXTFunc(device,display,&displayEventInfo,0,&fence),
	             __PRETTY_FUNCTION__,"register Vulkan display event");
	
	return Fence(*this,fence);
	}

void Device::setDisplayPowerState(VkDisplayKHR display,VkDisplayPowerStateEXT powerState)
	{
	if(vkDisplayPowerControlEXTFunc==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device does not support power control");
	
	/* Set up a display power structure: */
	VkDisplayPowerInfoEXT displayPowerInfo;
	displayPowerInfo.sType=VK_STRUCTURE_TYPE_DISPLAY_POWER_INFO_EXT;
	displayPowerInfo.pNext=0;
	displayPowerInfo.powerState=powerState;
	throwOnError(vkDisplayPowerControlEXTFunc(device,display,&displayPowerInfo),
	             __PRETTY_FUNCTION__,"set display power state");
	}

}

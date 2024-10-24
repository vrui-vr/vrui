/***********************************************************************
HMD - Class to representing a head-mounte display.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include "HMD.h"

#include <stdlib.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Vulkan/Instance.h>
#include <Vulkan/PhysicalDeviceDescriptor.h>
#include <Vulkan/ImageView.h>

#define HMD_USE_VBLANK_EVENTS 0
#define HMD_USE_VBLANK_COUNTER 1
#define HMD_PRINT_VBLANK_ESTIMATES 0
#define HMD_WRITE_TIMINGFILE 0

#if HMD_WRITE_TIMINGFILE
#include <IO/File.h>
#include <IO/OpenFile.h>

const char* vblankFileName="VsyncTimes.dat";
IO::FilePtr vblankFile; // File to which to write vblank estimates and errors
#endif

#if HMD_PRINT_VBLANK_ESTIMATES
#include <iostream>
#include <iomanip>
#endif

namespace {

/****************
Helper functions:
****************/

inline long diffNsec(const Realtime::Time& t1,const Realtime::Time& t2)
	{
	/* Convert seconds to nanoseconds and add the two component differences; will overflow for very large differences: */
	return (long(t1.tv_sec)-long(t2.tv_sec))*1000000000L+(t1.tv_nsec-t2.tv_nsec);
	}

inline Realtime::Time& addNsec(Realtime::Time& t,long diffNsec)
	{
	/* Add the difference to the nanosecond component and catch overflow in the second component: */
	t.tv_nsec+=diffNsec;
	long seconds=t.tv_nsec/1000000000L;
	t.tv_sec+=time_t(seconds);
	t.tv_nsec-=seconds*1000000000L;
	
	return t;
	}

Vulkan::Device createDevice(Vulkan::Instance& instance,VulkanXlib::DirectSurface& surface,const Vulkan::CStringList& deviceExtensions)
	{
	/* Create a descriptor and add required device extensions: */
	Vulkan::PhysicalDeviceDescriptor physicalDeviceDescriptor(&surface);
	
	/* Collect the list of required device extensions: */
	Vulkan::CStringList& pddde=physicalDeviceDescriptor.getDeviceExtensions();
	pddde.insert(pddde.end(),deviceExtensions.begin(),deviceExtensions.end());
	VulkanXlib::DirectSurface::addRequiredDeviceExtensions(pddde);
	
	/* Set the direct device: */
	surface.setPhysicalDevice(physicalDeviceDescriptor);
	
	/* Create and return the device: */
	return Vulkan::Device(instance,physicalDeviceDescriptor);
	}

Vulkan::RenderPass createRenderPass(Vulkan::Device& device,const Vulkan::Swapchain& swapchain)
	{
	/* Create a render pass: */
	Vulkan::RenderPass::Constructor rpc;
	
	/* Add a color attachment to the render pass: */
	VkAttachmentDescription colorAttachment;
	colorAttachment.flags=0x0;
	colorAttachment.format=swapchain.getImageFormat();
	colorAttachment.samples=VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	rpc.addAttachment(colorAttachment);
	
	/* Add a subpass to the render pass: */
	VkAttachmentReference subpassAttachment;
	subpassAttachment.attachment=0;
	subpassAttachment.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass;
	subpass.flags=0x0;
	subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount=0;
	subpass.pInputAttachments=0;
	subpass.colorAttachmentCount=1;
	subpass.pColorAttachments=&subpassAttachment;
	subpass.pResolveAttachments=0;
	subpass.pDepthStencilAttachment=0;
	subpass.preserveAttachmentCount=0;
	subpass.pPreserveAttachments=0;
	rpc.addSubpass(subpass);
	
	/* Add a subpass dependency to delay starting the render pass until the color attachment is available: */
	VkSubpassDependency dependency;
	dependency.srcSubpass=VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass=0;
	dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask=0x0;
	dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags=0x0;
	rpc.addSubpassDependency(dependency);
	
	return Vulkan::RenderPass(device,rpc);
	}

}

/********************
Methods of class HMD:
********************/

HMD::HMD(Vulkan::Instance& instance,const std::string& hmdName,double targetRefreshRate,const Vulkan::CStringList& deviceExtensions)
	:display(0),
	 surface(instance,display,hmdName,targetRefreshRate),
	 device(createDevice(instance,surface,deviceExtensions)),
	 swapchain(device,surface,true,0),
	 renderPass(createRenderPass(device,swapchain)),currentSwapchainImage(-1),
	 vblankCounter(0)
	{
	/* Create a framebuffer for each image in the swap chain: */
	framebuffers.reserve(swapchain.getImageViews().size());
	for(std::vector<Vulkan::ImageView>::const_iterator ivIt=swapchain.getImageViews().begin();ivIt!=swapchain.getImageViews().end();++ivIt)
		{
		/* Create a new framebuffer and attach the image view to it: */
		std::vector<VkImageView> attachments;
		attachments.push_back(ivIt->getHandle());
		framebuffers.push_back(Vulkan::Framebuffer(device,renderPass,attachments,swapchain.getImageExtent(),1));
		}
	}

HMD::~HMD(void)
	{
	#if HMD_WRITE_TIMINGFILE
	vblankFile=0;
	#endif
	}

Vulkan::CStringList& HMD::addRequiredInstanceExtensions(Vulkan::CStringList& extensions)
	{
	VulkanXlib::DirectSurface::addRequiredInstanceExtensions(extensions);
	
	return extensions;
	}

void HMD::acquireSwapchainImage(const Vulkan::Semaphore& imageReady)
	{
	/* Forward the request to the swapchain: */
	currentSwapchainImage=swapchain.acquireImage(imageReady);
	}

void HMD::present(const Vulkan::Semaphore& renderingFinished)
	{
	/* Forward the request to the logical device: */
	device.present(renderingFinished,swapchain,currentSwapchainImage);
	}

#if HMD_PRINT_VBLANK_ESTIMATES
HMD::TimePoint timeBase;
#endif

void HMD::startVblankEstimator(void)
	{
	#if HMD_PRINT_VBLANK_ESTIMATES
	timeBase.set();
	#endif
	
	/* Check whether the device supports display events and the swapchain supports the vblank surface counter: */
	if(!device.displayEventsSupported())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"HMD's device does not support display events");
	if(!swapchain.vblankCounterSupported())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"HMD's swapchain does not support surface vblank counters");
	
	#if HMD_USE_VBLANK_EVENTS
	
	/* Wait for the next vblank event on a throwaway fence: */
	Vulkan::Fence(device.registerVblankEvent(surface.getDirectDisplay())).wait();
	
	/* Get a vblank time sample: */
	vblankSample.set();
	
	/* Initialize the vblank counter: */
	vblankCounter=swapchain.getVblankCounter();
	
	#else // #if HMD_USE_VBLANK_EVENTS
	
	/* Busy-wait until the swapchain's vblank counter changes value: */
	uint64_t currentVblankCounter=swapchain.getVblankCounter();
	while((vblankCounter=swapchain.getVblankCounter())==currentVblankCounter)
		;
	
	/* Get a vblank time sample: */
	vblankSample.set();
	
	#endif
	
	/* Initialize the vblank estimator: */
	vblankEstimator.start(vblankSample,double(surface.getDirectDisplayModeParameters().refreshRate)/1000.0);
	
	#if HMD_PRINT_VBLANK_ESTIMATES
	std::cout<<0<<','<<vblankCounter<<','<<diffNsec(vblankSample,timeBase)<<','<<diffNsec(vblankEstimator.getVblankTime(),timeBase)<<','<<vblankEstimator.getFrameInterval().tv_nsec<<std::endl;
	#endif
	
	#if HMD_WRITE_TIMINGFILE
	vblankFile=IO::openFile(vblankFileName,IO::File::WriteOnly);
	vblankFile->setEndianness(Misc::LittleEndian);
	vblankFile->write(vblankSample.tv_sec);
	vblankFile->write(vblankSample.tv_nsec);
	#endif
	}

uint64_t HMD::vsync(void)
	{
	uint64_t missed=0;
	
	#if HMD_USE_VBLANK_EVENTS
	
	/* Wait for the next vblank event on a throwaway fence: */
	Vulkan::Fence(device.registerVblankEvent(surface.getDirectDisplay())).wait();
	
	/* Get a vblank time sample: */
	vblankSample.set();
	
	/* Get the new vblank counter: */
	uint64_t newVblankCounter=swapchain.getVblankCounter();
	
	/* Update the vblank estimator through missed vblank events: */
	if(newVblankCounter!=vblankCounter)
		{
		missed=newVblankCounter-(vblankCounter+1);
		for(uint64_t i=0;i<missed;++i)
			vblankEstimator.update();
		}
	else
		Misc::formattedConsoleWarning("HMD::vsync: Duplicate vsync counter %lu",vblankCounter);
	
	/* Update the vblank estimator: */
	vblankEstimator.update(vblankSample);
	
	#else // #if HMD_USE_VBLANK_EVENTS
	
	/* Check if the next vblank event already occurred: */
	uint64_t newVblankCounter=swapchain.getVblankCounter();
	if(vblankCounter!=newVblankCounter)
		{
		/* Calculate how many vblank events we missed: */
		missed=newVblankCounter-vblankCounter;
		
		/* Update the vblank estimator accordingly: */
		vblankSample.set();
		for(uint64_t i=0;i<missed;++i)
			vblankEstimator.update();
		}
	else
		{
		/* Busy-wait until the swapchain's vblank counter changes value: */
		while((newVblankCounter=swapchain.getVblankCounter())==vblankCounter)
			;
		
		/* Get a vblank time sample: */
		vblankSample.set();
		
		/* Update the vblank estimator: */
		vblankEstimator.update(vblankSample);
		
		missed=0;
		}
	
	#endif
	
	/* Update the vblank counter: */
	vblankCounter=newVblankCounter;
	
	#if HMD_PRINT_VBLANK_ESTIMATES
	std::cout<<0<<','<<vblankCounter<<','<<diffNsec(vblankSample,timeBase)<<','<<diffNsec(vblankEstimator.getVblankTime(),timeBase)<<','<<vblankEstimator.getFrameInterval().tv_nsec<<std::endl;
	#endif
	
	#if HMD_WRITE_TIMINGFILE
	vblankFile->write(vblankTimeSampler.tv_sec);
	vblankFile->write(vblankTimeSampler.tv_nsec);
	#endif
	
	return missed;
	}

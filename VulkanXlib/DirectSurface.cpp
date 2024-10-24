/***********************************************************************
DirectSurface - Class representing Vulkan presentation surfaces
associated with a direct-mode X display using the Xlib API.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <VulkanXlib/DirectSurface.h>

#include <ctype.h>
#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <X11/extensions/Xrandr.h>
#include <vulkan/vulkan_xlib_xrandr.h>

#include <Vulkan/Instance.h>
#include <Vulkan/PhysicalDeviceDescriptor.h>
#include <Vulkan/PhysicalDevice.h>
#include <VulkanXlib/XlibDisplay.h>

namespace VulkanXlib {

/******************************
Methods of class DirectSurface:
******************************/

DirectSurface::DirectSurface(const Vulkan::Instance& sInstance,const XlibDisplay& xlibDisplay,const std::string& displayName,double targetRefreshRate)
	:Vulkan::Surface(sInstance),
	 directDisplay(VK_NULL_HANDLE),
	 directDisplayPlaneIndex(~0U)
	{
	/*********************************************************************
	Find the requested display among all displays connected to all
	physical devices on the given Vulkan instance.
	*********************************************************************/
	
	/* Search all physical devices on the system: */
	std::vector<Vulkan::PhysicalDevice> physicalDevices=instance.getPhysicalDevices();
	for(std::vector<Vulkan::PhysicalDevice>::iterator pdIt=physicalDevices.begin();pdIt!=physicalDevices.end()&&!directDevice.isValid();++pdIt)
		{
		try
			{
			/* Search all displays connected to the physical device: */
			std::vector<VkDisplayPropertiesKHR> dps=pdIt->getDisplayProperties();
			for(std::vector<VkDisplayPropertiesKHR>::iterator dpIt=dps.begin();dpIt!=dps.end()&&!directDevice.isValid();++dpIt)
				{
				/* Match the given display name against the physical device's name as a substring inclusion: */
				std::string::const_iterator sdnIt=displayName.begin();
				const char* pdnPtr=dpIt->displayName;
				while(sdnIt!=displayName.end()&&*pdnPtr!='\0')
					{
					/* Advance the search iterator if there is a match: */
					if(tolower(*sdnIt)==tolower(*pdnPtr))
						++sdnIt;
					
					++pdnPtr;
					}
				
				/* Pick the display if the entire search display name has been matched: */
				if(sdnIt==displayName.end())
					{
					directDevice=*pdIt;
					directDisplay=dpIt->display;
					}
				}
			}
		catch(const std::runtime_error& err)
			{
			/* This really shouldn't happen, but oh well: */
			Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"Caught exception %s while enumerating displays",err.what());
			}
		}
	
	if(!directDevice.isValid())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Display \"%s\" not found",displayName.c_str());
	
	/*********************************************************************
	Find a display plane on the direct device that is compatible with the
	direct display and its picked display mode.
	*********************************************************************/
	
	/* Enumerate all display planes on the direct device: */
	std::vector<VkDisplayPlanePropertiesKHR> dpps=directDevice.getDisplayPlaneProperties();
	for(std::vector<VkDisplayPlanePropertiesKHR>::iterator dppIt=dpps.begin();dppIt!=dpps.end()&&directDisplayPlaneIndex==~0U;++dppIt)
		{
		/* Check that the display plane is not currently bound to a different display: */
		if(dppIt->currentDisplay==VK_NULL_HANDLE||dppIt->currentDisplay==directDisplay)
			{
			/* Query the list of displays supported by the display plane: */
			uint32_t pi=dppIt-dpps.begin();
			std::vector<VkDisplayKHR> dpsds=directDevice.getDisplayPlaneSupportedDisplays(pi);
			
			/* Check if the direct display is supported: */
			for(std::vector<VkDisplayKHR>::iterator dpsdIt=dpsds.begin();dpsdIt!=dpsds.end()&&directDisplayPlaneIndex==~0U;++dpsdIt)
				if(*dpsdIt==directDisplay)
					directDisplayPlaneIndex=pi;
			}
		}
	if(directDisplayPlaneIndex==~0U)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Display %s not supported by any available display planes",displayName);
	
	/*********************************************************************
	Select a display mode and find an alpha blending mode that is
	compatible with the found display plane.
	*********************************************************************/
	
	/* Query mode properties for the selected display: */
	std::vector<VkDisplayModePropertiesKHR> dpms=directDevice.getDisplayModeProperties(directDisplay);
	
	/* Pick the mode with the highest resolution that best matches the requested refresh rate: */
	std::vector<VkDisplayModePropertiesKHR>::iterator dpmIt=dpms.begin();
	VkDisplayModePropertiesKHR* bestMode=&*dpmIt;
	VkDeviceSize bestRes=bestMode->parameters.visibleRegion.width*bestMode->parameters.visibleRegion.height;
	double bestRateDelta=Math::abs(double(bestMode->parameters.refreshRate)/1000.0-targetRefreshRate);
	for(++dpmIt;dpmIt!=dpms.end();++dpmIt)
		{
		/* Check this mode against the current best mode and pick it if it's better: */
		const VkDisplayModeParametersKHR& dpmps=dpmIt->parameters;
		VkDeviceSize res=dpmps.visibleRegion.width*dpmps.visibleRegion.height;
		double rateDelta=Math::abs(double(dpmps.refreshRate)/1000.0-targetRefreshRate);
		if(res>bestRes||(res==bestRes&&rateDelta<bestRateDelta))
			{
			bestMode=&*dpmIt;
			bestRes=res;
			bestRateDelta=rateDelta;
			}
		}
	directDisplayMode=bestMode->displayMode;
	directDisplayModeParameters=bestMode->parameters;
	
	/* Pick an alpha blending mode compatible with the display plane: */
	VkDisplayPlaneCapabilitiesKHR dpc=directDevice.getDisplayPlaneCapabilities(directDisplayMode,directDisplayPlaneIndex);
	VkDisplayPlaneAlphaFlagBitsKHR alphaModes[4]=
		{
    VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR,
    VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR
		};
	VkDisplayPlaneAlphaFlagBitsKHR alphaMode=alphaModes[0];
	for(int i=0;i<4;++i)
		{
		/* Check if the alpha mode is supported by the display plane: */
		if(dpc.supportedAlpha&alphaModes[i])
			{
			alphaMode=alphaModes[i];
			break;
			}
		}
	if(alphaMode==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No matching alpha blending mode for display %s",displayName);
	
	/* Set up the display surface creation structure: */
	VkDisplaySurfaceCreateInfoKHR displaySurfaceCreateInfo;
	displaySurfaceCreateInfo.sType=VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	displaySurfaceCreateInfo.pNext=0;
	displaySurfaceCreateInfo.flags=0x0;
	displaySurfaceCreateInfo.displayMode=directDisplayMode;
	displaySurfaceCreateInfo.planeIndex=directDisplayPlaneIndex;
	displaySurfaceCreateInfo.planeStackIndex=dpps[directDisplayPlaneIndex].currentStackIndex;
	displaySurfaceCreateInfo.transform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	displaySurfaceCreateInfo.globalAlpha=1.0f;
	displaySurfaceCreateInfo.alphaMode=alphaMode;
	displaySurfaceCreateInfo.imageExtent=directDisplayModeParameters.visibleRegion;
	
	/* Create the display surface: */
	Vulkan::throwOnError(vkCreateDisplayPlaneSurfaceKHR(instance.getHandle(),&displaySurfaceCreateInfo,0,&surface),
	                     __PRETTY_FUNCTION__,"create display surface");
	
	/* Acquire the direct display: */
	PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT=instance.getFunction<PFN_vkAcquireXlibDisplayEXT>("vkAcquireXlibDisplayEXT");
	Vulkan::throwOnError(vkAcquireXlibDisplayEXT(directDevice.getHandle(),xlibDisplay.getDisplay(),directDisplay),
	                     __PRETTY_FUNCTION__,"acquire direct display");
	}

DirectSurface::~DirectSurface(void)
	{
	/* Release the direct display: */
	PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT=instance.getFunction<PFN_vkReleaseDisplayEXT>("vkReleaseDisplayEXT");
	vkReleaseDisplayEXT(directDevice.getHandle(),directDisplay);
	}

Vulkan::CStringList& DirectSurface::addRequiredInstanceExtensions(Vulkan::CStringList& extensions)
	{
	/* Add the base class's required extensions: */
	Vulkan::Surface::addRequiredInstanceExtensions(extensions);
	
	/* Add the XCB surface extension: */
	extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME);
	extensions.push_back(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME);
	
	return extensions;
	}

Vulkan::CStringList& DirectSurface::addRequiredDeviceExtensions(Vulkan::CStringList& extensions)
	{
	/* Add the base class's required extensions: */
	Surface::addRequiredDeviceExtensions(extensions);
	
	/* Add the swap chain and display control extensions: */
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME);
	
	return extensions;
	}

Vulkan::PhysicalDeviceDescriptor& DirectSurface::setPhysicalDevice(Vulkan::PhysicalDeviceDescriptor& physicalDeviceDescriptor) const
	{
	/* Set the descriptor's physical device: */
	physicalDeviceDescriptor.setPhysicalDevice(directDevice.getHandle());
	
	return physicalDeviceDescriptor;
	}

}

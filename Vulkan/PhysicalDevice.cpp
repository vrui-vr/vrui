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

#include <Vulkan/PhysicalDevice.h>

#include <string.h>
#include <iostream>

namespace Vulkan {

/*******************************
Methods of class PhysicalDevice:
*******************************/

std::string PhysicalDevice::getDeviceName(void) const
	{
	/* Query device properties: */
	VkPhysicalDeviceProperties pdps;
	vkGetPhysicalDeviceProperties(physicalDevice,&pdps);
	
	/* Return the device name: */
	return std::string(pdps.deviceName);
	}

VkPhysicalDeviceLimits PhysicalDevice::getDeviceLimits(void) const
	{
	/* Query device properties: */
	VkPhysicalDeviceProperties pdps;
	vkGetPhysicalDeviceProperties(physicalDevice,&pdps);
	
	/* Return the device limits: */
	return pdps.limits;
	}

CStringList PhysicalDevice::getExtensions(void) const
	{
	/* Query device extensions: */
	uint32_t numExtensions=0;
	throwOnError(vkEnumerateDeviceExtensionProperties(physicalDevice,0,&numExtensions,0),
	             __PRETTY_FUNCTION__,"enumerate extensions");
	std::vector<VkExtensionProperties> extensions(numExtensions);
	throwOnError(vkEnumerateDeviceExtensionProperties(physicalDevice,0,&numExtensions,extensions.data()),
	             __PRETTY_FUNCTION__,"enumerate extensions");
	
	/* Return a list of extension names: */
	CStringList result;
	result.reserve(numExtensions);
	for(std::vector<VkExtensionProperties>::iterator eIt=extensions.begin();eIt!=extensions.end();++eIt)
		result.push_back(eIt->extensionName);
	
	return result;
	}

bool PhysicalDevice::hasExtension(const char* extensionName) const
	{
	/* Get device extension names: */
	CStringList extensions=getExtensions();
	
	/* Search for the requested extension: */
	for(CStringList::iterator eIt=extensions.begin();eIt!=extensions.end();++eIt)
		if(strcmp(extensionName,*eIt)==0)
			return true;
	
	return false;
	}

std::vector<VkDisplayPropertiesKHR> PhysicalDevice::getDisplayProperties(void) const
	{
	/* Query properties of all displays connected to this physical device: */
	uint32_t numDisplayProperties=0;
	throwOnError(vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice,&numDisplayProperties,0),
	             __PRETTY_FUNCTION__,"enumerate display devices");
	std::vector<VkDisplayPropertiesKHR> displayProperties(numDisplayProperties);
	throwOnError(vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice,&numDisplayProperties,displayProperties.data()),
	             __PRETTY_FUNCTION__,"enumerate display devices");
	
	return displayProperties;
	}

std::vector<VkDisplayModePropertiesKHR> PhysicalDevice::getDisplayModeProperties(VkDisplayKHR display) const
	{
	/* Query mode properties of the given display: */
	uint32_t numModeProperties=0;
	throwOnError(vkGetDisplayModePropertiesKHR(physicalDevice,display,&numModeProperties,0),
	             __PRETTY_FUNCTION__,"enumerate display modes");
	std::vector<VkDisplayModePropertiesKHR> modeProperties(numModeProperties);
	throwOnError(vkGetDisplayModePropertiesKHR(physicalDevice,display,&numModeProperties,modeProperties.data()),
	             __PRETTY_FUNCTION__,"enumerate display modes");
	
	return modeProperties;
	}

std::vector<VkDisplayPlanePropertiesKHR> PhysicalDevice::getDisplayPlaneProperties(void) const
	{
	/* Query properties of all display planes on this physical device: */
	uint32_t numDisplayPlaneProperties=0;
	throwOnError(vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice,&numDisplayPlaneProperties,0),
	             __PRETTY_FUNCTION__,"enumerate display planes");
	std::vector<VkDisplayPlanePropertiesKHR> displayPlaneProperties(numDisplayPlaneProperties);
	throwOnError(vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice,&numDisplayPlaneProperties,displayPlaneProperties.data()),
	             __PRETTY_FUNCTION__,"enumerate display planes");
	
	return displayPlaneProperties;
	}

std::vector<VkDisplayKHR> PhysicalDevice::getDisplayPlaneSupportedDisplays(uint32_t displayPlaneIndex) const
	{
	/* Query the displays supported on this plane: */
	uint32_t numSupportedDisplays=0;
	throwOnError(vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice,displayPlaneIndex,&numSupportedDisplays,0),
	             __PRETTY_FUNCTION__,"enumerate supported displays");
	std::vector<VkDisplayKHR> supportedDisplays(numSupportedDisplays);
	throwOnError(vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice,displayPlaneIndex,&numSupportedDisplays,supportedDisplays.data()),
	             __PRETTY_FUNCTION__,"enumerate supported displays");
	
	return supportedDisplays;
	}

VkDisplayPlaneCapabilitiesKHR PhysicalDevice::getDisplayPlaneCapabilities(VkDisplayModeKHR displayMode,uint32_t displayPlaneIndex) const
	{
	/* Query the display plane's capabilities: */
	VkDisplayPlaneCapabilitiesKHR displayPlaneCapabilities;
	throwOnError(vkGetDisplayPlaneCapabilitiesKHR(physicalDevice,displayMode,displayPlaneIndex,&displayPlaneCapabilities),
	             __PRETTY_FUNCTION__,"query display plane capabilities");
	
	return displayPlaneCapabilities;
	}

void PhysicalDevice::dumpInfo(void) const
	{
	static const char* deviceTypes[]=
		{
		"an other device",
		"an integrated GPU",
		"a discrete GPU",
		"a virtual GPU",
		"a CPU",
		"an unknown device"
		};
	
	/* Show basic information about the physical device: */
	VkPhysicalDeviceProperties pdps;
	vkGetPhysicalDeviceProperties(physicalDevice,&pdps);
	std::cout<<"Device "<<pdps.deviceName;
	std::cout<<", api "<<VK_VERSION_MAJOR(pdps.apiVersion)<<'.'<<VK_VERSION_MINOR(pdps.apiVersion)<<'.'<<VK_VERSION_PATCH(pdps.apiVersion);
	std::cout<<", driver "<<VK_VERSION_MAJOR(pdps.driverVersion)<<'.'<<VK_VERSION_MINOR(pdps.driverVersion)<<'.'<<VK_VERSION_PATCH(pdps.driverVersion);
	std::cout<<", is "<<(pdps.deviceType<5?deviceTypes[pdps.deviceType]:deviceTypes[5])<<std::endl;
	
	/* Get device extension names: */
	CStringList extensions=getExtensions();
	std::cout<<"  Device extensions ("<<extensions.size()<<"):"<<std::endl;
	for(CStringList::iterator eIt=extensions.begin();eIt!=extensions.end();++eIt)
		std::cout<<"    "<<*eIt<<std::endl;
	
	/* Show information about the physical device's memory: */
	VkPhysicalDeviceMemoryProperties pdmps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice,&pdmps);
	std::cout<<"  Device memory heaps ("<<pdmps.memoryHeapCount<<"):"<<std::endl;
	for(uint32_t mhi=0;mhi<pdmps.memoryHeapCount;++mhi)
		{
		/* Show basic information about the memory heap: */
		const VkMemoryHeap& mh=pdmps.memoryHeaps[mhi];
		std::cout<<"    Heap "<<mhi<<": size "<<mh.size<<" B, flags "<<mh.flags;
		VkMemoryHeapFlags hf=mh.flags;
		if(hf&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			std::cout<<", device-local";
		if(hf&VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
			std::cout<<", multi-instance";
		std::cout<<std::endl;
		
		/* Count the memory heap's associated memory types: */
		uint32_t numHeapTypes=0;
		for(uint32_t mti=0;mti<pdmps.memoryTypeCount;++mti)
			{
			const VkMemoryType& mt=pdmps.memoryTypes[mti];
			
			/* Check if the memory type is associated with the current memory heap: */
			if(mt.heapIndex==mhi)
				++numHeapTypes;
			}
		std::cout<<"      Memory types ("<<numHeapTypes<<"):"<<std::endl;
		
		/* Show the memory heap's associated memory types: */
		for(uint32_t mti=0;mti<pdmps.memoryTypeCount;++mti)
			{
			const VkMemoryType& mt=pdmps.memoryTypes[mti];
			
			/* Check if the memory type is associated with the current memory heap: */
			if(mt.heapIndex==mhi)
				{
				VkMemoryPropertyFlags pfs=mt.propertyFlags;
				std::cout<<"        Type "<<mti<<", flags "<<pfs;
				if(pfs&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
					std::cout<<", device-local";
				if(pfs&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
					std::cout<<", host-visible";
				if(pfs&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
					std::cout<<", host-coherent";
				if(pfs&VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
					std::cout<<", host-cached";
				if(pfs&VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
					std::cout<<", lazily-allocated";
				if(pfs&VK_MEMORY_PROPERTY_PROTECTED_BIT)
					std::cout<<", protected";
				std::cout<<std::endl;
				}
			}
		}
	
	/* Enumerate all displays on the current device: */
	std::vector<VkDisplayPropertiesKHR> displays=getDisplayProperties();
	std::cout<<"  Displays ("<<displays.size()<<"):"<<std::endl;
	for(std::vector<VkDisplayPropertiesKHR>::iterator dIt=displays.begin();dIt!=displays.end();++dIt)
		{
		std::cout<<"  "<<dIt->displayName<<std::endl;
		std::cout<<"    "<<dIt->physicalDimensions.width<<"mm x "<<dIt->physicalDimensions.height<<"mm";
		std::cout<<", "<<dIt->physicalResolution.width<<" x "<<dIt->physicalResolution.height;
		if(dIt->planeReorderPossible)
			std::cout<<", plane reorder possible";
		if(dIt->persistentContent)
			std::cout<<", persistent content";
		std::cout<<std::endl;
		
		/* Get the display's modes: */
		std::vector<VkDisplayModePropertiesKHR> modes=getDisplayModeProperties(dIt->display);
		std::cout<<"    Modes ("<<modes.size()<<"):"<<std::endl;
		for(std::vector<VkDisplayModePropertiesKHR>::iterator mIt=modes.begin();mIt!=modes.end();++mIt)
			{
			const VkDisplayModeParametersKHR& mp=mIt->parameters;
			std::cout<<"      "<<mIt->displayMode<<": "<<mp.visibleRegion.width<<" x "<<mp.visibleRegion.height<<" @ "<<double(mp.refreshRate)/1000.0<<std::endl;
			}
		}
	
	/* Enumerate all display planes on the current device: */
	std::vector<VkDisplayPlanePropertiesKHR> planes=getDisplayPlaneProperties();
	std::cout<<"  Display planes ("<<planes.size()<<"):"<<std::endl;
	uint32_t pi=0;
	for(std::vector<VkDisplayPlanePropertiesKHR>::iterator pIt=planes.begin();pIt!=planes.end();++pi,++pIt)
		{
		std::cout<<"    Plane "<<pi<<std::endl;
		
		/* List the displays supported on this plane: */
		std::vector<VkDisplayKHR> planeDisplays=getDisplayPlaneSupportedDisplays(pi);
		std::cout<<"      Supported displays ("<<planeDisplays.size()<<"):";
		for(std::vector<VkDisplayKHR>::iterator pdspIt=planeDisplays.begin();pdspIt!=planeDisplays.end();++pdspIt)
			{
			for(std::vector<VkDisplayPropertiesKHR>::iterator dIt=displays.begin();dIt!=displays.end();++dIt)
				if(dIt->display==*pdspIt)
					std::cout<<' '<<dIt->displayName;
			}
		std::cout<<std::endl;
		
		/* Find the current display's properties in the display property list: */
		for(std::vector<VkDisplayPropertiesKHR>::iterator dIt=displays.begin();dIt!=displays.end();++dIt)
			if(dIt->display==pIt->currentDisplay)
				std::cout<<"      Current display: "<<dIt->displayName<<std::endl;
		std::cout<<"      Current stack index: "<<pIt->currentStackIndex<<std::endl;
		}
	}

}

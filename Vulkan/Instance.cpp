/***********************************************************************
Instance - Class representing Vulkan API instances.
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

#include <Vulkan/Instance.h>

#include <string.h>
#include <iostream>
#include <Misc/StdError.h>
#include <Vulkan/Common.h>
#include <Vulkan/Surface.h>
#include <Vulkan/PhysicalDeviceDescriptor.h>
#include <Vulkan/PhysicalDevice.h>

namespace Vulkan {

/*************************
Methods of class Instance:
*************************/

PFN_vkVoidFunction Instance::getFunctionPointer(const char* functionName,bool throwOnError) const
	{
	PFN_vkVoidFunction result=vkGetInstanceProcAddr(instance,functionName);
	if(throwOnError&&result==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot resolve function %s",functionName);
	
	return result;
	}

Instance::Instance(const ApplicationInfo& sApplicationInfo,const CStringList& extensions,const CStringList& sValidationLayers)
	:validationLayers(sValidationLayers),
	 instance(VK_NULL_HANDLE)
	{
	/* Set up instance creation parameters: */
	VkInstanceCreateInfo createInfo;
	createInfo.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext=0;
	createInfo.flags=0x0;
	createInfo.pApplicationInfo=&sApplicationInfo;
	createInfo.enabledLayerCount=validationLayers.size();
	createInfo.ppEnabledLayerNames=validationLayers.data();
	createInfo.enabledExtensionCount=extensions.size();
	createInfo.ppEnabledExtensionNames=createInfo.enabledExtensionCount>0?extensions.data():0;
	
	/* Create the Vulkan instance: */
	throwOnError(vkCreateInstance(&createInfo,0,&instance),
	             __PRETTY_FUNCTION__,"create Vulkan instance");
	}

Instance::~Instance(void)
	{
	/* Destroy the Vulkan instance: */
	vkDestroyInstance(instance,0);
	}

CStringList Instance::getExtensions(void)
	{
	/* Query API-wide extensions: */
	uint32_t numExtensions=0;
	throwOnError(vkEnumerateInstanceExtensionProperties(0,&numExtensions,0),
	             __PRETTY_FUNCTION__,"enumerate extensions");
	std::vector<VkExtensionProperties> extensions(numExtensions);
	throwOnError(vkEnumerateInstanceExtensionProperties(0,&numExtensions,extensions.data()),
	             __PRETTY_FUNCTION__,"enumerate extensions");
	
	/* Return a list of extension names: */
	CStringList result;
	result.reserve(numExtensions);
	for(std::vector<VkExtensionProperties>::iterator eIt=extensions.begin();eIt!=extensions.end();++eIt)
		result.push_back(eIt->extensionName);
	return result;
	}

bool Instance::hasExtension(const char* extensionName)
	{
	/* Get API-wide extension names: */
	CStringList extensions=getExtensions();
	
	/* Search for the requested extension: */
	for(CStringList::iterator eIt=extensions.begin();eIt!=extensions.end();++eIt)
		if(strcmp(extensionName,*eIt)==0)
			return true;
	
	return false;
	}

void Instance::dumpExtensions(void)
	{
	/* Get API-wide extension names: */
	CStringList extensions=getExtensions();
	
	/* Print all extension names: */
	std::cout<<"Instance extensions ("<<extensions.size()<<"):"<<std::endl;
	for(CStringList::iterator eIt=extensions.begin();eIt!=extensions.end();++eIt)
		std::cout<<"  "<<*eIt<<std::endl;
	}

bool Instance::hasValidationLayer(const char* validationLayerName)
	{
	/* Query API-wide validation layers: */
	uint32_t numLayers=0;
	throwOnError(vkEnumerateInstanceLayerProperties(&numLayers,0),
	             __PRETTY_FUNCTION__,"enumerate validation layers");
	std::vector<VkLayerProperties> layers(numLayers);
	throwOnError(vkEnumerateInstanceLayerProperties(&numLayers,layers.data()),
	             __PRETTY_FUNCTION__,"enumerate validation layers");
	
	/* Search for the requested validation layer: */
	for(std::vector<VkLayerProperties>::iterator lIt=layers.begin();lIt!=layers.end();++lIt)
		if(strcmp(validationLayerName,lIt->layerName)==0)
			return true;
	
	return false;
	}

std::vector<PhysicalDevice> Instance::getPhysicalDevices(void) const
	{
	/* Enumerate all physical devices attached to the instance: */
	uint32_t numPhysicalDevices=0;
	throwOnError(vkEnumeratePhysicalDevices(instance,&numPhysicalDevices,0),
	             __PRETTY_FUNCTION__,"enumerate physical devices");
	std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
	throwOnError(vkEnumeratePhysicalDevices(instance,&numPhysicalDevices,physicalDevices.data()),
	             __PRETTY_FUNCTION__,"enumerate physical devices");
	
	/* Wrap each device in a PhysicalDevice object and return that list: */
	std::vector<PhysicalDevice> result;
	result.reserve(numPhysicalDevices);
	for(std::vector<VkPhysicalDevice>::iterator pdIt=physicalDevices.begin();pdIt!=physicalDevices.end();++pdIt)
		result.push_back(*pdIt);
	return result;
	}

PhysicalDeviceDescriptor& Instance::setValidationLayers(PhysicalDeviceDescriptor& descriptor) const
	{
	/* Store the instance's validation layer list: */
	descriptor.validationLayers=validationLayers;
	
	return descriptor;
	}

PhysicalDeviceDescriptor& Instance::findPhysicalDevice(PhysicalDeviceDescriptor& descriptor) const
	{
	/* Enumerate all physical devices attached to the instance: */
	uint32_t numDevices=0;
	throwOnError(vkEnumeratePhysicalDevices(instance,&numDevices,0),
	             __PRETTY_FUNCTION__,"enumerate physical devices");
	std::vector<VkPhysicalDevice> devices(numDevices);
	throwOnError(vkEnumeratePhysicalDevices(instance,&numDevices,devices.data()),
	             __PRETTY_FUNCTION__,"enumerate physical devices");
	
	/* Test each physical device for its applicability: */
	VkPhysicalDevice foundPhysicalDevice=VK_NULL_HANDLE;
	for(std::vector<VkPhysicalDevice>::iterator dIt=devices.begin();dIt!=devices.end()&&foundPhysicalDevice==VK_NULL_HANDLE;++dIt)
		{
		bool applicable=true;
		
		if(applicable)
			{
			/* Query the device's features: */
			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(*dIt,&features);
			
			/* Check if the device supports all required features: */
			applicable=applicable&&(!descriptor.deviceFeatures.robustBufferAccess||features.robustBufferAccess);
			applicable=applicable&&(!descriptor.deviceFeatures.fullDrawIndexUint32||features.fullDrawIndexUint32);
			applicable=applicable&&(!descriptor.deviceFeatures.imageCubeArray||features.imageCubeArray);
			applicable=applicable&&(!descriptor.deviceFeatures.independentBlend||features.independentBlend);
			applicable=applicable&&(!descriptor.deviceFeatures.geometryShader||features.geometryShader);
			applicable=applicable&&(!descriptor.deviceFeatures.tessellationShader||features.tessellationShader);
			applicable=applicable&&(!descriptor.deviceFeatures.sampleRateShading||features.sampleRateShading);
			applicable=applicable&&(!descriptor.deviceFeatures.dualSrcBlend||features.dualSrcBlend);
			applicable=applicable&&(!descriptor.deviceFeatures.logicOp||features.logicOp);
			applicable=applicable&&(!descriptor.deviceFeatures.multiDrawIndirect||features.multiDrawIndirect);
			applicable=applicable&&(!descriptor.deviceFeatures.drawIndirectFirstInstance||features.drawIndirectFirstInstance);
			applicable=applicable&&(!descriptor.deviceFeatures.depthClamp||features.depthClamp);
			applicable=applicable&&(!descriptor.deviceFeatures.depthBiasClamp||features.depthBiasClamp);
			applicable=applicable&&(!descriptor.deviceFeatures.fillModeNonSolid||features.fillModeNonSolid);
			applicable=applicable&&(!descriptor.deviceFeatures.depthBounds||features.depthBounds);
			applicable=applicable&&(!descriptor.deviceFeatures.wideLines||features.wideLines);
			applicable=applicable&&(!descriptor.deviceFeatures.largePoints||features.largePoints);
			applicable=applicable&&(!descriptor.deviceFeatures.alphaToOne||features.alphaToOne);
			applicable=applicable&&(!descriptor.deviceFeatures.multiViewport||features.multiViewport);
			applicable=applicable&&(!descriptor.deviceFeatures.samplerAnisotropy||features.samplerAnisotropy);
			applicable=applicable&&(!descriptor.deviceFeatures.textureCompressionETC2||features.textureCompressionETC2);
			applicable=applicable&&(!descriptor.deviceFeatures.textureCompressionASTC_LDR||features.textureCompressionASTC_LDR);
			applicable=applicable&&(!descriptor.deviceFeatures.textureCompressionBC||features.textureCompressionBC);
			applicable=applicable&&(!descriptor.deviceFeatures.occlusionQueryPrecise||features.occlusionQueryPrecise);
			applicable=applicable&&(!descriptor.deviceFeatures.pipelineStatisticsQuery||features.pipelineStatisticsQuery);
			applicable=applicable&&(!descriptor.deviceFeatures.vertexPipelineStoresAndAtomics||features.vertexPipelineStoresAndAtomics);
			applicable=applicable&&(!descriptor.deviceFeatures.fragmentStoresAndAtomics||features.fragmentStoresAndAtomics);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderTessellationAndGeometryPointSize||features.shaderTessellationAndGeometryPointSize);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderImageGatherExtended||features.shaderImageGatherExtended);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderStorageImageExtendedFormats||features.shaderStorageImageExtendedFormats);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderStorageImageMultisample||features.shaderStorageImageMultisample);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderStorageImageReadWithoutFormat||features.shaderStorageImageReadWithoutFormat);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderStorageImageWriteWithoutFormat||features.shaderStorageImageWriteWithoutFormat);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderUniformBufferArrayDynamicIndexing||features.shaderUniformBufferArrayDynamicIndexing);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderSampledImageArrayDynamicIndexing||features.shaderSampledImageArrayDynamicIndexing);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderStorageBufferArrayDynamicIndexing||features.shaderStorageBufferArrayDynamicIndexing);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderStorageImageArrayDynamicIndexing||features.shaderStorageImageArrayDynamicIndexing);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderClipDistance||features.shaderClipDistance);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderCullDistance||features.shaderCullDistance);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderFloat64||features.shaderFloat64);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderInt64||features.shaderInt64);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderInt16||features.shaderInt16);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderResourceResidency||features.shaderResourceResidency);
			applicable=applicable&&(!descriptor.deviceFeatures.shaderResourceMinLod||features.shaderResourceMinLod);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseBinding||features.sparseBinding);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidencyBuffer||features.sparseResidencyBuffer);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidencyImage2D||features.sparseResidencyImage2D);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidencyImage3D||features.sparseResidencyImage3D);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidency2Samples||features.sparseResidency2Samples);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidency4Samples||features.sparseResidency4Samples);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidency8Samples||features.sparseResidency8Samples);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidency16Samples||features.sparseResidency16Samples);
			applicable=applicable&&(!descriptor.deviceFeatures.sparseResidencyAliased||features.sparseResidencyAliased);
			applicable=applicable&&(!descriptor.deviceFeatures.variableMultisampleRate||features.variableMultisampleRate);
			applicable=applicable&&(!descriptor.deviceFeatures.inheritedQueries||features.inheritedQueries);
			}
		
		if(applicable)
			{
			/* Query the device's extensions: */
			uint32_t numExtensions=0;
			throwOnError(vkEnumerateDeviceExtensionProperties(*dIt,0,&numExtensions,0),
			             __PRETTY_FUNCTION__,"enumerate device extensions");
			std::vector<VkExtensionProperties> extensions(numExtensions);
			throwOnError(vkEnumerateDeviceExtensionProperties(*dIt,0,&numExtensions,extensions.data()),
			             __PRETTY_FUNCTION__,"enumerate device extensions");
			
			/* Check if the device supports all required extensions: */
			for(CStringList::const_iterator deIt=descriptor.deviceExtensions.begin();deIt!=descriptor.deviceExtensions.end()&&applicable;++deIt)
				{
				/* Check if the device supports the current extension: */
				bool found=false;
				for(std::vector<VkExtensionProperties>::iterator eIt=extensions.begin();eIt!=extensions.end()&&!found;++eIt)
					if(strcmp(*deIt,eIt->extensionName)==0)
						found=true;
				
				applicable=found;
				}
			}
		
		/* Find queue families on the physical device: */
		descriptor.physicalDevice=*dIt;
		applicable=applicable&&descriptor.findQueueFamilies();
		
		/* Store the current device if it passed all tests: */
		if(applicable)
			foundPhysicalDevice=*dIt;
		}
	descriptor.physicalDevice=foundPhysicalDevice;
	
	if(foundPhysicalDevice!=VK_NULL_HANDLE)
		{
		/* Store the instance's validation layer list: */
		descriptor.validationLayers=validationLayers;
		}
	
	return descriptor;
	}

}

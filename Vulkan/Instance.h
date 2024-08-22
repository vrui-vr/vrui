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

#ifndef VULKAN_INSTANCE_INCLUDED
#define VULKAN_INSTANCE_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>
#include <Vulkan/Common.h>
#include <Vulkan/ApplicationInfo.h>

/* Forward declarations: */
namespace Vulkan {
class PhysicalDeviceDescriptor;
class PhysicalDevice;
}

namespace Vulkan {

class Instance
	{
	/* Elements: */
	private:
	CStringList validationLayers; // List of requested validation layers
	VkInstance instance; // Vulkan instance handle
	
	/* Private methods: */
	PFN_vkVoidFunction getFunctionPointer(const char* functionName,bool throwOnError) const; // Low-level method to retrieve function pointers
	
	/* Constructors and destructors: */
	public:
	Instance(const ApplicationInfo& sApplicationInfo,const CStringList& extensions,const CStringList& sValidationLayers); // Creates an instance for the given application info and lists of extensions and validation layers
	private:
	Instance(const Instance& source); // Prohibit copy constructor
	Instance& operator=(const Instance& source); // Prohibit assignment operator
	public:
	~Instance(void);
	
	/* Methods: */
	static CStringList getExtensions(void); // Returns the list of extensions supported by the local Vulkan library
	static bool hasExtension(const char* extensionName); // Returns true if the local Vulkan library supports the given Vulkan extension
	static void dumpExtensions(void); // Prints the list of extensions to stdout
	static bool hasValidationLayer(const char* validationLayerName); // Returns true if the local Vulkan library supports the given Vulkan validation layer
	const CStringList& getValidationLayers(void) const // Returns the list of validation layers for this instance
		{
		return validationLayers;
		}
	VkInstance getHandle(void) const // Returns the Vulkan instance handle
		{
		return instance;
		}
	template <class FunctionTypeParam>
	FunctionTypeParam getFunction(const char* functionName,bool throwOnError =true) const // Returns a function pointer for the extension function of the given name
		{
		return (FunctionTypeParam)getFunctionPointer(functionName,throwOnError);
		}
	std::vector<PhysicalDevice> getPhysicalDevices(void) const; // Returns a list of all physical devices on the local system
	PhysicalDeviceDescriptor& setValidationLayers(PhysicalDeviceDescriptor& descriptor) const; // Sets the given physical device descriptor's valiation layers to those of the instance
	PhysicalDeviceDescriptor& findPhysicalDevice(PhysicalDeviceDescriptor& descriptor) const; // Finds the first physical device matching the given device descriptor and updates the descriptor
	};

}

#endif

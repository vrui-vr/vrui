/***********************************************************************
DebugUtilsMessenger - Base class to receive debugging messages from
Vulkan.
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

#ifndef VULKAN_DEBUGUTILSMESSENGER_INCLUDED
#define VULKAN_DEBUGUTILSMESSENGER_INCLUDED

#include <vulkan/vulkan.h>

/* Forward declarations: */
namespace Vulkan {
class Instance;
}

namespace Vulkan {

class DebugUtilsMessenger
	{
	/* Elements: */
	private:
	const Instance& instance; // Reference to the Vulkan instance for which the debug messenger was created
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT; // The low-level construction function
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT; // The low-level destruction function
	VkDebugUtilsMessengerEXT debugUtilsMessenger; // Vulkan debug messenger handle
	
	/* Private methods: */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageType,const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,void* pUserData) // Callback wrapper; redirects to virtual method
		{
		/* Redirect to the virtual method: */
		DebugUtilsMessenger* thisPtr=static_cast<DebugUtilsMessenger*>(pUserData);
		
		return thisPtr->debug(messageSeverity,messageType,pCallbackData);
		}
	
	/* Protected methods: */
	protected:
	virtual VkBool32 debug(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageType,const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);
	
	/* Constructors and destructors: */
	public:
	DebugUtilsMessenger(const Instance& sInstance); // Creates a debug messenger for the given Vulkan instance
	private:
	DebugUtilsMessenger(const DebugUtilsMessenger& source); // Prohibit copy constructor
	DebugUtilsMessenger& operator=(const DebugUtilsMessenger& source); // Prohibit assignment operator
	public:
	virtual ~DebugUtilsMessenger(void);
	
	/* Methods: */
	VkDebugUtilsMessengerEXT getHandle(void) const // Returns the Vulkan debug messenger handle
		{
		return debugUtilsMessenger;
		}
	};

}

#endif

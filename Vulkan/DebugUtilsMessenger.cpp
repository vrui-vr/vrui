/***********************************************************************
DebugUtilsMessenger - Base class to receive debugging messages from
Vulkan.
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

#include <Vulkan/DebugUtilsMessenger.h>

#include <stdexcept>
#include <iostream>
#include <Misc/StdError.h>
#include <Vulkan/Instance.h>

namespace Vulkan {

/************************************
Methods of class DebugUtilsMessenger:
************************************/

VkBool32 DebugUtilsMessenger::debug(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageType,const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
	{
	/* Print the validation message: */
	std::cerr<<"Vulkan debug message (severity "<<messageSeverity<<", type "<<messageType<<"): "<<pCallbackData->pMessage<<std::endl;
	
	return VK_FALSE;
	}

DebugUtilsMessenger::DebugUtilsMessenger(const Instance& sInstance)
	:instance(sInstance),
	 vkCreateDebugUtilsMessengerEXT(0),vkDestroyDebugUtilsMessengerEXT(0),
	 debugUtilsMessenger(VK_NULL_HANDLE)
	{
	/* Acquire the low-level construction/destruction functions: */
	vkCreateDebugUtilsMessengerEXT=instance.getFunction<PFN_vkCreateDebugUtilsMessengerEXT>("vkCreateDebugUtilsMessengerEXT");
	vkDestroyDebugUtilsMessengerEXT=instance.getFunction<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT");
	if(vkCreateDebugUtilsMessengerEXT==0||vkDestroyDebugUtilsMessengerEXT==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Low-level constructor/destructor not found");
	
	/* Set up debug messenger creation parameters: */
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;
	debugUtilsMessengerCreateInfo.sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugUtilsMessengerCreateInfo.pNext=0;
	debugUtilsMessengerCreateInfo.flags=0x0;
	debugUtilsMessengerCreateInfo.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCreateInfo.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsMessengerCreateInfo.pfnUserCallback=debugCallback;
	debugUtilsMessengerCreateInfo.pUserData=this;
	
	/* Create the debug messenger: */
	VkResult result=vkCreateDebugUtilsMessengerEXT(instance.getHandle(),&debugUtilsMessengerCreateInfo,0,&debugUtilsMessenger);
	if(result!=VK_SUCCESS)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create debug utils manager due to Vulkan error %d",int(result));
	}

DebugUtilsMessenger::~DebugUtilsMessenger(void)
	{
	/* Destroy the Vulkan debug messenger: */
	vkDestroyDebugUtilsMessengerEXT(instance.getHandle(),debugUtilsMessenger,0);
	}

}

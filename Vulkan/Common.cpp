/***********************************************************************
Common - Common helper functions when using the Vulkan API.
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

#include <Vulkan/Common.h>

#include <Misc/StdError.h>

namespace Vulkan {

const char* resultToString(VkResult result)
	{
	switch(result)
		{
		case VK_SUCCESS:
			return "success";
		
		case VK_NOT_READY:
			return "not ready";
		
		case VK_TIMEOUT:
			return "timeout";
		
		case VK_EVENT_SET:
			return "event set";
		
		case VK_EVENT_RESET:
			return "event reset";
		
		case VK_INCOMPLETE:
			return "incomplete";
		
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "out of host memory";
		
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "out of device memory";
		
		case VK_ERROR_INITIALIZATION_FAILED:
			return "initialization failed";
		
		case VK_ERROR_DEVICE_LOST:
			return "device lost";
		
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "memory map failed";
		
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "layer not present";
		
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "extension not present";
		
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "feature not present";
		
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "incompatible driver";
		
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "too many objects";
		
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "format not supported";
		
		case VK_ERROR_FRAGMENTED_POOL:
			return "fragmented pool";
		
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return "out of pool memory";
		
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return "invalid external handle";
		
		case VK_ERROR_SURFACE_LOST_KHR:
			return "(KHR) surface lost";
		
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "(KHR) native window in use";
		
		case VK_SUBOPTIMAL_KHR:
			return "(KHR) suboptimal";
		
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "(KHR) out of date";
		
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "(KHR) incompatible display";
		
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "(EXT) validation failed";
		
		case VK_ERROR_INVALID_SHADER_NV:
			return "(NV) invalid shader";
		
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return "(EXT) invalid DRM format modifier plane layout";
		
		case VK_ERROR_FRAGMENTATION_EXT:
			return "(EXT) fragmentation";
		
		case VK_ERROR_NOT_PERMITTED_EXT:
			return "(EXT) not permitted";
		
		case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
			return "(EXT) invalid device address";
		
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return "(EXT) full-screen exclusive mode lost";
		
		default:
			return "(unknown)";
		}
	}

void throwOnError(VkResult result,const char* prettyFunction,const char* operation)
	{
	/* Throw an exception with the given format string if the given result indicates an error: */
	if(result!=VK_SUCCESS)
		throw Misc::makeStdErr(prettyFunction,"Cannot %s due to Vulkan error %d (%s)",operation,int(result),resultToString(result));
	}

}

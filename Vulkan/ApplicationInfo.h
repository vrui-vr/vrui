/***********************************************************************
ApplicationInfo - Wrapper class for Vulkan application info structure.
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

#ifndef VULKAN_APPLICATIONINFO_INCLUDED
#define VULKAN_APPLICATIONINFO_INCLUDED

#include <vulkan/vulkan.h>

namespace Vulkan {

struct APIVersion // Helper structure to communicate API versions
	{
	/* Elements: */
	public:
	uint32_t version; // Vulkan-style combined version number
	
	/* Constructors and destructors: */
	APIVersion(unsigned int variant,unsigned int major,unsigned int minor,unsigned int patch) // Makes a combined version number from the given components
		#ifdef VK_MAKE_API_VERSION
		:version(VK_MAKE_API_VERSION(variant,major,minor,patch))
		#else
		:version(VK_MAKE_VERSION(major,minor,patch))
		#endif
		{
		}
	APIVersion(unsigned int major,unsigned int minor,unsigned int patch) // Makes a combined version number from the given components with a zero variant
		#ifdef VK_MAKE_API_VERSION
		:version(VK_MAKE_API_VERSION(0,major,minor,patch))
		#else
		:version(VK_MAKE_VERSION(major,minor,patch))
		#endif
		{
		}
	APIVersion(unsigned int major,unsigned int minor) // Makes a combined version number from the given components with a zero variant and patch
		#ifdef VK_MAKE_API_VERSION
		:version(VK_MAKE_API_VERSION(0,major,minor,0))
		#else
		:version(VK_MAKE_VERSION(major,minor,0))
		#endif
		{
		}
	
	/* Methods to return combined version number components: */
	unsigned int getVariant(void) const
		{
		#ifdef VK_MAKE_API_VERSION
		return VK_API_VERSION_VARIANT(version);
		#else
		return 0;
		#endif
		}
	unsigned int getMajor(void) const
		{
		#ifdef VK_MAKE_API_VERSION
		return VK_API_VERSION_MAJOR(version);
		#else
		return VK_VERSION_MAJOR(version);
		#endif
		}
	unsigned int getMinor(void) const
		{
		#ifdef VK_MAKE_API_VERSION
		return VK_API_VERSION_MINOR(version);
		#else
		return VK_VERSION_MINOR(version);
		#endif
		}
	unsigned int getPatch(void) const
		{
		#ifdef VK_MAKE_API_VERSION
		return VK_API_VERSION_PATCH(version);
		#else
		return VK_VERSION_PATCH(version);
		#endif
		}
	};

class ApplicationInfo:public VkApplicationInfo
	{
	/* Constructors and destructors: */
	public:
	ApplicationInfo(const char* sApplicationName,const APIVersion& sApplicationVersion,const char* sEngineName,const APIVersion& sEngineVersion);
	};

}

#endif

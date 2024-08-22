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

#include <Vulkan/ApplicationInfo.h>

namespace Vulkan {

/********************************
Methods of class ApplicationInfo:
********************************/

ApplicationInfo::ApplicationInfo(const char* sApplicationName,const APIVersion& sApplicationVersion,const char* sEngineName,const APIVersion& sEngineVersion)
	{
	/* Initalize application info fields: */
	sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
	pNext=0; // &debugMessengerCreateInfo; Worry about debug messengers later!
	pApplicationName=sApplicationName;
	applicationVersion=sApplicationVersion.version;
	pEngineName=sEngineName;
	engineVersion=sEngineVersion.version;
	apiVersion=VK_API_VERSION_1_0;
	}

}

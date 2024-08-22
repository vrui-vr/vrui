/***********************************************************************
Common - Common helper functions when using the Vulkan API.
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

#ifndef VULKAN_COMMON_INCLUDED
#define VULKAN_COMMON_INCLUDED

#include <vector>
#include <vulkan/vulkan.h>

namespace Vulkan {

typedef std::vector<const char*> CStringList; // Type for lists of C-style strings

const char* resultToString(VkResult result); // Returns a short text string for the given result code

void throwOnError(VkResult result,const char* prettyFunction,const char* operation); // Throws an exception if the given result code is not VK_SUCCESS; prettyFunction parameter is expected to be evaluation of __PRETTY_FUNCTION__ macro

}

#endif

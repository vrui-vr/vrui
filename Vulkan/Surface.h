/***********************************************************************
Surface - Base class representing Vulkan presentation surfaces.
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

#ifndef VULKAN_SURFACE_INCLUDED
#define VULKAN_SURFACE_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/Common.h>

/* Forward declarations: */
namespace Vulkan {
class Instance;
}

namespace Vulkan {

class Surface
	{
	/* Elements: */
	protected:
	const Instance& instance; // Reference to the Vulkan instance for which the surface was created
	VkSurfaceKHR surface; // Vulkan surface handle
	
	/* Constructors and destructors: */
	public:
	Surface(const Instance& sInstance); // Creates an invalid surface; derived classes will handle actual initialization
	private:
	Surface(const Surface& source); // Prohibit copy constructor
	Surface& operator=(const Surface& source); // Prohibit assignment operator
	public:
	virtual ~Surface(void); // Destroys the surface
	
	/* Methods: */
	static CStringList& addRequiredInstanceExtensions(CStringList& extensions); // Adds the list of instance extensions required to create surfaces to the given extension list
	static CStringList& addRequiredDeviceExtensions(CStringList& extensions); // Adds the list of device extensions required to create surfaces to the given extension list
	VkSurfaceKHR getHandle(void) const // Returns the Vulkan surface handle
		{
		return surface;
		}
	};

}

#endif

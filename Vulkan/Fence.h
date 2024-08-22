/***********************************************************************
Fence - Class representing Vulkan fences for CPU-GPU synchronization.
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

#ifndef VULKAN_FENCE_INCLUDED
#define VULKAN_FENCE_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

namespace Vulkan {

class Fence:public DeviceAttached
	{
	/* Elements: */
	private:
	VkFence fence; // Vulkan fence handle
	
	/* Constructors and destructors: */
	public:
	Fence(Device& sDevice,VkFence sFence); // Creates a fence from the given existing Vulkan fence handle
	Fence(Device& sDevice,bool createSignaled =false); // Creates a fence for the given logical device; creates fence in signaled state if flag is true
	Fence(Fence&& source); // Move constructor
	virtual ~Fence(void); // Destroys the fence
	
	/* Methods: */
	VkFence getHandle(void) const // Returns the Vulkan fence handle
		{
		return fence;
		}
	void wait(bool reset =false); // Waits for the fence to be signaled; resets fence to non-signaled state after waiting if flag is true
	void reset(void); // Resets the fence to non-signaled state
	};

}

#endif

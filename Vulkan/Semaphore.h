/***********************************************************************
Semaphore - Class representing Vulkan semaphores for GPU-GPU
synchronization.
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

#ifndef VULKAN_SEMAPHORE_INCLUDED
#define VULKAN_SEMAPHORE_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

namespace Vulkan {

class Semaphore:public DeviceAttached
	{
	/* Elements: */
	private:
	VkSemaphore semaphore; // Vulkan semaphore handle
	
	/* Constructors and destructors: */
	public:
	Semaphore(Device& sDevice); // Creates a semaphore for the given logical device
	Semaphore(Semaphore&& source); // Move constructor
	virtual ~Semaphore(void); // Destroys the semaphore
	
	/* Methods: */
	VkSemaphore getHandle(void) const // Returns the Vulkan semaphore handle
		{
		return semaphore;
		}
	};

}

#endif

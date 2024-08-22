/***********************************************************************
CommandPool - Class representing Vulkan command pools.
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

#ifndef VULKAN_COMMANDPOOL_INCLUDED
#define VULKAN_COMMANDPOOL_INCLUDED

#include <vulkan/vulkan.h>
#include <Vulkan/DeviceAttached.h>

/* Forward declarations: */
namespace Vulkan {
class CommandBuffer;
}

namespace Vulkan {

class CommandPool:public DeviceAttached
	{
	/* Elements: */
	private:
	VkCommandPool commandPool; // Vulkan command pool handle
	
	/* Constructors and destructors: */
	public:
	CommandPool(Device& sDevice,uint32_t queueFamilyIndex,VkCommandPoolCreateFlags flags); // Creates a command pool for the given device and the queue family of the given index
	virtual ~CommandPool(void); // Destroys the command pool
	
	/* Methods: */
	VkCommandPool getHandle(void) const // Returns the Vulkan command pool handle
		{
		return commandPool;
		}
	CommandBuffer allocateCommandBuffer(void); // Allocates a single primary command buffer
	void freeCommandBuffer(CommandBuffer& commandBuffer); // Frees a single primary command buffer
	CommandBuffer beginOneshotCommand(void); // Returns a transient command buffer ready to record commands
	void executeOneshotCommand(CommandBuffer& commandBuffer); // Ends the given command buffer, submits it to the device's graphics queue, and waits until the command is executed
	};

}

#endif

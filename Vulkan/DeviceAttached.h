/***********************************************************************
DeviceAttached - Base class representing Vulkan objects that are
attached to logical Vulkan devices.
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

#ifndef VULKAN_DEVICEATTACHED_INCLUDED
#define VULKAN_DEVICEATTACHED_INCLUDED

/* Forward declarations: */
namespace Vulkan {
class Device;
}

namespace Vulkan {

class DeviceAttached
	{
	/* Elements: */
	protected:
	Device& device; // Logical device to which the object is attached
	
	/* Constructors and destructors: */
	DeviceAttached(Device& sDevice) // Creates an object attached to the given device
		:device(sDevice)
		{
		}
	DeviceAttached(DeviceAttached&& source) // Ditto
		:device(source.device)
		{
		}
	private:
	DeviceAttached(const DeviceAttached& source); // Prohibit copy constructor
	DeviceAttached& operator=(const DeviceAttached& source); // Prohibit assignment operator
	public:
	virtual ~DeviceAttached(void); // Releases the device attachment
	
	/* Methods: */
	const Device& getDevice(void) const // Returns the attachment device
		{
		return device;
		}
	Device& getDevice(void) // Ditto
		{
		return device;
		}
	};

}

#endif

/***********************************************************************
Device - Class representing a human interface device for raw access.
Version using Mac OS X 10.5+ IoKit HID Manager interface.
Copyright (c) 2014-2024 Oliver Kreylos

This file is part of the Raw HID Support Library (RawHID).

The Raw HID Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Raw HID Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Raw HID Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <RawHID/MacOSX/Device.h>

#include <Misc/StdError.h>

namespace RawHID {

/*******************************
Static elements of class Device:
*******************************/

Threads::Mutex Device::hidManagerMutex;
unsigned int Device::deviceCount=0U;
IOHIDManagerRef Device::hidManager=0;

/***********************
Methods of class Device:
***********************/

void Device::refHidManager(void)
	{
	/* Lock the HID manager mutex: */
	Threads::Mutex::Lock hidManagerLock(hidManagerMutex);
	
	/* Check if the HID manager needs to be created: */
	if(deviceCount==0)
		{
		/* Create a HID manager: */
		hidManager=IOHIDManagerCreate(,kIOHIDOptionsTypeNone);
		if(hidManager==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to create HID manager");
		}
	
	/* Register the device: */
	++deviceCount;
	}

void Device::unrefHidManager(void)
	{
	/* Lock the HID manager mutex: */
	Threads::Mutex::Lock hidManagerLock(hidManagerMutex);
	
	/* Unregister the device: */
	--deviceCount;
	
	/* Check if the HID manager needs to be destroyed: */
	if(deviceCount==0)
		{
		/* Destroy the HID manager: */
		CFRelease(hidManager);
		hidManager=0;
		}
	}

}

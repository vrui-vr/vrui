/***********************************************************************
Device - Class representing a human interface device for raw access.
Version using Mac OS X 10.5+ IoKit HID Manager interface.
Copyright (c) 2014 Oliver Kreylos

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

#ifndef RAWHID_MACOSX_DEVICE_INCLUDED
#define RAWHID_MACOSX_DEVICE_INCLUDED

#include <Threads/Mutex.h>

namespace RawHID {

class Device
	{
	/* Elements: */
	private:
	static Threads::Mutex hidManagerMutex; // Semaphore protecting the HID manager
	static unsigned int deviceCount; // Number of existing raw HID devices; when count reaches zero, HID manager is released
	static IOHIDManagerRef hidManager; // Reference to the HID manager used by all raw HID devices
	
	/* Private methods: */
	static void refHidManager(void); // Adds a reference to the HID manager; creates manager if necessary
	static void unrefHidManager(void); // Removes a reference from the HID manager; destroys manager when no more references
	
	/* Constructors and destructors: */
	public:
	};

}

#endif

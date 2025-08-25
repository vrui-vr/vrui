/***********************************************************************
PenDeviceConfig - Structure defining the layout of an event device
representing a touchscreen, graphics tablet, etc.
Copyright (c) 2025 Oliver Kreylos

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

#ifndef RAWHID_PENDEVICECONFIG_INCLUDED
#define RAWHID_PENDEVICECONFIG_INCLUDED

#include <vector>

/* Forward declarations: */
namespace RawHID {
class EventDevice;
}

namespace RawHID {

struct PenDeviceConfig
	{
	/* Embedded classes: */
	public:
	struct PenState // Current state of the pen
		{
		/* Elements: */
		public:
		bool valid; // Flag if the pen is within range of the device
		int pos[2]; // Pen position
		int tilt[2]; // Pen tilt angles if the device supports tilt
		bool pressed; // Flag if the pen is pressed against the device
		};
	
	/* Elements: */
	public:
	unsigned int posAxisIndices[2]; // Device indices of the device's position axes
	unsigned int tiltAxisIndices[2]; // Device indices of the device's tilt axes
	unsigned int touchKeyIndex; // Index of the device's touch key
	unsigned int pressKeyIndex; // Index of the device's press key
	std::vector<unsigned int> otherKeyIndices; // Indices of other keys available on the device
	bool valid; // Flag if the device has the required axes/buttons for a pen device
	bool haveTilt; // Flag if the device has tilt axes
	
	/* Constructors and destructors: */
	PenDeviceConfig(void); // Creates an invalid pen device configuration
	PenDeviceConfig(EventDevice& device); // Extracts a pen device configuration from the given event device
	
	/* Methods: */
	PenState getPenState(const EventDevice& device) const; // Returns the current pen state of the given event device
	};

}

#endif

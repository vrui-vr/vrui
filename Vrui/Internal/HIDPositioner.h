/***********************************************************************
HIDPositioner - Base class to calculate 3D tracking data for HID-class
devices.
Copyright (c) 2023-2025 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_INTERNAL_HIDPOSITIONER_INCLUDED
#define VRUI_INTERNAL_HIDPOSITIONER_INCLUDED

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace RawHID {
class EventDevice;
}
namespace Vrui {
class InputDevice;
}

namespace Vrui {

class HIDPositioner
	{
	/* Elements: */
	protected:
	RawHID::EventDevice& hid; // Reference to the HID with which this positioner is associated
	bool project; // Flag whether to project the device using the UI manager
	
	/* Constructors and destructors: */
	public:
	static HIDPositioner* create(RawHID::EventDevice& sHid,const Misc::ConfigurationFileSection& configFileSection,bool* ignoredFeatures); // Creates a HID positioner for the given device from the given configuration file section
	HIDPositioner(RawHID::EventDevice& sHid); // Creates a HID positioner for the given HID
	virtual ~HIDPositioner(void);
	
	/* Methods: */
	virtual int getTrackType(void) const =0; // Returns the tracking type supported by this positioner
	void setProject(bool newProject); // Sets the device projection flag
	virtual void prepareMainLoop(void); // Called right before Vrui starts its main loop
	virtual void updateDevice(InputDevice* device) =0; // Updates the given Vrui input device's tracking state
	};

}

#endif

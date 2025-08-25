/***********************************************************************
HIDPositionerPenPad - Base class for HID positioners representing stylus
and touchscreen devices.
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

#ifndef VRUI_INTERNAL_HIDPOSITIONERPENPAD_INCLUDED
#define VRUI_INTERNAL_HIDPOSITIONERPENPAD_INCLUDED

#include <string>
#include <vector>
#include <RawHID/PenDeviceConfig.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Vrui/Types.h>
#include <Vrui/Internal/HIDPositioner.h>

/* Forward declarations: */
namespace Vrui {
class VRScreen;
class PenPadCalibrator;
}

namespace Vrui {

class HIDPositionerPenPad:public HIDPositioner
	{
	/* Elements: */
	private:
	RawHID::PenDeviceConfig config; // Pen pad configuration of the associated HID
	unsigned int posAxisIndices[2]; // Feature indices of the pen pad's position axes features
	unsigned int touchKeyIndex; // Feature index of the pen pad's touch key feature
	PenPadCalibrator* calibrator; // Pointer to a calibrator for raw pen pad measurements
	std::string screenName; // Name of the screen to associate with the pen pad device
	VRScreen* screen; // VR screen associated with the pen pad device
	
	/* Constructors and destructors: */
	public:
	HIDPositionerPenPad(RawHID::EventDevice& sHid,const Misc::ConfigurationFileSection& configFileSection,bool* ignoredFeatures); // Creates a HID positioner for the given HID from the given configuration file section; updates the array of ignored features
	virtual ~HIDPositionerPenPad(void);
	
	/* Methods from class HIDPositioner: */
	virtual int getTrackType(void) const;
	virtual void prepareMainLoop(void);
	virtual void updateDevice(InputDevice* device);
	};

}

#endif

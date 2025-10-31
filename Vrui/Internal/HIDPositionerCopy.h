/***********************************************************************
HIDPositionerCopy - HID positioner class that copies the tracking state
of another input device.
Copyright (c) 2025 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_HIDPOSITIONERCOPY_INCLUDED
#define VRUI_INTERNAL_HIDPOSITIONERCOPY_INCLUDED

#include <Vrui/Internal/HIDPositioner.h>

namespace Vrui {

class HIDPositionerCopy:public HIDPositioner
	{
	/* Elements: */
	private:
	InputDevice* sourceDevice; // Pointer to the input device whose tracking state to copy
	
	/* Constructors and destructors: */
	public:
	HIDPositionerCopy(RawHID::EventDevice& sHid,const Misc::ConfigurationFileSection& configFileSection);
	
	/* Methods from class HIDPositioner: */
	virtual int getTrackType(void) const;
	virtual void updateDevice(InputDevice* sDevice);
	};

}

#endif

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

#include <Vrui/Internal/HIDPositionerCopy.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/UIManager.h>

namespace Vrui {

HIDPositionerCopy::HIDPositionerCopy(RawHID::EventDevice& sHid,const Misc::ConfigurationFileSection& configFileSection)
	:HIDPositioner(sHid),
	 sourceDevice(0)
	{
	/* Get the name of the input device whose tracking state to copy: */
	std::string sourceDeviceName=configFileSection.retrieveString("./sourceDeviceName");
	sourceDevice=Vrui::findInputDevice(sourceDeviceName.c_str());
	if(sourceDevice==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Tracking source device %s not found",sourceDeviceName.c_str());
	}

int HIDPositionerCopy::getTrackType(void) const
	{
	/* Return the source device's track type: */
	return sourceDevice->getTrackType();
	}

void HIDPositionerCopy::updateDevice(InputDevice* device)
	{
	if(getInputGraphManager()->isEnabled(sourceDevice))
		{
		/* Copy the source device's tracking state: */
		device->copyTrackingState(sourceDevice);
		
		/* Project the copied tracking state with the UI manager if requested: */
		if(project)
			getUiManager()->projectDevice(device,device->getTransformation());
		
		getInputGraphManager()->enable(device);
		}
	else
		getInputGraphManager()->disable(device);
	}

}

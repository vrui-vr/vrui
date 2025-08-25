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

#include <Vrui/Internal/HIDPositioner.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <RawHID/EventDevice.h>
#include <Vrui/Internal/HIDPositionerCopy.h>
#include <Vrui/Internal/HIDPositionerPenPad.h>

namespace Vrui {

/******************************
Methods of class HIDPositioner:
******************************/

HIDPositioner* HIDPositioner::create(RawHID::EventDevice& hid,const Misc::ConfigurationFileSection& configFileSection,bool* ignoredFeatures)
	{
	/* Retrieve the type of positioner: */
	std::string type=configFileSection.retrieveString("./type");
	
	/* Create a positioner of the requested type: */
	if(type=="Copy")
		return new HIDPositionerCopy(hid,configFileSection);
	else if(type=="PenPad")
		return new HIDPositionerPenPad(hid,configFileSection,ignoredFeatures);
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid HID positioner type %s",type.c_str());
	}

HIDPositioner::HIDPositioner(RawHID::EventDevice& sHid)
	:hid(sHid),
	 project(false)
	{
	}

HIDPositioner::~HIDPositioner(void)
	{
	}

void HIDPositioner::setProject(bool newProject)
	{
	project=newProject;
	}

void HIDPositioner::prepareMainLoop(void)
	{
	}

}

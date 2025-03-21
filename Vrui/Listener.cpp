/***********************************************************************
Listener - Class for listeners/ sound observers in VR environments.
Copyright (c) 2008-2024 Oliver Kreylos

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

#include <string.h>
#include <stdio.h>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Vrui.h>

#include <Vrui/Listener.h>

namespace Vrui {

/*************************
Methods of class Listener:
*************************/

Listener::Listener(void)
	:listenerName(0),headDevice(0),
	 deviceHeadPosition(Point::origin),
	 deviceListenDirection(Vector(0,1,0)),
	 deviceUpDirection(Vector(0,0,1)),
	 gain(1)
	{
	}

Listener::~Listener(void)
	{
	delete[] listenerName;
	}

void Listener::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read the listener's name: */
	std::string name=configFileSection.retrieveString("./name",configFileSection.getName());
	listenerName=new char[name.size()+1];
	strcpy(listenerName,name.c_str());
	
	/* Determine whether the listener is head-tracked: */
	headTracked=configFileSection.retrieveValue("./headTracked",false);
	if(headTracked)
		{
		/* Retrieve head tracking device pointer: */
		headDevice=findInputDevice(configFileSection.retrieveString("./headDevice").c_str());
		if(headDevice==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Head device \"%s\" not found",configFileSection.retrieveString("./headDevice").c_str());
		
		/* Initialize the head device transformation: */
		headDeviceTransformation=headDevice->getTransformation();
		}
	else
		{
		/* Retrieve fixed head position/orientation: */
		headDeviceTransformation=configFileSection.retrieveValue<TrackerState>("./headDeviceTransformation");
		}
	
	/* Get head position and listening and up directions in head device coordinates: */
	configFileSection.updateValue("./headPosition",deviceHeadPosition);
	configFileSection.updateValue("./listenDirection",deviceListenDirection);
	deviceListenDirection.normalize();
	configFileSection.updateValue("./upDirection",deviceUpDirection);
	deviceUpDirection.normalize();
	
	/* Get listener's gain factor in dB: */
	Scalar dbGain=Math::log10(gain)*Scalar(10);
	dbGain=configFileSection.retrieveValue("./gain",dbGain);
	gain=dbGain>Scalar(-30)?Math::pow(Scalar(10),dbGain/Scalar(10)):Scalar(0);
	}

void Listener::update(void)
	{
	/* Update state if head tracking is enabled: */
	if(headTracked)
		{
		/* Update the head device transformation: */
		headDeviceTransformation=headDevice->getTransformation();
		}
	}

void Listener::setGain(Scalar newGain)
	{
	/* Set the listener's gain factor: */
	gain=newGain;
	}

}

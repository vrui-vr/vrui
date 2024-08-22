/***********************************************************************
VRBaseStation - Class describing a tracking base station (specifically a
Lighthouse-like base station or a Constellation camera) represented by a
VR device daemon.
Copyright (c) 2021 Oliver Kreylos

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

#include <Vrui/Internal/VRBaseStation.h>

#include <Misc/ArrayMarshallers.h>
#include <Misc/StringMarshaller.h>
#include <IO/File.h>
#include <Geometry/GeometryMarshallers.h>

namespace Vrui {

VRBaseStation::VRBaseStation(void)
	:tracking(false)
	{
	/* Initialize field-of-view extents and tracking range: */
	for(int i=0;i<4;++i)
		fov[i]=Scalar(0);
	for(int i=0;i<2;++i)
		range[i]=Scalar(0);
	}

VRBaseStation::VRBaseStation(const std::string& sSerialNumber)
	:serialNumber(sSerialNumber),
	 tracking(false)
	{
	/* Initialize field-of-view extents and tracking range: */
	for(int i=0;i<4;++i)
		fov[i]=Scalar(0);
	for(int i=0;i<2;++i)
		range[i]=Scalar(0);
	}

void VRBaseStation::setFov(int component,VRBaseStation::Scalar newValue)
	{
	/* Set field of view component: */
	fov[component]=newValue;
	}

void VRBaseStation::setRange(int component,VRBaseStation::Scalar newValue)
	{
	/* Set tracking range component: */
	range[component]=newValue;
	}

void VRBaseStation::setTracking(bool newTracking)
	{
	/* Set the tracking flag: */
	tracking=newTracking;
	}

void VRBaseStation::setPositionOrientation(const VRBaseStation::PositionOrientation& newPositionOrientation)
	{
	/* Set pose: */
	positionOrientation=newPositionOrientation;
	}

void VRBaseStation::write(IO::File& sink) const
	{
	/* Write the state components: */
	Misc::writeCppString(serialNumber,sink);
	Misc::FixedArrayMarshaller<Scalar>::write(fov,4,sink);
	Misc::FixedArrayMarshaller<Scalar>::write(range,2,sink);
	sink.write(tracking);
	if(tracking)
		Misc::Marshaller<PositionOrientation>::write(positionOrientation,sink);
	}

void VRBaseStation::read(IO::File& source)
	{
	/* Read the state components: */
	Misc::readCppString(source,serialNumber);
	Misc::FixedArrayMarshaller<Scalar>::read(fov,4,source);
	Misc::FixedArrayMarshaller<Scalar>::read(range,2,source);
	source.read(tracking);
	if(tracking)
		Misc::Marshaller<PositionOrientation>::read(source,positionOrientation);
	}

}

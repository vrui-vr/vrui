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

#ifndef VRUI_INTERNAL_VRBASESTATION_INCLUDED
#define VRUI_INTERNAL_VRBASESTATION_INCLUDED

#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/ArrayMarshallers.h>
#include <IO/File.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryMarshallers.h>

/* Forward declarations: */
namespace IO {
class File;
}

namespace Vrui {

class VRBaseStation
	{
	/* Embedded classes: */
	public:
	typedef Misc::Float32 Scalar; // Type for scalars sent over the network
	typedef Geometry::OrthonormalTransformation<Scalar,3> PositionOrientation; // Type for base station position/orientation
	
	/* Elements: */
	private:
	std::string serialNumber; // Base station's serial number or other unique identifier
	Scalar fov[4]; // Base station's tangent-space field of view (left, right, bottom, top)
	Scalar range[2]; // Base station's minimum and maximum tracking range in physical-space units
	bool tracking; // Flag whether the base station is currently participating in tracking devices
	PositionOrientation positionOrientation; // Base station's pose in physical space; only valid if tracking flag is true
	
	/* Constructors and destructors: */
	public:
	VRBaseStation(void); // Creates uninitialized base station
	VRBaseStation(const std::string& sSerialNumber); // Creates a base station with the given serial number
	
	/* Methods: */
	const std::string& getSerialNumber(void) const // Returns the base station's serial number
		{
		return serialNumber;
		}
	const Scalar* getFov(void) const // Returns the base station's field-of-view extents
		{
		return fov;
		}
	const Scalar* getRange(void) const // Returns the base station's tracking range
		{
		return range;
		}
	bool getTracking(void) const // Returns the tracking flag
		{
		return tracking;
		}
	const PositionOrientation& getPositionOrientation(void) const // Returns the base station's pose
		{
		return positionOrientation;
		}
	void setFov(int component,Scalar newValue); // Sets one component of the base station's field-of-view extents
	void setRange(int component,Scalar newValue); // Sets one component of the base station's tracking range
	void setTracking(bool newTracking); // Sets the tracking flag
	void setPositionOrientation(const PositionOrientation& newPositionOrientation); // Sets the base station's pose
	void write(IO::File& sink) const; // Writes base station state to given data sink
	void read(IO::File& source); // Reads base station state from given data source
	};

}

#endif

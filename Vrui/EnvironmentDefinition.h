/***********************************************************************
EnvironmentDefinition - Class defining a Vrui environment's physical
space.
Copyright (c) 2023 Oliver Kreylos

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

#ifndef VRUI_ENVIRONMENTDEFINITION_INCLUDED
#define VRUI_ENVIRONMENTDEFINITION_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Plane.h>
#include <Geometry/LinearUnit.h>
#include <Vrui/Types.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace IO {
class File;
}

namespace Vrui {

class EnvironmentDefinition
	{
	/* Embedded classes: */
	public:
	typedef Geometry::LinearUnit Unit; // Type for units of measurement
	typedef std::vector<Point> Polygon; // Type for non-convex planar polygons defined by a loop of vertices
	typedef std::vector<Polygon> PolygonList; // Type for lists of polygons
	
	/* Elements: */
	Unit unit; // Physical space's unit of measurement
	Vector up; // Vector pointing "up" in physical space
	Vector forward; // Vector pointing "forward" in physical space; not necessarily orthogonal to "up" vector or parallel to floor plane
	Point center; // Center point of physical space; typically at user's waist height above floor
	Scalar radius; // Radius of a sphere around the center point that contains the environment's usable space
	Plane floor; // Plane defining the floor; not necessarily orthogonal to "up" vector
	PolygonList boundary; // List of polygons defining the boundary of accessible physical space
	
	/* Constructors and destructors: */
	EnvironmentDefinition(void); // Creates a default environment definition
	
	/* Methods: */
	void configure(const Misc::ConfigurationFileSection& configFileSection); // Updates the environment definition from the given configuration file section
	void read(IO::File& file); // Updates the environment definition from a binary file or pipe
	void write(IO::File& file) const; // Writes the environment definition to a binary file or pipe
	Scalar getMeterFactor(void) const // Returns the length of a meter expressed in the physical space's unit of measurement
		{
		return unit.getMeterFactor();
		}
	Scalar getInchFactor(void) const // Returns the length of an inch expressed in the physical space's unit of measurement
		{
		return unit.getInchFactor();
		}
	Vector calcHorizontalForward(void) const; // Returns a unit-length vector orthogonal to the "up" direction that points in the same lateral direction as the "forward" vector
	Rotation calcStandardRotation(void) const; // Returns a rotation whose z axis is the "up" direction and whose y axis is the horizontal "forward" direction
	ONTransform calcStandardFrame(void) const; // Returns a transformation whose origin is the center point and whose rotation is the standard rotation
	Scalar calcFloorHeight(const Point& p) const; // Returns the height of the given point above the floor, measured along the "up" direction
	Point calcFloorPoint(const Point& p) const; // Returns a point on the floor directly underneath the given point
	};

}

#endif

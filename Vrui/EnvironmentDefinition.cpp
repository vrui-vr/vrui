/***********************************************************************
EnvironmentDefinition - Class defining a Vrui environment's physical
space.
Copyright (c) 2023-2024 Oliver Kreylos

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

#include <Vrui/EnvironmentDefinition.h>

#include <Misc/SizedTypes.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/Marshaller.h>
#include <Misc/StandardMarshallers.h>
#include <Misc/CompoundMarshallers.h>
#include <IO/File.h>
#include <Math/Math.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Geometry/GeometryMarshallers.h>

namespace Vrui {

/**************************************
Methods of class EnvironmentDefinition:
**************************************/

EnvironmentDefinition::EnvironmentDefinition(void)
	:unit(Unit::INCH,1),
	 up(0,0,1),forward(0,1,0),
	 center(0,0,0),radius(1),
	 floor(up,0)
	{
	}

void EnvironmentDefinition::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Update the physical space definition from the given configuration file section: */
	if(configFileSection.hasTag("unit"))
		{
		/* Read a linear unit definition: */
		configFileSection.updateValue("unit",unit);
		}
	else if(configFileSection.hasTag("meterScale"))
		{
		/* Set the linear unit to scaled meters: */
		unit=Unit(Unit::METER,Unit::Scalar(1)/configFileSection.retrieveValue<Unit::Scalar>("meterScale"));
		}
	else if(configFileSection.hasTag("inchScale"))
		{
		/* Set the linear unit to scaled inches: */
		unit=Unit(Unit::INCH,Unit::Scalar(1)/configFileSection.retrieveValue<Unit::Scalar>("inchScale"));
		}
	if(configFileSection.hasTag("up"))
		configFileSection.updateValue("up",up);
	else
		configFileSection.updateValue("upDirection",up);
	up.normalize();
	if(configFileSection.hasTag("forward"))
		configFileSection.updateValue("forward",forward);
	else
		configFileSection.updateValue("forwardDirection",forward);
	forward.normalize();
	if(configFileSection.hasTag("center"))
		configFileSection.updateValue("center",center);
	else
		configFileSection.updateValue("displayCenter",center);
	if(configFileSection.hasTag("radius"))
		configFileSection.updateValue("radius",radius);
	else
		configFileSection.updateValue("displaySize",radius);
	configFileSection.updateValue("floorPlane",floor);
	floor.normalize();
	
	/* Read boundary polygons: */
	if(configFileSection.hasTag("boundary"))
		configFileSection.updateValue("boundary",boundary);
	else
		configFileSection.updateValue("screenProtectorAreas",boundary);
	}

void EnvironmentDefinition::read(IO::File& file)
	{
	/* Read the environment definition: */
	unit.unit=Unit::Unit(file.read<Misc::UInt8>());
	file.read(unit.factor);
	Misc::read(file,up);
	Misc::read(file,forward);
	Misc::read(file,center);
	file.read(radius);
	Misc::read(file,floor);
	Misc::read(file,boundary);
	}

void EnvironmentDefinition::write(IO::File& file) const
	{
	/* Write the environment definition: */
	file.write(Misc::UInt8(unit.unit));
	file.write(unit.factor);
	Misc::write(up,file);
	Misc::write(forward,file);
	Misc::write(center,file);
	file.write(radius);
	Misc::write(floor,file);
	Misc::write(boundary,file);
	}

Vector EnvironmentDefinition::calcHorizontalForward(void) const
	{
	/* Orthogonalize the "forward" direction with respect to the "up" direction: */
	Vector result=forward;
	return result.orthogonalize(up).normalize();
	}

Rotation EnvironmentDefinition::calcStandardRotation(void) const
	{
	/* Calculate standard frame axes: */
	Vector x=forward^up;
	Vector y=up^x;
	
	/* Return a rotation with those axes: */
	return Rotation::fromBaseVectors(x,y);
	}

ONTransform EnvironmentDefinition::calcStandardFrame(void) const
	{
	/* Calculate standard frame axes: */
	Vector x=forward^up;
	Vector y=up^x;
	
	/* Return a transformation with those axes and origin at the center: */
	return ONTransform(center-Point::origin,Rotation::fromBaseVectors(x,y));
	}

Scalar EnvironmentDefinition::calcFloorHeight(const Point& p) const
	{
	/* Project the point onto the floor along the up direction: */
	return (p*floor.getNormal()-floor.getOffset())/(up*floor.getNormal());
	}

Point EnvironmentDefinition::calcFloorPoint(const Point& p) const
	{
	/* Project the point onto the floor along the up direction: */
	Scalar floorLambda=(floor.getOffset()-p*floor.getNormal())/(up*floor.getNormal());
	return Geometry::addScaled(p,up,floorLambda);
	}

}

/***********************************************************************
TouchscreenCalibrator - Base class to calibrate raw measurements from a
touchscreen device to rectified screen space.
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

#ifndef VRUI_INTERNAL_TOUCHSCREENCALIBRATOR_INCLUDED
#define VRUI_INTERNAL_TOUCHSCREENCALIBRATOR_INCLUDED

#include <utility>
#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Vrui/Types.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}

namespace Vrui {

class TouchscreenCalibrator
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Point<Scalar,2> Point; // Type for points in a screen plane
	typedef Geometry::Box<Scalar,2> Box; // Type for boxes in a screen plane
	
	struct TiePoint // Structure tying a raw touchscreen measurement to a rectified screen space position
		{
		/* Elements: */
		public:
		Point raw; // Raw touchscreen measurement
		Point screen; // Screen space position
		};
	
	/* Protected methods: */
	protected:
	static Point normalize(const Box& rawDomain,const Point& raw) // Normalizes the given raw point from the given raw domain to [0, 1]^2
		{
		return Point((raw[0]-rawDomain.min[0])/(rawDomain.max[0]-rawDomain.min[0]),(raw[1]-rawDomain.min[1])/(rawDomain.max[1]-rawDomain.min[1]));
		}
	
	/* Constructors and destructors: */
	public:
	static TouchscreenCalibrator* createCalibrator(const Misc::ConfigurationFileSection& configFileSection); // Returns a touchscreen calibrator from reading the given configuration file section
	virtual ~TouchscreenCalibrator(void);
	
	/* Methods: */
	virtual std::pair<Scalar,Scalar> getResiduals(const std::vector<TiePoint>& tiePoints) const; // Returns the L^2 and L^infinity residuals of approximating the given list of tie points
	virtual void writeConfig(Misc::ConfigurationFileSection& configFileSection) const =0; // Writes the calibrator's configuration to the given configuration file section
	virtual Point calibrate(const Point& raw) const =0; // Returns a calibrated rectified screen space position for the given raw touchscreen measurement
	};

}

#endif

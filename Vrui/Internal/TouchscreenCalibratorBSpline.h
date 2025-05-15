/***********************************************************************
TouchscreenCalibratorBSpline - Class to calibrate raw measurements
from a touchscreen device to rectified screen space using tensor-product
B-Splines.
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

#ifndef VRUI_INTERNAL_TOUCHSCREENCALIBRATORBSPLINE_INCLUDED
#define VRUI_INTERNAL_TOUCHSCREENCALIBRATORBSPLINE_INCLUDED

#include <Misc/Size.h>
#include <Vrui/Internal/TouchscreenCalibrator.h>

namespace Vrui {

class TouchscreenCalibratorBSpline:public TouchscreenCalibrator
	{
	/* Embedded classes: */
	public:
	typedef Misc::Size<2> Size; // Type for B-Spline degrees and mesh sizes
	
	/* Elements: */
	private:
	double rawScale[2],rawOffset[2]; // Scale and offset to transform from touchscreen measurement space to B-Spline space
	Size degree; // B-Spline degree
	Size size; // B-Spline mesh size
	double* coxDeBoor; // Cox-de Boor evaluation array
	Point* mesh; // Array of B-Spline mesh control points
	Point* xs; // X-direction B-Spline evaluation array
	Point* ys; // Y-direction B-Spline evaluation array
	
	/* Private methods: */
	double bspline(unsigned int i,unsigned int n,double t) const; // Returns the value of a B-Spline basis function
	
	/* Constructors and destructors: */
	public:
	TouchscreenCalibratorBSpline(const Size& sDegree,const Size& sSize,const Box& rawDomain,const std::vector<TiePoint>& tiePoints); // Creates a B-Spline calibrator with the given degree and mesh size from the given set of tie points from the given raw measurement domain
	TouchscreenCalibratorBSpline(const Misc::ConfigurationFileSection& configFileSection); // Creates a B-Spline calibrator from the given configuration file section
	virtual ~TouchscreenCalibratorBSpline(void);
	
	/* Methods from class TouchscreenCalibrator: */
	virtual void writeConfig(Misc::ConfigurationFileSection& configFileSection) const;
	virtual Point calibrate(const Point& raw) const;
	};

}

#endif

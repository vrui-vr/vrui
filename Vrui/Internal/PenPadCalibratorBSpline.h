/***********************************************************************
PenPadCalibratorBSpline - Class to calibrate a pen pad's position
using a tensor-product B-Spline.
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

#ifndef VRUI_INTERNAL_PENPADCALIBRATORBSPLINE_INCLUDED
#define VRUI_INTERNAL_PENPADCALIBRATORBSPLINE_INCLUDED

#include <Misc/Size.h>
#include <Vrui/Internal/PenPadCalibrator.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}

namespace Vrui {

class PenPadCalibratorBSpline:public PenPadCalibrator
	{
	/* Embedded classes: */
	public:
	typedef Misc::Size<2> Size; // Type for B-Spline degrees and mesh sizes
	
	/* Elements: */
	private:
	Size degree; // B-Spline degree
	Size size; // B-Spline mesh size
	Scalar rawScale[2],rawOffset[2]; // Scale and offset to transform from raw pen pad space to B-Spline space
	Point2* mesh; // Array of B-Spline mesh control points
	Scalar* coxDeBoor; // Cox-de Boor evaluation array
	Point2* xs; // X-direction B-Spline evaluation array
	Point2* ys; // Y-direction B-Spline evaluation array
	
	/* Private methods: */
	Scalar bspline(unsigned int i,unsigned int n,Scalar t) const; // Returns the value of a B-Spline basis function
	
	/* Constructors and destructors: */
	public:
	PenPadCalibratorBSpline(const Size& sDegree,const Size& sSize,const TiePointList& tiePoints,const Box2& rawDomain,Misc::ConfigurationFileSection& configFileSection); // Calculates a calibration from the given tie points and raw measurement domain and writes the result to the given configuration file section
	PenPadCalibratorBSpline(const Misc::ConfigurationFileSection& configFileSection,const Box2& rawDomain); // Creates a calibrator by reading from a configuration file section based on the given raw measurement domain
	virtual ~PenPadCalibratorBSpline(void);
	
	/* Methods from class PenPadCalibrator: */
	virtual Point2 calibrate(const Point2& raw) const;
	};

}

#endif

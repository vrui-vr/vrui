/***********************************************************************
TouchscreenCalibratorAffine - Class to calibrate raw measurements from a
touchscreen device to rectified screen space using an affine
transformation.
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

#ifndef VRUI_INTERNAL_TOUCHSCREENCALIBRATORAFFINE_INCLUDED
#define VRUI_INTERNAL_TOUCHSCREENCALIBRATORAFFINE_INCLUDED

#include <Geometry/AffineTransformation.h>
#include <Vrui/Internal/TouchscreenCalibrator.h>

namespace Vrui {

class TouchscreenCalibratorAffine:public TouchscreenCalibrator
	{
	/* Embedded classes: */
	private:
	typedef Geometry::AffineTransformation<Scalar,2> Transform; // Type for affine transformations
	
	/* Elements: */
	Transform transform; // Transformation from raw measurement space to rectified screen space
	
	/* Constructors and destructors: */
	public:
	TouchscreenCalibratorAffine(const Box& rawDomain,const std::vector<TiePoint>& tiePoints); // Creates an affine calibrator from the given set of tie points from the given raw measurement domain
	TouchscreenCalibratorAffine(const Misc::ConfigurationFileSection& configFileSection); // Creates an affine calibrator from the given configuration file section
	
	/* Methods from class TouchscreenCalibrator: */
	virtual void writeConfig(Misc::ConfigurationFileSection& configFileSection) const;
	virtual Point calibrate(const Point& raw) const;
	};

}

#endif

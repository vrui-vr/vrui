/***********************************************************************
TouchscreenCalibratorRectilinear - Class to calibrate raw measurements
from a touchscreen device to rectified screen space using a rectilinear
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

#ifndef VRUI_INTERNAL_TOUCHSCREENCALIBRATORRECTILINEAR_INCLUDED
#define VRUI_INTERNAL_TOUCHSCREENCALIBRATORRECTILINEAR_INCLUDED

#include <Vrui/Internal/TouchscreenCalibrator.h>

namespace Vrui {

class TouchscreenCalibratorRectilinear:public TouchscreenCalibrator
	{
	/* Elements: */
	private:
	Scalar scale[2],offset[2]; // Transformation coefficients from raw measurement space to rectified screen space
	
	/* Constructors and destructors: */
	public:
	TouchscreenCalibratorRectilinear(const Box& rawDomain,const std::vector<TiePoint>& tiePoints); // Creates a rectilinear calibrator from the given set of tie points from the given raw measurement domain
	TouchscreenCalibratorRectilinear(const Misc::ConfigurationFileSection& configFileSection); // Creates a rectilinear calibrator from the given configuration file section
	
	/* Methods from class TouchscreenCalibrator: */
	virtual void writeConfig(Misc::ConfigurationFileSection& configFileSection) const;
	virtual Point calibrate(const Point& raw) const;
	};

}

#endif

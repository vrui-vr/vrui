/***********************************************************************
PenPadCalibratorRectilinear - Class to calibrate a pen pad's position
using a rectilinear transformation.
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

#ifndef VRUI_INTERNAL_PENPADCALIBRATORRECTILINEAR_INCLUDED
#define VRUI_INTERNAL_PENPADCALIBRATORRECTILINEAR_INCLUDED

#include <Vrui/Internal/PenPadCalibrator.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}

namespace Vrui {

class PenPadCalibratorRectilinear:public PenPadCalibrator
	{
	/* Elements: */
	private:
	Scalar scale[2],offset[2]; // Transformation coefficients from raw measurement space to rectified normalized screen space
	
	/* Constructors and destructors: */
	public:
	PenPadCalibratorRectilinear(const TiePointList& tiePoints,const Box2& rawDomain,Misc::ConfigurationFileSection& configFileSection); // Calculates a calibration from the given tie points and raw measurement domain and writes the result to the given configuration file section
	PenPadCalibratorRectilinear(const Misc::ConfigurationFileSection& configFileSection,const Box2& rawDomain); // Creates a calibrator by reading from a configuration file section based on the given raw measurement domain
	
	/* Methods from class PenPadCalibrator: */
	virtual Point2 calibrate(const Point2& raw) const;
	};

}

#endif

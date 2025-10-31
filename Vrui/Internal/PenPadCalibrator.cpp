/***********************************************************************
PenPadCalibrator - Base class to calibrate a pen pad's position.
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

#include <Vrui/Internal/PenPadCalibrator.h>

#include <Math/Math.h>

namespace Vrui {

/*********************************
Methods of class PenPadCalibrator:
*********************************/

PenPadCalibrator::~PenPadCalibrator(void)
	{
	}

std::pair<Scalar,Scalar> PenPadCalibrator::calcResiduals(const TiePointList& tiePoints,const Scalar screenSize[2]) const
	{
	Scalar sqrDistSum(0);
	Scalar maxSqrDist(0);
	for(TiePointList::const_iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
		{
		/* Calibrate the raw measurement: */
		Point2 cal=calibrate(tpIt->raw);
		
		/* Convert the calibrated measurement and the ideal screen-space point to physical units: */
		Point2 uCal(cal[0]*screenSize[0],cal[1]*screenSize[1]);
		Point2 uScreen(tpIt->screen[0]*screenSize[0],tpIt->screen[1]*screenSize[1]);
		double sqrDist=Geometry::sqrDist(uCal,uScreen);
		sqrDistSum+=sqrDist;
		if(maxSqrDist<sqrDist)
			maxSqrDist=sqrDist;
		}
	
	return std::make_pair(Math::sqrt(sqrDistSum/Scalar(tiePoints.size())),Math::sqrt(maxSqrDist));
	}

}

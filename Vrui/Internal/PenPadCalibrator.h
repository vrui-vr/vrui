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

#ifndef VRUI_INTERNAL_PENPADCALIBRATOR_INCLUDED
#define VRUI_INTERNAL_PENPADCALIBRATOR_INCLUDED

#include <utility>
#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Vrui/Types.h>

namespace Vrui {

class PenPadCalibrator
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Point<Scalar,2> Point2; // Type for points in a screen plane
	typedef Geometry::Box<Scalar,2> Box2; // Type for boxes in a screen plane
	
	struct TiePoint // Structure tying a raw pen pad measurement to a rectified normalized screen space position
		{
		/* Elements: */
		public:
		Point2 raw; // Raw pen pad measurement
		Point2 screen; // Screen space position, normalized to [0, 1]^2
		};
	
	typedef std::vector<TiePoint> TiePointList; // Type for lists of tie points
	
	/* Constructors and destructors: */
	virtual ~PenPadCalibrator(void);
	
	/* Methods: */
	std::pair<Scalar,Scalar> calcResiduals(const TiePointList& tiePoints,const Scalar screenSize[2]) const; // Returns the L^2 and L^infinity residuals of the given tie point list
	virtual Point2 calibrate(const Point2& raw) const =0; // Returns a calibrated position in screen space from the given raw pen pad measurement and screen size array
	};

}

#endif

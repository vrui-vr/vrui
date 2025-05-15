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

#include <Vrui/Internal/TouchscreenCalibrator.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Vrui/Internal/TouchscreenCalibratorRectilinear.h>
#include <Vrui/Internal/TouchscreenCalibratorAffine.h>
#include <Vrui/Internal/TouchscreenCalibratorProjective.h>
#include <Vrui/Internal/TouchscreenCalibratorBSpline.h>

namespace Vrui {

/**************************************
Methods of class TouchscreenCalibrator:
**************************************/

TouchscreenCalibrator* TouchscreenCalibrator::createCalibrator(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read the calibrator type: */
	std::string type=configFileSection.retrieveString("./type");
	
	/* Create and return a new calibrator of the read type: */
	if(type=="Rectilinear")
		return new TouchscreenCalibratorRectilinear(configFileSection);
	else if(type=="Affine")
		return new TouchscreenCalibratorAffine(configFileSection);
	else if(type=="Projective")
		return new TouchscreenCalibratorProjective(configFileSection);
	else if(type=="BSpline")
		return new TouchscreenCalibratorBSpline(configFileSection);
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid touchscreen calibrator type %s",type.c_str());
	}

TouchscreenCalibrator::~TouchscreenCalibrator(void)
	{
	}

std::pair<Scalar,Scalar> TouchscreenCalibrator::getResiduals(const std::vector<TouchscreenCalibrator::TiePoint>& tiePoints) const
	{
	/* Calculate the calibration residual in rectified screen space: */
	Scalar rms2(0);
	Scalar linf2(0);
	for(std::vector<TiePoint>::const_iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
		{
		Scalar dist2=Geometry::sqrDist(calibrate(tpIt->raw),tpIt->screen);
		rms2+=dist2;
		linf2=Math::max(linf2,dist2);
		}
	return std::pair<Scalar,Scalar>(Math::sqrt(rms2/Scalar(tiePoints.size())),Math::sqrt(linf2));
	}

}

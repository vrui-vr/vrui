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

#include <Vrui/Internal/TouchscreenCalibratorAffine.h>

#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/GaussNewtonMinimizer.h>
#include <Geometry/PointAlignerATransform.h>
#include <Geometry/GeometryValueCoders.h>

namespace Vrui {

/********************************************
Methods of class TouchscreenCalibratorAffine:
********************************************/

TouchscreenCalibratorAffine::TouchscreenCalibratorAffine(const TouchscreenCalibrator::Box& rawDomain,const std::vector<TouchscreenCalibrator::TiePoint>& tiePoints)
	{
	/* Set up a point aligner based on an affine transformation: */
	typedef Geometry::PointAlignerATransform<double,2> Aligner;
	Aligner pointAligner;
	for(std::vector<TiePoint>::const_iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
		pointAligner.addPointPair(tpIt->raw,tpIt->screen);
	
	/* Estimate the initial calibration transformation: */
	pointAligner.condition();
	pointAligner.estimateTransform();
	
	/* Improve the calibration transformation with a few steps of Gauss-Newton iteration: */
	Math::GaussNewtonMinimizer<Aligner> minimizer(1000);
	minimizer.minimize(pointAligner);
	
	/* Retrieve the final calibration transformation: */
	transform=pointAligner.getTransform();
	}

TouchscreenCalibratorAffine::TouchscreenCalibratorAffine(const Misc::ConfigurationFileSection& configFileSection)
	{
	try
		{
		/* Read the transformation from the configuration file section: */
		transform=configFileSection.retrieveValue<Transform>("./transform");
		}
	catch(const std::runtime_error& err)
		{
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Can not initialize calibrator due to exception %s",err.what());
		}
	}

void TouchscreenCalibratorAffine::writeConfig(Misc::ConfigurationFileSection& configFileSection) const
	{
	try
		{
		/* Write the type of this calibrator: */
		configFileSection.storeString("./type","Affine");
		
		/* Write the transformation: */
		configFileSection.storeValue<Transform>("./transform",transform);
		}
	catch(const std::runtime_error& err)
		{
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Can not write configuration due to exception %s",err.what());
		}
	}

TouchscreenCalibrator::Point TouchscreenCalibratorAffine::calibrate(const TouchscreenCalibrator::Point& raw) const
	{
	return transform.transform(raw);
	}

}

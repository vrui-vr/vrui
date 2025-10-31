/***********************************************************************
PenPadCalibratorAffine - Class to calibrate a pen pad's position
using an affine transformation.
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

#include <Vrui/Internal/PenPadCalibratorAffine.h>

#include <Misc/StdError.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Matrix.h>
#include <Math/GaussNewtonMinimizer.h>
#include <Geometry/PointAlignerATransform.h>
#include <Geometry/GeometryValueCoders.h>

namespace Vrui {

/***************************************
Methods of class PenPadCalibratorAffine:
***************************************/

PenPadCalibratorAffine::PenPadCalibratorAffine(const PenPadCalibrator::TiePointList& tiePoints,const PenPadCalibrator::Box2& rawDomain,Misc::ConfigurationFileSection& configFileSection)
	{
	/* Create a normalization transformation: */
	Transform norm=Transform::identity;
	for(int i=0;i<2;++i)
		{
		norm.getMatrix()(i,i)=Scalar(1)/(rawDomain.max[i]-rawDomain.min[i]);
		norm.getMatrix()(i,2)=-rawDomain.min[i]/(rawDomain.max[i]-rawDomain.min[i]);
		}
	
	/* Set up an affine point aligner from normalized raw space: */
	typedef Geometry::PointAlignerATransform<double,2> Aligner;
	Aligner pointAligner;
	for(TiePointList::const_iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
		pointAligner.addPointPair(norm.transform(tpIt->raw),tpIt->screen);
	
	/* Estimate the initial calibration transformation: */
	pointAligner.condition();
	pointAligner.estimateTransform();
	
	/* Improve the calibration transformation with a few steps of Gauss-Newton iteration: */
	Math::GaussNewtonMinimizer<Aligner> minimizer(1000); // LOL
	minimizer.minimize(pointAligner);
	
	/* Retrieve the final calibration transformation: */
	transform=pointAligner.getTransform();
	
	/* Write the solution in normalized raw space: */
	try
		{
		/* Write the type of this calibrator: */
		configFileSection.storeString("./calibratorType","Affine");
		
		/* Write the transformation: */
		configFileSection.storeValue<Transform>("./transform",transform);
		}
	catch(const std::runtime_error& err)
		{
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Can not write configuration due to exception %s",err.what());
		}
	
	/* De-normalize the calibration: */
	transform*=norm;
	}

PenPadCalibratorAffine::PenPadCalibratorAffine(const Misc::ConfigurationFileSection& configFileSection,const PenPadCalibrator::Box2& rawDomain)
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
	
	/* Create a normalization transformation: */
	Transform norm=Transform::identity;
	for(int i=0;i<2;++i)
		{
		norm.getMatrix()(i,i)=Scalar(1)/(rawDomain.max[i]-rawDomain.min[i]);
		norm.getMatrix()(i,2)=-rawDomain.min[i]/(rawDomain.max[i]-rawDomain.min[i]);
		}
	
	/* De-normalize the calibration: */
	transform*=norm;
	}

PenPadCalibrator::Point2 PenPadCalibratorAffine::calibrate(const PenPadCalibrator::Point2& raw) const
	{
	return transform.transform(raw);
	}

}

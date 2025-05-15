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

#include <Vrui/Internal/TouchscreenCalibratorRectilinear.h>

#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/FixedArray.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Matrix.h>

namespace Vrui {

/*************************************************
Methods of class TouchscreenCalibratorRectilinear:
*************************************************/

TouchscreenCalibratorRectilinear::TouchscreenCalibratorRectilinear(const TouchscreenCalibrator::Box& rawDomain,const std::vector<TouchscreenCalibrator::TiePoint>& tiePoints)
	{
	/* Calculate a rectilinear calibration using two least-squares linear systems: */
	Math::Matrix atas[2];
	Math::Matrix atbs[2];
	for(int i=0;i<2;++i)
		{
		atas[i]=Math::Matrix(2,2,0.0);
		atbs[i]=Math::Matrix(2,1,0.0);
		}
	
	/* Enter all tie points into the least-squares systems: */
	for(std::vector<TiePoint>::const_iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
		{
		/* Normalize the raw measurement to condition the linear system: */
		Point nRaw=normalize(rawDomain,tpIt->raw);
		
		/* Enter the tie point pair into the least-squares systems: */
		for(int i=0;i<2;++i)
			{
			atas[i](0,0)+=nRaw[i]*nRaw[i];
			atas[i](0,1)+=nRaw[i];
			atas[i](1,0)+=nRaw[i];
			atas[i](1,1)+=1.0;
			atbs[i](0)+=nRaw[i]*tpIt->screen[i];
			atbs[i](1)+=tpIt->screen[i];
			}
		}
	
	/* Solve the least-squares systems: */
	for(int i=0;i<2;++i)
		{
		/* Solve the least-squares system: */
		Math::Matrix x=atbs[i];
		x.divideFullPivot(atas[i]);
		
		/* Transform the solution from normalized space to raw touchscreen measurement space: */
		scale[i]=Scalar(x(0))/(rawDomain.max[i]-rawDomain.min[i]);
		offset[i]=Scalar(x(1))-rawDomain.min[i]/(rawDomain.max[i]-rawDomain.min[i]);
		}
	}

TouchscreenCalibratorRectilinear::TouchscreenCalibratorRectilinear(const Misc::ConfigurationFileSection& configFileSection)
	{
	try
		{
		/* Read the transformation coefficients from the configuration file section: */
		typedef Misc::FixedArray<Scalar,2> Coefficients;
		configFileSection.retrieveValue<Coefficients>("./scale").writeElements(scale);
		configFileSection.retrieveValue<Coefficients>("./offset").writeElements(offset);
		}
	catch(const std::runtime_error& err)
		{
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Can not initialize calibrator due to exception %s",err.what());
		}
	}

void TouchscreenCalibratorRectilinear::writeConfig(Misc::ConfigurationFileSection& configFileSection) const
	{
	try
		{
		/* Write the type of this calibrator: */
		configFileSection.storeString("./type","Rectilinear");
		
		/* Write the transformation coefficients: */
		typedef Misc::FixedArray<Scalar,2> Coefficients;
		configFileSection.storeValue<Coefficients>("./scale",Coefficients(scale));
		configFileSection.storeValue<Coefficients>("./offset",Coefficients(offset));
		}
	catch(const std::runtime_error& err)
		{
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Can not write configuration due to exception %s",err.what());
		}
	}

TouchscreenCalibrator::Point TouchscreenCalibratorRectilinear::calibrate(const TouchscreenCalibrator::Point& raw) const
	{
	return Point(raw[0]*scale[0]+offset[0],raw[1]*scale[1]+offset[1]);
	}

}

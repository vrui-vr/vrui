/***********************************************************************
VblankEstimator - Helper class to estimate the next time at which a
vblank event will occur using a Kalman filter.
Copyright (c) 2023 Oliver Kreylos

This file is part of the Vrui VR Compositing Server (VRCompositor).

The Vrui VR Compositing Server is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Compositing Server is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Compositing Server; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "VblankEstimator.h"

#include <Math/Math.h>

/* Dump Kalman filter system state to file on every update to track down errors: */
#define DUMP_SYSTEMSTATE 0

#if DUMP_SYSTEMSTATE
#include <IO/File.h>
#include <IO/OpenFile.h>

const char* systemStateFileName="VblankEstimatorState.dat";
IO::FilePtr systemStateFile;
#endif

/********************************
Methods of class VblankEstimator:
********************************/

void VblankEstimator::start(const VblankEstimator::TimePoint& vblankTime,double frameRate)
	{
	/* Initialize the process noise to some small value: */
	Q00=Math::sqr(50.0);
	Q11=Math::sqr(20.0);
	
	/* Initialize the measurement noise: */
	R00=Math::sqr(1.5e5); // Empirically determined from estimation error standard deviation
	
	/* Initialize process state from the given estimates: */
	xk0=vblankTime;
	xk1=TimeVector(1.0/frameRate);
	
	/* Initialize the estimate covariance: */
	Pk[0][0]=Math::sqr(1.0e9/frameRate);
	Pk[0][1]=0.0;
	Pk[1][0]=0.0;
	Pk[1][1]=Math::sqr(1.0e9/(frameRate*10.0));
	
	#if DUMP_SYSTEMSTATE
	systemStateFile=IO::openFile(systemStateFileName,IO::File::WriteOnly);
	systemStateFile->setEndianness(Misc::LittleEndian);
	
	systemStateFile->write(long(xk0.tv_sec));
	systemStateFile->write(long(xk0.tv_nsec));
	systemStateFile->write(long(xk1.tv_sec));
	systemStateFile->write(long(xk1.tv_nsec));
	for(int i=0;i<2;++i)
		systemStateFile->write(Pk[i],2);
	#endif
	}

VblankEstimator::TimePoint VblankEstimator::predictNextVblankTime(void) const
	{
	/* Calculate the predicted (a-priori) state estimate: */
	TimePoint xkhat0=xk0;
	xkhat0+=xk1;
	
	return xkhat0;
	}

void VblankEstimator::update(void)
	{
	/* Calculate the predicted (a-priori) state estimate: */
	xk0+=xk1;
	
	/* Calculate the predicted (a-priori) estimate covariance: */
	Pk[0][0]+=Pk[0][1]+Pk[1][0]+Pk[1][1]+Q00;
	Pk[0][1]+=Pk[1][1];
	Pk[1][0]+=Pk[1][1];
	Pk[1][1]+=Q11;
	
	#if DUMP_SYSTEMSTATE
	systemStateFile->write(long(0));
	systemStateFile->write(long(0));
	systemStateFile->write(long(xk0.tv_sec));
	systemStateFile->write(long(xk0.tv_nsec));
	systemStateFile->write(long(xk1.tv_sec));
	systemStateFile->write(long(xk1.tv_nsec));
	for(int i=0;i<2;++i)
		systemStateFile->write(Pk[i],2);
	#endif
	}

VblankEstimator::TimeVector VblankEstimator::update(const VblankEstimator::TimePoint& vblankTime)
	{
	/* Calculate the predicted (a-priori) state estimate: */
	TimePoint xkhat0=xk0;
	xkhat0+=xk1;
	TimeVector xkhat1=xk1;
	
	/* Calculate the predicted (a-priori) estimate covariance: */
	double Pkhat[2][2];
	Pkhat[0][0]=Pk[0][0]+Pk[0][1]+Pk[1][0]+Pk[1][1]+Q00;
	Pkhat[0][1]=Pk[0][1]+Pk[1][1];
	Pkhat[1][0]=Pk[1][0]+Pk[1][1];
	Pkhat[1][1]=Pk[1][1]+Q11;
	
	/* Calculate the measurement pre-fit residual: */
	TimeVector ykhat=vblankTime-xkhat0;
	double ykhatd=double(long(ykhat.tv_sec)*1000000000L+ykhat.tv_nsec);
	
	/* Calculate the pre-fit residual covariance: */
	double Sk00=Pkhat[0][0]+R00;
	
	/* Calculate the optimal Kalman gain: */
	double Kk[2];
	Kk[0]=Pkhat[0][0]/Sk00;
	Kk[1]=Pkhat[1][0]/Sk00;
	
	/* Calculate the updated (a-posteriori) state estimate: */
	TimePoint oldXk0=xk0;
	xk0=xkhat0;
	xk0+=TimeVector(0,long(Kk[0]*ykhatd));
	xk1=xkhat1;
	xk1+=TimeVector(0,long(Kk[1]*ykhatd));
	
	/* Calculate the updated (a-posteriori) estimate covariance: */
	for(int i=0;i<2;++i)
		for(int j=0;j<2;++j)
			Pk[i][j]=Pkhat[i][j]-Kk[i]*Pkhat[0][j];
	
	#if DUMP_SYSTEMSTATE
	systemStateFile->write(long(vblankTime.tv_sec));
	systemStateFile->write(long(vblankTime.tv_nsec));
	systemStateFile->write(long(xk0.tv_sec));
	systemStateFile->write(long(xk0.tv_nsec));
	systemStateFile->write(long(xk1.tv_sec));
	systemStateFile->write(long(xk1.tv_nsec));
	for(int i=0;i<2;++i)
		systemStateFile->write(Pk[i],2);
	#endif
	
	/* Calculate and return the measurement post-fit residual: */
	return vblankTime-xk0;
	}

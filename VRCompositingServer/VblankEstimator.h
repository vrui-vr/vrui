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

#ifndef VBLANKESTIMATOR_INCLUDED
#define VBLANKESTIMATOR_INCLUDED

#include <Realtime/Time.h>
#include <Vrui/Types.h>

class VblankEstimator
	{
	/* Embedded classes: */
	public:
	typedef Vrui::TimePoint TimePoint; // Type for points in time
	typedef Vrui::TimeVector TimeVector; // Type for time intervals
	
	/* Elements: */
	private:
	double Q00,Q11; // Kalman filter process noise
	double R00; // Kalman filter measurement noise
	TimePoint xk0; // Current vblank time estimate
	TimeVector xk1; // Current frame interval estimate
	double Pk[2][2]; // Current estimate covariance
	
	/* Methods: */
	public:
	void start(const TimePoint& vblankTime,double frameRate); // Starts the vblank estimator from the given time estimate and the given framerate estimate in Hz
	const TimePoint& getVblankTime(void) const // Returns the current vblank time estimate
		{
		return xk0;
		}
	const TimeVector& getFrameInterval(void) const // Returns the current frame interval estimate
		{
		return xk1;
		}
	TimePoint predictNextVblankTime(void) const; // Returns a prediction at which time the next vblank event will occur
	void update(void); // Updates the vblank estimator without a vblank event time
	TimeVector update(const TimePoint& vblankTime); // Updates the vblank estimator with a vblank event at the given time; returns the measurement post-fit residual
	};

#endif

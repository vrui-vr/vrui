/***********************************************************************
ActState - Class encapsulating the traversal state of a scene graph
during action processing.
Copyright (c) 2025 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_ACTSTATE_INCLUDED
#define SCENEGRAPH_ACTSTATE_INCLUDED

#include <SceneGraph/TraversalState.h>

namespace SceneGraph {

class ActState:public TraversalState
	{
	/* Elements: */
	private:
	double lastTime; // Time point at which the previous action traversal took place in seconds
	double time; // Time point at which the current action traversal is taking place in seconds
	double deltaT; // Time difference between current and previous action traversal in seconds
	double defaultNextTime; // Time at which the next frame will be scheduled by default
	double nextTime; // Soonest time at which any traversed node requested another frame
	
	/* Constructors and destructors: */
	public:
	ActState(void); // Creates an uninitialized action state
	
	/* New methods: */
	
	/* Traversal preparation: */
	void setTimePoints(double newLastTime,double newTime,double newDeltaT,double newDefaultNextTime); // Sets the action time points for the current traversal
	void updateTime(double newTime,double newDefaultNextTime); // Updates the current traversal times; calculates time delta from previous traversal time
	
	/* Traversal methods: */
	double getLastTime(void) const // Returns the time point of the previous traversal
		{
		return lastTime;
		}
	double getTime(void) const // Returns the time point of the current traversal
		{
		return time;
		}
	double getDeltaT(void) const // Returns the time difference between the current and previous traversals
		{
		return deltaT;
		}
	void scheduleFrame(void) // Schedules another frame at the default time
		{
		if(nextTime>defaultNextTime)
			nextTime=defaultNextTime;
		}
	void scheduleFrame(double requestedNextTime) // Schedules another frame at the given time
		{
		if(nextTime>requestedNextTime)
			nextTime=requestedNextTime;
		}
	
	/* Traversal result collection: */
	bool requireFrame(void) const; // Returns true if any traversed nodes requested another frame
	double getNextTime(void) const // Returns the earliest time at which any traversed node requested another frame
		{
		return nextTime;
		}
	};

}

#endif

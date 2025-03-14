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

#include <SceneGraph/ActState.h>

#include <Math/Constants.h>

namespace SceneGraph {

/*************************
Methods of class ActState:
*************************/

ActState::ActState(void)
	:lastTime(0),time(0),deltaT(0),
	 defaultNextTime(0),nextTime(Math::Constants<double>::max)
	{
	}

void ActState::setTimePoints(double newLastTime,double newTime,double newDeltaT,double newDefaultNextTime)
	{
	/* Override the time points: */
	lastTime=newLastTime;
	time=newTime;
	deltaT=newDeltaT;
	defaultNextTime=newDefaultNextTime;
	nextTime=Math::Constants<double>::max;
	}

void ActState::updateTime(double newTime,double newDefaultNextTime)
	{
	/* Update the time points: */
	lastTime=time;
	time=newTime;
	deltaT=time-lastTime;
	defaultNextTime=newDefaultNextTime;
	nextTime=Math::Constants<double>::max;
	}

bool ActState::requireFrame(void) const
	{
	return nextTime<Math::Constants<double>::max;
	}

}

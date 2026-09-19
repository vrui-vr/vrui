/***********************************************************************
EventTypes - Basic data type declarations used by run loops and
associated classes.
Copyright (c) 2016-2026 Oliver Kreylos

This file is part of the Portable Threading Library (Threads).

The Portable Threading Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Portable Threading Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Threading Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef THREADS_EVENTTYPES_INCLUDED
#define THREADS_EVENTTYPES_INCLUDED

#include <Realtime/Time.h>
#include <Threads/Ownable.h>

/* Forward declarations: */
namespace Misc {
template <class TargetParam>
class Autopointer;
}
namespace Threads {
template <class ParameterParam>
class FunctionCall;
class RunLoop;
}

namespace Threads {

typedef Realtime::TimePointMonotonic EventTime; // Type for absolute time points
typedef Realtime::TimeVector EventInterval; // Type for time intervals

class EventSource:public Ownable // Base class for event sources like I/O watchers etc.
	{
	/* Elements: */
	protected:
	RunLoop& runLoop; // Reference to the run loop with which this event source is associated
	bool enabled; // Flag whether this event source is currently delivering events
	
	/* Constructors and destructors: */
	EventSource(RunLoop& sRunLoop) // Creates a disabled event source associated with the given run loop
		:runLoop(sRunLoop),enabled(false)
		{
		}
	
	/* Methods: */
	public:
	RunLoop& getRunLoop(void) // Returns the run loop with which this event source is associated
		{
		return runLoop;
		}
	bool isEnabled(void) const // Returns true if this event source is currently delivering events
		{
		return enabled;
		}
	};

typedef OwningPointer<EventSource> EventSourceOwner; // Type for ownership-establishing pointers to event sources
typedef Misc::Autopointer<EventSource> EventSourcePtr; // Type for non-ownership-establishing pointers to event sources

template <class EventSourceParam>
class Event // Base class for events delivered by event sources
	{
	/* Elements: */
	protected:
	EventSourceParam* source; // Pointer to the event source that delivered this event
	EventTime dispatchTime; // Time point at which this event was dispatched
	
	/* Constructors and destructors: */
	Event(EventSourceParam* sSource,const EventTime& sDispatchTime) // Elementwise constructor
		:source(sSource),dispatchTime(sDispatchTime)
		{
		}
	
	/* Methods: */
	public:
	EventSourceParam& getSource(void) // Returns the event source that delivered this event
		{
		return *source;
		}
	RunLoop& getRunLoop(void) // Returns the run loop with which the source that delivered this event is associated
		{
		return source->getRunLoop();
		}
	const EventTime& getDispatchTime(void) const // Returns the time point at which this event was dispatched
		{
		return dispatchTime;
		}
	};

}

#endif

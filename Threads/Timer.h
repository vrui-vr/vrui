/***********************************************************************
Timer - Class to handle one-short or recurring timer events in the
context of a run loop.
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

#ifndef THREADS_TIMER_INCLUDED
#define THREADS_TIMER_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/EventTypes.h>

namespace Threads {

class TimerEvent; // Forward declaration of descriptor class passed to timer event handlers
typedef Threads::FunctionCall<TimerEvent&> TimerEventHandler; // Type for timer event handlers

class Timer:public EventSource
	{
	friend class RunLoop;
	friend class TimerEvent;
	
	/* Elements: */
	private:
	EventTime timeout; // The next time point at which the timer elapses
	EventInterval interval; // Interval for recurring timers, or 0 if the timer is one-shot
	Misc::Autopointer<TimerEventHandler> eventHandler; // The event handler
	unsigned int activeIndex; // Index of an enabled timer's entry in the run loop's active timer heap
	
	/* Protected methods from class Ownable: */
	virtual void disowned(void);
	
	/* New private methods: */
	void enableInternal(void); // Enables this timer in its run loop
	void enablePM(void); // Enables this timer in response to a received pipe message
	void disableInternal(bool block); // Disables this timer in its run loop; if flag is true, blocks until it has actually been disabled
	void disablePM(void); // Disables this timer in response to a received pipe message
	void setTimeoutPM(const EventTime& newTimeout,bool reenable); // Sets this timer's timeout in response to a received pipe message
	void setIntervalPM(const EventInterval& newInterval); // Sets this timer's interval in response to a received pipe message
	void setEventHandlerPM(TimerEventHandler* newEventHandler); // Sets this timer's event handler in response to a received pipe message
	
	/* Constructors and destructors: */
	public:
	Timer(RunLoop& sRunLoop,const EventTime& sTimeout,TimerEventHandler& sEventHandler); // Creates an enabled one-shot timer
	Timer(RunLoop& sRunLoop,const EventTime& sTimeout,const EventInterval& sInterval,bool sEnabled,TimerEventHandler& sEventHandler); // Creates a recurring timer
	
	/* Methods: */
	const EventTime& getTimeout(void) const // Returns the next time point at which the timer expires
		{
		return timeout;
		}
	const EventInterval& getInterval(void) const // Returns the interval for recurring timers, or 0 if the timer is one-shot
		{
		return interval;
		}
	bool isRecurring(void) const // Returns true if the timer is a recurring timer
		{
		/* Recurring timers have non-zero intervals: */
		return interval.tv_sec!=0||interval.tv_nsec!=0;
		}
	void enable(void) // Enables the timer
		{
		/* Delegate to the internal method: */
		enableInternal();
		}
	void disable(void) // Disables the timer
		{
		/* Delegate to the internal method: */
		disableInternal(false);
		}
	void setEnabled(bool newEnabled) // Sets the timer's enabled state
		{
		/* Delegate to the internal methods: */
		if(newEnabled)
			enableInternal();
		else
			disableInternal(false);
		}
	void setTimeout(const EventTime& newTimeout,bool reenable =false); // Sets the next time point at which the timer expires; re-enables a disabled timer if the given flag is true
	void setInterval(const EventInterval& newInterval); // Sets the timer interval for a recurring timer; an interval of 0 turns the timer into a one-shot timer that will automatically be disabled when it elapses
	void setEventHandler(TimerEventHandler& newEventHandler); // Sets the timer's event handler
	};

typedef OwningPointer<Timer> TimerOwner; // Type for ownership-establishing pointers to timers
typedef Misc::Autopointer<Timer> TimerPtr; // Type for non-ownership-establishing pointers to timers

class TimerEvent:public Event<Timer> // Class for event descriptors passed to event handlers
	{
	friend class RunLoop;
	
	/* Elements: */
	private:
	EventTime scheduledTime; // Time at which the timer was scheduled to elapse
	
	/* Constructors and destructors: */
	TimerEvent(Timer* sTimer,const EventTime& sDispatchTime,const EventTime& sScheduledTime) // Creates an event for the given timer, dispatch time, and scheduled time-out
		:Event<Timer>(sTimer,sDispatchTime),
		 scheduledTime(sScheduledTime)
		{
		}
	
	/* Methods: */
	public:
	const EventTime& getScheduledTime(void) const // Returns the time point at which the timer was scheduled to elapse
		{
		return scheduledTime;
		}
	};

}

#endif

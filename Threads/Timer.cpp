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

#include <Threads/Timer.h>

#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <Threads/RunLoopInternal.h>

namespace Threads {

/**********************
Methods of class Timer:
**********************/

void Timer::disowned(void)
	{
	/* Disable this timer synchronously, thus dropping all references to it and not sending further events: */
	disableInternal(true);
	}

void Timer::enableInternal(void)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this timer is not already enabled and still has an owner: */
		if(!enabled&&isOwned())
			{
			/* Insert this timer into the active timers heap: */
			runLoop.insertActiveTimer(this,timeout);
			ref(); // Take a reference to this timer for the active timers heap
			
			/* Mark this timer as enabled: */
			enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::EnableTimer);
		pm.enableTimer.timer=this;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void Timer::enablePM(void)
	{
	/* Check that this timer is not already enabled and still has an owner: */
	if(!enabled&&isOwned())
		{
		/* Insert this timer into the active timers heap: */
		runLoop.insertActiveTimer(this,timeout);
		ref(); // Take a reference to this timer for the active timers heap
		
		/* Mark this timer as enabled: */
		enabled=true;
		}
	
	/* Drop the message's reference to this timer: */
	unref();
	}

void Timer::disableInternal(bool block)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this timer is not already disabled: */
		if(enabled)
			{
			/* Remove this timer from the active timers heap by moving the last active timer in the heap to its heap slot: */
			Timer* lastTimer=runLoop.activeTimers[runLoop.activeTimers.size()-1].timer;
			runLoop.activeTimers.pop_back();
			if(lastTimer!=this)
				{
				/* Let the last timer take over this timer's heap slot and then fix the heap: */
				lastTimer->activeIndex=activeIndex;
				runLoop.updateActiveTimer(lastTimer);
				}
			
			/* Mark this timer as disabled: */
			enabled=false;
			
			/* Drop the active timers heap's reference to this timer: */
			unref();
			}
		}
	else
		{
		/* Create a pipe message: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::DisableTimer);
		pm.disableTimer.timer=this;
		
		/* Check if we need to block until the pipe message has been processed: */
		if(block)
			{
			/*******************************************************************
			Make a synchronous request by creating and locking a temporary
			condition variable, then writing to the self-pipe, then waiting on
			the condition variable:
			*******************************************************************/
			
			/* Create a temporary condition variable and put it into the pipe message: */
			RunLoop::TempCond tempCond;
			pm.disableTimer.cond=&tempCond;
			
			/* Write to the self-pipe, but only wait on the condition variable if the run loop is still running: */
			if(runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this))
				tempCond.wait();
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableTimer.cond=0;
			runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
			}
		}
	}

void Timer::disablePM(void)
	{
	/* Check that this timer is not already disabled: */
	if(enabled)
		{
		/* Remove this timer from the active timers heap by moving the last active timer in the heap to its heap slot: */
		Timer* lastTimer=runLoop.activeTimers[runLoop.activeTimers.size()-1].timer;
		runLoop.activeTimers.pop_back();
		if(lastTimer!=this)
			{
			/* Let the last timer take over this timer's heap slot and then fix the heap: */
			lastTimer->activeIndex=activeIndex;
			runLoop.updateActiveTimer(lastTimer);
			}
		
		/* Drop the active timers heap's reference to this timer: */
		unref();
		
		/* Mark this timer as disabled: */
		enabled=false;
		}
	
	/* Drop the message's reference to this timer: */
	unref();
	}

void Timer::setTimeoutPM(const EventTime& newTimeout,bool reenable)
	{
	/* Set this timer's time-out: */
	timeout=newTimeout;
	
	/* If this timer is enabled, fix the active timer heap, otherwise, if requested and this timer still has an owner, re-enable it: */
	if(enabled)
		runLoop.updateActiveTimer(this);
	else if(reenable&&isOwned())
		{
		/* Insert this timer into the active timers heap: */
		runLoop.insertActiveTimer(this,timeout);
		ref(); // Take a reference to this timer for the active timers heap
		
		/* Mark this timer as enabled: */
		enabled=true;
		}
	
	/* Drop the message's reference to this timer: */
	unref();
	}

void Timer::setIntervalPM(const EventInterval& newInterval)
	{
	/* Set this timer's interval: */
	interval=newInterval;
	
	/* Drop the message's reference to this timer: */
	unref();
	}

void Timer::setEventHandlerPM(TimerEventHandler* newEventHandler)
	{
	/* Replace this timer's event handler: */
	eventHandler=newEventHandler;
	}

Timer::Timer(RunLoop& sRunLoop,const EventTime& sTimeout,TimerEventHandler& sEventHandler)
	:EventSource(sRunLoop),
	 timeout(sTimeout),interval(0,0),eventHandler(&sEventHandler)
	{
	/* Enable this timer immediately: */
	enableInternal();
	}

Timer::Timer(RunLoop& sRunLoop,const EventTime& sTimeout,const EventInterval& sInterval,bool sEnabled,TimerEventHandler& sEventHandler)
	:EventSource(sRunLoop),
	 timeout(sTimeout),interval(sInterval),eventHandler(&sEventHandler)
	{
	/* If the enabled flag is set, enable this timer immediately: */
	if(sEnabled)
		enableInternal();
	}

void Timer::setTimeout(const EventTime& newTimeout,bool reenable)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Set this timer's timeout: */
		timeout=newTimeout;
		
		/* If this timer is enabled, fix the active timer heap; otherwise, if requested and this timer still has an owner, re-enable it: */
		if(enabled)
			runLoop.updateActiveTimer(this);
		else if(reenable&&isOwned())
			{
			/* Insert this timer into the active timers heap: */
			runLoop.insertActiveTimer(this,timeout);
			ref(); // Take a reference to this timer for the active timers heap
			
			/* Mark this timer as enabled: */
			enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(reenable?RunLoop::PipeMessage::SetTimerTimeoutReenable:RunLoop::PipeMessage::SetTimerTimeout);
		pm.setTimerTimeout.timer=this;
		pm.setTimerTimeout.timeout=newTimeout;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void Timer::setInterval(const EventInterval& newInterval)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Set this timer's interval: */
		interval=newInterval;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetTimerInterval);
		pm.setTimerInterval.timer=this;
		pm.setTimerInterval.interval=newInterval;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void Timer::setEventHandler(TimerEventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Replace this timer's event handler: */
		eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetTimerEventHandler);
		pm.setTimerEventHandler.timer=this;
		pm.setTimerEventHandler.eventHandler=&newEventHandler;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this,&newEventHandler);
		}
	}

}

/***********************************************************************
UserSignal - Class for user-defined signals to synchronously notify
clients of asynchronous events.
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

#include <Threads/UserSignal.h>

#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <Threads/RunLoopInternal.h>

namespace Threads {

/***************************
Methods of class UserSignal:
***************************/

void UserSignal::disowned(void)
	{
	/* Disable this user signal handler synchronously, thus dropping all references to it and not sending further events: */
	disableInternal(true);
	}

void UserSignal::enableInternal(void)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this user signal is not already enabled and still has an owner: */
		if(!enabled&&isOwned())
			{
			/* Enable this user signal: */
			enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::EnableUserSignal);
		pm.enableUserSignal.userSignal=this;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void UserSignal::enablePM(void)
	{
	/* Check that this user signal is not already enabled and still has an owner: */
	if(!enabled&&isOwned())
		{
		/* Enable this user signal: */
		enabled=true;
		}
	}

void UserSignal::disableInternal(bool block)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Disable this user signal: */
		enabled=false;
		}
	else
		{
		/* Create a pipe message: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::DisableUserSignal);
		pm.disableUserSignal.userSignal=this;
		
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
			pm.disableUserSignal.cond=&tempCond;
			
			/* Write to the self-pipe, but only wait on the condition variable if the run loop is still running: */
			if(runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this))
				tempCond.wait();
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableUserSignal.cond=0;
			runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
			}
		}
	}

void UserSignal::disablePM(void)
	{
	/* Disable this user signal: */
	enabled=false;
	}

void UserSignal::setEventHandlerPM(UserSignalEventHandler* newEventHandler)
	{
	/* Replace this user signal's event handler: */
	eventHandler=newEventHandler;
	}

void UserSignal::signalInternal(RefCounted* signalData)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check if this user signal is enabled: */
		if(enabled)
			{
			/* Call this user signal's event handler directly: */
			UserSignalEvent event(this,runLoop.lastDispatchTime,signalData);
			(*eventHandler)(event);
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SignalUserSignal);
		pm.signalUserSignal.userSignal=this;
		pm.signalUserSignal.signalData=signalData;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this,signalData);
		}
	}

void UserSignal::signalPM(RefCounted* signalData)
	{
	/* Check if this user signal is enabled: */
	if(enabled)
		{
		/* Call this user signal's event handler: */
		UserSignalEvent event(this,runLoop.lastDispatchTime,signalData);
		(*eventHandler)(event);
		}
	}

UserSignal::UserSignal(RunLoop& sRunLoop,bool sEnabled,UserSignalEventHandler& sEventHandler)
	:EventSource(sRunLoop),
	 eventHandler(&sEventHandler)
	{
	/* If the enabled flag is set, enable this user signal immediately: */
	if(sEnabled)
		enableInternal();
	}

void UserSignal::setEventHandler(UserSignalEventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Replace this user signal's event handler: */
		eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetUserSignalEventHandler);
		pm.setUserSignalEventHandler.userSignal=this;
		pm.setUserSignalEventHandler.eventHandler=&newEventHandler;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this,&newEventHandler);
		}
	}

}

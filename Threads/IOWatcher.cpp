/***********************************************************************
IOWatcher - Class to watch for input/output events on operating system
file descriptors in the context of a run loop.
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

#include <Threads/IOWatcher.h>

#include <poll.h>
#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <Threads/RunLoopInternal.h>

namespace Threads {

/**************************
Methods of class IOWatcher:
**************************/

void IOWatcher::disowned(void)
	{
	/* Disable this I/O watcher synchronously, thus dropping all references to it and not sending further events: */
	disableInternal(true);
	}

void IOWatcher::enableInternal(void)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this I/O watcher is not already enabled and still has an owner: */
		if(!enabled&&isOwned())
			{
			/* Append an entry for this I/O watcher to the end of the active list: */
			runLoop.activeIOWatchers.push_back(RunLoop::ActiveIOWatcher(this));
			ref(); // Take a reference to this I/O watcher for the active list
			
			/* Append a poll request for this I/O watcher to the end of the poll request list: */
			struct pollfd pollFd;
			pollFd.fd=fd;
			pollFd.events=eventMaskToPollEvents(eventMask);
			pollFd.revents=0x0;
			runLoop.pollFds.push_back(pollFd);
			
			/* Set this I/O watcher's position in the active list and increase the number of active I/O watchers: */
			activeIndex=runLoop.numActiveIOWatchers;
			++runLoop.numActiveIOWatchers;
			
			/* Mark this I/O watcher as enabled: */
			enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::EnableIOWatcher);
		pm.enableIOWatcher.ioWatcher=this;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void IOWatcher::enablePM(void)
	{
	/* Check that this I/O watcher is not already enabled and still has an owner: */
	if(!enabled&&isOwned())
		{
		/* Append an entry for this I/O watcher to the end of the active list: */
		runLoop.activeIOWatchers.push_back(RunLoop::ActiveIOWatcher(this));
		ref(); // Take a reference to this I/O watcher for the active list
		
		/* Append a poll request for this I/O watcher to the end of the poll request list: */
		struct pollfd pollFd;
		pollFd.fd=fd;
		pollFd.events=eventMaskToPollEvents(eventMask);
		pollFd.revents=0x0;
		runLoop.pollFds.push_back(pollFd);
		
		/* Set this I/O watcher's position in the active list and increase the number of active I/O watchers: */
		activeIndex=runLoop.numActiveIOWatchers;
		++runLoop.numActiveIOWatchers;
		
		/* Mark this I/O watcher as enabled: */
		enabled=true;
		}
	}

void IOWatcher::disableInternal(bool block)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this I/O watcher is not already disabled: */
		if(enabled)
			{
			/*****************************************************************
			Shuffle the active I/O watchers in the list such that the one that
			actually has to be removed is the last entry. This guarantees that
			disabling an I/O watcher is an O(1) operation.
			We have to take extra care if the one to be removed is before the
			one that is currently handled by event dispatching, to avoid
			missing events for the watcher that is currently the last entry.
			*****************************************************************/
			
			/* Check if the run loop is currently handling I/O watchers and the to-be-disabled one is in the list not after the currently-handled one: */
			unsigned int aiowi=activeIndex; // List index of the to-be-disabled I/O watcher
			unsigned int hiowi=runLoop.handledIOWatcherIndex; // List index of the currently handled I/O watcher
			if(runLoop.handlingIOWatchers&&aiowi<=hiowi)
				{
				/* Move the currently-handled I/O watcher to the place of the to-be-deleted one: */
				runLoop.activeIOWatchers[aiowi]=runLoop.activeIOWatchers[hiowi];
				runLoop.pollFds[aiowi+1]=runLoop.pollFds[hiowi+1]; // Take account of the extra self-pipe entry at the head of the list
				runLoop.activeIOWatchers[aiowi].ioWatcher->activeIndex=aiowi;
				
				/* Move the last I/O watcher in the lists to the place of the currently-handled one: */
				runLoop.activeIOWatchers[hiowi]=runLoop.activeIOWatchers[runLoop.numActiveIOWatchers-1];
				runLoop.pollFds[hiowi+1]=runLoop.pollFds[runLoop.numActiveIOWatchers]; // Take account of the extra self-pipe entry at the head of the list
				runLoop.activeIOWatchers[hiowi].ioWatcher->activeIndex=hiowi;
				
				/* Reduce the currently-handled index to handle the I/O watcher that used to be the last in the lists next: */
				--runLoop.handledIOWatcherIndex;
				}
			else
				{
				/* Move the last I/O watcher in the lists to the place of the to-be-disabled one: */
				runLoop.activeIOWatchers[aiowi]=runLoop.activeIOWatchers[runLoop.numActiveIOWatchers-1];
				runLoop.pollFds[aiowi+1]=runLoop.pollFds[runLoop.numActiveIOWatchers]; // Take account of the extra self-pipe entry at the head of the list
				runLoop.activeIOWatchers[aiowi].ioWatcher->activeIndex=aiowi;
				}
			
			/* Remove the now unused last entries from the active I/O watcher lists: */
			runLoop.activeIOWatchers.pop_back();
			runLoop.pollFds.pop_back();
			--runLoop.numActiveIOWatchers;
			
			/* Mark this I/O watcher as disabled: */
			enabled=false;
			
			/* Drop the active I/O watcher list's reference to this I/O watcher: */
			unref();
			}
		}
	else
		{
		/* Create a pipe message: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::DisableIOWatcher);
		pm.disableIOWatcher.ioWatcher=this;
		
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
			pm.disableIOWatcher.cond=&tempCond;
			
			/* Write to the self-pipe, but only wait on the condition variable if the run loop is still running: */
			if(runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this))
				tempCond.wait();
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableIOWatcher.cond=0;
			runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
			}
		}
	}

void IOWatcher::disablePM(void)
	{
	/* Check that this I/O watcher is not already disabled: */
	if(enabled)
		{
		/* Replace this I/O watcher's entries in the active I/O watchers list and the poll request list with the last entries in the respective lists, then drop the last entries: */
		unsigned int aiowi=activeIndex;
		runLoop.activeIOWatchers[aiowi]=runLoop.activeIOWatchers[runLoop.numActiveIOWatchers-1];
		runLoop.activeIOWatchers[aiowi].ioWatcher->activeIndex=aiowi;
		runLoop.activeIOWatchers.pop_back();
		runLoop.pollFds[aiowi+1]=runLoop.pollFds[runLoop.numActiveIOWatchers]; // Take account of the extra self-pipe entry at the head of the list
		runLoop.pollFds.pop_back();
		--runLoop.numActiveIOWatchers;
		
		/* Mark this I/O watcher as disabled: */
		enabled=false;
		
		/* Drop the active I/O watcher list's reference to this I/O watcher: */
		unref();
		}
	}

void IOWatcher::setEventMaskPM(unsigned int newEventMask)
	{
	/* If this I/O watcher is enabled, update its poll request: */
	if(enabled)
		{
		/* Update the I/O watcher's poll request: */
		runLoop.pollFds[activeIndex+1].events=eventMaskToPollEvents(newEventMask);
		}
	
	/* Update this I/O watcher's event mask: */
	eventMask=newEventMask;
	}

void IOWatcher::setEventHandlerPM(IOWatcherEventHandler* newEventHandler)
	{
	/* Replace this I/O watcher's event handler: */
	eventHandler=newEventHandler;
	}

IOWatcher::IOWatcher(RunLoop& sRunLoop,int sFd,unsigned int sEventMask,bool sEnabled,IOWatcherEventHandler& sEventHandler)
	:EventSource(sRunLoop),
	 fd(sFd),eventMask(sEventMask),eventHandler(&sEventHandler)
	{
	/* If the enabled flag is set, enable this I/O watcher immediately: */
	if(sEnabled)
		enableInternal();
	}

int IOWatcher::eventMaskToPollEvents(unsigned int eventMask)
	{
	/* Compile-time check if the event mask bits exposed at the interface match the POLL* macros defined in poll.h: */
	#if POLLIN==0x1&&POLLPRI==0x2&&POLLOUT==0x4&&POLLERR==0x8&&POLLHUP==0x10&&POLLNVAL==0x20
	
	/* Return the event mask directly: */
	return eventMask;
	
	#else
	
	/* Assemble the poll event set bit-by-bit: */
	int events=0x0;
	if(eventMask&IOWatcher::Read)
		events|=POLLIN;
	if(eventMask&IOWatcher::Exception)
		events|=POLLPRI;
	if(eventMask&IOWatcher::Write)
		events|=POLLOUT;
	if(eventMask&IOWatcher::Error)
		events|=POLLERR;
	if(eventMask&IOWatcher::HangUp)
		events|=POLLHUP;
	if(eventMask&IOWatcher::Invalid)
		events|=POLLNVAL;
	
	return events;
	#endif
	}

unsigned int IOWatcher::pollEventsToEventMask(int events)
	{
	/* Compile-time check if the event mask bits exposed at the interface match the POLL* macros defined in poll.h: */
	#if POLLIN==0x1&&POLLPRI==0x2&&POLLOUT==0x4&&POLLERR==0x8&&POLLHUP==0x10&&POLLNVAL==0x20
	
	/* Return the poll revent set directly: */
	return events;
	
	#else
	
	/* Assemble the event mask bit-by-bit: */
	unsigned int result=0x0U;
	if((events&POLLIN)!=0x0)
		result|=IOWatcher::Read;
	if((events&POLLPRI)!=0x0)
		result|=IOWatcher::Exception;
	if((events&POLLOUT)!=0x0)
		result|=IOWatcher::Write;
	if((events&POLLERR)!=0x0)
		result|=IOWatcher::Error;
	if((events&POLLHUP)!=0x0)
		result|=IOWatcher::HangUp;
	if((events&POLLNVAL)!=0x0)
		result|=IOWatcher::Invalid;
	
	return result;
	
	#endif
	}

void IOWatcher::setEventMask(unsigned int newEventMask)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* If this I/O watcher is currently enabled, update its active poll request: */
		if(enabled)
			runLoop.pollFds[activeIndex+1].events=eventMaskToPollEvents(newEventMask);
		
		/* Update this I/O watcher's event mask: */
		eventMask=newEventMask;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetIOWatcherEventMask);
		pm.setIOWatcherEventMask.ioWatcher=this;
		pm.setIOWatcherEventMask.newEventMask=newEventMask;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void IOWatcher::setEventHandler(IOWatcherEventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Replace this I/O watcher's event handler: */
		eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetIOWatcherEventHandler);
		pm.setIOWatcherEventHandler.ioWatcher=this;
		pm.setIOWatcherEventHandler.eventHandler=&newEventHandler;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this,&newEventHandler);
		}
	}

}

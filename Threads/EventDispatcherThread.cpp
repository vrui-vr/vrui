/***********************************************************************
EventDispatcherThread - Class to run an event dispatcher in its own
background thread.
Copyright (c) 2023 Oliver Kreylos

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

#include <Threads/EventDispatcherThread.h>

namespace Threads {

/**************************************
Methods of class EventDispatcherThread:
**************************************/

void* EventDispatcherThread::threadMethod(void)
	{
	/* Dispatch events until shut down: */
	dispatchEvents();
	
	return 0;
	}

EventDispatcherThread::EventDispatcherThread(bool startThread)
	{
	/* Start the event dispatcher thread if requested: */
	if(startThread)
		thread.start(this,&EventDispatcherThread::threadMethod);
	}

EventDispatcherThread::~EventDispatcherThread(void)
	{
	/* Stop the event dispatcher thread: */
	stopThread();
	}

void EventDispatcherThread::startThread(void)
	{
	/* Start the event dispatcher thread if it is not already running: */
	if(thread.isJoined())
		thread.start(this,&EventDispatcherThread::threadMethod);
	}

void EventDispatcherThread::stopThread(void)
	{
	/* Check if the event dispatcher thread is running: */
	if(!thread.isJoined())
		{
		/* Shut down the event dispatcher: */
		stop();
		
		/* Stop the event dispatcher thread: */
		thread.join();
		}
	}

}

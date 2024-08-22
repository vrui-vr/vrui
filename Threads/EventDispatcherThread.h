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

#ifndef THREADS_EVENTDISPATCHERTHREAD_INCLUDED
#define THREADS_EVENTDISPATCHERTHREAD_INCLUDED

#include <Threads/Thread.h>
#include <Threads/EventDispatcher.h>

namespace Threads {

class EventDispatcherThread:public EventDispatcher
	{
	/* Elements: */
	private:
	Thread thread; // Thread running the event dispatcher
	
	/* Private methods: */
	void* threadMethod(void); // Method running the event dispatcher thread
	
	/* Constructors and destructors: */
	public:
	EventDispatcherThread(bool startThread =true); // Creates an event dispatcher; immediately starts the dispatching thread if given flag is true
	~EventDispatcherThread(void); // Shuts down the event dispatcher and its thread
	
	/* Methods: */
	void startThread(void); // Starts running the event dispatcher in its own background thread; does nothing if thread is already running
	void stopThread(void); // Stops running the event dispatcher in its own background thread; does nothing if thread is not running
	};

}

#endif

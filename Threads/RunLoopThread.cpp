/***********************************************************************
RunLoopThread - Class that executes a run loop in a new background
thread.
Copyright (c) 2023-2026 Oliver Kreylos

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

#include <Threads/RunLoopThread.h>

namespace Threads {

/******************************
Methods of class RunLoopThread:
******************************/

void* RunLoopThread::threadMethod(void)
	{
	/* Run the run loop until it is shut down: */
	while(dispatchNextEvents())
		;
	
	/* Shut down the run loop: */
	shutdown();
	
	return 0;
	}

RunLoopThread::RunLoopThread(void)
	{
	/* Start the background thread: */
	thread.start(this,&RunLoopThread::threadMethod);
	
	/* Associate the run loop with the background thread: */
	threadId=thread.getLocalId();
	}

RunLoopThread::~RunLoopThread(void)
	{
	/* Stop the run loop and collect the background thread: */
	stop();
	thread.join();
	}

}

/***********************************************************************
WorkerBarrier - Class that allows a single supervisor to wait for the
completion of a dynamic number of jobs executed by any number of
workers.
Copyright (c) 2026 Oliver Kreylos

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

#ifndef THREADS_WORKERBARRIER_INCLUDED
#define THREADS_WORKERBARRIER_INCLUDED

#include <Threads/Mutex.h>
#include <Threads/Cond.h>

namespace Threads {

class WorkerBarrier
	{
	/* Elements: */
	private:
	Mutex mutex; // A mutex protecting the number of pending jobs and the completion condition variable
	unsigned int numPendingJobs; // The number of started jobs that have not finished yet
	Cond completionCond; // Condition variable that signals when all started jobs have been completed
	
	/* Constructors and destructors: */
	public:
	WorkerBarrier(void) // Creates an idle worker barrier
		:numPendingJobs(0)
		{
		}
	
	/* Methods: */
	void startJobs(unsigned int numJobs) // Starts the given number of jobs; must be called before any workers actually start working
		{
		/* Atomically add to the number of pending jobs: */
		Mutex::Lock lock(mutex);
		numPendingJobs+=numJobs;
		}
	void completeJob(void) // Completes a job; must be called after the worker actually finishes its work, and after the worker starts any continuation jobs
		{
		/* Lock this object's state: */
		Mutex::Lock lock(mutex);
		
		/* Atomically decrement the number of pending jobs and check if all have been completed: */
		if(--numPendingJobs==0)
			{
			/* Signal completion of all jobs to a potentially blocked supervisor: */
			completionCond.signal();
			}
		}
	void wait(void) // Blocks the calling supervisor until all started jobs have been completed
		{
		/* Lock this object's state and keep blocking until all started jobs have been completed: */
		Mutex::Lock lock(mutex);
		while(numPendingJobs!=0)
			completionCond.wait(mutex);
		}
	};

}

#endif

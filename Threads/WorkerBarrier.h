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

#include <Threads/MutexCond.h>

namespace Threads {

class WorkerBarrier
	{
	/* Elements: */
	private:
	volatile unsigned int numStartedJobs; // The total number of started jobs
	volatile unsigned int numCompletedJobs; // The total number of completed jobs
	MutexCond completionCond; // Condition variable that signals when all started jobs have been completed
	
	/* Constructors and destructors: */
	public:
	WorkerBarrier(void) // Creates an idle worker barrier
		:numStartedJobs(0),numCompletedJobs(0)
		{
		}
	
	/* Methods: */
	void startJobs(unsigned int numJobs) // Starts the given number of jobs; must be called before any workers actually start working
		{
		/* Atomically increment the number of started jobs: */
		__atomic_add_fetch(&numStartedJobs,numJobs,__ATOMIC_ACQ_REL);
		}
	void completeJob(void) // Completes a job; must be called after the worker actually finishes its work, and after the worker starts any additional jobs
		{
		/* Atomically receive the number of started jobs and increment the number of completed jobs: */
		unsigned int nsj=__atomic_fetch_n(&numStartedJobs,__ATOMIC_ACQ);
		unsigned int ncj=__atomic_add_fetch(&numCompletedJobs,1,__ATOMIC_ACQ_REL);
		
		/* Check if all started jobs have been completed: */
		if(ncj==nsj)
			{
			/* Signal completion of all jobs to a blocked supervisor: */
			completionCond.signal();
			}
		}
	void wait(void) // Blocks the calling supervisor until all started jobs have been completed
		{
		MutexCond::Lock completionLock(completionCond)
		while(numCompletedJobs!=numStartedJobs)
			completionCond.wait(completionLock);
		}
	};

}

#endif

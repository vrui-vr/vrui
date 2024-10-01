/***********************************************************************
WorkerPool - Class to represent a set of worker pools that
asynchronously execute submitted one-off jobs.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <Threads/WorkerPool.h>

#include <stdexcept>
#include <Misc/Autopointer.h>
#include <Threads/FunctionCalls.h>
#include <Misc/MessageLogger.h>
#include <Threads/Thread.h>

namespace Threads {

/********************************************
Declaration of struct WorkerPool::Submission:
********************************************/

struct WorkerPool::Submission
	{
	/* Elements: */
	public:
	Misc::Autopointer<JobFunction> job; // Job function to execute
	Misc::Autopointer<JobCompleteCallback> completeCallback; // Callback to call from worker thread when job is finished
	EventDispatcher* dispatcher; // Event dispatcher to which to send a signal when job is finished
	EventDispatcher::ListenerKey signalKey; // Listener key of signal to send when job is finished
	
	/* Constructors and destructors: */
	Submission(void) // Dummy constructor
		:dispatcher(0),signalKey(0)
		{
		}
	Submission(JobFunction& sJob) // Creates a submission for a job without a completion callback
		:job(&sJob),
		 dispatcher(0),signalKey(0)
		{
		}
	Submission(JobFunction& sJob,JobCompleteCallback& sCompleteCallback) // Creates a submission for a job with a completion callback
		:job(&sJob),
		 completeCallback(&sCompleteCallback),
		 dispatcher(0),signalKey(0)
		{
		}
	Submission(JobFunction& sJob,EventDispatcher& sDispatcher,EventDispatcher::ListenerKey sSignalKey) // Creates a submission for a job with a completion signal on the given event dispatcher
		:job(&sJob),
		 dispatcher(&sDispatcher),signalKey(sSignalKey)
		{
		}
	};

/***********************************
Static elements of class WorkerPool:
***********************************/

WorkerPool WorkerPool::theWorkerPool(8);

/***************************
Methods of class WorkerPool:
***************************/

void* WorkerPool::workerThreadMethod(void)
	{
	/* Enable asynchronous cancellation for this thread: */
	Thread::setCancelState(Thread::CANCEL_ENABLE);
	Thread::setCancelType(Thread::CANCEL_ASYNCHRONOUS);
	
	/* Keep waiting for and executing jobs until the pool shuts down: */
	while(keepRunning)
		{
		/* Wait for the next job: */
		Submission submission;
		{
		MutexCond::Lock submissionCondLock(submissionCond);
		
		/* Wait while the submission queue is empty: */
		++numIdleWorkers;
		while(keepRunning&&submissionQueue.empty())
			submissionCond.wait(submissionCondLock);
		--numIdleWorkers;
		
		/* Bail out if shutting down: */
		if(!keepRunning)
			break;
		
		/* Grab the next job submission from the queue: */
		submission=submissionQueue.front();
		submissionQueue.pop_front();
		}
		
		try
			{
			/* Execute the next job: */
			(*submission.job)(0);
			
			/* Only emit callbacks or signals if the worker pool is not shutting down: */
			bool signalCompletion;
			{
			MutexCond::Lock submissionCondLock(submissionCond);
			signalCompletion=keepRunning;
			}
			
			if(signalCompletion)
				{
				/* Tell someone that the job has finished: */
				if(submission.dispatcher!=0)
					{
					/* Add an additional reference to the job object and send a signal to the dispatcher: */
					submission.job->ref();
					submission.dispatcher->signal(submission.signalKey,submission.job.getPointer());
					}
				else if(submission.completeCallback!=0)
					{
					/* Call the completion callback: */
					(*submission.completeCallback)(submission.job.getPointer());
					}
				}
			}
		catch(const std::runtime_error& err)
			{
			/* Do something useful... */
			Misc::formattedUserError("Threads::WorkerPool: Job terminated with exception %s",err.what());
			}
		}
	
	return 0;
	}

void WorkerPool::submitJob(const WorkerPool::Submission& submission)
	{
	MutexCond::Lock submissionCondLock(submissionCond);
	
	/* Remember if the submission queue was empty: */
	bool empty=submissionQueue.empty();
	
	/* Post the given submission structure to the submission queue: */
	submissionQueue.push_back(submission);
	
	/* Spin up a new worker thread if there are no idle workers and there is room left in the worker pool: */
	if(numActiveWorkers<maxNumWorkers&&numIdleWorkers==0)
		{
		/* Allocate the worker thread array if it hasn't been done yet: */
		if(workers==0)
			workers=new Thread[maxNumWorkers];
		
		/* Start the next worker thread: */
		workers[numActiveWorkers].start(this,&WorkerPool::workerThreadMethod);
		++numActiveWorkers;
		}
	
	/* Wake up a worker thread if the submission queue was empty: */
	if(empty)
		submissionCond.signal();
	}

void WorkerPool::doShutdown(void)
	{
	/* Signal all active workers that the pool is being shut down: */
	size_t numActiveJobs=0;
	{
	MutexCond::Lock submissionCondLock(submissionCond);
	numActiveJobs=numActiveWorkers-numIdleWorkers;
	keepRunning=false;
	submissionCond.broadcast();
	
	/* Print a warning and attempt to cancel the workers if there are active jobs: */
	if(numActiveJobs>0)
		{
		Misc::formattedLogNote("Threads::WorkerPool::shutdown: Attempting to cancel %u unfinished jobs; completion callbacks will not be called",(unsigned int)(numActiveJobs));
		for(size_t i=0;i<numActiveWorkers;++i)
			workers[i].cancel();
		}
	}
	
	/* Wait for all active workers to terminate: */
	for(size_t i=0;i<numActiveWorkers;++i)
		workers[i].join();
	numActiveWorkers=0;
	numIdleWorkers=0;
	}

WorkerPool::WorkerPool(size_t sMaxNumWorkers)
	:maxNumWorkers(sMaxNumWorkers),workers(0),
	 numActiveWorkers(0),numIdleWorkers(0),
	 submissionQueue(16),
	 keepRunning(true)
	{
	}

WorkerPool::~WorkerPool(void)
	{
	/* Shut down the worker pool (it's okay if this has been called before): */
	doShutdown();
	
	/* Clean up: */
	delete[] workers;
	}

void WorkerPool::shutdown(void)
	{
	/* Call the private method on the static worker pool: */
	theWorkerPool.doShutdown();
	}

void WorkerPool::submitJob(WorkerPool::JobFunction& job)
	{
	/* Submit the job: */
	theWorkerPool.submitJob(Submission(job));
	}

void WorkerPool::submitJob(WorkerPool::JobFunction& job,WorkerPool::JobCompleteCallback& completeCallback)
	{
	/* Submit the job: */
	theWorkerPool.submitJob(Submission(job,completeCallback));
	}

void WorkerPool::submitJob(WorkerPool::JobFunction& job,EventDispatcher& dispatcher,EventDispatcher::ListenerKey signalKey)
	{
	/* Submit the job: */
	theWorkerPool.submitJob(Submission(job,dispatcher,signalKey));
	}

}

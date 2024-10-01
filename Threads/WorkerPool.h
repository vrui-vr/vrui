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

#ifndef THREADS_WORKERPOOL_INCLUDED
#define THREADS_WORKERPOOL_INCLUDED

#include <stddef.h>
#include <Misc/RingBuffer.h>
#include <Threads/MutexCond.h>
#include <Threads/EventDispatcher.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
class Thread;
}

namespace Threads {

class WorkerPool
	{
	/* Embedded classes: */
	public:
	typedef Threads::FunctionCall<int> JobFunction; // Type for functions executing submitted jobs; the int parameter is bogus, but Threads::FunctionCall doesn't support void calls.
	typedef Threads::FunctionCall<JobFunction*> JobCompleteCallback; // Type for callbacks called when a submitted job is finished; parameter is pointer to submitted job function
	
	private:
	struct Submission; // Structure holding a pending submitted job
	typedef Misc::RingBuffer<Submission> SubmissionQueue; // Type for queues of pending job submissions
	
	/* Elements: */
	private:
	static WorkerPool theWorkerPool; // Static worker pool
	size_t maxNumWorkers; // Maximum number of active worker threads
	Thread* workers; // Array of worker threads
	size_t numActiveWorkers; // Number of currently active worker threads
	size_t numIdleWorkers; // Number of threads currently waiting for new job submissions
	Threads::MutexCond submissionCond; // Condition variable protecting the job submission queue and signalling the arrival of new jobs
	SubmissionQueue submissionQueue; // Dynamically growing ring buffer holding pending submitted jobs
	volatile bool keepRunning; // Flag to request the worker threads to shut down on pool destruction
	
	/* Private methods: */
	void* workerThreadMethod(void); // Method executing a worker thread
	void submitJob(const Submission& submission); // Submits the given job submission structure
	void doShutdown(void); // Shuts down the worker pool
	
	/* Constructors and destructors: */
	private:
	WorkerPool(size_t sMaxNumWorkers); // Creates a worker pool with the given maximum number of worker threads
	~WorkerPool(void);
	
	/* Methods: */
	public:
	static void shutdown(void); // Shuts down the worker pool and blocks until all currently active jobs finish; no completion callbacks or signals will be emitted, and function objects will be destroyed
	static void submitJob(JobFunction& job); // Executes the given job function asynchronously from a worker pool thread
	static void submitJob(JobFunction& job,JobCompleteCallback& completeCallback); // Executes the given job function asynchronously from a worker pool thread; calls given callback from worker thread when job is finished
	static void submitJob(JobFunction& job,EventDispatcher& dispatcher,EventDispatcher::ListenerKey signalKey); // Ditto, but raises a signal for the given listener key on the given event dispatcher with a plain pointer to the job function as signal data; the pointer will have an extra reference, the signal handler must unref() the object
	};

}

#endif

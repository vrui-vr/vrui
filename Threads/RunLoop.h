/***********************************************************************
RunLoop - Class for event handlers and message dispatchers constituting
a thread's run loop.
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

#ifndef THREADS_RUNLOOP_INCLUDED
#define THREADS_RUNLOOP_INCLUDED

#include <Misc/Autopointer.h>
#include <Misc/StdError.h>
#include <Misc/DynamicArray.h>
#include <Threads/Mutex.h>
#include <Threads/Thread.h>
#include <Threads/EventTypes.h>

/* Forward declarations: */
struct pollfd;
namespace Threads {
class RefCounted;
class Ownable;
class Timer;
}

namespace Threads {

class RunLoop
	{
	friend class IOWatcher;
	friend class Timer;
	friend class SignalHandler;
	friend class UserSignal;
	friend class ProcessFunction;
	
	/* Embedded classes: */
	private:
	class TempCond; // Class for temporary condition variables created as needed to synchronize object destruction requests from other threads
	struct PipeMessage; // Structure for messages sent on a run loop's self-pipe
	struct ActiveIOWatcher; // Structure keeping track of currently active I/O watchers
	typedef Misc::DynamicArray<ActiveIOWatcher> ActiveIOWatcherList; // Type for lists of active I/O watchers
	typedef Misc::DynamicArray<struct pollfd> PollFdList; // Type for lists of polling request structures
	struct ActiveTimer; // Structure keeping track of currently active timers
	struct ActiveProcessFunction; // Structure keeping track of currently active process functions
	typedef Misc::DynamicArray<ActiveProcessFunction> ActiveProcessFunctionList; // Type for lists of active process functions
	
	/* Elements: */
	protected:
	Threads::Thread::LocalID threadId; // Per-process ID of the thread calling the run loop's event handling methods
	private:
	int pipeFds[2]; // File descriptors for the run loop's self-pipe
	bool pipeClosed; // Flag if the pipe has been closed by a call to shutdown
	static const size_t messageBufferSize; // Size of the self-pipe message buffer
	PipeMessage* messageBuffer; // A buffer to read messages from the self-pipe
	PipeMessage* messageEnd; // Pointer to the end of the self-pipe message buffer
	PipeMessage* messagePtr; // Pointer to the next unhandled self-pipe message
	unsigned int numActiveIOWatchers; // Number of active I/O watchers
	ActiveIOWatcherList activeIOWatchers; // List of currently active I/O watchers
	PollFdList pollFds; // List of polling request structures paralleling the list of active I/O watchers, with an extra entry at the beginning for the self-pipe's read end
	Misc::DynamicArray<ActiveTimer> activeTimers; // Priority heap of active timers sorted by next time-out; unfortunately, we can't use Misc::PriorityHeap due to the additional bookkeeping we require
	ActiveProcessFunctionList activeProcessFunctions; // List of currently active process functions
	unsigned int numSpinningProcessFunctions; // Number of currently active process functions that want to spin
	EventTime lastDispatchTime; // The last point in time for which any events were dispatched
	bool shutdownRequested; // Flag to request a running run loop to shut down
	bool handlingIOWatchers; // Flag if the dispatchNextEvents method is currently handling I/O watchers
	unsigned int handledIOWatcherIndex; // Index of the active I/O watcher whose event is currently being handled
	bool handlingProcessFunctions; // Flag if the dispatchNextEvents method is currently handling process functions
	unsigned int handledProcessFunctionIndex; // Index of the active process function which is currently being handled
	
	/* Private methods: */
	bool writePipeMessage(const PipeMessage& pm,const char* methodName,Ownable* messageSender =0,RefCounted* messageObject =0); // Writes a message to the self-pipe; adds references to the given sender and/or object if the write succeeds; returns false if the self-pipe was closed during run loop shutdown, throws exception on other errors
	void insertActiveTimer(Timer* newTimer,const EventTime& newTimeout); // Inserts a newly-active timer into the active timer heap
	void updateActiveTimer(Timer* timer); // Updates an active timer in the active timer heap
	void replaceFirstActiveTimer(Timer* newTimer,const EventTime& newTimeout); // Replaces the first active timer in the active timer heap with the given timer and timeout
	void handlePipeMessages(PipeMessage* end); // Handles the batch of self-pipe messages between messagePtr and the given end pointer; sets messagePtr to messageEnd
	
	/* Constructors and destructors: */
	public:
	RunLoop(void); // Creates a run loop associated with the calling thread
	~RunLoop(void);
	
	/* Methods to capture OS signals: */
	void stopOnSignal(int signum); // Instructs this run loop to stop if the OS signal of the given number is received; throws exception if another run loop already listens to that signal
	
	/* Methods to wake up or terminate a potentially blocked run loop: */
	void wakeUp(void); // Wakes up a potentially blocked run loop; waitForEvents() call will return true
	void stop(void); // Orders the run loop to stop dispatching events; some subsequent waitForEvents() call will return false
	
	/* Event dispatching methods; must only be called from the thread to which the run loop is attached: */
	void attachToThread(void); // Attaches the run loop to the calling thread; it is the caller's responsibility to prevent asynchronous use around this call
	void restart(void); // Restarts a run loop that was previously stopped by calling stop() and/or shut down by subsequently calling shutdown()
	bool waitForEvents(void); // Blocks until any event happens; does not block and returns false if the stop() method was called; updates dispatch time immediately before returning
	const EventTime& getDispatchTime(void) const // Returns the most recent dispatch time sample
		{
		return lastDispatchTime;
		}
	void dispatchPendingEvents(void); // Dispatches all events that were detected during the previous waitForEvents call
	void run(void); // Convenience method to restart the run loop if it was shut down and dispatch events until stopped by calling the stop() method
	
	/* Clean-up methods: */
	void shutdown(void); // Drains the self-pipe and releases all resources after stop() has been called; is implicitly called by destructor
	};

}

#endif

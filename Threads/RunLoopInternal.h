/***********************************************************************
RunLoopInternal - Declarations of embedded classes of Threads::RunLoop
required by component implementations.
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

#ifndef THREADS_RUNLOOPINTERNAL_INCLUDED
#define THREADS_RUNLOOPINTERNAL_INCLUDED

#include <string.h>
#include <time.h>
#include <Threads/Mutex.h>
#include <Threads/Cond.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
class IOWatcher;
class IOWatcherEvent;
typedef Threads::FunctionCall<IOWatcherEvent&> IOWatcherEventHandler;
class Timer;
class TimerEvent;
typedef Threads::FunctionCall<TimerEvent&> TimerEventHandler;
class SignalHandler;
class SignalHandlerEvent;
typedef Threads::FunctionCall<SignalHandlerEvent&> SignalHandlerEventHandler;
class UserSignal;
class UserSignalEvent;
typedef Threads::FunctionCall<UserSignalEvent&> UserSignalEventHandler;
class ProcessFunction;
typedef Threads::FunctionCall<ProcessFunction&> ProcessFunctionFunction;
}

namespace Threads {

/***************************************
Declaration of struct RunLoop::TempCond:
***************************************/

class RunLoop::TempCond
	{
	/* Elements: */
	private:
	Threads::Mutex mutex; // A mutex protecting the condition variable
	Threads::Cond cond; // A condition variable
	bool signaled; // A flag to prevent spurious wake-ups from waiting on the condition variable
	
	/* Constructors and destructors: */
	public:
	TempCond(void) // Creates an unsignaled condition variable in locked state
		:signaled(false)
		{
		/* Lock the mutex: */
		mutex.lock();
		}
	~TempCond(void) // Unlocks and destroys the condition variable
		{
		/* Unlock the mutex: */
		mutex.unlock();
		}
	
	/* Methods: */
	void wait(void) // Waits on the condition variable
		{
		/* Wait until the flag changes value: */
		while(!signaled)
			cond.wait(mutex);
		}
	void signal(void) // Signals the condition variable
		{
		/* Lock the mutex: */
		mutex.lock();
		
		/* Set the flag: */
		signaled=true;
		
		/* Signal the condition variable: */
		cond.signal();
		
		/* Unlock the mutex: */
		mutex.unlock();
		}
	};

/******************************************
Declaration of struct RunLoop::PipeMessage:
******************************************/

struct RunLoop::PipeMessage
	{
	/* Embedded classes: */
	public:
	enum MessageType // Enumerated type for pipe message types
		{
		/* Messages indicating events that have to be dispatched: */
		WakeUp=0, // Wake up a run loop blocked on I/O
		Stop, // Wake up and stop a run loop
		Signal, // Sends an OS signal to a run loop
		SignalUserSignal, // Sends a signal to a user signal
		
		/* Internal messages related to run loop operation that should not cause a return from waitForEvents: */
		
		/* Messages related to I/O watchers: */
		SetIOWatcherEventMask, // Change the event mask of an I/O watcher
		EnableIOWatcher, // Enable an I/O watcher
		DisableIOWatcher, // Disable an I/O watcher, possibly terminally
		SetIOWatcherEventHandler, // Set an I/O watcher's event handler
		
		/* Messages related to timers: */
		SetTimerTimeout, // Set the next time-out of a timer
		SetTimerTimeoutReenable, // Ditto, and also re-enable a disabled timer
		SetTimerInterval, // Set the interval of a recurring timer
		EnableTimer, // Enable a timer
		DisableTimer, // Disable a timer, possibly terminally
		SetTimerEventHandler, // Set a timer's event handler
		
		/* Messages related to OS signal handlers: */
		EnableSignalHandler, // Enable an OS signal handler
		DisableSignalHandler, // Disable an OS signal handler
		SetSignalHandlerEventHandler, // Set a signal handler's event handler
		
		/* Messages related to user signals: */
		EnableUserSignal, // Enable a user signal
		DisableUserSignal, // Disable a user signal
		SetUserSignalEventHandler, // Set a signal handler's event handler
		
		/* Messages related to process functions: */
		SetProcessFunctionSpinning, // Sets a process function's spinning request flag
		EnableProcessFunction, // Enable a process function
		DisableProcessFunction, // Disable a process function, possibly terminally
		SetProcessFunctionFunction, // Set a process function's function
		};
	
	/* Elements: */
	unsigned int messageType; // Type of this message
	union
		{
		/* Messages related to I/O watchers: */
		struct
			{
			IOWatcher* ioWatcher; // Pointer to the I/O watcher whose event mask is to be changed; the pipe message holds a reference to the I/O watcher
			unsigned int newEventMask; // The new event mask
			} setIOWatcherEventMask;
		struct
			{
			IOWatcher* ioWatcher; // Pointer to the I/O watcher to be enabled; the pipe message holds a reference to the I/O watcher
			} enableIOWatcher;
		struct
			{
			IOWatcher* ioWatcher; // Pointer to the I/O watcher to be disabled; the pipe message holds a reference to the I/O watcher
			RunLoop::TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableIOWatcher;
		struct
			{
			IOWatcher* ioWatcher; // Pointer to the I/O watcher whose event handler is to be set; the pipe message holds a reference to the I/O watcher
			IOWatcherEventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
			} setIOWatcherEventHandler;
		
		/* Messages related to timers: */
		struct
			{
			Timer* timer; // Pointer to the timer whose time-out is to be set; the pipe message holds a reference to the timer
			struct timespec timeout; // New time-out; specified as a struct timespec to avoid the EventTime default constructor
			} setTimerTimeout;
		struct
			{
			Timer* timer; // Pointer to the timer whose interval is to be set; the pipe message holds a reference to the timer
			struct timespec interval; // New interval; specified as a struct timespec to avoid the EventInterval default constructor
			} setTimerInterval;
		struct
			{
			Timer* timer; // Pointer to the timer to be enabled; the pipe message holds a reference to the timer
			} enableTimer;
		struct
			{
			Timer* timer; // Pointer to the timer to be disabled; the pipe message holds a reference to the timer
			RunLoop::TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableTimer;
		struct
			{
			Timer* timer; // Pointer to the timer whose event handler is to be set; the pipe message holds a reference to the timer
			TimerEventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
			} setTimerEventHandler;
		
		/* Messages related to OS signal handlers: */
		struct
			{
			SignalHandler* signalHandler; // Pointer to the OS signal handler to be enabled; the pipe message holds a reference to the OS signal handler
			} enableSignalHandler;
		struct
			{
			SignalHandler* signalHandler; // Pointer to the OS signal handler to be disabled; the pipe message holds a reference to the OS signal handler
			RunLoop::TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableSignalHandler;
		struct
			{
			SignalHandler* signalHandler; // Pointer to the OS signal handler whose event handler is to be set; the pipe message holds a reference to the OS signal handler
			SignalHandlerEventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
			} setSignalHandlerEventHandler;
		struct
			{
			int signum; // The OS signal number
			} signal;
		
		/* Messages related to user signals: */
		struct
			{
			UserSignal* userSignal; // Pointer to the user signal to be enabled; the pipe message holds a reference to the user signal
			} enableUserSignal;
		struct
			{
			UserSignal* userSignal; // Pointer to the user signal to be disabled; the pipe message holds a reference to the user signal
			RunLoop::TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableUserSignal;
		struct
			{
			UserSignal* userSignal; // Pointer to the user signal whose event handler is to be set; the pipe message holds a reference to the user signal
			UserSignalEventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
			} setUserSignalEventHandler;
		struct
			{
			UserSignal* userSignal; // Pointer to the user signal to whom the signal is addressed; the pipe message holds a reference to the user signal
			RefCounted* signalData; // Pointer to the signal data; the pipe message holds a reference to the data
			} signalUserSignal;
		
		/* Messages related to process functions: */
		struct
			{
			ProcessFunction* processFunction; // Pointer to the process function whose spinning request flag is to be set; the pipe message holds a reference to the process function
			bool spinning; // The new value of the spinning request flag
			} setProcessFunctionSpinning;
		struct
			{
			ProcessFunction* processFunction; // Pointer to the process function to be enabled; the pipe message holds a reference to the process function
			} enableProcessFunction;
		struct
			{
			ProcessFunction* processFunction; // Pointer to the process function to be disabled; the pipe message holds a reference to the process function
			RunLoop::TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableProcessFunction;
		struct
			{
			ProcessFunction* processFunction; // Pointer to the process function whose event handler is to be set; the pipe message holds a reference to the process function
			ProcessFunctionFunction* function; // Pointer to the new process function function; the pipe message holds a reference to the function
			} setProcessFunctionFunction;
		};
	
	/* Constructors and destructors: */
	PipeMessage(void) // Default constructor zeroes the pipe message structure to avoid uninitialized memory warnings
		{
		memset(this,0,sizeof(PipeMessage));
		}
	PipeMessage(unsigned int sMessageType) // Creates a zeroed-out pipe message with the given message type
		{
		memset(this,0,sizeof(PipeMessage));
		messageType=sMessageType;
		}
	};

/**********************************************
Declaration of struct RunLoop::ActiveIOWatcher:
**********************************************/

struct RunLoop::ActiveIOWatcher
	{
	/* Elements: */
	public:
	IOWatcher* ioWatcher; // Pointer to the I/O watcher object; holds a reference to the I/O watcher object
	
	/* Constructors and destructors: */
	ActiveIOWatcher(IOWatcher* sIoWatcher) // Elementwise constructor
		:ioWatcher(sIoWatcher)
		{
		}
	};

/******************************************
Declaration of struct RunLoop::ActiveTimer:
******************************************/

struct RunLoop::ActiveTimer
	{
	/* Elements: */
	public:
	Timer* timer; // Pointer to the timer object; holds a reference to the timer object
	EventTime timeout; // Next time point at which the timer elapses; copy of the timer's time-out element
	
	/* Constructors and destructors: */
	ActiveTimer(Timer* sTimer,const EventTime& sTimeout) // Elementwise constructor
		:timer(sTimer),timeout(sTimeout)
		{
		}
	};

/****************************************************
Declaration of struct RunLoop::ActiveProcessFunction:
****************************************************/

struct RunLoop::ActiveProcessFunction
	{
	/* Elements: */
	public:
	ProcessFunction* processFunction; // Pointer to the process function object; holds a reference to the process function object
	
	/* Constructors and destructors: */
	ActiveProcessFunction(ProcessFunction* sProcessFunction) // Elementwise constructor
		:processFunction(sProcessFunction)
		{
		}
	};

}

#endif

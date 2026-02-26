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

#include <Threads/RunLoop.h>

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Threads/RefCounted.h>
#include <Threads/Mutex.h>
#include <Threads/Cond.h>
#include <Threads/FunctionCalls.h>

namespace Threads {

namespace {

/**************
Helper classes:
**************/

class TempCond // Class for temporary condition variables for inter-thread synchronization
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

}

/******************************************
Declaration of struct RunLoop::PipeMessage:
******************************************/

struct RunLoop::PipeMessage
	{
	/* Embedded classes: */
	public:
	enum MessageType // Enumerated type for pipe message types
		{
		/* Messages related to the internal operation of the run loop: */
		WakeUp=0, // Wake up a run loop blocked on I/O
		Stop, // Wake up and stop a run loop
		
		/* Messages related to I/O watchers: */
		SetIOWatcherEventMask, // Change the event mask of an I/O watcher
		EnableIOWatcher, // Enable an I/O watcher
		DisableIOWatcher, // Disable an I/O watcher, possibly terminally
		SetIOWatcherEventHandler, // Set an I/O watcher's event handler
		
		/* Messages related to timers: */
		SetTimerTimeout, // Set the next time-out of a timer
		SetTimerInterval, // Set the interval of a recurring timer
		EnableTimer, // Enable a timer
		DisableTimer, // Disable a timer, possibly terminally
		SetTimerEventHandler, // Set a timer's event handler
		
		/* Messages related to OS signal handlers: */
		EnableSignalHandler, // Enable an OS signal handler
		DisableSignalHandler, // Disable an OS signal handler
		SetSignalHandlerEventHandler, // Set a signal handler's event handler
		Signal, // Sends an OS signal to a run loop
		
		/* Messages related to user signals: */
		EnableUserSignal, // Enable a user signal
		DisableUserSignal, // Disable a user signal
		SetUserSignalEventHandler, // Set a signal handler's event handler
		SignalUserSignal, // Sends a signal to a user signal
		
		/* Messages related to process functions: */
		SetProcessFunctionSpinning, // Sets a process function's spinning request flag
		EnableProcessFunction, // Enable a process function
		DisableProcessFunction, // Disable a process function, possibly terminally
		SetProcessFunctionEventHandler, // Set a process function's event handler
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
			TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableIOWatcher;
		struct
			{
			IOWatcher* ioWatcher; // Pointer to the I/O watcher whose event handler is to be set; the pipe message holds a reference to the I/O watcher
			IOWatcher::EventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
			} setIOWatcherEventHandler;
		
		/* Messages related to timers: */
		struct
			{
			Timer* timer; // Pointer to the timer whose time-out is to be set; the pipe message holds a reference to the timer
			struct timespec timeout; // New time-out; specified as a struct timespec to avoid the Time default constructor
			} setTimerTimeout;
		struct
			{
			Timer* timer; // Pointer to the timer whose interval is to be set; the pipe message holds a reference to the timer
			struct timespec interval; // New interval; specified as a struct timespec to avoid the Interval default constructor
			} setTimerInterval;
		struct
			{
			Timer* timer; // Pointer to the timer to be enabled; the pipe message holds a reference to the timer
			} enableTimer;
		struct
			{
			Timer* timer; // Pointer to the timer to be disabled; the pipe message holds a reference to the timer
			TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableTimer;
		struct
			{
			Timer* timer; // Pointer to the timer whose event handler is to be set; the pipe message holds a reference to the timer
			Timer::EventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
			} setTimerEventHandler;
		
		/* Messages related to OS signal handlers: */
		struct
			{
			SignalHandler* signalHandler; // Pointer to the OS signal handler to be enabled; the pipe message holds a reference to the OS signal handler
			} enableSignalHandler;
		struct
			{
			SignalHandler* signalHandler; // Pointer to the OS signal handler to be disabled; the pipe message holds a reference to the OS signal handler
			TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableSignalHandler;
		struct
			{
			SignalHandler* signalHandler; // Pointer to the OS signal handler whose event handler is to be set; the pipe message holds a reference to the OS signal handler
			SignalHandler::EventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
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
			TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableUserSignal;
		struct
			{
			UserSignal* userSignal; // Pointer to the user signal whose event handler is to be set; the pipe message holds a reference to the user signal
			UserSignal::EventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
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
			TempCond* cond; // Pointer to a temporary condition variable on which the caller is waiting for confirmation, or null if call is asynchronous
			} disableProcessFunction;
		struct
			{
			ProcessFunction* processFunction; // Pointer to the process function whose event handler is to be set; the pipe message holds a reference to the process function
			ProcessFunction::EventHandler* eventHandler; // Pointer to the new event handler; the pipe message holds a reference to the handler
			} setProcessFunctionEventHandler;
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

/***********************************
Methods of class RunLoop::IOWatcher:
***********************************/

void RunLoop::IOWatcher::disowned(void)
	{
	/* Ask the run loop to disable this I/O watcher synchronously, thus dropping all references to it and not sending further events: */
	runLoop.disableIOWatcher(this,true);
	}

RunLoop::IOWatcher::IOWatcher(RunLoop& sRunLoop,int sFd,unsigned int sEventMask,bool sEnabled,RunLoop::IOWatcher::EventHandler& sEventHandler)
	:runLoop(sRunLoop),fd(sFd),eventMask(sEventMask),enabled(false),eventHandler(&sEventHandler)
	{
	/* If the enabled flag is set, enable this I/O watcher immediately: */
	if(sEnabled)
		runLoop.enableIOWatcher(this);
	}

/**********************************************
Declaration of struct RunLoop::ActiveIOWatcher:
**********************************************/

struct RunLoop::ActiveIOWatcher
	{
	/* Elements: */
	public:
	IOWatcher* ioWatcher; // Pointer to the I/O watcher object; holds a reference to the I/O watcher object
	};

/*******************************
Methods of class RunLoop::Timer:
*******************************/

void RunLoop::Timer::disowned(void)
	{
	/* Ask the run loop to disable this timer synchronously, thus dropping all references to it and not sending further events: */
	runLoop.disableTimer(this,true);
	}

RunLoop::Timer::Timer(RunLoop& sRunLoop,const RunLoop::Time& sTimeout,const RunLoop::Interval& sInterval,bool sEnabled,RunLoop::Timer::EventHandler& sEventHandler)
	:runLoop(sRunLoop),timeout(sTimeout),interval(sInterval),enabled(false),eventHandler(&sEventHandler)
	{
	/* If the enabled flag is set, enable this timer immediately: */
	if(sEnabled)
		runLoop.enableTimer(this);
	}

/******************************************
Declaration of struct RunLoop::ActiveTimer:
******************************************/

struct RunLoop::ActiveTimer
	{
	/* Elements: */
	public:
	Timer* timer; // Pointer to the timer object; holds a reference to the timer object
	Time timeout; // Next time point at which the timer elapses; copy of the timer's time-out element
	
	/* Constructors and destructors: */
	ActiveTimer(Timer* sTimer,const Time& sTimeout) // Elementwise constructor
		:timer(sTimer),timeout(sTimeout)
		{
		}
	};

/******************************************************
Declaration of struct RunLoop::RegisteredSignalHandler:
******************************************************/

struct RunLoop::RegisteredSignalHandler
	{
	/* Elements: */
	public:
	RunLoop* runLoop; // The run loop that registered this signal or null for unhandled signals
	RunLoop::SignalHandler* signalHandler; // Pointer to the OS signal handler registered for this signal, or 0 if the run loop should stop on receiving the signal; holds a reference to the OS signal handler object
	
	/* Constructors and destructors: */
	RegisteredSignalHandler(void) // Default constructor represents unhandled signal
		:runLoop(0),signalHandler(0)
		{
		}
	};

/***************************************
Methods of class RunLoop::SignalHandler:
***************************************/

void RunLoop::SignalHandler::disowned(void)
	{
	/* Ask the run loop to disable this OS signal handler synchronously, thus dropping all references to it and not sending further events: */
	runLoop.disableSignalHandler(this,true);
	}

RunLoop::SignalHandler::SignalHandler(RunLoop& sRunLoop,int sSignum,bool sEnabled,RunLoop::SignalHandler::EventHandler& sEventHandler)
	:runLoop(sRunLoop),signum(sSignum),enabled(sEnabled),eventHandler(&sEventHandler)
	{
	}

/************************************
Methods of class RunLoop::UserSignal:
************************************/

void RunLoop::UserSignal::disowned(void)
	{
	/* Ask the run loop to disable this user signal synchronously, thus dropping all references to it and not sending further events: */
	runLoop.disableUserSignal(this,true);
	}

RunLoop::UserSignal::UserSignal(RunLoop& sRunLoop,bool sEnabled,EventHandler& sEventHandler)
	:runLoop(sRunLoop),enabled(false),eventHandler(&sEventHandler)
	{
	/* If the enabled flag is set, enable this user signal immediately: */
	if(sEnabled)
		runLoop.enableUserSignal(this);
	}

/*****************************************
Methods of class RunLoop::ProcessFunction:
*****************************************/

void RunLoop::ProcessFunction::disowned(void)
	{
	/* Ask the run loop to disable this process function synchronously, thus dropping all references to it and not sending further events: */
	runLoop.disableProcessFunction(this,true);
	}

RunLoop::ProcessFunction::ProcessFunction(RunLoop& sRunLoop,bool sSpinning,bool sEnabled,EventHandler& sEventHandler)
	:runLoop(sRunLoop),spinning(sSpinning),enabled(false),eventHandler(&sEventHandler)
	{
	/* If the enabled flag is set, enable this process function immediately: */
	if(sEnabled)
		runLoop.enableProcessFunction(this);
	}

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

/********************************
Static elements of class RunLoop:
********************************/

const size_t RunLoop::messageBufferSize=PIPE_BUF/sizeof(RunLoop::PipeMessage); // Reserve for the number of messages that fits into the guaranteed atomic write size
RunLoop::RegisteredSignalHandler RunLoop::registeredSignalHandlers[RunLoop::maxSignal+1];

/************************
Methods of class RunLoop:
************************/

bool RunLoop::writePipeMessage(const RunLoop::PipeMessage& pm,const char* methodName,Ownable* messageSender,RefCounted* messageObject)
	{
	bool result=true;
	
	/* If there is a message sender and/or object, take references to them: */
	if(messageSender!=0)
		messageSender->ref();
	if(messageObject!=0)
		messageObject->ref();
	
	/* Make a blocking atomic write to the self-pipe: */
	ssize_t writeResult=write(pipeFds[1],&pm,sizeof(PipeMessage));
	if(writeResult<0)
		{
		/* Check if the self-pipe was closed because the run loop is shutting down: */
		if(errno==EBADF)
			{
			/* Drop the references to a message sender and/or object again and signal failure: */
			if(messageSender!=0)
				messageSender->unref();
			if(messageObject!=0)
				messageObject->unref();
			result=false;
			}
		else
			throw Misc::makeLibcErr(methodName,errno,"Cannot write to event pipe");
		}
	else if(writeResult!=sizeof(PipeMessage))
		throw Misc::makeStdErr(methodName,"Partial write to event pipe");
	
	return result;
	}

namespace {

/****************
Helper functions:
****************/

inline void setPollRequestEvents(struct pollfd& pollFd,unsigned int newEventMask) // Updates the given poll request structure with the given event mask
	{
	/* Compile-time check if the event mask bits exposed at the interface match the POLL* macros defined in poll.h: */
	#if POLLIN==0x1&&POLLPRI==0x2&&POLLOUT==0x4
	
	/* Assign the poll request's event set directly: */
	pollFd.events=newEventMask;
	
	#else
	
	/* Assemble the poll request's event set bit-by-bit: */
	pollFd.events=0x0;
	if(newEventMask&IOWatcher::Read)
		pollFd.events|=POLLIN;
	if(newEventMask&IOWatcher::Exception)
		pollFd.events|=POLLPRI;
	if(newEventMask&IOWatcher::Write)
		pollFd.events|=POLLOUT;
	
	#endif
	}

inline unsigned int getPollRequestEvents(const struct pollfd& pollFd) // Retrieves the given poll request's return event mask
	{
	/* Compile-time check if the event mask bits exposed at the interface match the POLL* macros defined in poll.h: */
	#if POLLIN==0x1&&POLLPRI==0x2&&POLLOUT==0x4&&POLLERR==0x8&&POLLHUP==0x10&&POLLNVAL==0x20
	
	/* Return the event mask directly: */
	return pollFd.revents;
	
	#else
	
	/* Assemble the event mask bit-by-bit: */
	unsigned int result=0x0U;
	if((pollFd.revents&POLLIN)!=0x0)
		result|=IOWatcher::Read;
	if((pollFd.revents&POLLPRI)!=0x0)
		result|=IOWatcher::Exception;
	if((pollFd.revents&POLLOUT)!=0x0)
		result|=IOWatcher::Write;
	if((pollFd.revents&POLLERR)!=0x0)
		result|=IOWatcher::Error;
	if((pollFd.revents&POLLHUP)!=0x0)
		result|=IOWatcher::HangUp;
	if((pollFd.revents&POLLNVAL)!=0x0)
		result|=IOWatcher::Invalid;
	
	#endif
	}

}

/***********************************
Internal interface for I/O watchers:
***********************************/

void RunLoop::setIOWatcherEventMask(RunLoop::IOWatcher* ioWatcher,unsigned int newEventMask)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the I/O watcher is enabled: */
		if(ioWatcher->enabled)
			{
			/* Update the I/O watcher's poll request: */
			setPollRequestEvents(pollFds[ioWatcher->activeIndex+1],newEventMask);
			}
		
		/* Update the I/O watcher's event mask: */
		ioWatcher->eventMask=newEventMask;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetIOWatcherEventMask);
		pm.setIOWatcherEventMask.ioWatcher=ioWatcher;
		pm.setIOWatcherEventMask.newEventMask=newEventMask;
		writePipeMessage(pm,__PRETTY_FUNCTION__,ioWatcher);
		}
	}

void RunLoop::enableIOWatcher(RunLoop::IOWatcher* ioWatcher)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the I/O watcher is not already enabled and still has an owner: */
		if(!ioWatcher->enabled&&ioWatcher->isOwned())
			{
			/* Append an entry for the I/O watcher to the end of the active lists: */
			ActiveIOWatcher newIOWatcher;
			newIOWatcher.ioWatcher=ioWatcher;
			activeIOWatchers.push_back(newIOWatcher);
			ioWatcher->ref(); // Take another reference to the I/O watcher
			
			/* Append a poll request for the I/O watcher to the end of the poll request list: */
			struct pollfd pollFd;
			pollFd.fd=ioWatcher->fd;
			setPollRequestEvents(pollFd,ioWatcher->eventMask);
			pollFd.revents=0x0;
			pollFds.push_back(pollFd);
			
			/* Set the I/O watcher's position in the active list and increase the number of active I/O watchers: */
			ioWatcher->activeIndex=numActiveIOWatchers;
			++numActiveIOWatchers;
			
			/* Mark the I/O watcher as enabled: */
			ioWatcher->enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::EnableIOWatcher);
		pm.enableIOWatcher.ioWatcher=ioWatcher;
		writePipeMessage(pm,__PRETTY_FUNCTION__,ioWatcher);
		}
	}

void RunLoop::disableIOWatcher(RunLoop::IOWatcher* ioWatcher,bool willDestroy)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the I/O watcher is not already disabled: */
		if(ioWatcher->enabled)
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
			unsigned int aiowi=ioWatcher->activeIndex; // List index of the to-be-disabled I/O watcher
			unsigned int hiowi=handledIOWatcherIndex; // List index of the currently handled I/O watcher
			if(handlingIOWatchers&&aiowi<=hiowi)
				{
				/* Move the currently-handled I/O watcher to the place of the to-be-deleted one: */
				activeIOWatchers[aiowi]=activeIOWatchers[hiowi];
				pollFds[aiowi+1]=pollFds[hiowi+1]; // Take account of the extra self-pipe entry at the head of the list
				activeIOWatchers[aiowi].ioWatcher->activeIndex=aiowi;
				
				/* Move the last I/O watcher in the lists to the place of the currently-handled one: */
				activeIOWatchers[hiowi]=activeIOWatchers[numActiveIOWatchers-1];
				pollFds[hiowi+1]=pollFds[numActiveIOWatchers]; // Take account of the extra self-pipe entry at the head of the list
				activeIOWatchers[hiowi].ioWatcher->activeIndex=hiowi;
				
				/* Reduce the currently-handled index to handle the I/O watcher that used to be the last in the lists next: */
				--handledIOWatcherIndex;
				}
			else
				{
				/* Move the last I/O watcher in the lists to the place of the to-be-disabled one: */
				activeIOWatchers[aiowi]=activeIOWatchers[numActiveIOWatchers-1];
				pollFds[aiowi+1]=pollFds[numActiveIOWatchers]; // Take account of the extra self-pipe entry at the head of the list
				activeIOWatchers[aiowi].ioWatcher->activeIndex=aiowi;
				}
			
			/* Remove the now unused last entries from the active I/O watcher lists: */
			activeIOWatchers.pop_back();
			pollFds.pop_back();
			--numActiveIOWatchers;
			
			/* Mark the I/O watcher as disabled: */
			ioWatcher->enabled=false;
			
			/* Drop the active I/O watcher list's reference to the I/O watcher: */
			ioWatcher->unref();
			}
		}
	else
		{
		/* Create a pipe message: */
		PipeMessage pm(PipeMessage::DisableIOWatcher);
		pm.disableIOWatcher.ioWatcher=ioWatcher;
		
		/* Check if the caller will destroy the I/O watcher immediately after this call returns: */
		if(willDestroy)
			{
			/*******************************************************************
			Make a synchronous request by creating and locking a temporary
			condition variable, then writing to the self-pipe, then waiting on
			the condition variable:
			*******************************************************************/
			
			/* Create a temporary condition variable and put it into the pipe message: */
			TempCond cond;
			pm.disableIOWatcher.cond=&cond;
			
			/* Write to the self-pipe, but only wait on completion if the write succeeded: */
			if(writePipeMessage(pm,__PRETTY_FUNCTION__,ioWatcher))
				{
				/* Wait on the condition variable: */
				cond.wait();
				}
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableIOWatcher.cond=0;
			writePipeMessage(pm,__PRETTY_FUNCTION__,ioWatcher);
			}
		}
	}

void RunLoop::setIOWatcherEventHandler(RunLoop::IOWatcher* ioWatcher,RunLoop::IOWatcher::EventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Replace the I/O watcher's event handler: */
		ioWatcher->eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetIOWatcherEventHandler);
		pm.setIOWatcherEventHandler.ioWatcher=ioWatcher;
		pm.setIOWatcherEventHandler.eventHandler=&newEventHandler;
		writePipeMessage(pm,__PRETTY_FUNCTION__,ioWatcher,&newEventHandler);
		}
	}

void RunLoop::insertActiveTimer(RunLoop::Timer* newTimer,const RunLoop::Time& newTimeout)
	{
	/* Insert a new (dummy) entry at the end of the heap array: */
	activeTimers.push_back(ActiveTimer(newTimer,newTimeout));
	
	/* Find the correct heap slot for the new timer by iteratively fixing the heap invariance from the bottom up: */
	unsigned int heapSlot=activeTimers.size()-1;
	ActiveTimer* slots=activeTimers.data();
	unsigned int parentSlot;
	while(heapSlot>0&&slots[parentSlot=(heapSlot-1)>>1].timeout>newTimeout)
		{
		/* Move the parent into the proposed slot and continue checking from the parent: */
		slots[heapSlot]=slots[parentSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=parentSlot;
		}
	
	/* Place the new timer into the final heap slot: */
	slots[heapSlot].timer=newTimer;
	slots[heapSlot].timeout=newTimeout;
	slots[heapSlot].timer->activeIndex=heapSlot;
	}

void RunLoop::updateActiveTimer(RunLoop::Timer* timer)
	{
	/* Fix any broken heap invariances from changes to an active timer's time-out: */
	unsigned int numSlots=activeTimers.size();
	ActiveTimer* slots=activeTimers.data();
	unsigned int heapSlot=timer->activeIndex;
	
	/* Fix the heap invariance from the current heap slot up: */
	bool mustFix=true;
	unsigned int parentSlot;
	while(heapSlot>0&&slots[parentSlot=(heapSlot-1)>>1].timeout>timer->timeout)
		{
		/* Move the parent into the proposed slot and continue checking from the parent: */
		slots[heapSlot]=slots[parentSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=parentSlot;
		
		mustFix=false; // The heap will be fixed once we're done with this loop
		}
	
	/* Fix the heap invariance from the current heap slot down; this is not necessary if we already fixed the heap: */
	while(mustFix)
		{
		/* Find the earliest time-out between the last time-out and the up to two children of its proposed heap slot: */
		unsigned int minSlot=heapSlot;
		const Time* minTimeout=&timer->timeout;
		unsigned int child1Slot=(heapSlot<<1)+1;
		if(child1Slot<numSlots&&*minTimeout>slots[child1Slot].timeout)
			{
			minTimeout=&slots[child1Slot].timeout;
			minSlot=child1Slot;
			}
		unsigned int child2Slot=(heapSlot<<1)+2;
		if(child2Slot<numSlots&&*minTimeout>slots[child2Slot].timeout)
			{
			minTimeout=&slots[child2Slot].timeout;
			minSlot=child2Slot;
			}
		
		/* Bail out if the new time-out is the earliest: */
		if(minSlot==heapSlot)
			break;
		
		/* Move the earliest child into the parent's slot and continue checking from the earliest child: */
		slots[heapSlot]=slots[minSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=minSlot;
		}
	
	/* Place the timer's active timer heap entry into the final heap slot: */
	slots[heapSlot].timer=timer;
	slots[heapSlot].timeout=timer->timeout;
	slots[heapSlot].timer->activeIndex=heapSlot;
	}

void RunLoop::replaceFirstActiveTimer(RunLoop::Timer* newTimer,const RunLoop::Time& newTimeout)
	{
	/* Find the correct heap slot for the new timer by iteratively fixing the heap invariance from the top down: */
	unsigned int numSlots=activeTimers.size();
	ActiveTimer* slots=activeTimers.data();
	unsigned int heapSlot=0;
	while(true)
		{
		/* Find the earliest time-out between the new time-out and the up to two children of its proposed heap slot: */
		unsigned int minSlot=heapSlot;
		const Time* minTimeout=&newTimeout;
		unsigned int child1Slot=(heapSlot<<1)+1;
		if(child1Slot<numSlots&&*minTimeout>slots[child1Slot].timeout)
			{
			minTimeout=&slots[child1Slot].timeout;
			minSlot=child1Slot;
			}
		unsigned int child2Slot=(heapSlot<<1)+2;
		if(child2Slot<numSlots&&*minTimeout>slots[child2Slot].timeout)
			{
			minTimeout=&slots[child2Slot].timeout;
			minSlot=child2Slot;
			}
		
		/* Bail out if the new time-out is the earliest: */
		if(minSlot==heapSlot)
			break;
		
		/* Move the earliest child into the parent's slot and continue checking from the earliest child: */
		slots[heapSlot]=slots[minSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=minSlot;
		}
	
	/* Place the new timer into the final heap slot: */
	slots[heapSlot].timer=newTimer;
	slots[heapSlot].timeout=newTimeout;
	newTimer->activeIndex=heapSlot;
	}

/*****************************
Internal interface for timers:
*****************************/

void RunLoop::setTimerTimeout(RunLoop::Timer* timer,const RunLoop::Time& newTimeout)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Set the timer's time-out and ensure that the new time-out is not before lastDispatchTime: */
		timer->timeout=newTimeout;
		if(timer->timeout<lastDispatchTime)
			timer->timeout=lastDispatchTime;
		
		/* If the timer is enabled, fix the active timer heap: */
		if(timer->enabled)
			updateActiveTimer(timer);
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetTimerTimeout);
		pm.setTimerTimeout.timer=timer;
		pm.setTimerTimeout.timeout=newTimeout;
		writePipeMessage(pm,__PRETTY_FUNCTION__,timer);
		}
	}

void RunLoop::setTimerInterval(RunLoop::Timer* timer,const RunLoop::Interval& newInterval)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Set the timer's interval: */
		timer->interval=newInterval;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetTimerInterval);
		pm.setTimerInterval.timer=timer;
		pm.setTimerInterval.interval=newInterval;
		writePipeMessage(pm,__PRETTY_FUNCTION__,timer);
		}
	}

void RunLoop::enableTimer(RunLoop::Timer* timer)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the timer is not already enabled and still has an owner: */
		if(!timer->enabled&&timer->isOwned())
			{
			/* Ensure that the timer's time-out is not before lastDispatchTime: */
			if(timer->timeout<lastDispatchTime)
				timer->timeout=lastDispatchTime;
			
			/* Insert the timer into the active timers heap: */
			insertActiveTimer(timer,timer->timeout);
			timer->ref(); // Take an additional reference to the timer
			
			/* Mark the timer as enabled: */
			timer->enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::EnableTimer);
		pm.enableTimer.timer=timer;
		writePipeMessage(pm,__PRETTY_FUNCTION__,timer);
		}
	}

void RunLoop::disableTimer(RunLoop::Timer* timer,bool willDestroy)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the timer is not already disabled: */
		if(timer->enabled)
			{
			/* Remove the timer from the active timers heap by moving the last active timer in the heap to its heap slot: */
			Timer* lastTimer=activeTimers[activeTimers.size()-1].timer;
			activeTimers.pop_back();
			if(lastTimer!=timer)
				{
				/* Let the last timer take over the to-be-removed timer's heap slot and then fix the heap: */
				lastTimer->activeIndex=timer->activeIndex;
				updateActiveTimer(lastTimer);
				}
			
			/* Mark the timer as disabled: */
			timer->enabled=false;
			
			/* Drop the active timers heap's reference to the timer: */
			timer->unref();
			}
		}
	else
		{
		/* Create a pipe message: */
		PipeMessage pm(PipeMessage::DisableTimer);
		pm.disableTimer.timer=timer;
		
		/* Check if the caller will destroy the timer immediately after this call returns: */
		if(willDestroy)
			{
			/*******************************************************************
			Make a synchronous request by creating and locking a temporary
			condition variable, then writing to the self-pipe, then waiting on
			the condition variable:
			*******************************************************************/
			
			/* Create a temporary condition variable and put it into the pipe message: */
			TempCond cond;
			pm.disableTimer.cond=&cond;
			
			/* Write to the self-pipe, but only wait on completion if the write succeeded: */
			if(writePipeMessage(pm,__PRETTY_FUNCTION__,timer))
				{
				/* Wait on the condition variable: */
				cond.wait();
				}
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableTimer.cond=0;
			writePipeMessage(pm,__PRETTY_FUNCTION__,timer);
			}
		}
	}

void RunLoop::setTimerEventHandler(RunLoop::Timer* timer,RunLoop::Timer::EventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Replace the timer's event handler: */
		timer->eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetTimerEventHandler);
		pm.setTimerEventHandler.timer=timer;
		pm.setTimerEventHandler.eventHandler=&newEventHandler;
		writePipeMessage(pm,__PRETTY_FUNCTION__,timer,&newEventHandler);
		}
	}

/*****************************************
Internal interface for OS signal handlers:
*****************************************/

void RunLoop::enableSignalHandler(RunLoop::SignalHandler* signalHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the OS signal handler is not already enabled and still has an owner: */
		if(!signalHandler->enabled&&signalHandler->isOwned())
			{
			/* Enable the OS signal handler: */
			signalHandler->enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::EnableSignalHandler);
		pm.enableSignalHandler.signalHandler=signalHandler;
		writePipeMessage(pm,__PRETTY_FUNCTION__,signalHandler);
		}
	}

void RunLoop::disableSignalHandler(RunLoop::SignalHandler* signalHandler,bool willDestroy)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Disable the OS signal handler: */
		signalHandler->enabled=false;
		
		if(willDestroy)
			{
			/* Lock the OS signal handler table: */
			Threads::Mutex::Lock signalHandlersLock(signalHandlersMutex);
			
			/* Unregister this run loop and the signal handler: */
			int signum=signalHandler->signum;
			registeredSignalHandlers[signum].runLoop=0;
			registeredSignalHandlers[signum].signalHandler=0;
			
			/* Return the signal to default disposition: */
			struct sigaction sigAction;
			memset(&sigAction,0,sizeof(struct sigaction));
			sigAction.sa_handler=SIG_DFL;
			if(sigaction(signum,&sigAction,0)<0)
				Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Cannot restore OS signal %d",signum);
			
			/* Drop the OS signal handler table's reference to the OS signal handler: */
			signalHandler->unref();
			}
		}
	else
		{
		/* Create a pipe message: */
		PipeMessage pm(PipeMessage::DisableSignalHandler);
		pm.disableSignalHandler.signalHandler=signalHandler;
		
		/* Check if the caller will destroy the signal handler immediately after this call returns: */
		if(willDestroy)
			{
			/*******************************************************************
			Make a synchronous request by creating and locking a temporary
			condition variable, then writing to the self-pipe, then waiting on
			the condition variable:
			*******************************************************************/
			
			/* Create a temporary condition variable and put it into the pipe message: */
			TempCond cond;
			pm.disableSignalHandler.cond=&cond;
			
			/* Write to the self-pipe, but only wait on completion if the write succeeded: */
			if(writePipeMessage(pm,__PRETTY_FUNCTION__,signalHandler))
				{
				/* Wait on the condition variable: */
				cond.wait();
				}
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableSignalHandler.cond=0;
			writePipeMessage(pm,__PRETTY_FUNCTION__,signalHandler);
			}
		}
	}

void RunLoop::setSignalHandlerEventHandler(RunLoop::SignalHandler* signalHandler,RunLoop::SignalHandler::EventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Replace the signal handler's event handler: */
		signalHandler->eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetSignalHandlerEventHandler);
		pm.setSignalHandlerEventHandler.signalHandler=signalHandler;
		pm.setSignalHandlerEventHandler.eventHandler=&newEventHandler;
		writePipeMessage(pm,__PRETTY_FUNCTION__,signalHandler,&newEventHandler);
		}
	}

void RunLoop::signalHandlerFunction(int signum)
	{
	/* Send a message to the run loop that registered for this signal: */
	if(registeredSignalHandlers[signum].runLoop!=0) // Just double-checking to minimize impacts from the race conditions inherent in OS signals
		{
		/* Create a pipe message: */
		PipeMessage pm(PipeMessage::Signal);
		pm.signal.signum=signum;
		
		/* Make a blocking atomic write to the self-pipe, but don't bother checking for errors; nothing we can do: */
		int savedErrno=errno;
		write(registeredSignalHandlers[signum].runLoop->pipeFds[1],&pm,sizeof(PipeMessage));
		errno=savedErrno;
		}
	}

/***********************************
Internal interface for user signals:
***********************************/

void RunLoop::enableUserSignal(RunLoop::UserSignal* userSignal)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the user signal is not already enabled and still has an owner: */
		if(!userSignal->enabled&&userSignal->isOwned())
			{
			/* Enable the user signal: */
			userSignal->enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::EnableUserSignal);
		pm.enableUserSignal.userSignal=userSignal;
		writePipeMessage(pm,__PRETTY_FUNCTION__,userSignal);
		}
	}

void RunLoop::disableUserSignal(RunLoop::UserSignal* userSignal,bool willDestroy)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Disable the user signal: */
		userSignal->enabled=false;
		}
	else
		{
		/* Create a pipe message: */
		PipeMessage pm(PipeMessage::DisableUserSignal);
		pm.disableUserSignal.userSignal=userSignal;
		
		/* Check if the caller will destroy the user signal immediately after this call returns: */
		if(willDestroy)
			{
			/*******************************************************************
			Make a synchronous request by creating and locking a temporary
			condition variable, then writing to the self-pipe, then waiting on
			the condition variable:
			*******************************************************************/
			
			/* Create a temporary condition variable and put it into the pipe message: */
			TempCond cond;
			pm.disableUserSignal.cond=&cond;
			
			/* Write to the self-pipe, but only wait on completion if the write succeeded: */
			if(writePipeMessage(pm,__PRETTY_FUNCTION__,userSignal))
				{
				/* Wait on the condition variable: */
				cond.wait();
				}
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableUserSignal.cond=0;
			writePipeMessage(pm,__PRETTY_FUNCTION__,userSignal);
			}
		}
	}

void RunLoop::setUserSignalEventHandler(RunLoop::UserSignal* userSignal,RunLoop::UserSignal::EventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Replace the user signal's event handler: */
		userSignal->eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetUserSignalEventHandler);
		pm.setUserSignalEventHandler.userSignal=userSignal;
		pm.setUserSignalEventHandler.eventHandler=&newEventHandler;
		writePipeMessage(pm,__PRETTY_FUNCTION__,userSignal,&newEventHandler);
		}
	}

void RunLoop::signalUserSignal(RunLoop::UserSignal* userSignal,RefCounted& signalData)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		if(userSignal->enabled)
			{
			/* Call the user signal's event handler directly: */
			UserSignal::Event event(userSignal,lastDispatchTime,&signalData);
			(*userSignal->eventHandler)(event);
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SignalUserSignal);
		pm.signalUserSignal.userSignal=userSignal;
		pm.signalUserSignal.signalData=&signalData;
		writePipeMessage(pm,__PRETTY_FUNCTION__,userSignal,&signalData);
		}
	}

/****************************************
Internal interface for process functions:
****************************************/

void RunLoop::setProcessFunctionSpinning(RunLoop::ProcessFunction* processFunction,bool newSpinning)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the spinning state actually changed: */
		if(processFunction->spinning!=newSpinning)
			{
			/* If the process function is currently enabled, update the number of spinning process functions: */
			if(processFunction->enabled)
				{
				if(newSpinning)
					++numSpinningProcessFunctions;
				else
					--numSpinningProcessFunctions;
				}
			
			/* Update the process function's spinning state: */
			processFunction->spinning=newSpinning;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetProcessFunctionSpinning);
		pm.setProcessFunctionSpinning.processFunction=processFunction;
		pm.setProcessFunctionSpinning.spinning=newSpinning;
		writePipeMessage(pm,__PRETTY_FUNCTION__,processFunction);
		}
	}

void RunLoop::enableProcessFunction(RunLoop::ProcessFunction* processFunction)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the process function is not already enabled and still has an owner: */
		if(!processFunction->enabled&&processFunction->isOwned())
			{
			/* Append an entry for the process function to the end of the active list: */
			activeProcessFunctions.push_back(ActiveProcessFunction(processFunction));
			processFunction->ref(); // Take another reference to the process function object
			processFunction->activeIndex=activeProcessFunctions.size()-1;
			
			/* Increase the number of spinning process functions if the process function wants to spin: */
			if(processFunction->spinning)
				++numSpinningProcessFunctions;
			
			/* Mark the process function as enabled: */
			processFunction->enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::EnableProcessFunction);
		pm.enableProcessFunction.processFunction=processFunction;
		writePipeMessage(pm,__PRETTY_FUNCTION__,processFunction);
		}
	}

void RunLoop::disableProcessFunction(RunLoop::ProcessFunction* processFunction,bool willDestroy)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Check that the process function is not already disabled: */
		if(processFunction->enabled)
			{
			/*****************************************************************
			Shuffle the active process functions in the list such that the one
			that actually has to be removed is the last entry. This guarantees
			that disabling a process function is an O(1) operation.
			We have to take extra care if the one to be removed is before the
			one that is currently handled by event dispatching, to avoid
			missing events for the process function that is currently the last
			entry.
			*****************************************************************/
			
			/* Check if the run loop is currently handling process functions and the to-be-disabled one is in the list not after the currently-handled one: */
			unsigned int apfi=processFunction->activeIndex; // List index of the to-be-disabled process function
			unsigned int hpfi=handledProcessFunctionIndex; // List index of the currently handled process function
			if(handlingProcessFunctions&&apfi<=hpfi)
				{
				/* Move the currently-handled process function to the place of the to-be-deleted one: */
				activeProcessFunctions[apfi]=activeProcessFunctions[hpfi];
				activeProcessFunctions[apfi].processFunction->activeIndex=apfi;
				
				/* Move the last process function in the lists to the place of the currently-handled one: */
				activeProcessFunctions[hpfi]=activeProcessFunctions[activeProcessFunctions.size()-1];
				activeProcessFunctions[hpfi].processFunction->activeIndex=hpfi;
				
				/* Reduce the currently-handled index to handle the process function that used to be the last in the lists next: */
				--handledProcessFunctionIndex;
				}
			else
				{
				/* Move the last I/O watcher in the lists to the place of the to-be-disabled one: */
				activeProcessFunctions[apfi]=activeProcessFunctions[activeProcessFunctions.size()-1];
				activeProcessFunctions[apfi].processFunction->activeIndex=apfi;
				}
			
			/* Remove the now unused last entry from the active process function list: */
			activeProcessFunctions.pop_back();
			
			/* Decrease the number of spinning process functions if the process function wants to spin: */
			if(processFunction->spinning)
				--numSpinningProcessFunctions;
			
			/* Mark the process function as disabled: */
			processFunction->enabled=false;
			
			/* Drop the active process function list's reference to the process function object: */
			processFunction->unref();
			}
		}
	else
		{
		/* Create a pipe message: */
		PipeMessage pm(PipeMessage::DisableProcessFunction);
		pm.disableProcessFunction.processFunction=processFunction;
		
		/* Check if the caller will destroy the process function immediately after this call returns: */
		if(willDestroy)
			{
			/*******************************************************************
			Make a synchronous request by creating and locking a temporary
			condition variable, then writing to the self-pipe, then waiting on
			the condition variable:
			*******************************************************************/
			
			/* Create a temporary condition variable and put it into the pipe message: */
			TempCond cond;
			pm.disableProcessFunction.cond=&cond;
			
			/* Write to the self-pipe, but only wait on completion if the write succeeded: */
			if(writePipeMessage(pm,__PRETTY_FUNCTION__,processFunction))
				{
				/* Wait on the condition variable: */
				cond.wait();
				}
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableProcessFunction.cond=0;
			writePipeMessage(pm,__PRETTY_FUNCTION__,processFunction);
			}
		}
	}

void RunLoop::setProcessFunctionEventHandler(RunLoop::ProcessFunction* processFunction,RunLoop::ProcessFunction::EventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Replace the process function's event handler: */
		processFunction->eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::SetProcessFunctionEventHandler);
		pm.setProcessFunctionEventHandler.processFunction=processFunction;
		pm.setProcessFunctionEventHandler.eventHandler=&newEventHandler;
		writePipeMessage(pm,__PRETTY_FUNCTION__,processFunction,&newEventHandler);
		}
	}

bool RunLoop::handlePipeMessages(void)
	{
	/* Read a batch of messages from the self-pipe: */
	PipeMessage messageBuffer[messageBufferSize];
	ssize_t readResult=read(pipeFds[0],messageBuffer,messageBufferSize*sizeof(PipeMessage));
	if(readResult<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot read from event pipe");
	
	/* Check for end-of-file on the pipe during shutdown: */
	if(readResult==0)
		return false;
	
	for(PipeMessage* pmPtr=messageBuffer;size_t(readResult)>=sizeof(PipeMessage);++pmPtr,readResult-=sizeof(PipeMessage))
		{
		/* Handle the message based on its type: */
		switch(pmPtr->messageType)
			{
			case PipeMessage::WakeUp:
				/* Do nothing... */
				
				break;
			
			case PipeMessage::Stop:
				/* Set the shutdown flag: */
				shutdownRequested=true;
				
				break;
			
			case PipeMessage::SetIOWatcherEventMask:
				{
				/* Retrieve a pointer to the I/O watcher from the pipe message: */
				IOWatcher* ioWatcher=pmPtr->setIOWatcherEventMask.ioWatcher;
				
				/* Retrieve the new event mask from the pipe message: */
				unsigned int newEventMask=pmPtr->setIOWatcherEventMask.newEventMask;
					
				/* Check that the I/O watcher is enabled: */
				if(ioWatcher->enabled)
					{
					/* Update the I/O watcher's poll request: */
					setPollRequestEvents(pollFds[ioWatcher->activeIndex+1],newEventMask);
					}
				
				/* Update the I/O watcher's event mask: */
				ioWatcher->eventMask=newEventMask;
				
				/* Drop the message's reference to the I/O watcher: */
				ioWatcher->unref();
				
				break;
				}
			
			case PipeMessage::EnableIOWatcher:
				{
				/* Retrieve a pointer to the I/O watcher from the pipe message: */
				IOWatcher* ioWatcher=pmPtr->enableIOWatcher.ioWatcher;
				
				/* Check that the I/O watcher is not already enabled and still has an owner: */
				if(!ioWatcher->enabled&&ioWatcher->isOwned())
					{
					/* Append an entry for the I/O watcher to the end of the active lists: */
					ActiveIOWatcher newIOWatcher;
					newIOWatcher.ioWatcher=ioWatcher;
					activeIOWatchers.push_back(newIOWatcher);
					ioWatcher->ref(); // Add another reference to the I/O watcher
					
					/* Append a poll request for the I/O watcher to the end of the poll request list: */
					struct pollfd pollFd;
					pollFd.fd=ioWatcher->fd;
					setPollRequestEvents(pollFd,ioWatcher->eventMask);
					pollFd.revents=0x0;
					pollFds.push_back(pollFd);
					
					/* Set the I/O watcher's position in the active list and increase the number of active I/O watchers: */
					ioWatcher->activeIndex=numActiveIOWatchers;
					++numActiveIOWatchers;
					
					/* Mark the I/O watcher as enabled: */
					ioWatcher->enabled=true;
					}
				
				/* Drop the message's reference to the I/O watcher: */
				ioWatcher->unref();
				
				break;
				}
			
			case PipeMessage::DisableIOWatcher:
				{
				/* Retrieve a pointer to the I/O watcher from the pipe message: */
				IOWatcher* ioWatcher=pmPtr->disableIOWatcher.ioWatcher;
				
				/* Check that the I/O watcher is not already disabled: */
				if(ioWatcher->enabled)
					{
					/* Replace the I/O watcher's entries in the active I/O watchers list and the poll request list with the last entries in the respective lists, then drop the last entries: */
					unsigned int aiowi=ioWatcher->activeIndex;
					activeIOWatchers[aiowi]=activeIOWatchers[numActiveIOWatchers-1];
					activeIOWatchers.pop_back();
					pollFds[aiowi+1]=pollFds[numActiveIOWatchers]; // Take account of the extra self-pipe entry at the head of the list
					pollFds.pop_back();
					activeIOWatchers[aiowi].ioWatcher->activeIndex=aiowi;
					--numActiveIOWatchers;
					
					/* Mark the I/O watcher as disabled: */
					ioWatcher->enabled=false;
					
					/* Drop the active I/O watcher list's reference to the I/O watcher: */
					ioWatcher->unref();
					}
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(pmPtr->disableIOWatcher.cond!=0)
					pmPtr->disableIOWatcher.cond->signal();
				
				/* Drop the message's reference to the I/O watcher: */
				ioWatcher->unref();
				
				break;
				}
			
			case PipeMessage::SetIOWatcherEventHandler:
				{
				/* Retrieve a pointer to the I/O watcher from the pipe message: */
				IOWatcher* ioWatcher=pmPtr->setIOWatcherEventHandler.ioWatcher;
				
				/* Replace the I/O watcher's event handler: */
				ioWatcher->eventHandler=pmPtr->setIOWatcherEventHandler.eventHandler;
				
				/* Drop the message's references to the I/O watcher and the event handler: */
				ioWatcher->eventHandler->unref();
				ioWatcher->unref();
				
				break;
				}
			
			case PipeMessage::SetTimerTimeout:
				{
				/* Retrieve a pointer to the timer from the pipe message: */
				Timer* timer=pmPtr->setTimerTimeout.timer;
				
				/* Set the timer's time-out and ensure that the new time-out is not before lastDispatchTime: */
				timer->timeout=Time(pmPtr->setTimerTimeout.timeout);
				if(timer->timeout<lastDispatchTime)
					timer->timeout=lastDispatchTime;
				
				/* If the timer is enabled, fix the active timer heap: */
				if(timer->enabled)
					updateActiveTimer(timer);
				
				/* Drop the message's reference to the timer: */
				timer->unref();
				
				break;
				}
			
			case PipeMessage::SetTimerInterval:
				{
				/* Retrieve a pointer to the timer from the pipe message: */
				Timer* timer=pmPtr->setTimerInterval.timer;
				
				/* Set the timer's interval: */
				timer->interval=Interval(pmPtr->setTimerInterval.interval);
				
				/* Drop the message's reference to the timer: */
				timer->unref();
				
				break;
				}
			
			case PipeMessage::EnableTimer:
				{
				/* Retrieve a pointer to the timer from the pipe message: */
				Timer* timer=pmPtr->enableTimer.timer;
				
				/* Check that the timer is not already enabled and still has an owner: */
				if(!timer->enabled&&timer->isOwned())
					{
					/* Ensure that the timer's time-out is not before lastDispatchTime: */
					if(timer->timeout<lastDispatchTime)
						timer->timeout=lastDispatchTime;
					
					/* Insert the timer into the active timers heap: */
					insertActiveTimer(timer,timer->timeout);
					timer->ref(); // Take another reference to the timer
					
					/* Mark the timer as enabled: */
					timer->enabled=true;
					}
				
				/* Drop the message's reference to the timer: */
				timer->unref();
				
				break;
				}
			
			case PipeMessage::DisableTimer:
				{
				/* Retrieve a pointer to the timer from the pipe message: */
				Timer* timer=pmPtr->disableTimer.timer;
				
				/* Check that the timer is not already disabled: */
				if(timer->enabled)
					{
					/* Remove the timer from the active timers heap by moving the last active timer in the heap to its heap slot: */
					Timer* lastTimer=activeTimers[activeTimers.size()-1].timer;
					activeTimers.pop_back();
					if(lastTimer!=timer)
						{
						/* Let the last timer take over the to-be-removed timer's heap slot and then fix the heap: */
						lastTimer->activeIndex=timer->activeIndex;
						updateActiveTimer(lastTimer);
						}
					
					/* Drop the active timer heap's reference to the timer: */
					timer->unref();
					
					/* Mark the timer as disabled: */
					timer->enabled=false;
					}
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(pmPtr->disableTimer.cond!=0)
					pmPtr->disableTimer.cond->signal();
				
				/* Drop the message's reference to the timer: */
				timer->unref();
				
				break;
				}
			
			case PipeMessage::SetTimerEventHandler:
				{
				/* Retrieve a pointer to the timer from the pipe message: */
				Timer* timer=pmPtr->setTimerEventHandler.timer;
				
				/* Replace the timer's event handler: */
				timer->eventHandler=pmPtr->setTimerEventHandler.eventHandler;
				
				/* Drop the message's references to the timer and the event handler: */
				timer->eventHandler->unref();
				timer->unref();
				
				break;
				}
			
			case PipeMessage::EnableSignalHandler:
				{
				/* Retrieve a pointer to the OS signal handler from the pipe message: */
				SignalHandler* signalHandler=pmPtr->enableSignalHandler.signalHandler;
				
				/* Check that the OS signal handler is not already enabled and still has an owner: */
				if(!signalHandler->enabled&&signalHandler->isOwned())
					{
					/* Mark the OS signal handler as enabled: */
					signalHandler->enabled=true;
					}
				
				/* Drop the message's reference to the OS signal handler: */
				signalHandler->unref();
				
				break;
				}
			
			case PipeMessage::DisableSignalHandler:
				{
				/* Retrieve a pointer to the OS signal handler from the pipe message: */
				SignalHandler* signalHandler=pmPtr->disableSignalHandler.signalHandler;
				
				/* Mark the OS signal handler as disabled: */
				signalHandler->enabled=true;
				
				/* If the caller provided a condition variable for synchronization, unregister the OS signal, then signal it: */
				if(pmPtr->disableSignalHandler.cond!=0)
					{
					/* Lock the OS signal handler table: */
					Threads::Mutex::Lock signalHandlersLock(signalHandlersMutex);
					
					/* Unregister this run loop and the OS signal handler: */
					int signum=signalHandler->signum;
					registeredSignalHandlers[signum].runLoop=0;
					registeredSignalHandlers[signum].signalHandler=0;
					
					/* Return the OS signal to default disposition: */
					struct sigaction sigAction;
					memset(&sigAction,0,sizeof(struct sigaction));
					sigAction.sa_handler=SIG_DFL;
					if(sigaction(signum,&sigAction,0)<0)
						Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Cannot restore OS signal %d",signum);
					
					/* Signal the condition variable: */
					pmPtr->disableSignalHandler.cond->signal();
					
					/* Drop the OS signal handler table's reference to the OS signal handler: */
					signalHandler->unref();
					}
				
				/* Drop the message's reference to the OS signal handler: */
				signalHandler->unref();
				
				break;
				}
			
			case PipeMessage::SetSignalHandlerEventHandler:
				{
				/* Retrieve a pointer to the signal handler from the pipe message: */
				SignalHandler* signalHandler=pmPtr->setSignalHandlerEventHandler.signalHandler;
				
				/* Replace the signal handler's event handler: */
				signalHandler->eventHandler=pmPtr->setSignalHandlerEventHandler.eventHandler;
				
				/* Drop the message's references to the OS signal handler and the event handler: */
				signalHandler->eventHandler->unref();
				signalHandler->unref();
				
				break;
				}
			
			case PipeMessage::Signal:
				{
				/* Retrieve the OS signal handler from the OS signal handler table: */
				int signum=pmPtr->signal.signum;
				bool isForUs=false;
				SignalHandler* signalHandler=0;
				{
				Threads::Mutex::Lock signalHandlersLock(signalHandlersMutex);
				isForUs=registeredSignalHandlers[signum].runLoop==this;
				signalHandler=registeredSignalHandlers[signum].signalHandler;
				}
				
				/* Double-check that this signal is really for us: */
				if(isForUs)
					{
					/* Check if there is an OS signal handler: */
					if(signalHandler!=0)
						{
						/* Call the OS signal handler if it is enabled: */
						if(signalHandler->enabled)
							{
							SignalHandler::Event event(signalHandler,lastDispatchTime,signum);
							(*signalHandler->eventHandler)(event);
							}
						}
					else
						{
						/* Set the shutdown flag: */
						shutdownRequested=true;
						}
					}
				
				break;
				}
			
			case PipeMessage::EnableUserSignal:
				{
				/* Retrieve a pointer to the user signal from the pipe message: */
				UserSignal* userSignal=pmPtr->enableUserSignal.userSignal;
				
				/* Check that the user signal is not already enabled and still has an owner: */
				if(!userSignal->enabled&&userSignal->isOwned())
					{
					/* Mark the user signal as enabled: */
					userSignal->enabled=true;
					}
				
				/* Drop the message's reference to the user signal: */
				userSignal->unref();
				
				break;
				}
			
			case PipeMessage::DisableUserSignal:
				{
				/* Retrieve a pointer to the user signal from the pipe message: */
				UserSignal* userSignal=pmPtr->disableUserSignal.userSignal;
				
				/* Mark the user signal as disabled: */
				userSignal->enabled=true;
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(pmPtr->disableUserSignal.cond!=0)
					pmPtr->disableUserSignal.cond->signal();
				
				/* Drop the message's reference to the user signal: */
				userSignal->unref();
				
				break;
				}
			
			case PipeMessage::SetUserSignalEventHandler:
				{
				/* Retrieve a pointer to the user signal from the pipe message: */
				UserSignal* userSignal=pmPtr->setUserSignalEventHandler.userSignal;
				
				/* Replace the user signal's event handler: */
				userSignal->eventHandler=pmPtr->setUserSignalEventHandler.eventHandler;
				
				/* Drop the message's references to the user signal and the event handler: */
				userSignal->eventHandler->unref();
				userSignal->unref();
				
				break;
				}
			
			case PipeMessage::SignalUserSignal:
				{
				/* Retrieve a pointer to the user signal from the pipe message: */
				UserSignal* userSignal=pmPtr->signalUserSignal.userSignal;
				
				/* Check if the user signal is enabled: */
				if(userSignal->enabled)
					{
					/* Call the user signal's event handler: */
					UserSignal::Event event(userSignal,lastDispatchTime,pmPtr->signalUserSignal.signalData);
					(*userSignal->eventHandler)(event);
					}
				
				/* Drop the message's references to the user signal and the signal data: */
				if(pmPtr->signalUserSignal.signalData!=0)
					pmPtr->signalUserSignal.signalData->unref();
				userSignal->unref();
				
				break;
				}
			
			case PipeMessage::SetProcessFunctionSpinning:
				{
				/* Retrieve a pointer to the process function and the new spinning flag from the pipe message: */
				ProcessFunction* processFunction=pmPtr->setProcessFunctionSpinning.processFunction;
				bool newSpinning=pmPtr->setProcessFunctionSpinning.spinning;
				
				/* Check that the spinning status actually changed: */
				if(processFunction->spinning!=newSpinning)
					{
					/* Check if the process function is currently enabled: */
					if(processFunction->enabled)
						{
						/* Update the number of spinning process functions: */
						if(newSpinning)
							++numSpinningProcessFunctions;
						else
							--numSpinningProcessFunctions;
						}
					
					/* Update the process function's spinning flag: */
					processFunction->spinning=newSpinning;
					}
				
				/* Drop the message's reference to the process function: */
				processFunction->unref();
				
				break;
				}
			
			case PipeMessage::EnableProcessFunction:
				{
				/* Retrieve a pointer to the process function from the pipe message: */
				ProcessFunction* processFunction=pmPtr->enableProcessFunction.processFunction;
				
				/* Check that the process function is not already enabled and still has an owner: */
				if(!processFunction->enabled&&processFunction->isOwned())
					{
					/* Append an entry for the process function to the end of the active list: */
					activeProcessFunctions.push_back(ActiveProcessFunction(processFunction));
					processFunction->ref(); // Take another reference to the process function
					
					/* Set the process function's position in the active list: */
					processFunction->activeIndex=activeProcessFunctions.size()-1;
					
					/* Mark the process function as enabled: */
					processFunction->enabled=true;
					}
				
				/* Drop the message's reference to the process function: */
				processFunction->unref();
				
				break;
				}
			
			case PipeMessage::DisableProcessFunction:
				{
				/* Retrieve a pointer to the process function from the pipe message: */
				ProcessFunction* processFunction=pmPtr->disableProcessFunction.processFunction;
				
				/* Check that the process function is not already disabled: */
				if(processFunction->enabled)
					{
					/* Replace the process function's entry in the active process function list with the last entry in the list, then drop the last entry: */
					unsigned int apfi=processFunction->activeIndex;
					activeProcessFunctions[apfi]=activeProcessFunctions[activeProcessFunctions.size()-1];
					activeProcessFunctions[apfi].processFunction->activeIndex=apfi;
					activeProcessFunctions.pop_back();
					
					/* Drop the active process function list's reference to the process function: */
					processFunction->unref();
					
					/* Mark the process function as disabled: */
					processFunction->enabled=false;
					}
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(pmPtr->disableProcessFunction.cond!=0)
					pmPtr->disableProcessFunction.cond->signal();
				
				/* Drop the message's reference to the process function: */
				processFunction->unref();
				
				break;
				}
			
			case PipeMessage::SetProcessFunctionEventHandler:
				{
				/* Retrieve a pointer to the process function from the pipe message: */
				ProcessFunction* processFunction=pmPtr->setProcessFunctionEventHandler.processFunction;
				
				/* Replace the process function's event handler: */
				processFunction->eventHandler=pmPtr->setProcessFunctionEventHandler.eventHandler;
				
				/* Drop the message's references to the process function and the event handler: */
				processFunction->eventHandler->unref();
				processFunction->unref();
				
				break;
				}
			}
		}
	
	/* Check for partial reads, which should never happen: */
	if(readResult>0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Partial read from event pipe");
	
	return true;
	}

RunLoop::RunLoop(void)
	:threadId(Threads::Thread::getSelfId()),
	 pipeClosed(false),
	 messageBuffer(new PipeMessage[messageBufferSize]),
	 numActiveIOWatchers(0),
	 numSpinningProcessFunctions(0),
	 shutdownRequested(false),
	 handlingIOWatchers(false),
	 handlingProcessFunctions(false)
	{
	/* Create the self-pipe: */
	pipeFds[1]=pipeFds[0]=-1;
	if(pipe(pipeFds)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create event pipe");
	
	/* Create the permanent poll request for the self-pipe's read end: */
	struct pollfd pollFd;
	pollFd.fd=pipeFds[0];
	pollFd.events=POLLIN;
	pollFds.push_back(pollFd);
	}

RunLoop::~RunLoop(void)
	{
	/* Drain and close the self-pipe if it hasn't been done already: */
	if(!pipeClosed)
		{
		/* Close the write end of the self-pipe: */
		close(pipeFds[1]);
		
		/* Handle any remaining messages in the self-pipe: */
		while(handlePipeMessages())
			;
		
		/* Close the read end of the self-pipe: */
		close(pipeFds[0]);
		}
	
	/* Drop all references held by the active I/O watcher list: */
	for(unsigned int i=0;i<numActiveIOWatchers;++i)
		activeIOWatchers[i].ioWatcher->unref();
	
	/* Drop all references held by the active timer heap: */
	for(size_t i=0;i<activeTimers.size();++i)
		activeTimers[i].timer->unref();
	
	/* Unregister this run loop from all OS signals: */
	{
	Threads::Mutex::Lock signalHandlersLock(signalHandlersMutex);
	for(int signum=0;signum<=maxSignal;++signum)
		if(registeredSignalHandlers[signum].runLoop==this)
			{
			/* Unregister this run loop: */
			registeredSignalHandlers[signum].runLoop=0;
			SignalHandler* signalHandler=registeredSignalHandlers[signum].signalHandler;
			registeredSignalHandlers[signum].signalHandler=0;
			
			/* Return the signal to default disposition: */
			struct sigaction sigAction;
			memset(&sigAction,0,sizeof(struct sigaction));
			sigAction.sa_handler=SIG_DFL;
			if(sigaction(signum,&sigAction,0)<0)
				Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Cannot restore OS signal %d",signum);
			
			/* Drop the OS signal table's reference to the OS signal handler: */
			if(signalHandler!=0)
				signalHandler->unref();
			}
	}
	
	/* Drop all references held by the active process function list: */
	for(size_t i=0;i<activeProcessFunctions.size();++i)
		activeProcessFunctions[i].processFunction->unref();
	
	/* Release the message buffer: */
	delete[] messageBuffer;
	}

RunLoop::IOWatcher* RunLoop::createIOWatcher(int fd,unsigned int eventMask,bool enabled,RunLoop::IOWatcher::EventHandler& eventHandler)
	{
	/* Create and return a new I/O watcher object: */
	return new IOWatcher(*this,fd,eventMask,enabled,eventHandler);
	}

RunLoop::Timer* RunLoop::createTimer(const RunLoop::Time& timeout,RunLoop::Timer::EventHandler& eventHandler)
	{
	/* Create and return a new timer object: */
	return new Timer(*this,timeout,Interval(0,0),true,eventHandler);
	}

RunLoop::Timer* RunLoop::createTimer(const RunLoop::Time& timeout,const RunLoop::Interval& interval,bool enabled,RunLoop::Timer::EventHandler& eventHandler)
	{
	/* Create and return a new timer object: */
	return new Timer(*this,timeout,interval,enabled,eventHandler);
	}

RunLoop::SignalHandler* RunLoop::createSignalHandler(int signum,bool enabled,RunLoop::SignalHandler::EventHandler& eventHandler)
	{
	/* Check if the signal number is valid: */
	if(signum<0||signum>maxSignal)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid OS signal number %d",signum);
	
	/* Lock the OS signal handler table: */
	{
	Threads::Mutex::Lock signalHandlersLock(signalHandlersMutex);
	
	/* Check if there is already a run loop registered for the given OS signal: */
	if(registeredSignalHandlers[signum].runLoop!=0)
		{
		if(registeredSignalHandlers[signum].runLoop!=this)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled by another run loop",signum);
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled",signum);
		}
	
	/* Register this run loop and a new OS signal handler for the given signal: */
	registeredSignalHandlers[signum].runLoop=this;
	registeredSignalHandlers[signum].signalHandler=new SignalHandler(*this,signum,enabled,eventHandler);
	registeredSignalHandlers[signum].signalHandler->ref(); // Take a reference to the new OS signal handler
	
	/* Capture the given signal: */
	struct sigaction sigAction;
	memset(&sigAction,0,sizeof(struct sigaction));
	sigAction.sa_handler=signalHandlerFunction;
	if(sigaction(signum,&sigAction,0)<0)
		{
		/* Unregister this run loop and destroy the new signal handler again and throw an exception: */
		registeredSignalHandlers[signum].runLoop=0;
		registeredSignalHandlers[signum].signalHandler->unref();
		registeredSignalHandlers[signum].signalHandler=0;
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot intercept OS signal %d",signum);
		}
	
	/* Return the new signal handler: */
	return registeredSignalHandlers[signum].signalHandler;
	}
	}

RunLoop::UserSignal* RunLoop::createUserSignal(bool enabled,RunLoop::UserSignal::EventHandler& eventHandler)
	{
	/* Create and return a new user signal object: */
	return new UserSignal(*this,enabled,eventHandler);
	}

RunLoop::ProcessFunction* RunLoop::createProcessFunction(bool spinning,bool enabled,RunLoop::ProcessFunction::EventHandler& eventHandler)
	{
	/* Create and return a new process function object: */
	return new ProcessFunction(*this,spinning,enabled,eventHandler);
	}

void RunLoop::stopOnSignal(int signum)
	{
	/* Check if the signal number is valid: */
	if(signum<0||signum>maxSignal)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid OS signal number %d",signum);
	
	/* Lock the signal handler table: */
	{
	Threads::Mutex::Lock signalHandlersLock(signalHandlersMutex);
	
	/* Check if there is already a run loop registered for the given signal: */
	if(registeredSignalHandlers[signum].runLoop!=0)
		{
		if(registeredSignalHandlers[signum].runLoop!=this)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled by another run loop",signum);
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled",signum);
		}
	
	/* Register this run loop for the given signal: */
	registeredSignalHandlers[signum].runLoop=this;
	registeredSignalHandlers[signum].signalHandler=0;
	
	/* Capture the given signal: */
	struct sigaction sigAction;
	memset(&sigAction,0,sizeof(struct sigaction));
	sigAction.sa_handler=signalHandlerFunction;
	if(sigaction(signum,&sigAction,0)<0)
		{
		/* Unregister this run loop again and throw an exception: */
		registeredSignalHandlers[signum].runLoop=0;
		registeredSignalHandlers[signum].signalHandler=0;
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot intercept OS signal %d",signum);
		}
	}
	}

void RunLoop::wakeUp(void)
	{
	/* Check if this call was made from outside the run loop's thread: */
	if(!Threads::Thread::isSelfEqual(threadId))
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::WakeUp);
		writePipeMessage(pm,__PRETTY_FUNCTION__);
		}
	}

void RunLoop::stop(void)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Set the shutdown flag: */
		shutdownRequested=true;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::Stop);
		writePipeMessage(pm,__PRETTY_FUNCTION__);
		}
	}

bool RunLoop::dispatchNextEvents(void)
	{
	/* Check if there are active timers: */
	if(!activeTimers.empty())
		{
		/* Sample the current time: */
		lastDispatchTime.set();
		
		/* Handle all elapsed active timers, i.e., timers whose time-out is strictly before the current time: */
		while(!activeTimers.empty()&&activeTimers[0].timeout<lastDispatchTime)
			{
			/* Create an event descriptor structure: */
			Timer::Event event(activeTimers[0].timer,lastDispatchTime,activeTimers[0].timeout);
			
			/* Check if this is a repeating time-out: */
			bool dropRef=false;
			if(event.timer->interval.tv_sec!=0||event.timer->interval.tv_nsec!=0)
				{
				/* Advance the timer's time-out, and bump it to lastDispatchTime if it's too early: */
				event.timer->timeout+=event.timer->interval;
				if(event.timer->timeout<lastDispatchTime)
					event.timer->timeout=lastDispatchTime;
				
				/* Re-schedule the timer for the next time-out: */
				replaceFirstActiveTimer(event.timer,event.timer->timeout);
				}
			else
				{
				/* Disable the timer: */
				event.timer->enabled=false;
				
				/* Replace the timer's entry in the active timers heap by re-inserting the current last element in the heap's array: */
				ActiveTimer last=activeTimers[activeTimers.size()-1];
				activeTimers.pop_back();
				replaceFirstActiveTimer(last.timer,last.timeout);
				
				/* Drop the active timer heap's reference to the timer after the callback returns: */
				dropRef=true;
				}
			
			/* Call the timer's event handler: */
			(*event.timer->eventHandler)(event);
			
			if(dropRef)
				event.timer->unref();
			}
		}
	
	/* Bail out right before blocking if shutdown was requested: */
	if(shutdownRequested)
		return false;
	
	#if 1 // On Linux, we have ppoll()
	
	/* Calculate a time-out for the poll() call: */
	Interval pollTimeout(0,0); // In case we don't want to block, only poll
	Interval* pt=0; // Assume that we'll block forever
	if(numSpinningProcessFunctions>0)
		{
		/* Don't block for I/O events; only poll: */
		pt=&pollTimeout;
		}
	else if(!activeTimers.empty())
		{
		/* Calculate the interval from now to the next timer to elapse: */
		lastDispatchTime.set(); // We can't re-use the previous lastDispatchTime sample because time has passed in timer event handling
		if(activeTimers[0].timeout>lastDispatchTime)
			pollTimeout=activeTimers[0].timeout-lastDispatchTime;
		pt=&pollTimeout;
		}
	
	/* Block until an I/O event occurs or the time-out expires: */
	int pollResult=ppoll(pollFds.data(),numActiveIOWatchers+1,pt,0); // Account for the extra watcher for the self-pipe's read end
	
	#else
	
	/* Calculate a time-out for the poll() call: */
	int pollTimeout=-1; // Assume that we'll block forever
	if(numSpinningProcessFunctions>0)
		{
		/* Don't block for I/O events; only poll: */
		pollTimeout=0;
		}
	else if(!activeTimers.empty())
		{
		/* Calculate the interval from now to the next timer to elapse: */
		lastDispatchTime.set(); // We can't re-use the previous lastTispatchTime sample because time has passed in timer event handling
		if(activeTimers[0].timeout>lastDispatchTime)
			{
			Realtime::TimeVector timeout=activeTimers[0].timeout-lastDispatchTime;
			pollTimeout=int(timeout.tv_sec*1000L+(timeout.tv_nsec+999999L)/1000000L); // poll() takes timeouts in ms, which is a tad unfortunate
			}
		else
			{
			/* Don't block for I/O events; only poll: */
			pollTimeout=0;
			}
		}
	
	/* Block until an I/O event occurs or the time-out expires: */
	int pollResult=poll(pollFds.data(),numActiveIOWatchers+1,pollTimeout); // Account for the extra watcher for the self-pipe's read end
	
	#endif
	
	/* Sample the current time: */
	lastDispatchTime.set();
	
	/* Handle messages on the self-pipe: */
	if((pollFds[0].revents&POLLIN)!=0x0)
		handlePipeMessages();
	
	/* Handle all active I/O watchers: */
	handlingIOWatchers=true;
	IOWatcher::Event event(lastDispatchTime);
	for(handledIOWatcherIndex=0;handledIOWatcherIndex<numActiveIOWatchers;++handledIOWatcherIndex)
		if(pollFds[handledIOWatcherIndex+1].revents!=0x0)
			{
			/* Set the I/O watcher who is to receive this event: */
			event.ioWatcher=activeIOWatchers[handledIOWatcherIndex].ioWatcher;
			
			/* Set the bit mask of events that actually occurred and mask out events in which the I/O watcher is not interested: */
			event.eventMask=getPollRequestEvents(pollFds[handledIOWatcherIndex+1]);
			event.eventMask&=event.ioWatcher->eventMask|IOWatcher::ProblemMask; // Can't mask out "problem" indicators
			
			/* Call the I/O watcher's event handler: */
			(*event.ioWatcher->eventHandler)(event);
			}
	handlingIOWatchers=false;
	
	/* Handle all active process functions: */
	handlingProcessFunctions=true;
	for(handledProcessFunctionIndex=0;handledProcessFunctionIndex<activeProcessFunctions.size();++handledProcessFunctionIndex)
		{
		/* Call the process function's event handler: */
		ProcessFunction* pf=activeProcessFunctions[handledProcessFunctionIndex].processFunction;
		(*pf->eventHandler)(*pf);
		}
	handlingProcessFunctions=false;
	
	return !shutdownRequested;
	}

void RunLoop::run(void)
	{
	/* Check if the self-pipe needs to be re-opened: */
	if(pipeClosed)
		{
		/* Create the self-pipe: */
		pipeFds[1]=pipeFds[0]=-1;
		if(pipe(pipeFds)<0)
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create event pipe");
		
		/* Re-enable the self-pipe's poll request: */
		pollFds[0].fd=pipeFds[0];
		
		/* Mark the self-pipe as open: */
		pipeClosed=false;
		}
	
	/* Reset the shutdown flag: */
	shutdownRequested=false;
	
	/* Handle batches of events until stopped: */
	while(dispatchNextEvents())
		;
	}

void RunLoop::shutdown(void)
	{
	/* Check that the self-pipe is not already closed: */
	if(!pipeClosed)
		{
		/* Close the write end of the self-pipe: */
		close(pipeFds[1]);
		
		/* Handle any remaining messages in the self-pipe: */
		while(handlePipeMessages())
			;
		
		/* Close the read end of the self-pipe: */
		close(pipeFds[0]);
		
		/* Disable the self-pipe's poll request: */
		pollFds[0].fd=-1; // This is the specified way to disable a poll request
		
		/* Mark the pipe as closed: */
		pipeClosed=true;
		}
	}

}

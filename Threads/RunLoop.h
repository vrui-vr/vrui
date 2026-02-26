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
#include <Realtime/Time.h>
#include <Threads/Ownable.h>
#include <Threads/Mutex.h>
#include <Threads/Thread.h>

/* Forward declarations: */
struct pollfd;
namespace Threads {
template <class ParameterParam>
class FunctionCall;
class RefCounted;
}

namespace Threads {

class RunLoop
	{
	/* Embedded classes: */
	public:
	typedef Realtime::TimePointMonotonic Time; // Type for absolute time points
	typedef Realtime::TimeVector Interval; // Type for time intervals
	
	class IOWatcher:public Ownable // Class to watch for I/O events on file descriptors
		{
		friend class RunLoop;
		
		/* Embedded classes: */
		public:
		enum EventType // Enumerated type for I/O event types
			{
			Read=0x01, // It is possible to read from the file descriptor; note: on some file types (sockets), a read call may still block unless the file is O_NONBLOCK
			Exception=0x02, // Some exception, such as arrival of priority out-of-band data, has occurred on the file descriptor
			Write=0x04, // It is possible to write to the file descriptor
			ReadWrite=0x05, // It is both possible to read from and write to the file descriptor; see note for Read
			Error=0x08, // Some error occurred on the file descriptor, such as watching the write end of a pipe when the read end has been closed
			HangUp=0x10, // The peer on the other end of a pipe or stream socket has closed its end of the channel
			Invalid=0x20, // The file descriptor is invalid because the file was closed
			ProblemMask=0x38 // Bit mask for "problem" events that get sent to signal handlers even if they weren't interested in them
			};
		
		class Event // Class for I/O event descriptors passed to event handlers
			{
			friend class RunLoop;
			
			/* Elements: */
			private:
			IOWatcher* ioWatcher; // Pointer to the I/O watcher associated with the event
			Time dispatchTime; // Time point at which the event was dispatched
			unsigned int eventMask; // The bit mask of I/O events that actually happened
			
			/* Constructors and destructors: */
			Event(const Time& sDispatchTime) // Creates an event for the given dispatch time; rest of the elements will be filled in later
				:dispatchTime(sDispatchTime)
				{
				}
			
			/* Methods: */
			public:
			IOWatcher& getIOWatcher(void) // Returns the I/O watcher associated with this event
				{
				return *ioWatcher;
				}
			RunLoop& getRunLoop(void) // Returns the run loop associated with this event
				{
				return ioWatcher->runLoop;
				}
			int getFd(void) const // Returns the file descripter on which the event happened
				{
				return ioWatcher->fd;
				}
			const Time& getDispatchTime(void) const // Returns the time point at which this event was dispatched
				{
				return dispatchTime;
				}
			unsigned int getEventMask(void) const // Returns the bit mask of events that actually occurred
				{
				return eventMask;
				}
			bool canRead(void) const // Returns true if the file descriptor can be read from
				{
				return (eventMask&Read)!=0x0U;
				}
			bool canWrite(void) const // Returns true if the file descriptor can be written to
				{
				return (eventMask&Write)!=0x0U;
				}
			bool hasException(void) const // Returns true if the file descriptor has an exception
				{
				return (eventMask&Exception)!=0x0U;
				}
			bool hadProblem(void) const // Returns true if there was some kind of problem with the file descriptor
				{
				return (eventMask&ProblemMask)!=0x0U;
				}
			bool hadError(void) const // Returns true if there was an error, such as watching the write end of a pipe when the read end has been closed
				{
				return (eventMask&Error)!=0x0U;
				}
			bool hadHangUp(void) const // Returns true if the peer on the other end of a pipe or stream socket has closed its end of the channel
				{
				return (eventMask&HangUp)!=0x0U;
				}
			bool isInvalid(void) const // Returns true if the watched file descriptor became invalid because the file was closed
				{
				return (eventMask&Invalid)!=0x0U;
				}
			};
		
		friend class Event;
		
		typedef Threads::FunctionCall<Event&> EventHandler; // Type for I/O event handlers
		
		/* Elements: */
		private:
		RunLoop& runLoop; // Reference to the run loop with which this I/O watcher is associated
		int fd; // File descriptor to watch for I/O events
		unsigned int eventMask; // Bit mask of I/O events in which the I/O watcher is interested
		bool enabled; // Flag whether the I/O watcher is currently enabled
		Misc::Autopointer<EventHandler> eventHandler; // The event handler
		unsigned int activeIndex; // Index of an enabled I/O watcher's entry in the run loop's active I/O watcher list
		
		/* Protected methods from class Ownable: */
		virtual void disowned(void);
		
		/* Constructors and destructors: */
		IOWatcher(RunLoop& sRunLoop,int sFd,unsigned int sEventMask,bool sEnabled,EventHandler& sEventHandler); // Elementwise constructor
		
		/* Methods: */
		public:
		RunLoop& getRunLoop(void) // Returns a reference to the run loop with which this I/O watcher is associated
			{
			return runLoop;
			}
		int getFd(void) const // Returns the file descriptor this I/O watcher is watching
			{
			return fd;
			}
		unsigned int getEventMask(void) const // Returns the bit mask of I/O events in which the I/O watcher is interested
			{
			return eventMask;
			}
		bool isEnabled(void) const // Returns true if the I/O watcher is currently enabled
			{
			return enabled;
			}
		void setEventMask(unsigned int newEventMask) // Sets the bit mask of I/O events in which the I/O watcher is interested
			{
			/* Delegate to the run loop: */
			runLoop.setIOWatcherEventMask(this,newEventMask);
			}
		void enable(void) // Enables the I/O watcher
			{
			/* Delegate to the run loop: */
			runLoop.enableIOWatcher(this);
			}
		void disable(void) // Disables the I/O watcher
			{
			/* Delegate to the run loop: */
			runLoop.disableIOWatcher(this);
			}
		void setEnabled(bool newEnabled) // Sets the I/O watcher's enabled state
			{
			/* Delegate to the run loop: */
			if(newEnabled)
				runLoop.enableIOWatcher(this);
			else
				runLoop.disableIOWatcher(this);
			}
		void setEventHandler(EventHandler& newEventHandler) // Sets the I/O watcher's event handler
			{
			/* Delegate to the run loop: */
			runLoop.setIOWatcherEventHandler(this,newEventHandler);
			}
		};
	
	friend class IOWatcher;
	
	typedef OwningPointer<IOWatcher> IOWatcherOwner; // Type for ownership-establishing pointers to I/O watchers
	typedef Misc::Autopointer<IOWatcher> IOWatcherPtr; // Type for non-ownership-establishing pointers to I/O watchers
	
	class Timer:public Ownable // Class to schedule timer events
		{
		friend class RunLoop;
		
		/* Embedded classes: */
		public:
		class Event // Class for timer event descriptors passed to event handlers
			{
			friend class RunLoop;
			
			/* Elements: */
			private:
			Timer* timer; // Pointer to the timer associated with the event
			Time dispatchTime; // Time point at which the event was dispatched
			Time scheduledTime; // Time at which the timer was scheduled to elapse
			
			/* Constructors and destructors: */
			Event(Timer* sTimer,const Time& sDispatchTime,const Time& sScheduledTime) // Creates an event for the given timer, dispatch time, and scheduled time-out
				:timer(sTimer),dispatchTime(sDispatchTime),scheduledTime(sScheduledTime)
				{
				}
			
			/* Methods: */
			public:
			Timer& getTimer(void) // Returns the timer associated with this event
				{
				return *timer;
				}
			RunLoop& getRunLoop(void) // Returns the run loop associated with this event
				{
				return timer->runLoop;
				}
			const Time& getDispatchTime(void) const // Returns the time point at which this event was dispatched
				{
				return dispatchTime;
				}
			const Time& getScheduledTime(void) const // Returns the time point at which the timer was scheduled to elapse
				{
				return scheduledTime;
				}
			};
		
		friend class Event;
		
		typedef Threads::FunctionCall<Event&> EventHandler; // Type for timer event handlers
		
		/* Elements: */
		private:
		RunLoop& runLoop; // Reference to the run loop with which this timer is associated
		Time timeout; // The next time point at which the timer elapses
		Interval interval; // Interval for recurring timers, or 0 if the timer is one-shot
		bool enabled; // Flag if the timer is enabled
		Misc::Autopointer<EventHandler> eventHandler; // The event handler
		unsigned int activeIndex; // Index of an enabled timer's entry in the run loop's active timer heap
		
		/* Protected methods from class Ownable: */
		virtual void disowned(void);
		
		/* Constructors and destructors: */
		Timer(RunLoop& sRunLoop,const Time& sTimeout,const Interval& sInterval,bool sEnabled,EventHandler& sEventHandler); // Elementwise constructor
		
		/* Methods: */
		public:
		RunLoop& getRunLoop(void) // Returns a reference to the run loop with which this timer is associated
			{
			return runLoop;
			}
		const Time& getTimeout(void) const // Returns the next time point at which the timer expires
			{
			return timeout;
			}
		const Interval& getInterval(void) const // Returns the interval for recurring timers, or 0 if the timer is one-shot
			{
			return interval;
			}
		bool isRecurring(void) const // Returns true if the timer is a recurring timer
			{
			/* Recurring timers have non-zero intervals: */
			return interval.tv_sec!=0||interval.tv_nsec!=0;
			}
		bool isEnabled(void) const // Returns true if the timer is currently enabled
			{
			return enabled;
			}
		void setTimeout(const Time& newTimeout,bool reenable =false) // Sets the next time point at which the timer expires; re-enables a disabled timer if the given flag is true
			{
			/* Delegate to the run loop: */
			runLoop.setTimerTimeout(this,newTimeout,reenable);
			}
		void setInterval(const Interval& newInterval) // Sets the timer interval for a recurring timer; an interval of 0 turns the timer into a one-shot timer that will automatically be disabled when it elapses
			{
			/* Delegate to the run loop: */
			runLoop.setTimerInterval(this,newInterval);
			}
		void enable(void) // Enables the timer
			{
			/* Delegate to the run loop: */
			runLoop.enableTimer(this);
			}
		void disable(void) // Disables the timer
			{
			/* Delegate to the run loop: */
			runLoop.disableTimer(this);
			}
		void setEnabled(bool newEnabled) // Sets the timer's enabled state
			{
			/* Delegate to the run loop: */
			if(newEnabled)
				runLoop.enableTimer(this);
			else
				runLoop.disableTimer(this);
			}
		void setEventHandler(EventHandler& newEventHandler) // Sets the timer's event handler
			{
			/* Delegate to the run loop: */
			runLoop.setTimerEventHandler(this,newEventHandler);
			}
		};
	
	friend class Timer;
	
	typedef OwningPointer<Timer> TimerOwner; // Type for ownership-establishing pointers to timers
	typedef Misc::Autopointer<Timer> TimerPtr; // Type for non-ownership-establishing pointers to timers
	
	class SignalHandler:public Ownable // Class to handle OS signals
		{
		friend class RunLoop;
		
		/* Embedded classes: */
		public:
		class Event // Class for signal event descriptors passed to event handlers
			{
			friend class RunLoop;
			
			/* Elements: */
			private:
			SignalHandler* signalHandler; // Pointer to the signal handler associated with the event
			Time dispatchTime; // Time point at which the event was dispatched
			int signum; // OS signal number that was raised (SIGINT, SIGTERM, ..., defined in signal.h)
			
			/* Constructors and destructors: */
			Event(SignalHandler* sSignalHandler,const Time& sDispatchTime,int sSignum) // Elementwise constructor
				:signalHandler(sSignalHandler),dispatchTime(sDispatchTime),signum(sSignum)
				{
				}
			
			/* Methods: */
			public:
			SignalHandler& getSignalHandler(void) // Returns the signal handler associated with this event
				{
				return *signalHandler;
				}
			RunLoop& getRunLoop(void) // Returns the run loop associated with this event
				{
				return signalHandler->runLoop;
				}
			const Time& getDispatchTime(void) const // Returns the time point at which this event was dispatched
				{
				return dispatchTime;
				}
			int getSignum(void) const // Returns the OS signal number that was raised (SIGINT, SIGTERM, ..., defined in signal.h)
				{
				return signum;
				}
			};
		
		friend class Event;
		
		typedef Threads::FunctionCall<Event&> EventHandler; // Type for signal handler event handlers
		
		/* Elements: */
		private:
		RunLoop& runLoop; // Reference to the run loop with which this signal handler is associated
		int signum; // OS signal number handled by the signal handler (SIGINT, SIGTERM, ..., defined in signal.h)
		bool enabled; // Flag if the signal handler is enabled
		Misc::Autopointer<EventHandler> eventHandler; // The event handler
		
		/* Protected methods from class Ownable: */
		virtual void disowned(void);
		
		/* Constructors and destructors: */
		SignalHandler(RunLoop& sRunLoop,int sSignum,bool sEnabled,EventHandler& sEventHandler); // Elementwise constructor
		
		/* Methods: */
		public:
		RunLoop& getRunLoop(void) // Returns a reference to the run loop with which this signal handler is associated
			{
			return runLoop;
			}
		int getSignum(void) const // Returns the OS signal number handled by the signal handler (SIGINT, SIGTERM, ..., defined in signal.h)
			{
			return signum;
			}
		bool isEnabled(void) const // Returns true if the signal handler is currently enabled
			{
			return enabled;
			}
		void enable(void) // Enables the signal handler
			{
			/* Delegate to the run loop: */
			runLoop.enableSignalHandler(this);
			}
		void disable(void) // Disables the signal handler
			{
			/* Delegate to the run loop: */
			runLoop.disableSignalHandler(this);
			}
		void setEnabled(bool newEnabled) // Sets the signal handler's enabled state
			{
			/* Delegate to the run loop: */
			if(newEnabled)
				runLoop.enableSignalHandler(this);
			else
				runLoop.disableSignalHandler(this);
			}
		void setEventHandler(EventHandler& newEventHandler) // Sets the signal handler's event handler
			{
			/* Delegate to the run loop: */
			runLoop.setSignalHandlerEventHandler(this,newEventHandler);
			}
		};
	
	friend class SignalHandler;
	
	typedef OwningPointer<SignalHandler> SignalHandlerOwner; // Type for ownership-establishing pointers to OS signal handlers
	typedef Misc::Autopointer<SignalHandler> SignalHandlerPtr; // Type for non-ownership-establishing pointers to OS signal handlers
	
	class UserSignal:public Ownable // Class for user-defined signals to synchronously notify clients of asynchronous events
		{
		friend class RunLoop;
		
		/* Embedded classes: */
		public:
		class Event // Class for user signal event descriptors passed to event handlers
			{
			friend class RunLoop;
			
			/* Elements: */
			private:
			UserSignal* userSignal; // Pointer to the user signal associated with the event
			Time dispatchTime; // Time point at which the event was dispatched
			RefCounted* signalData; // Pointer to the user-defined data that was sent along with the signal
			
			/* Constructors and destructors: */
			Event(UserSignal* sUserSignal,const Time& sDispatchTime,RefCounted* sSignalData) // Elementwise constructor
				:userSignal(sUserSignal),dispatchTime(sDispatchTime),signalData(sSignalData)
				{
				}
			
			/* Methods: */
			public:
			UserSignal& getUserSignal(void) // Returns the user signal associated with this event
				{
				return *userSignal;
				}
			RunLoop& getRunLoop(void) // Returns the run loop associated with this event
				{
				return userSignal->runLoop;
				}
			const Time& getDispatchTime(void) const // Returns the time point at which this event was dispatched
				{
				return dispatchTime;
				}
			template <class SignalDataParam>
			SignalDataParam& getSignalData(void) // Returns a dynamically cast reference to the signal data; throws exception if types don't match
				{
				/* Cast the signal data pointer and check for errors: */
				SignalDataParam* data=dynamic_cast<SignalDataParam*>(signalData);
				if(data==0)
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching signal data type");
				return *data;
				}
			};
		
		friend class Event;
		
		typedef Threads::FunctionCall<Event&> EventHandler; // Type for user signal event handlers
		
		/* Elements: */
		private:
		RunLoop& runLoop; // Reference to the run loop with which this user signal is associated
		bool enabled; // Flag if the user signal is enabled
		Misc::Autopointer<EventHandler> eventHandler; // The event handler
		
		/* Protected methods from class Ownable: */
		virtual void disowned(void);
		
		/* Constructors and destructors: */
		UserSignal(RunLoop& sRunLoop,bool enabled,EventHandler& sEventHandler); // Elementwise constructor
		
		/* Methods: */
		public:
		RunLoop& getRunLoop(void) // Returns a reference to the run loop with which this user signal is associated
			{
			return runLoop;
			}
		bool isEnabled(void) const // Returns true if the user signal is currently enabled
			{
			return enabled;
			}
		void enable(void) // Enables the user signal
			{
			/* Delegate to the run loop: */
			runLoop.enableUserSignal(this);
			}
		void disable(void) // Disables the user signal
			{
			/* Delegate to the run loop: */
			runLoop.disableUserSignal(this);
			}
		void setEnabled(bool newEnabled) // Sets the user signal's enabled state
			{
			/* Delegate to the run loop: */
			if(newEnabled)
				runLoop.enableUserSignal(this);
			else
				runLoop.disableUserSignal(this);
			}
		void setEventHandler(EventHandler& newEventHandler) // Sets the user signal's event handler
			{
			/* Delegate to the run loop: */
			runLoop.setUserSignalEventHandler(this,newEventHandler);
			}
		void signal(RefCounted& signalData) // Sends a signal with the given data to the user signal handler
			{
			/* Delegate to the run loop: */
			runLoop.signalUserSignal(this,signalData);
			}
		};
	
	friend class UserSignal;
	
	typedef OwningPointer<UserSignal> UserSignalOwner; // Type for ownership-establishing pointers to user signals
	typedef Misc::Autopointer<UserSignal> UserSignalPtr; // Type for non-ownership-establishing pointers to user signals
	
	class ProcessFunction:public Ownable // Class for functions that are called every time after the run loop processes events
		{
		friend class RunLoop;
		
		/* Embedded classes: */
		public:
		typedef Threads::FunctionCall<ProcessFunction&> EventHandler; // Type for process function event handlers
		
		/* Elements: */
		private:
		RunLoop& runLoop; // Reference to the run loop with which this process function is associated
		bool spinning; // Flag if this process function should be called continuously, preventing blocking in the run loop
		bool enabled; // Flag if the process function is enabled
		Misc::Autopointer<EventHandler> eventHandler; // The event handler
		unsigned int activeIndex; // Index of an enabled process function's entry in the run loop's active process function list
		
		/* Protected methods from class Ownable: */
		virtual void disowned(void);
		
		/* Constructors and destructors: */
		ProcessFunction(RunLoop& sRunLoop,bool sSpinning,bool sEnabled,EventHandler& sEventHandler); // Elementwise constructor
		
		/* Methods: */
		public:
		RunLoop& getRunLoop(void) // Returns a reference to the run loop with which this process function is associated
			{
			return runLoop;
			}
		bool isSpinning(void) const // Returns true if the process function wants to be called continuously, preventing blocking in the run loop
			{
			return spinning;
			}
		bool isEnabled(void) const // Returns true if the process function is currently enabled
			{
			return enabled;
			}
		void setSpinning(bool newSpinning) // Sets the process function's spinning request
			{
			/* Delegate to the run loop: */
			runLoop.setProcessFunctionSpinning(this,newSpinning);
			}
		void enable(void) // Enables the process function
			{
			/* Delegate to the run loop: */
			runLoop.enableProcessFunction(this);
			}
		void disable(void) // Disables the process function
			{
			/* Delegate to the run loop: */
			runLoop.disableProcessFunction(this);
			}
		void setEnabled(bool newEnabled) // Sets the process function's enabled state
			{
			/* Delegate to the run loop: */
			if(newEnabled)
				runLoop.enableProcessFunction(this);
			else
				runLoop.disableProcessFunction(this);
			}
		void setEventHandler(EventHandler& newEventHandler) // Sets the process function's event handler
			{
			/* Delegate to the run loop: */
			runLoop.setProcessFunctionEventHandler(this,newEventHandler);
			}
		};
	
	typedef OwningPointer<ProcessFunction> ProcessFunctionOwner; // Type for ownership-establishing pointers to process functions
	typedef Misc::Autopointer<ProcessFunction> ProcessFunctionPtr; // Type for non-ownership-establishing pointers to process functions
	
	private:
	struct PipeMessage; // Structure for messages sent on a run loop's self-pipe
	struct ActiveIOWatcher; // Structure keeping track of currently active I/O watchers
	typedef Misc::DynamicArray<ActiveIOWatcher> ActiveIOWatcherList; // Type for lists of active I/O watchers
	typedef Misc::DynamicArray<struct pollfd> PollFdList; // Type for lists of polling request structures
	struct ActiveTimer; // Structure keeping track of currently active timers
	struct RegisteredSignalHandler; // Structure keeping track of registered OS signal handlers
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
	unsigned int numActiveIOWatchers; // Number of active I/O watchers
	ActiveIOWatcherList activeIOWatchers; // List of currently active I/O watchers
	PollFdList pollFds; // List of polling request structures paralleling the list of active I/O watchers, with an extra entry at the beginning for the self-pipe's read end
	Misc::DynamicArray<ActiveTimer> activeTimers; // Priority heap of active timers sorted by next time-out; unfortunately, we can't use Misc::PriorityHeap due to the additional bookkeeping we require
	Threads::Mutex signalHandlersMutex; // Mutex protecting the global table of registered signal handlers
	static const int maxSignal=64; // Largest signum of any OS signal
	static RegisteredSignalHandler registeredSignalHandlers[maxSignal+1]; // Global table of registered OS signal handlers
	ActiveProcessFunctionList activeProcessFunctions; // List of currently active process functions
	unsigned int numSpinningProcessFunctions; // Number of currently active process functions that want to spin
	Time lastDispatchTime; // The last point in time for which any events were dispatched
	bool shutdownRequested; // Flag to request a running run loop to shut down
	bool handlingIOWatchers; // Flag if the dispatchNextEvents method is currently handling I/O watchers
	unsigned int handledIOWatcherIndex; // Index of the active I/O watcher whose event is currently being handled
	bool handlingProcessFunctions; // Flag if the dispatchNextEvents method is currently handling process functions
	unsigned int handledProcessFunctionIndex; // Index of the active process function which is currently being handled
	
	/* Private methods: */
	bool writePipeMessage(const PipeMessage& pm,const char* methodName,Ownable* messageSender =0,RefCounted* messageObject =0); // Writes a message to the self-pipe; adds references to the given sender and/or object if the write succeeds; returns false if the self-pipe was closed during run loop shutdown, throws exception on other errors
	
	/* Internal interface for I/O watchers: */
	void setIOWatcherEventMask(IOWatcher* ioWatcher,unsigned int newEventMask); // Sets bit mask of I/O events in which the given I/O watcher is interested
	void enableIOWatcher(IOWatcher* ioWatcher); // Enables the given I/O watcher
	void disableIOWatcher(IOWatcher* ioWatcher,bool willDestroy =false); // Disables the given I/O watcher; if the willDestroy flag is true, the caller will destroy the I/O watcher immediately after disabling it, requiring extra synchronization
	void setIOWatcherEventHandler(IOWatcher* ioWatcher,IOWatcher::EventHandler& newEventHandler); // Sets the given I/O watcher's event handler
	
	void insertActiveTimer(Timer* newTimer,const Time& newTimeout); // Inserts a new timer into the active timer heap
	void updateActiveTimer(Timer* timer); // Fixes the active timer heap after the given timer's time-out has been changed
	void replaceFirstActiveTimer(Timer* newTimer,const Time& newTimeout); // Replaces the first active timer in the active timer heap with the given timer
	
	/* Internal interface for I/O watchers: */
	void setTimerTimeout(Timer* timer,const Time& newTimeout,bool reenable); // Sets the given timer's next time-out; re-enables a disabled timer if the given flag is true
	void setTimerInterval(Timer* timer,const Interval& newInterval); // Sets the interval for a recurring timer; an interval of 0 marks the timer as a one-shot timer that will automatically be disabled when it elapses
	void enableTimer(Timer* timer); // Enables the given timer
	void disableTimer(Timer* timer,bool willDestroy =false); // Disables the given timer; if the willDestroy flag is true, the caller will destroy the timer immediately after disabling it, requiring extra synchronization
	void setTimerEventHandler(Timer* timer,Timer::EventHandler& newEventHandler); // Sets the given timer's event handler
	
	/* Internal interface for OS signal handlers: */
	void enableSignalHandler(SignalHandler* signalHandler); // Enables the given signal handler
	void disableSignalHandler(SignalHandler* signalHandler,bool willDestroy =false); // Disables the given signal handler; if the willDestroy flag is true, the caller will destroy the signal handler immediately after disabling it, requiring extra synchronization
	void setSignalHandlerEventHandler(SignalHandler* signalHandler,SignalHandler::EventHandler& newEventHandler); // Sets the given signal handler's event handler
	static void signalHandlerFunction(int signum); // OS signal handler function
	
	/* Internal interface for user signals: */
	void enableUserSignal(UserSignal* userSignal); // Enables the given user signal
	void disableUserSignal(UserSignal* userSignal,bool willDestroy =false); // Disables the given user signal; if the willDestroy flag is true, the caller will destroy the user signal immediately after disabling it, requiring extra synchronization
	void setUserSignalEventHandler(UserSignal* userSignal,UserSignal::EventHandler& newEventHandler); // Sets the given user signal's event handler
	void signalUserSignal(UserSignal* userSignal,RefCounted& signalData); // Calls the given user signal with the given signal data
	
	/* Internal interface for process functions: */
	void setProcessFunctionSpinning(ProcessFunction* processFunction,bool newSpinning); // Sets the spinning request flag for the given process function
	void enableProcessFunction(ProcessFunction* processFunction); // Enables the given process function
	void disableProcessFunction(ProcessFunction* processFunction,bool willDestroy =false); // Disables the given process function; if the willDestroy flag is true, the caller will destroy the process function immediately after disabling it, requiring extra synchronization
	void setProcessFunctionEventHandler(ProcessFunction* processFunction,ProcessFunction::EventHandler& newEventHandler); // Sets the given process function's event handler
	
	bool handlePipeMessages(void); // Handles a batch of messages received on the self-pipe; returns false when the self-pipe signals end-of-file during shutdown
	
	/* Constructors and destructors: */
	public:
	RunLoop(void); // Creates a run loop associated with the calling thread
	~RunLoop(void);
	
	/* Methods to create events handlers: */
	IOWatcher* createIOWatcher(int fd,unsigned int eventMask,bool enabled,IOWatcher::EventHandler& eventHandler); // Creates an I/O watcher
	Timer* createTimer(const Time& timeout,Timer::EventHandler& eventHandler); // Creates an enabled one-shot timer that will automatically be disabled when the timer elapses
	Timer* createTimer(const Time& timeout,const Interval& interval,bool enabled,Timer::EventHandler& eventHandler); // Creates a repeating timer
	SignalHandler* createSignalHandler(int signum,bool enabled,SignalHandler::EventHandler& eventHandler); // Creates an OS signal handler
	UserSignal* createUserSignal(bool enabled,UserSignal::EventHandler& eventHandler); // Creates a user signal
	ProcessFunction* createProcessFunction(bool spinning,bool enabled,ProcessFunction::EventHandler& eventHandler); // Creates a process function
	
	/* Methods to capture OS signals: */
	void stopOnSignal(int signum); // Instructs this run loop to stop if the OS signal of the given number is received; throws exception if another run loop already listens to that signal
	
	/* Dispatching methods: */
	void wakeUp(void); // Wakes up a potentially blocked run loop; dispatchNextEvents() call will return true
	void stop(void); // Orders the run loop to stop dispatching events; some subsequent dispatchNextEvents() call will return false
	bool dispatchNextEvents(void); // Dispatches the next batch of events, blocking on I/O at most once; returns true if the run loop has not been stopped
	void run(void); // Dispatches events until stopped by calling the stop() method
	void shutdown(void); // Called after a run loop has been stopped to drain the self-pipe and release all resources; is called internally by destructor as well
	};

}

#endif

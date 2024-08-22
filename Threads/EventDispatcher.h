/***********************************************************************
EventDispatcher - Class to dispatch events from a central listener to
any number of interested clients.
Copyright (c) 2016-2023 Oliver Kreylos

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

#ifndef THREADS_EVENTDISPATCHER_INCLUDED
#define THREADS_EVENTDISPATCHER_INCLUDED

#include <sys/types.h>
#include <sys/time.h>
#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <Misc/PriorityHeap.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/Spinlock.h>

namespace Threads {

class EventDispatcher
	{
	/* Embedded classes: */
	public:
	typedef unsigned int ListenerKey; // Type for keys to uniquely identify registered event listeners
	
	enum IOEventType // Enumerated type for input/output event types
		{
		Read=0x01,Write=0x02,ReadWrite=0x03,Exception=0x04
		};
	
	class Time:public timeval // Class to specify time points or time intervals for timer events; microseconds are assumed in [0, 1000000) even if interval is negative
		{
		/* Constructors and destructors: */
		public:
		Time(void) // Dummy constructor
			{
			}
		Time(long sSec,long sUsec) // Initializes time interval
			{
			tv_sec=sSec;
			tv_usec=sUsec;
			}
		Time(const struct timeval& tv) // Constructor from base class
			{
			tv_sec=tv.tv_sec;
			tv_usec=tv.tv_usec;
			}
		Time(double sSeconds); // Initializes time interval from non-negative number of seconds
		
		/* Methods: */
		static Time now(void); // Returns the current wall-clock time as a time point
		bool operator==(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec==other.tv_sec&&tv_usec==other.tv_usec;
			}
		bool operator!=(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec!=other.tv_sec||tv_usec!=other.tv_usec;
			}
		bool operator<=(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec<other.tv_sec||(tv_sec==other.tv_sec&&tv_usec<=other.tv_usec);
			}
		Time& operator+=(const Time& other) // Adds a time interval to a time point or another time interval
			{
			/* Add time components: */
			tv_sec+=other.tv_sec;
			tv_usec+=other.tv_usec;
			
			/* Handle overflow: */
			if(tv_usec>=1000000)
				{
				++tv_sec;
				tv_usec-=1000000;
				}
			return *this;
			}
		Time& operator-=(const Time& other) // Subtracts a time interval from a time point, or two time intervals, or two time points
			{
			/* Subtract time components: */
			tv_sec-=other.tv_sec;
			tv_usec-=other.tv_usec;
			
			/* Handle underflow: */
			if(tv_usec<0)
				{
				--tv_sec;
				tv_usec+=1000000;
				}
			return *this;
			}
		};
	
	/* Classes representing events handed to listener event callbacks: */
	class Event // Base class for event information passed to event callbacks
		{
		friend class EventDispatcher;
		
		/* Elements: */
		private:
		const Time& dispatchTime; // Time at which the event was dispatched
		ListenerKey key; // Key identifying the event
		void* userData; // Opaque data pointer registered during event listener creation
		
		/* Constructors and destructors: */
		public:
		Event(const Time& sDispatchTime) // Creates an event for the given dispatch time
			:dispatchTime(sDispatchTime)
			{
			}
		
		/* Methods: */
		const Time& getDispatchTime(void) const
			{
			return dispatchTime;
			}
		ListenerKey getKey(void) const
			{
			return key;
			}
		void* getUserData(void) const
			{
			return userData;
			}
		virtual void removeListener(void) =0; // Tells the event dispatcher to remove this event listener immediately after the callback returns
		};
	
	/* Forward declarations of specific event objects: */
	class IOEvent;
	class TimerEvent;
	class ProcessEvent;
	class SignalEvent;
	
	/* Declaration of event listener callback functions: */
	typedef void (*IOEventCallback)(IOEvent& event); // Type for input/output event callback functions
	typedef void (*TimerEventCallback)(TimerEvent& event); // Type for timer event callback functions
	typedef void (*ProcessCallback)(ProcessEvent& event); // Type for process callback functions
	typedef void (*SignalCallback)(SignalEvent& event); // Type for signal callback functions
	
	/* Helper functions to register class methods as callback functions, assuming that userData is a pointer to an object of the class: */
	template <class ClassParam,void (ClassParam::*methodParam)(IOEvent&)>
	static void wrapMethod(IOEvent& event) // Wraps an input/output event callback
		{
		(static_cast<ClassParam*>(event.getUserData())->*methodParam)(event);
		}
	template <class ClassParam,void (ClassParam::*methodParam)(TimerEvent&)>
	static void wrapMethod(TimerEvent& event) // Wraps a timer event callback
		{
		(static_cast<ClassParam*>(event.getUserData())->*methodParam)(event);
		}
	template <class ClassParam,void (ClassParam::*methodParam)(ProcessEvent&)>
	static void wrapMethod(ProcessEvent& event) // Wraps a process callback
		{
		(static_cast<ClassParam*>(event.getUserData())->*methodParam)(event);
		}
	template <class ClassParam,void (ClassParam::*methodParam)(SignalEvent&)>
	static void wrapMethod(SignalEvent& event) // Wraps a signal event callback
		{
		(static_cast<ClassParam*>(event.getUserData())->*methodParam)(event);
		}
	
	class IOEvent:public Event // Class for input/output events
		{
		friend class EventDispatcher;
		
		/* Elements: */
		private:
		int eventTypeMask; // Bit mask of events that happened on the registered input/output port
		
		/* Constructors and destructors: */
		public:
		IOEvent(const Time& sDispatchTime) // Creates an input/output event for the given dispatch time
			:Event(sDispatchTime)
			{
			}
		
		/* Methods: */
		int getEventTypeMask(void) const
			{
			return eventTypeMask;
			}
		virtual void setCallback(IOEventCallback newCallback,void* newUserData) =0; // Tells the event dispatcher to change this event listener's callback immediately after the callback returns
		virtual void setEventTypeMask(int newEventTypeMask) =0; // Tells the event dispatcher to change this event listener's event type mask of interest immediately after the callback returns
		};
	
	class TimerEvent:public Event // Structure for timer events
		{
		/* Elements: */
		private:
		
		/* Constructors and destructors: */
		public:
		TimerEvent(const Time& sDispatchTime) // Creates a timer event for the given dispatch time
			:Event(sDispatchTime)
			{
			}
		
		/* Methods: */
		virtual size_t getNumMissedEvents(void) const =0; // Returns the number of repeating event occurrences that were missed since the last callback
		virtual void setCallback(TimerEventCallback newCallback,void* newUserData) =0; // Tells the event dispatcher to change this event listener's callback immediately after the callback returns
		virtual void setEventTime(const Time& newEventTime) =0; // Tells the event dispatcher to change this event listener's next event time immediately after the callback returns
		virtual void setEventInterval(const Time& newEventInterval) =0; // Tells the event dispatcher to change this event listener's event interval immediately after the callback returns
		virtual void suspendListener(void) =0; // Suspends the timer event listener until it is resumed or removed
		};
	
	class ProcessEvent:public Event // Structure for process events
		{
		/* Elements: */
		private:
		
		/* Constructors and destructors: */
		public:
		ProcessEvent(const Time& sDispatchTime) // Creates a process event for the given dispatch time
			:Event(sDispatchTime)
			{
			}
		
		/* Methods: */
		virtual void setCallback(ProcessCallback newCallback,void* newUserData) =0; // Tells the event dispatcher to change this event listener's callback immediately after the callback returns
		};
	
	class SignalEvent:public Event // Structure for user-defined signal events
		{
		friend class EventDispatcher;
		
		/* Elements: */
		private:
		void* signalData; // Opaque data pointer passed along when the signal was raised
		
		/* Constructors and destructors: */
		public:
		SignalEvent(const Time& sDispatchTime) // Creates a signal event for the given dispatch time
			:Event(sDispatchTime)
			{
			}
		
		/* Methods: */
		void* getSignalData(void) const
			{
			return signalData;
			}
		virtual void setCallback(SignalCallback newCallback,void* newUserData) =0; // Tells the event dispatcher to change this event listener's callback immediately after the callback returns
		};
	
	/* Forward declarations of event listener structures: */
	private:
	struct IOEventListener; // Structure representing listeners that have registered interest in some input/output event(s)
	struct TimerEventListener; // Structure representing listeners that have registered interest in timer events
	struct ProcessListener; // Structure representing listeners that are called after any event has been handled
	struct SignalListener; // Structure representing listeners that react to user-defined signals
	
	/* Forward declarations of actual event classes: */
	class IOEventImpl;
	class TimerEventImpl;
	class ProcessEventImpl;
	class SignalEventImpl;
	
	class TimerEventListenerComp; // Helper class to compare timer event listener structures by next event time
	typedef Misc::HashTable<ListenerKey,TimerEventListener> TimerEventListenerMap; // Hash table mapping listener keys to timer event listeners
	typedef Misc::PriorityHeap<TimerEventListener*,TimerEventListenerComp> TimerEventListenerHeap; // Type for heap of timer event listeners, ordered by next event time
	typedef Misc::HashTable<ListenerKey,SignalListener> SignalListenerMap; // Hash table mapping listener keys to signal listeners
	struct PipeMessage;
	
	/* Elements: */
	private:
	Spinlock pipeMutex; // Mutex protecting the self-pipe used to change the dispatcher's internal state or raise signals
	int pipeFds[2]; // A uni-directional unnamed pipe to trigger events internal to the dispatcher
	size_t numMessages; // Number of messages in the self-pipe read buffer
	PipeMessage* messages; // A buffer to read pipe messages from the self-pipe
	size_t messageReadSize; // Number of bytes read during previous call to readPipeMessages
	ListenerKey nextKey; // Next key to be assigned to an event listener
	std::vector<IOEventListener> ioEventListeners; // List of currently registered input/output event listeners
	TimerEventListenerMap timerEventListeners; // Map of currently registered timer event listeners
	TimerEventListenerHeap timerEventListenerHeap; // Heap of currently active timer event listeners, sorted by next event time
	std::vector<ProcessListener> processListeners; // List of currently registered process event listeners
	SignalListenerMap signalListeners; // Map of currently registered signal event listeners
	fd_set readFds,writeFds,exceptionFds; // Three sets of file descriptors waiting for reads, writes, and exceptions, respectively
	int numReadFds,numWriteFds,numExceptionFds; // Number of file descriptors in the three descriptor sets
	int maxFd; // Largest file descriptor set in any of the three descriptor sets
	bool hadBadFd; // Flag if the last invocation of dispatchNextEvent() tripped on a bad file descriptor
	
	/* Private methods: */
	ListenerKey getNextKey(void); // Returns a new listener key
	size_t readPipeMessages(void); // Reads messages from the self-pipe; returns number of complete messages read
	void writePipeMessage(const PipeMessage& pm,const char* methodName); // Writes a message to the self-pipe
	void updateFdSets(int fd,int oldEventMask,int newEventMask); // Updates the three descriptor sets based on the given file descriptor changing its interest mask
	
	/* Constructors and destructors: */
	public:
	EventDispatcher(void); // Creates an event dispatcher
	private:
	EventDispatcher(const EventDispatcher& source); // Prohibit copy constructor
	EventDispatcher& operator=(const EventDispatcher& source); // Prohibit assignment operator
	public:
	~EventDispatcher(void);
	
	/* Methods: */
	bool dispatchNextEvent(bool wait =true); // Waits for the next event if wait flag is true, then dispatches all pending events; returns false if the stop() method was called
	void dispatchEvents(void); // Waits for and dispatches events until stopped
	void interrupt(void); // Forces an invocation of dispatchNextEvent() to return with a true value
	void stop(void); // Forces an invocation of dispatchNextEvent() to return with a false value, or an invocation of dispatchEvents() to return
	void stopOnSignals(void); // Installs a signal handler that stops the event dispatcher when a SIGINT or SIGTERM occur
	
	/* Methods to manage event listeners: */
	ListenerKey addIOEventListener(int eventFd,int eventTypeMask,IOEventCallback eventCallback,void* eventCallbackUserData); // Adds a new input/output event listener for the given file descriptor and event type mask; returns unique event listener key
	void setIOEventListenerEventTypeMask(ListenerKey listenerKey,int newEventTypeMask); // Changes the event type mask of the input/output event listener with the given listener key
	void removeIOEventListener(ListenerKey listenerKey); // Removes the input/output event listener with the given listener key
	
	ListenerKey addTimerEventListener(const Time& eventTime,const Time& eventInterval,TimerEventCallback eventCallback,void* eventCallbackUserData,bool startSuspended =false); // Adds a new timer event listener for the given event time and repeat interval; starts timer in suspended mode if flag is true; returns unique event listener key
	void suspendTimerEventListener(ListenerKey listenerKey); // Temporarily suspends the timer event listener with the given listener key
	void resumeTimerEventListener(ListenerKey listenerKey,const Time& eventTime); // Resumes the timer event listener with the given listener key at the given event time
	void removeTimerEventListener(ListenerKey listenerKey); // Removes the timer event listener with the given listener key
	
	ListenerKey addProcessListener(ProcessCallback eventCallback,void* eventCallbackUserData); // Adds a new process listener; returns unique event listener key
	void removeProcessListener(ListenerKey listenerKey); // Removes the process listener with the given listener key
	
	ListenerKey addSignalListener(SignalCallback eventCallback,void* eventCallbackUserData); // Adds a new signal listener; returns unique event listener key
	void removeSignalListener(ListenerKey listenerKey); // Removes the signal listener with the given listener key
	
	/* Methods to trigger user-generated events: */
	void signal(ListenerKey listenerKey,void* signalData); // Raises a signal with the given listener key and opaque data pointer
	};

}

#endif

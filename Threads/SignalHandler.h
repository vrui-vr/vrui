/***********************************************************************
SignalHandler - Class to handle operating system signals in the context
of a run loop.
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

#ifndef THREADS_SIGNAL_INCLUDED
#define THREADS_SIGNAL_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/Mutex.h>
#include <Threads/EventTypes.h>

namespace Threads {

class SignalHandlerEvent; // Forward declaration of descriptor class passed to OS signal handler event handlers
typedef Threads::FunctionCall<SignalHandlerEvent&> SignalHandlerEventHandler; // Type for OS signal handler event handlers

class SignalHandler:public EventSource
	{
	friend class RunLoop;
	friend class SignalHandlerEvent;
	
	/* Embedded classes: */
	private:
	struct RegisteredSignalHandler; // Structure keeping track of registered OS signal handlers
	
	/* Elements: */
	static const int maxSignal=64; // Largest signum of any OS signal
	static Threads::Mutex registeredSignalHandlersMutex; // Mutex protecting the global table of registered OS signal handlers
	static RegisteredSignalHandler registeredSignalHandlers[maxSignal+1]; // Global table of registered OS signal handlers
	int signum; // OS signal number handled by the signal handler (SIGINT, SIGTERM, ..., defined in signal.h)
	Misc::Autopointer<SignalHandlerEventHandler> eventHandler; // The event handler
	
	/* Protected methods from class Ownable: */
	virtual void disowned(void);
	
	/* New private methods: */
	void enableInternal(void); // Enables this OS signal handler inside its run loop
	void enablePM(void); // Enables this OS signal handler in response to a received pipe message
	void disableInternal(bool block); // Disables this OS signal handler inside its run loop; if flag is true, blocks until it has actually been disabled
	void disablePM(bool unregister); // Disables this OS signal handler in response to a received pipe message
	void setEventHandlerPM(SignalHandlerEventHandler* newEventHandler); // Sets this OS signal handler's event handler in response to a received pipe message
	static void registerRunLoop(RunLoop* runLoop,int signum); // Registers the given run loop for the given OS signal without a handler
	static void unregisterRunLoop(RunLoop* runLoop); // Unregisters the given run loop from all OS signals
	static void signalHandlerFunction(int signum); // Low-level OS signal handler
	static bool signalPM(RunLoop* runLoop,int signum); // Dispatches the given OS signal in response to a received pipe message
	
	/* Constructors and destructors: */
	public:
	SignalHandler(RunLoop& sRunLoop,int sSignum,bool sEnabled,SignalHandlerEventHandler& sEventHandler); // Elementwise constructor
	
	/* Methods: */
	int getSignum(void) const // Returns the OS signal number handled by the signal handler (SIGINT, SIGTERM, ..., defined in signal.h)
		{
		return signum;
		}
	void enable(void) // Enables the signal handler
		{
		/* Delegate to the internal method: */
		enableInternal();
		}
	void disable(void) // Disables the signal handler
		{
		/* Delegate to the internal method: */
		disableInternal(false);
		}
	void setEnabled(bool newEnabled) // Sets the signal handler's enabled state
		{
		/* Delegate to the internal methods: */
		if(newEnabled)
			enableInternal();
		else
			disableInternal(false);
		}
	void setEventHandler(SignalHandlerEventHandler& newEventHandler); // Sets the signal handler's event handler
	};

typedef OwningPointer<SignalHandler> SignalHandlerOwner; // Type for ownership-establishing pointers to OS signal handlers
typedef Misc::Autopointer<SignalHandler> SignalHandlerPtr; // Type for non-ownership-establishing pointers to OS signal handlers

class SignalHandlerEvent:public Event<SignalHandler> // Class for event descriptors passed to OS signal event handlers
	{
	friend class RunLoop;
	friend class SignalHandler;
	
	/* Elements: */
	private:
	int signum; // OS signal number that was raised (SIGINT, SIGTERM, ..., defined in signal.h)
	
	/* Constructors and destructors: */
	SignalHandlerEvent(SignalHandler* sSignalHandler,const EventTime& sDispatchTime,int sSignum) // Elementwise constructor
		:Event<SignalHandler>(sSignalHandler,sDispatchTime),
		 signum(sSignum)
		{
		}
	
	/* Methods: */
	public:
	int getSignum(void) const // Returns the OS signal number that was raised (SIGINT, SIGTERM, ..., defined in signal.h)
		{
		return signum;
		}
	};

}

#endif

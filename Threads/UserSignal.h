/***********************************************************************
UserSignal - Class for user-defined signals to synchronously notify
clients of asynchronous events.
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

#ifndef THREADS_USERSIGNAL_INCLUDED
#define THREADS_USERSIGNAL_INCLUDED

#include <Misc/Autopointer.h>
#include <Misc/StdError.h>
#include <Threads/EventTypes.h>

/* Forward declarations: */
namespace Threads {
class RefCounted;
}

namespace Threads {

class UserSignalEvent; // Forward declaration of descriptor class passed to user signal event handlers
typedef Threads::FunctionCall<UserSignalEvent&> UserSignalEventHandler; // Type for user signal event handlers

class UserSignal:public EventSource
	{
	friend class RunLoop;
	friend class UserSignalEvent;
	
	/* Elements: */
	private:
	Misc::Autopointer<UserSignalEventHandler> eventHandler; // The event handler
	
	/* Protected methods from class Ownable: */
	virtual void disowned(void);
	
	/* New private methods: */
	void enableInternal(void); // Enables this user signal handler inside its run loop
	void enablePM(void); // Enables this user signal handler in response to a received pipe message
	void disableInternal(bool block); // Disables this user signal handler inside its run loop; if flag is true, blocks until it has actually been disabled
	void disablePM(void); // Disables this user signal handler in response to a received pipe message
	void setEventHandlerPM(UserSignalEventHandler* newEventHandler); // Sets this user signal handler's event handler in response to a received pipe message
	void signalInternal(RefCounted* signalData); // Signals a user signal inside its run loop
	void signalPM(RefCounted* signalData); // Signals a user signal in response to a received pipe message
	
	/* Constructors and destructors: */
	public:
	UserSignal(RunLoop& sRunLoop,bool enabled,UserSignalEventHandler& sEventHandler); // Elementwise constructor
	
	/* Methods: */
	void enable(void) // Enables the user signal
		{
		/* Delegate to the internal method: */
		enableInternal();
		}
	void disable(void) // Disables the user signal
		{
		/* Delegate to the internal method: */
		disableInternal(false);
		}
	void setEnabled(bool newEnabled) // Sets the user signal's enabled state
		{
		/* Delegate to the internal methods: */
		if(newEnabled)
			enableInternal();
		else
			disableInternal(false);
		}
	void setEventHandler(UserSignalEventHandler& newEventHandler); // Sets the user signal's event handler
	void signal(RefCounted& signalData) // Sends a signal with the given data to the user signal handler
		{
		/* Delegate to the internal method: */
		signalInternal(&signalData);
		}
	void signal(void) // Ditto, without signal data
		{
		/* Delegate to the internal method: */
		signalInternal(0);
		}
	};

typedef OwningPointer<UserSignal> UserSignalOwner; // Type for ownership-establishing pointers to user signals
typedef Misc::Autopointer<UserSignal> UserSignalPtr; // Type for non-ownership-establishing pointers to user signals

class UserSignalEvent:public Event<UserSignal> // Class for user signal event descriptors passed to event handlers
	{
	friend class RunLoop;
	friend class UserSignal;
	
	/* Elements: */
	private:
	RefCounted* signalData; // Pointer to the user-defined data that was sent along with the signal
	
	/* Constructors and destructors: */
	UserSignalEvent(UserSignal* sUserSignal,const EventTime& sDispatchTime,RefCounted* sSignalData) // Elementwise constructor
		:Event<UserSignal>(sUserSignal,sDispatchTime),
		 signalData(sSignalData)
		{
		}
	
	/* Methods: */
	public:
	template <class UserSignalDataParam>
	UserSignalDataParam& getSignalData(void) // Returns a dynamically cast reference to the signal data; throws exception if types don't match
		{
		/* Cast the signal data pointer and check for errors: */
		UserSignalDataParam* data=dynamic_cast<UserSignalDataParam*>(signalData);
		if(data==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching user signal data type");
		return *data;
		}
	};

}

#endif

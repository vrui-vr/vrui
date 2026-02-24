/***********************************************************************
PendingCall - Class encapsulating pending pendingCall reply handlers.
Copyright (c) 2026 Oliver Kreylos

This file is part of the DBus C++ Wrapper Library (DBus).

The DBus C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The DBus C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the DBus C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef DBUS_PENDINGCALL_INCLUDED
#define DBUS_PENDINGCALL_INCLUDED

#include <dbus/dbus.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}
namespace DBus {
class Message;
class Connection;
}

namespace DBus {

class PendingCall
	{
	friend class Connection;
	
	/* Embedded classes: */
	public:
	typedef Threads::FunctionCall<Message&> ReplyHandler; // Type for reply handlers
	
	/* Elements: */
	private:
	DBusPendingCall* pendingCall; // Low-level DBus pending call handle
	
	/* Private methods: */
	static void notifyFunction(DBusPendingCall* pendingCall,void* userData); // Function called when a pending call completes
	
	/* Constructors and destructors: */
	public:
	PendingCall(void) // Creates an invalid pending call
		:pendingCall(0)
		{
		}
	PendingCall(DBusPendingCall* sPendingCall) // Creates a wrapper around the given low-level DBus pending call pointer
		:pendingCall(sPendingCall)
		{
		/* Take another reference to the pending call: */
		dbus_pending_call_ref(pendingCall);
		}
	private:
	PendingCall(DBusPendingCall* sPendingCall,int) // Creates a wrapper around the given low-level DBus pending call pointer without taking a reference; only called internally by other parts of the library
		:pendingCall(sPendingCall)
		{
		}
	public:
	PendingCall(const PendingCall& source) // Copy constructor
		:pendingCall(source.pendingCall)
		{
		/* Take another reference to the source's low-level DBus pending call: */
		dbus_pending_call_ref(pendingCall);
		}
	PendingCall(PendingCall&& source) // Move constructor
		:pendingCall(source.pendingCall)
		{
		/* Invalidate the source: */
		source.pendingCall=0;
		}
	~PendingCall(void) // Releases the low-level DBus pending call
		{
		/* Release the held reference to the low-level DBus pending call: */
		dbus_pending_call_unref(pendingCall); // Unref-ing 0 is a valid no-op
		}
	
	/* Methods: */
	PendingCall& operator=(const PendingCall& source) // Copy assignment operator
		{
		if(this!=&source)
			{
			/* Release the held reference to the current low-level DBus pending call: */
			dbus_pending_call_unref(pendingCall); // Unref-ing 0 is a valid no-op
			
			/* Copy and take another reference to the source's low-level DBus pending call: */
			pendingCall=source.pendingCall;
			dbus_pending_call_ref(pendingCall);
			}
		
		return *this;
		}
	PendingCall& operator=(PendingCall&& source) // Move assignment operator
		{
		if(this!=&source)
			{
			/* Release the held reference to the current low-level DBus pending call: */
			dbus_pending_call_unref(pendingCall); // Unref-ing 0 is a valid no-op
			
			/* Copy the source's low-level DBus pending call and invalidate the source: */
			pendingCall=source.pendingCall;
			source.pendingCall=0;
			}
		
		return *this;
		}
	bool valid(void) const // Returns true if this pending call is valid
		{
		return pendingCall!=0;
		}
	bool operator==(const DBusPendingCall* other) const // Equality operator
		{
		return pendingCall==other;
		}
	bool operator==(const PendingCall& other) const // Equality operator
		{
		return pendingCall==other.pendingCall;
		}
	bool operator!=(const DBusPendingCall* other) const // Inequality operator
		{
		return pendingCall!=other;
		}
	bool operator!=(const PendingCall& other) const // Inequality operator
		{
		return pendingCall!=other.pendingCall;
		}
	void cancel(void) // Cancels this pending call; any reply or error that arrives later will be ignored
		{
		dbus_pending_call_cancel(pendingCall);
		}
	bool hasCompleted(void) // Returns true if this pending call has already received a reply or error
		{
		return dbus_pending_call_get_completed(pendingCall);
		}
	Message stealReply(void); // Returns the pending call's reply; throws exception if the pending call hasn't completed yet
	void block(void) // Blocks the caller until the pending call completes
		{
		dbus_pending_call_block(pendingCall);
		}
	void setReplyHandler(ReplyHandler& replyHandler); // Sets a reply handler that will be called when the pending call completes
	};

}

#endif

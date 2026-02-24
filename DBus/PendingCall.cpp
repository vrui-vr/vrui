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

#include <DBus/PendingCall.h>

#include <Misc/StdError.h>
#include <Threads/FunctionCalls.h>
#include <DBus/Message.h>

// DEBUGGING
#include <iostream>

namespace DBus {

/****************************
Methods of class PendingCall:
****************************/

void PendingCall::notifyFunction(DBusPendingCall* pendingCall,void* userData)
	{
	/* Steal the message and wrap it in a Message object without taking another reference: */
	Message message(dbus_pending_call_steal_reply(pendingCall),0);
	
	/* Call the message handler: */
	(*static_cast<ReplyHandler*>(userData))(message);
	}

Message PendingCall::stealReply(void)
	{
	/* Steal the message and check for errors: */
	DBusMessage* message=dbus_pending_call_steal_reply(pendingCall);
	if(message==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No reply received yet");
	
	/* Wrap the message in a Message object without taking another reference: */
	return Message(message,0);
	}

namespace {

/****************
Helper functions:
****************/

void unrefReplyHandler(void* memory) // Unreferences a reply handler if the pending call it's associated with drops
	{
	/* Drop the reference held by the pending call: */
	// std::cout<<"DBus::PendingCall: Dropping reference to reply handler"<<std::endl;
	static_cast<PendingCall::ReplyHandler*>(memory)->unref();
	}

}

void PendingCall::setReplyHandler(PendingCall::ReplyHandler& replyHandler)
	{
	/* Hold a reference on the caller-supplied reply handler: */
	replyHandler.ref();
	
	/* Add the notify function and check for errors: */
	if(!dbus_pending_call_set_notify(pendingCall,notifyFunction,&replyHandler,unrefReplyHandler))
		{
		/* Drop the reference on the reply handler and throw an exception: */
		replyHandler.unref();
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set reply handler");
		}
	}

}

/***********************************************************************
Connection - Class referencing a DBus connection.
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

#include <DBus/Connection.h>

#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <DBus/Error.h>
#include <DBus/Message.h>
#include <DBus/PendingCall.h>

#define DEBUG_PROTOCOL 0

#if DEBUG_PROTOCOL
#include <iostream>
#endif

namespace DBus {

/*********************************************
Declaration of class Connection::WatchHandler:
*********************************************/

class Connection::WatchHandler:public Threads::RunLoop::IOWatcher::EventHandler
	{
	/* Elements: */
	private:
	DBusWatch* watch; // The DBus watch handled by this handler
	
	/* Constructors and destructors: */
	public:
	WatchHandler(DBusWatch* sWatch)
		:watch(sWatch)
		{
		}
	virtual ~WatchHandler(void)
		{
		}
	
	/* Methods from class Threads::RunLoop::IOWatcher::EventHandler: */
	virtual void operator()(Threads::RunLoop::IOWatcher::Event& event)
		{
		/* Assemble an event mask: */
		unsigned int flags=0x0U;
		if(event.canRead())
			flags|=DBUS_WATCH_READABLE;
		if(event.canWrite())
			flags|=DBUS_WATCH_WRITABLE;
		if(event.hadError())
			flags|=DBUS_WATCH_ERROR;
		if(event.hadHangUp())
			flags|=DBUS_WATCH_HANGUP;
		
		/* Call the DBus watch handler: */
		if(!dbus_watch_handle(watch,flags))
			{
			/* What are we supposed to do? Magic new memory into existence?? */
			Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Out of memory");
			}
		}
	};

class Connection::TimeoutHandler:public Threads::RunLoop::Timer::EventHandler
	{
	/* Elements: */
	private:
	DBusTimeout* timeout; // The DBus timeout handled by this handler
	
	/* Constructors and destructors: */
	public:
	TimeoutHandler(DBusTimeout* sTimeout)
		:timeout(sTimeout)
		{
		}
	virtual ~TimeoutHandler(void)
		{
		}
	
	/* Methods from class Threads::RunLoop::Timer::EventHandler: */
	virtual void operator()(Threads::RunLoop::Timer::Event& event)
		{
		/* Call the DBus timeout handler: */
		if(!dbus_timeout_handle(timeout))
			{
			/* What are we supposed to do? Magic new memory into existence?? */
			Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Out of memory");
			}
		}
	};

/***************************
Methods of class Connection:
***************************/

dbus_bool_t Connection::addWatchFunction(DBusWatch* watch,void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Create watch "<<watch<<" for file descriptor "<<dbus_watch_get_unix_fd(watch)<<" for";
	if(dbus_watch_get_flags(watch)&DBUS_WATCH_READABLE)
		std::cout<<" reading";
	if(dbus_watch_get_flags(watch)&DBUS_WATCH_WRITABLE)
		std::cout<<" writing";
	std::cout<<" in "<<(dbus_watch_get_enabled(watch)?"enabled":"disabled")<<" state"<<std::endl;
	#endif
	
	/* Create an I/O watcher for the watch's file descriptor: */
	Threads::RunLoop* runLoop=static_cast<Threads::RunLoop*>(data);
	unsigned int eventMask=0x0U;
	unsigned int flags=dbus_watch_get_flags(watch);
	if(flags&DBUS_WATCH_READABLE)
		eventMask|=Threads::RunLoop::IOWatcher::Read;
	if(flags&DBUS_WATCH_WRITABLE)
		eventMask|=Threads::RunLoop::IOWatcher::Write;
	Threads::RunLoop::IOWatcher* ioWatcher=runLoop->createIOWatcher(dbus_watch_get_unix_fd(watch),eventMask,dbus_watch_get_enabled(watch),*new WatchHandler(watch));
	#if DEBUG_PROTOCOL
	std::cout<<"Created I/O watcher "<<ioWatcher<<std::endl;
	#endif
	
	/* Store the I/O watcher pointer with the watch: */
	dbus_watch_set_data(watch,ioWatcher,0);
	
	return true;
	}

void Connection::removeWatchFunction(DBusWatch* watch,void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Remove watch "<<watch<<" for file descriptor "<<dbus_watch_get_unix_fd(watch)<<std::endl;
	#endif
	
	/* Retrieve the I/O watcher pointer: */
	Threads::RunLoop::IOWatcher* ioWatcher=static_cast<Threads::RunLoop::IOWatcher*>(dbus_watch_get_data(watch));
	
	/* Delete the I/O watcher: */
	#if DEBUG_PROTOCOL
	std::cout<<"Deleting I/O watcher "<<ioWatcher<<std::endl;
	#endif
	delete ioWatcher;
	}

void Connection::watchToggledFunction(DBusWatch* watch,void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Watch "<<watch<<" for file descriptor "<<dbus_watch_get_unix_fd(watch)<<" toggled to "<<(dbus_watch_get_enabled(watch)?"enabled":"disabled")<<" state"<<std::endl;
	#endif
	
	/* Retrieve the I/O watcher pointer: */
	Threads::RunLoop::IOWatcher* ioWatcher=static_cast<Threads::RunLoop::IOWatcher*>(dbus_watch_get_data(watch));
	
	/* Set the I/O watcher's enabled state: */
	#if DEBUG_PROTOCOL
	std::cout<<"Toggling I/O watcher "<<ioWatcher<<std::endl;
	#endif
	ioWatcher->setEnabled(dbus_watch_get_enabled(watch));
	}

dbus_bool_t Connection::addTimeoutFunction(DBusTimeout* timeout,void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Add timeout "<<timeout<<" with interval "<<dbus_timeout_get_interval(timeout)<<" in "<<(dbus_timeout_get_enabled(timeout)?"enabled":"disabled")<<" state"<<std::endl;
	#endif
	
	/* Create a timer for the timeout: */
	Threads::RunLoop* runLoop=static_cast<Threads::RunLoop*>(data);
	Threads::RunLoop::Time firstTimeout;
	long nanoseconds=dbus_timeout_get_interval(timeout)*1000000L;
	Threads::RunLoop::Interval interval(nanoseconds/1000000000L,nanoseconds%1000000000L);
	firstTimeout+=interval;
	Threads::RunLoop::Timer* timer=runLoop->createTimer(firstTimeout,interval,dbus_timeout_get_enabled(timeout),*new TimeoutHandler(timeout));
	
	/* Store the timer pointer with the timeout: */
	dbus_timeout_set_data(timeout,timer,0);
	
	return true;
	}

void Connection::removeTimeoutFunction(DBusTimeout* timeout,void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Remove timeout "<<timeout<<" with interval "<<dbus_timeout_get_interval(timeout)<<std::endl;
	#endif
	
	/* Retrieve the timer pointer: */
	Threads::RunLoop::Timer* timer=static_cast<Threads::RunLoop::Timer*>(dbus_timeout_get_data(timeout));
	
	/* Delete the timer: */
	#if DEBUG_PROTOCOL
	std::cout<<"Deleting timer "<<timer<<std::endl;
	#endif
	delete timer;
	}

void Connection::timeoutToggledFunction(DBusTimeout* timeout,void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Timeout "<<timeout<<" with interval "<<dbus_timeout_get_interval(timeout)<<" toggled to "<<(dbus_timeout_get_enabled(timeout)?"enabled":"disabled")<<std::endl;
	#endif
	
	/* Retrieve the timer pointer: */
	Threads::RunLoop::Timer* timer=static_cast<Threads::RunLoop::Timer*>(dbus_timeout_get_data(timeout));
	
	/* Set the timer's enabled state: */
	#if DEBUG_PROTOCOL
	std::cout<<"Toggling timer "<<timer<<std::endl;
	#endif
	timer->setEnabled(dbus_timeout_get_enabled(timeout));
	}

void Connection::wakeupMainFunction(void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Wake-up main"<<std::endl;
	#endif
	
	/* Wake up the run loop: */
	Threads::RunLoop* runLoop=static_cast<Threads::RunLoop*>(data);
	runLoop->wakeUp();
	}

void Connection::dispatchFunction(Threads::RunLoop::ProcessFunction& processFunction,DBusConnection* connection)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Dispatching messages on connection "<<connection<<std::endl;
	#endif
	
	/* Dispatch messages on the connection and disable the process function if no more dispatching is to be done: */
	if(dbus_connection_dispatch(connection)==DBUS_DISPATCH_COMPLETE)
		processFunction.disable();
	}

void Connection::dispatchStatusFunction(DBusConnection* connection,DBusDispatchStatus newStatus,void* data)
	{
	#if DEBUG_PROTOCOL
	std::cout<<"Connection "<<connection<<" changed dispatch status to";
	switch(newStatus)
		{
		case DBUS_DISPATCH_DATA_REMAINS:
			std::cout<<" data remains"<<std::endl;
			break;
			
		case DBUS_DISPATCH_COMPLETE:
			std::cout<<" complete"<<std::endl;
			break;
		
		case DBUS_DISPATCH_NEED_MEMORY:
			std::cout<<" need memory"<<std::endl;
			break;
		}
	#endif
	
	/* Enable the process function if dispatching is to be done: */
	Threads::RunLoop::ProcessFunction* processFunction=static_cast<Threads::RunLoop::ProcessFunction*>(data);
	if(newStatus==DBUS_DISPATCH_DATA_REMAINS)
		processFunction->enable();
	}

DBusHandlerResult Connection::filterFunction(DBusConnection* connection,DBusMessage* message,void* userData)
	{
	/* Wrap the given raw message in a Message object: */
	Message myMessage(message);
	
	/* Call the user-defined message handler: */
	(*static_cast<MessageHandler*>(userData))(myMessage);
	
	/* Tell DBus that we handled the message (this needs to be fixed): */
	return DBUS_HANDLER_RESULT_HANDLED;
	}

void Connection::replyNotifyFunction(DBusPendingCall* pendingCall,void* userData)
	{
	/* Steal the pending call's message and wrap it in a Message object without taking another reference: */
	Message message(dbus_pending_call_steal_reply(pendingCall),0);
	
	/* Call the message handler: */
	(*static_cast<Connection::MessageHandler*>(userData))(message);
	
	/* Drop a reference to the pending call to destroy it: */
	#if DEBUG_PROTOCOL
	std::cout<<"DBus::Connection: Dropping reference to DBus pending call object "<<pendingCall<<std::endl;
	#endif
	dbus_pending_call_unref(pendingCall);
	}

Connection::Connection(DBusBusType busType)
	:connection(0),isPrivate(false)
	{
	/* Connect to the given message bus and handle errors: */
	Error error;
	if((connection=dbus_bus_get(busType,&error))==0)
		{
		const char* busName;
		switch(busType)
			{
			case DBUS_BUS_SESSION:
				busName="session";
				break;
			
			case DBUS_BUS_SYSTEM:
				busName="system";
				break;
			
			case DBUS_BUS_STARTER:
				busName="starter";
				break;
			
			default:
				busName="unkown";
			}
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot connect to %s message bus due to error %s: %s",busName,error.name,error.message);
		}
	
	/* Don't exit the program when the connection disconnects: */
	dbus_connection_set_exit_on_disconnect(connection,false);
	}

Connection::Connection(const char* address,bool sIsPrivate)
	:connection(0),isPrivate(sIsPrivate)
	{
	/* Connect to the given address and handle errors: */
	Error error;
	if(isPrivate)
		connection=dbus_connection_open_private(address,&error);
	else
		connection=dbus_connection_open(address,&error);
	if(connection==0)
		{
		isPrivate=false;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot connect to address %s due to error %s: %s",address,error.name,error.message);
		}
	}

Connection::Connection(const Connection& source)
	:connection(source.connection),isPrivate(false)
	{
	/* Increment the connection's reference count: */
	dbus_connection_ref(connection);
	}

Connection::~Connection(void)
	{
	/* Close the connection if it was opened privately: */
	if(isPrivate)
		dbus_connection_close(connection);
	
	/* Decrement the connection's reference count: */
	dbus_connection_unref(connection);
	}

Connection& Connection::operator=(const Connection& source)
	{
	if(connection!=source.connection)
		{
		/* Close and decrement the current connection's reference count, copy the source's connection, and increment its reference count: */
		if(isPrivate)
			dbus_connection_close(connection);
		dbus_connection_unref(connection);
		connection=source.connection;
		isPrivate=false;
		dbus_connection_ref(connection);
		}
	
	return *this;
	}

Connection& Connection::operator=(Connection&& source)
	{
	if(connection!=source.connection)
		{
		/* Close and decrement the current connection's reference count, copy and invalidate the source's connection: */
		if(isPrivate)
			dbus_connection_close(connection);
		dbus_connection_unref(connection);
		connection=source.connection;
		source.connection=0;
		isPrivate=source.isPrivate;
		source.isPrivate=false;
		}
	
	return *this;
	}

namespace {

/****************
Helper functions:
****************/

void unrefProcessFunction(void* memory) // Unreferences a process function if the connection it's associated with drops
	{
	/* Drop the reference held by the connection: */
	#if DEBUG_PROTOCOL
	std::cout<<"DBus::Connection: Dropping reference to process function"<<std::endl;
	#endif
	static_cast<Threads::RunLoop::ProcessFunction*>(memory)->unref();
	}

}

void Connection::watchConnection(Threads::RunLoop& runLoop)
	{
	/* Register watch functions: */
	if(!dbus_connection_set_watch_functions(connection,addWatchFunction,removeWatchFunction,watchToggledFunction,&runLoop,0))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot register watch functions");
	
	/* Register timeout functions: */
	if(!dbus_connection_set_timeout_functions(connection,addTimeoutFunction,removeTimeoutFunction,timeoutToggledFunction,&runLoop,0))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot register timeout functions");
	
	/* Register a wake-up function: */
	dbus_connection_set_wakeup_main_function(connection,wakeupMainFunction,&runLoop,0);
	
	/* Create a process function to dispatch messages: */
	Threads::RunLoop::ProcessFunction* processFunction=runLoop.createProcessFunction(false,true,*Threads::createFunctionCall(dispatchFunction,connection));
	processFunction->ref();
	
	/* Register a dispatch status function: */
	dbus_connection_set_dispatch_status_function(connection,dispatchStatusFunction,processFunction,unrefProcessFunction);
	}

namespace {

/****************
Helper functions:
****************/

void unrefMessageHandler(void* memory) // Unreferences a message handler if the filter it's associated with drops
	{
	/* Drop the reference held by the filter: */
	#if DEBUG_PROTOCOL
	std::cout<<"DBus::Connection: Dropping reference to message handler"<<std::endl;
	#endif
	static_cast<Connection::MessageHandler*>(memory)->unref();
	}

}

void Connection::addFilter(Connection::MessageHandler& messageHandler)
	{
	/* Hold a reference on the caller-supplied message handler: */
	messageHandler.ref();
	
	/* Add the filter function and check for errors: */
	if(!dbus_connection_add_filter(connection,filterFunction,&messageHandler,unrefMessageHandler))
		{
		/* Drop the reference on the message handler and throw an exception: */
		messageHandler.unref();
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot add filter function");
		}
	}

Message::Serial Connection::send(Message& message)
	{
	/* Queue the message for sending and check for errors: */
	Message::Serial serialNumber;
	if(!dbus_connection_send(connection,message.message,&serialNumber))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot send message");
	
	return serialNumber;
	}

PendingCall Connection::sendWithReply(Message& message,int timeout)
	{
	/* Queue the message for sending and check for errors: */
	DBusPendingCall* pendingCall;
	if(!dbus_connection_send_with_reply(connection,message.message,&pendingCall,timeout))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot send message");
	
	/* Wrap the low-level DBus pending call in a PendingCall object without taking another reference: */
	return PendingCall(pendingCall,0);
	}

namespace {

/****************
Helper functions:
****************/

void unrefReplyHandler(void* memory)
	{
	/* Drop the reference held by the pending call: */
	#if DEBUG_PROTOCOL
	std::cout<<"DBus::Connection: Dropping reference to pending call reply handler"<<std::endl;
	#endif
	static_cast<Connection::MessageHandler*>(memory)->unref();
	}

}

void Connection::sendWithReply(Message& message,int timeout,Connection::MessageHandler& replyHandler)
	{
	/* Take a reference to the reply handler: */
	replyHandler.ref();
	
	/* Queue the message for sending and check for errors: */
	DBusPendingCall* pendingCall;
	if(!dbus_connection_send_with_reply(connection,message.message,&pendingCall,timeout))
		{
		/* Drop the reference to the reply handler and throw an exception: */
		replyHandler.unref();
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot send message");
		}
	
	if(!dbus_pending_call_set_notify(pendingCall,replyNotifyFunction,&replyHandler,unrefReplyHandler))
		{
		/* Drop the reference to the reply handler and throw an exception: */
		replyHandler.unref();
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set reply handler");
		}
	}

DBusDispatchStatus Connection::getDispatchStatus(void)
	{
	return dbus_connection_get_dispatch_status(connection);
	}

DBusDispatchStatus Connection::dispatch(void)
	{
	return dbus_connection_dispatch(connection);
	}

bool Connection::hasMessagesToSend(void)
	{
	return dbus_connection_has_messages_to_send(connection);
	}

void Connection::flush(void)
	{
	dbus_connection_flush(connection);
	}

std::string Connection::getServerId(void)
	{
	std::string result;
	
	/* Request the server ID: */
	char* resultString=dbus_connection_get_server_id(connection);
	if(resultString!=0)
		{
		/* Wrap the ID in a std::string and then free() it: */
		result=resultString;
		free(resultString);
		}
	
	return result;
	}

const char* Connection::getUniqueName(void)
	{
	return dbus_bus_get_unique_name(connection);
	}

std::string Connection::getBusId(void)
	{
	/* Send a request and handle errors: */
	Error error;
	char* resultString;
	if((resultString=dbus_bus_get_id(connection,&error))==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s",error.name,error.message);
	
	/* Wrap the result string, which has to be free()d, in a std::string: */
	std::string result(resultString);
	free(resultString);
	
	return result;
	}

bool Connection::doesNameHaveOwner(const char* name)
	{
	/* Send a request and handle errors: */
	Error error;
	bool result=dbus_bus_name_has_owner(connection,name,&error);
	if(error.isSet())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s: %s",name,error.name,error.message);
	
	return result;
	}

unsigned long Connection::getUNIXUser(const char* name)
	{
	/* Send a request and handle errors: */
	Error error;
	unsigned long result;
	if((result=dbus_bus_get_unix_user(connection,name,&error))==(unsigned long)-1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s: %s",name,error.name,error.message);
	
	return result;
	}

int Connection::requestName(const char* name,unsigned int flags)
	{
	/* Send a request and handle errors: */
	Error error;
	int result;
	if((result=dbus_bus_request_name(connection,name,flags,&error))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s: %s",name,error.name,error.message);
	
	return result;
	}

int Connection::releaseName(const char* name)
	{
	/* Send a request and handle errors: */
	Error error;
	int result;
	if((result=dbus_bus_release_name(connection,name,&error))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s: %s",name,error.name,error.message);
	
	return result;
	}

bool Connection::startServiceByName(const char* name)
	{
	/* Send a request and handle errors: */
	Error error;
	dbus_uint32_t result;
	if(!dbus_bus_start_service_by_name(connection,name,0,&result,&error))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s: %s",name,error.name,error.message);
	
	return result==DBUS_START_REPLY_SUCCESS;
	}

void Connection::addMatchRule(const char* rule,bool waitForReply)
	{
	/* Send a request and handle errors: */
	Error error;
	dbus_bus_add_match(connection,rule,waitForReply?&error:0);
	if(error.isSet())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s: %s",rule,error.name,error.message);
	}

void Connection::removeMatchRule(const char* rule,bool waitForReply)
	{
	/* Send a request and handle errors: */
	Error error;
	dbus_bus_remove_match(connection,rule,waitForReply?&error:0);
	if(error.isSet())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"%s: %s: %s",rule,error.name,error.message);
	}

}

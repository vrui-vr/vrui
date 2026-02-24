/***********************************************************************
Connection - Class representing a (shared) DBus connection.
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

#ifndef DBUS_CONNECTION_INCLUDED
#define DBUS_CONNECTION_INCLUDED

#include <string>
#include <Threads"RunLoop.h>
#include <dbus/dbus.h>
#include <DBus/Message.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
class RunLoop;
}
namespace DBus {
class PendingCall;
}

namespace DBus {

class Connection
	{
	/* Embedded classes: */
	public:
	typedef Threads::FunctionCall<Message&> MessageHandler; // Type for message handlers
	
	private:
	class WatchHandler; // A class to handle I/O events on a watched DBus connection
	class TimeoutHandler; // A class to handle timeout events on a watched DBus connection
	
	/* Elements: */
	DBusConnection* connection; // Low-level DBus connection handle
	bool isPrivate; // Flag if the connection was opened as private
	
	/* Private methods: */
	static dbus_bool_t addWatchFunction(DBusWatch* watch,void* data);
	static void removeWatchFunction(DBusWatch* watch,void* data);
	static void watchToggledFunction(DBusWatch* watch,void* data);
	static dbus_bool_t addTimeoutFunction(DBusTimeout* timeout,void* data);
	static void removeTimeoutFunction(DBusTimeout* timeout,void* data);
	static void timeoutToggledFunction(DBusTimeout* timeout,void* data);
	static void wakeupMainFunction(void* data);
	static void dispatchFunction(Threads::RunLoop::ProcessFunction& processFunction,DBusConnection* connection);
	static void dispatchStatusFunction(DBusConnection* connection,DBusDispatchStatus newStatus,void* data);
	static DBusHandlerResult filterFunction(DBusConnection* connection,DBusMessage* message,void* userData); // Trampoline to call user-defined message handlers
	static void replyNotifyFunction(DBusPendingCall* pendingCall,void* userData); // Trampoline to call user-defined messsage reply handlers
	
	/* Constructors and destructors: */
	public:
	Connection(void) // Creates an invalid DBus connection
		:connection(0),isPrivate(false)
		{
		}
	Connection(DBusBusType bus); // Opens a connection to a well-known DBus message bus
	Connection(const char* address,bool sIsPrivate); // Opens a connection to the given DBus address; opens the connection as private if the flag is true
	Connection(const Connection& source); // Copy constructor; creates another reference to the source's connection
	Connection(Connection&& source) // Move constructor; takes over the source's connection
		:connection(source.connection),isPrivate(source.isPrivate)
		{
		/* Invalidate the source's connection: */
		source.connection=0;
		source.isPrivate=false;
		}
	~Connection(void); // Destructor; drops a reference to the connection handle
	
	/* Methods: */
	Connection& operator=(const Connection& source); // Copy assignment; creates another reference to the source's connection
	Connection& operator=(Connection&& source); // Move assignment; takes over the source's connection
	bool isValid(void) const // Returns true if the connection is valid
		{
		return connection!=0;
		}
	bool isConnected(void) const // Returns true if the connection is valid and connected
		{
		return connection!=0&&dbus_connection_get_is_connected(connection);
		}
	bool isAuthenticated(void) const // Returns true if the connection is valid and authenticated
		{
		return connection!=0&&dbus_connection_get_is_authenticated(connection);
		}
	bool isAnonymous(void) const // Returns true if the connection is valid and anonymous
		{
		return connection!=0&&dbus_connection_get_is_anonymous(connection);
		}
	std::string getServerId(void); // Returns the ID of the server address to which this connection is authenticated, if on the client side
	
	/* Event-based message dispatching methods: */
	void watchConnection(Threads::RunLoop& runLoop); // Watches the connection from the given run loop
	void addFilter(MessageHandler& messageHandler); // Registers a message handler that receives messages arriving on the connection
	Message::Serial send(Message& message); // Queues the given message for sending on the connection; returns the sent message's serial number
	PendingCall sendWithReply(Message& message,int timeout); // Queues the given message for sending on the connection; returns a pending call object to track arrival of the message's reply
	void sendWithReply(Message& message,int timeout,MessageHandler& replyHandler); // Queues the given message for sending on the connection and calls the given message handler when the message's reply arrives
	
	/* Manual message transport methods: */
	DBusDispatchStatus getDispatchStatus(void); // Returns the connection's current dispatch status
	DBusDispatchStatus dispatch(void); // Reads available data on the connection and handles at most one complete message; returns connection's new dispatch status
	bool hasMessagesToSend(void); // Returns true if the outgoing message queue is not empty
	void flush(void); // Blocks until the outgoing message queue is empty
	
	/* The following methods work on connections that are connected to a message bus instead of an address: */
	const char* getUniqueName(void); // Returns the unique name of the connection as assigned by the message bus
	std::string getBusId(void); // Returns the globally unique ID of the message bus
	bool doesNameHaveOwner(const char* name); // Returns true if the given name already has an owner on the message bus
	unsigned long getUNIXUser(const char* name); // Returns the UNIX user ID as which the owner of the given name on the message bus authenticated
	int requestName(const char* name,unsigned int flags); // Asks the message bus to assign the given name to this connection, using the given flags
	int releaseName(const char* name); // Releases the given name from the message bus connection
	bool startServiceByName(const char* name); // Asks the message bus to start the service associated with the given name; returns true if the requested service was not already running
	void addMatchRule(const char* rule,bool waitForReply); // Adds a match rule to filter incoming messages from the bus; if flag is set, waits for confirmation and throws exception on errors; otherwise, does not wait and does not signal errors
	void removeMatchRule(const char* rule,bool waitForReply); // Removes the match rule from the bus; if flag is set, waits for confirmation and throws exception on errors; otherwise, does not wait and does not signal errors
	};

}

#endif

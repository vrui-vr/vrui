/***********************************************************************
Message - Class encapsulating operations on DBus messages.
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

#ifndef DBUS_MESSAGE_INCLUDED
#define DBUS_MESSAGE_INCLUDED

#include <string>
#include <dbus/dbus.h>

/* Forward declarations: */
namespace DBus {
class PendingCall;
class Connection;
}

namespace DBus {

class Message
	{
	friend class PendingCall;
	friend class Connection;
	
	/* Embedded classes: */
	public:
	enum MessageType // Enumerated type for DBus message types
		{
		MethodCall=DBUS_MESSAGE_TYPE_METHOD_CALL,
		MethodReturn=DBUS_MESSAGE_TYPE_METHOD_RETURN,
		Error=DBUS_MESSAGE_TYPE_ERROR,
		Signal=DBUS_MESSAGE_TYPE_SIGNAL
		};
	typedef dbus_uint32_t Serial; // Type for message serial numbers
	
	class ReadIterator // Class to read a message's payload one element at a time
		{
		friend class Message;
		
		/* Elements: */
		private:
		DBusMessageIter iter; // The low-level DBus message iterator
		int currentType; // The type of element the iterator is currently pointing to, or DBUS_TYPE_INVALID if the iterator is at the end
		
		/* Constructors and destructors: */
		ReadIterator(const DBusMessageIter& sIter,int sCurrentType) // Creates a read iterator from a low-level DBus message iterator
			:iter(sIter),currentType(sCurrentType)
			{
			}
		
		/* Methods: */
		public:
		bool valid(void) const // Returns true if the iterator is pointing to a valid element
			{
			return currentType!=DBUS_TYPE_INVALID;
			}
		int getType(void) const // Returns the type of the element the iterator is currently pointing to
			{
			return currentType;
			}
		bool isObjectPath(void) const // Returns true if the element the iterator is currently pointing to is an object path
			{
			return currentType==DBUS_TYPE_OBJECT_PATH;
			}
		bool isUnixFd(void) const // Returns true if the element the iterator is currently pointing to is a UNIX file descriptor
			{
			return currentType==DBUS_TYPE_UNIX_FD;
			}
		bool isCompound(void) const // Returns true if the element the iterator is currently pointing to is a compound type
			{
			return currentType==DBUS_TYPE_ARRAY||currentType==DBUS_TYPE_VARIANT||currentType==DBUS_TYPE_STRUCT||currentType==DBUS_TYPE_DICT_ENTRY;
			}
		bool isArray(void) const // Returns true if the element the iterator is currently pointing to is an array
			{
			return currentType==DBUS_TYPE_ARRAY;
			}
		int getArrayElementType(void); // Returns the type of elements stored in the array the iterator is currently pointing to; throws exception if the iterator is not pointing to an array
		unsigned int getArrayElementCount(void); // Returns the number of elements stored in the array the iterator is currently pointing to; throws exception if the iterator is not pointing to an array; this message may be slow depending on the array's element type
		bool isVariant(void) const // Returns true if the element the iterator is currently pointing to is a variant
			{
			return currentType==DBUS_TYPE_VARIANT;
			}
		bool isStruct(void) const // Returns true if the element the iterator is currently pointing to is a structure
			{
			return currentType==DBUS_TYPE_STRUCT;
			}
		bool isDictEntry(void) const // Returns true if the element the iterator is currently pointing to is a dictionary entry
			{
			return currentType==DBUS_TYPE_DICT_ENTRY;
			}
		std::string getSignature(void); // Returns the type signature of the element the iterator is currently pointing to
		ReadIterator& operator++(void) // Pre-increments a valid iterator
			{
			/* Increment the iterator and check if it remains valid: */
			if(dbus_message_iter_next(&iter))
				{
				/* Update the iterator type: */
				currentType=dbus_message_iter_get_arg_type(&iter);
				}
			else
				{
				/* Mark the iterator as invalid: */
				currentType=DBUS_TYPE_INVALID;
				}
			
			return *this;
			}
		ReadIterator operator++(int) // Post-increments a valid iterator
			{
			ReadIterator result(*this);
			
			/* Increment the iterator and check if it remains valid: */
			if(dbus_message_iter_next(&iter))
				{
				/* Update the iterator type: */
				currentType=dbus_message_iter_get_arg_type(&iter);
				}
			else
				{
				/* Mark the iterator as invalid: */
				currentType=DBUS_TYPE_INVALID;
				}
			
			return result;
			}
		ReadIterator recurse(void); // Returns an iterator to read the elements of a container element; throws an exception if the iterator is not pointing to a container element
		template <class ElementParam>
		ElementParam read(void); // Returns the value of the element of a DBus basic type the iterator is pointing to; throws exception if the element type doesn't match the requested type; read strings via std::string
		const char* readString(void); // Returns the string the iterator is pointing to; result must not be freed; throws exception if the element type is not an object path
		const char* readObjectPath(void); // Returns the object path the iterator is pointing to; result must not be freed; throws exception if the element type is not an object path
		int readUnixFd(void); // Returns the UNIX file descriptor the iterator is pointing to; caller must close the file descriptor when done with it; throws exception if the element type is not a UNIX file descriptor
		template <class ElementParam>
		unsigned int readArray(ElementParam*& elements); // Reads the entire contents of an array element; the returned array must not be free'd; throws an exception if the iterator doesn't point to an array element, or the type of array elements does not mach the requested type
		unsigned int readBoolArray(dbus_bool_t*& elements); // Special version to read arrays of booleans; DBus's declaration of dbus_bool_t as unsigned int causes ambiguity
		};
	
	/* Elements: */
	private:
	DBusMessage* message; // Low-level DBus message handle
	
	/* Constructors and destructors: */
	public:
	Message(void) // Creates an invalid message
		:message(0)
		{
		}
	Message(DBusMessage* sMessage) // Creates a wrapper around the given low-level DBus message pointer
		:message(sMessage)
		{
		/* Take another reference to the low-level DBus message: */
		dbus_message_ref(message);
		}
	private:
	Message(DBusMessage* sMessage,int) // Creates a wrapper around the given low-level DBus message pointer without taking a reference; only called internally by other parts of the library
		:message(sMessage)
		{
		}
	public:
	Message(MessageType messageType); // Creates a new message of the given message type; don't call this, use the following static methods instead
	static Message createMethodCall(const char* destination,const char* path,const char* interface,const char* method); // Creates a method call message for the specified method
	static Message createMethodReturn(const Message& methodCallMessage); // Creates a method return message for the given method call message
	static Message createError(const Message& replyTo,const char* errorName,const char* errorMessage); // Creates an error message in reply to the given message
	static Message createSignal(const char* path,const char* interface,const char* signalName); // Creates a signal message
	Message(const Message& source) // Copy constructor
		:message(source.message)
		{
		/* Take another reference to the source's low-level DBus message: */
		dbus_message_ref(message);
		}
	Message(Message&& source) // Move constructor
		:message(source.message)
		{
		/* Invalidate the source: */
		source.message=0;
		}
	~Message(void) // Releases the low-level DBus message
		{
		/* Release the held reference to the low-level DBus message: */
		dbus_message_unref(message); // Unref-ing 0 is a valid no-op
		}
	
	/* Methods: */
	Message& operator=(const Message& source) // Copy assignment operator
		{
		if(this!=&source)
			{
			/* Release the held reference to the current low-level DBus message: */
			dbus_message_unref(message); // Unref-ing 0 is a valid no-op
			
			/* Copy and take another reference to the source's low-level DBus message: */
			message=source.message;
			dbus_message_ref(message);
			}
		
		return *this;
		}
	Message& operator=(Message&& source) // Move assignment operator
		{
		if(this!=&source)
			{
			/* Release the held reference to the current low-level DBus message: */
			dbus_message_unref(message); // Unref-ing 0 is a valid no-op
			
			/* Copy the source's low-level DBus message and invalidate the source: */
			message=source.message;
			source.message=0;
			}
		
		return *this;
		}
	bool valid(void) const // Returns true if this message is valid
		{
		return message!=0;
		}
	bool operator==(const DBusMessage* other) const // Equality operator; checks message references only, not message contents
		{
		return message==other;
		}
	bool operator==(const Message& other) const // Equality operator; checks message references only, not message contents
		{
		return message==other.message;
		}
	bool operator!=(const DBusMessage* other) const // Inequality operator; checks message references only, not message contents
		{
		return message!=other;
		}
	bool operator!=(const Message& other) const // Inequality operator; checks message references only, not message contents
		{
		return message!=other.message;
		}
	Message copy(void) const; // Returns a new private and unlocked message that is otherwise an exact copy of this message
	void lock(void) // Locks this message; this is generally done internally by DBus and not by user code
		{
		dbus_message_lock(message);
		}
	
	/* Methods to process received messages: */
	MessageType getType(void) // Returns the type of this message
		{
		return MessageType(dbus_message_get_type(message));
		}
	Serial getSerial(void) // Returns the serial number of this message or 0 if none has been specified
		{
		return dbus_message_get_serial(message);
		}
	Serial getReplySerial(void) // Returns the reply serial number of this message, i.e., the serial number of the message to which this message is a reply
		{
		return dbus_message_get_reply_serial(message);
		}
	const char* getSender(void) // Returns the unique name of the connection that originated this message or 0 if unknown or inapplicable
		{
		return dbus_message_get_sender(message);
		}
	bool hasSender(const char* sender) // Returns true if this message has the given sender
		{
		return dbus_message_has_sender(message,sender);
		}
	const char* getDestination(void) // Returns this message's destination, or 0 if none has been set
		{
		return dbus_message_get_destination(message);
		}
	bool hasDestination(const char* destination) // Returns true if this message has the given destination
		{
		return dbus_message_has_destination(message,destination);
		}
	const char* getPath(void) // Returns this message's path, or 0 if none has been set
		{
		return dbus_message_get_path(message);
		}
	bool hasPath(const char* path) // Returns true if this message has the given path
		{
		return dbus_message_has_path(message,path);
		}
	const char* getInterface(void) // Returns this message's interface, or 0 if none has been set
		{
		return dbus_message_get_interface(message);
		}
	bool hasInterface(const char* interface) // Returns true if this message has the given interface
		{
		return dbus_message_has_interface(message,interface);
		}
	const char* getMember(void) // Returns this message's member, or 0 if none has been set
		{
		return dbus_message_get_member(message);
		}
	bool hasMember(const char* member) // Returns true if this message has the given member
		{
		return dbus_message_has_member(message,member);
		}
	bool isMethodCall(const char* interface,const char* member) // Returns true if this message is a method call for the given member of the given interface
		{
		return dbus_message_is_method_call(message,interface,member);
		}
	const char* getErrorName(void) // Returns this message's error name, or 0 if none has been set or this message is not an error message
		{
		return dbus_message_get_error_name(message);
		}
	bool isError(const char* errorName) // Returns true if this message has the given error name
		{
		return dbus_message_is_error(message,errorName);
		}
	bool isSignal(const char* interface,const char* signalName) // Returns true if this message is a signal of the given name of the given interface
		{
		return dbus_message_is_signal(message,interface,signalName);
		}
	bool getNoReply(void) // Returns true if this message does not expect a reply
		{
		return dbus_message_get_no_reply(message);
		}
	bool getAutostart(void) // Returns true if this message requests that an owner for its destination will be started before this message is delivered
		{
		return dbus_message_get_auto_start(message);
		}
	const char* getSignature(void) // Returns the type signature of this message's payload as a string
		{
		return dbus_message_get_signature(message);
		}
	bool hasSignature(const char* signature) // Returns true if this message's payload has the given type signature
		{
		return dbus_message_has_signature(message,signature);
		}
	bool containsUnixFds(void) // Returns true if this message contains any UNIX file descriptors
		{
		return dbus_message_contains_unix_fds(message);
		}
	ReadIterator getReadIterator(void); // Returns an iterator to read this message's payload
	
	/* Methods to prepare outgoing messages: */
	void setSerial(Serial newSerial) // Sets the serial number of this message
		{
		dbus_message_set_serial(message,newSerial);
		}
	void setReplySerial(Serial newReplySerial); // Sets the reply serial number of this message
	void setDestination(const char* newDestination); // Sets this message's destination; resets the destination if the given pointer is 0
	void setPath(const char* newPath); // Sets this message's path; resets the path if the given pointer is 0
	void setInterface(const char* newInterface); // Sets this message's interface; resets the interface if the given pointer is 0
	void setMember(const char* newMember); // Sets this message's member; resets the member if the given pointer is 0
	void setErrorName(const char* newErrorName); // Sets this message's error name; resets the error name if the given pointer is 0
	void setNoReply(bool newNoReply) // Sets this message's noReply flag
		{
		dbus_message_set_no_reply(message,newNoReply);
		}
	void setAutostart(bool newAutostart) // Sets this message's autostart flag
		{
		dbus_message_set_auto_start(message,newAutostart);
		}
	template <class ArgumentParam>
	void appendArgument(const ArgumentParam& argument); // Appends a single argument of a DBus basic type to this message
	void appendString(const char* string); // Appends a string argument to this message
	template <class ArgumentParam>
	void appendArray(const ArgumentParam* array,unsigned int numElements); // Appends an array of a DBus basic type to this message
	};

}

#endif

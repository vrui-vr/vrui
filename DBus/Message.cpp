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

#include <DBus/Message.h>

#include <Misc/StdError.h>

namespace DBus {

/**************************************
Methods of class Message::ReadIterator:
**************************************/

int Message::ReadIterator::getArrayElementType(void)
	{
	/* Check that the iterator points to an array: */
	if(currentType!=DBUS_TYPE_ARRAY)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Iterator is not pointing to an array");
	
	/* Return the array element type: */
	return dbus_message_iter_get_element_type(&iter);
	}

unsigned int Message::ReadIterator::getArrayElementCount(void)
	{
	/* Check that the iterator points to an array: */
	if(currentType!=DBUS_TYPE_ARRAY)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Iterator is not pointing to an array");
	
	/* Return the array element count: */
	return dbus_message_iter_get_element_count(&iter);
	}

std::string Message::ReadIterator::getSignature(void)
	{
	/* Call the low-level method and check for errors: */
	char* signature=dbus_message_iter_get_signature(&iter);
	if(signature==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Out of memory");
	
	/* Copy the signature to a std::string, then free it: */
	std::string result(signature);
	dbus_free(signature);
	
	return result;
	}

Message::ReadIterator Message::ReadIterator::recurse(void)
	{
	/* Check if the iterator is actually pointing at a container element: */
	if(currentType!=DBUS_TYPE_ARRAY&&currentType!=DBUS_TYPE_VARIANT&&currentType!=DBUS_TYPE_STRUCT&&currentType!=DBUS_TYPE_DICT_ENTRY)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Iterator is not pointing to a compound element");
	
	/* Initialize a low-level sub-iterator: */
	DBusMessageIter sub;
	int currentType=DBUS_TYPE_INVALID;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Query the sub-iterator's initial type: */
	int subCurrentType=dbus_message_iter_get_arg_type(&sub);
	
	return ReadIterator(sub,subCurrentType);
	}

namespace {

/****************
Helper functions:
****************/

void checkType(int haveType,int wantType,const char* source)
	{
	if(haveType!=wantType)
		throw Misc::makeStdErr(source,"Iterator points to %c, not %c",haveType,wantType);
	}

void checkArrayType(int containerType,DBusMessageIter* iter,int wantType,const char* source)
	{
	if(containerType!=DBUS_TYPE_ARRAY)
		throw Misc::makeStdErr(source,"Iterator is not pointing to an array");
	int haveType=dbus_message_iter_get_element_type(iter);
	if(haveType!=wantType)
		throw Misc::makeStdErr(source,"Iterator points to an array of %c, not %c",haveType,wantType);
	}

}

template <>
bool Message::ReadIterator::read<bool>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_BOOLEAN,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	dbus_bool_t result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
unsigned char Message::ReadIterator::read<unsigned char>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_BYTE,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	unsigned char result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
dbus_int16_t Message::ReadIterator::read<dbus_int16_t>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_INT16,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	dbus_int16_t result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
dbus_uint16_t Message::ReadIterator::read<dbus_uint16_t>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_UINT16,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	dbus_int16_t result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
dbus_int32_t Message::ReadIterator::read<dbus_int32_t>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_INT32,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	dbus_int32_t result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
dbus_uint32_t Message::ReadIterator::read<dbus_uint32_t>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_UINT32,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	dbus_int32_t result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
dbus_int64_t Message::ReadIterator::read<dbus_int64_t>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_INT64,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	dbus_int64_t result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
dbus_uint64_t Message::ReadIterator::read<dbus_uint64_t>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_UINT64,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	dbus_int64_t result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
double Message::ReadIterator::read<double>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_DOUBLE,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	double result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
std::string Message::ReadIterator::read<std::string>(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_STRING,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	char* result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

const char* Message::ReadIterator::readString(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_STRING,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	char* result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

const char* Message::ReadIterator::readObjectPath(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_OBJECT_PATH,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element: */
	char* result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

int Message::ReadIterator::readUnixFd(void)
	{
	/* Check that the iterator points at the correct type: */
	checkType(currentType,DBUS_TYPE_UNIX_FD,__PRETTY_FUNCTION__);
	
	/* Retrieve and return the element as an integer: */
	int result;
	dbus_message_iter_get_basic(&iter,&result);
	return result;
	}

template <>
unsigned int Message::ReadIterator::readArray<unsigned char>(unsigned char*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_BYTE,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

template <>
unsigned int Message::ReadIterator::readArray<dbus_int16_t>(dbus_int16_t*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_INT16,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

template <>
unsigned int Message::ReadIterator::readArray<dbus_uint16_t>(dbus_uint16_t*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_UINT16,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

template <>
unsigned int Message::ReadIterator::readArray<dbus_int32_t>(dbus_int32_t*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_INT32,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

template <>
unsigned int Message::ReadIterator::readArray<dbus_uint32_t>(dbus_uint32_t*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_UINT32,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

template <>
unsigned int Message::ReadIterator::readArray<dbus_int64_t>(dbus_int64_t*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_INT64,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

template <>
unsigned int Message::ReadIterator::readArray<dbus_uint64_t>(dbus_uint64_t*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_UINT64,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

template <>
unsigned int Message::ReadIterator::readArray<double>(double*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_DOUBLE,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

unsigned int Message::ReadIterator::readBoolArray(dbus_bool_t*& elements)
	{
	/* Check that the iterator points at an array of the correct type: */
	checkArrayType(currentType,&iter,DBUS_TYPE_BOOLEAN,__PRETTY_FUNCTION__);
	
	/* Recurse into the array: */
	DBusMessageIter sub;
	dbus_message_iter_recurse(&iter,&sub);
	
	/* Read the array's elements: */
	int numElements;
	dbus_message_iter_get_fixed_array(&iter,&elements,&numElements);
	return numElements;
	}

/************************
Methods of class Message:
************************/

Message::Message(Message::MessageType messageType)
	:message(0)
	{
	/* Create a new message of the requested type and check for errors: */
	message=dbus_message_new(messageType);
	if(message==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Out of memory");
	}

Message Message::createMethodCall(const char* destination,const char* path,const char* interface,const char* method)
	{
	/* Create a new method call message and check for errors: */
	DBusMessage* message=dbus_message_new_method_call(destination,path,interface,method);
	if(message==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Out of memory");
	
	/* Wrap the DBus message in a Message object without taking another reference: */
	return Message(message,0);
	}

Message Message::createMethodReturn(const Message& methodCallMessage)
	{
	/* Create a new method return message and check for errors: */
	DBusMessage* message=dbus_message_new_method_return(methodCallMessage.message);
	if(message==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Out of memory");
	
	/* Wrap the DBus message in a Message object without taking another reference: */
	return Message(message,0);
	}

Message Message::createError(const Message& replyTo,const char* errorName,const char* errorMessage)
	{
	/* Create a new error message and check for errors: */
	DBusMessage* message=dbus_message_new_error(replyTo.message,errorName,errorMessage);
	if(message==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Out of memory");
	
	/* Wrap the DBus message in a Message object without taking another reference: */
	return Message(message,0);
	}

Message Message::createSignal(const char* path,const char* interface,const char* signalName)
	{
	/* Create a new signal message and check for errors: */
	DBusMessage* message=dbus_message_new_signal(path,interface,signalName);
	if(message==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Out of memory");
	
	/* Wrap the DBus message in a Message object without taking another reference: */
	return Message(message,0);
	}

Message Message::copy(void) const
	{
	/* Copy this message and check for errors: */
	DBusMessage* result=dbus_message_copy(message);
	if(result==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Out of memory");
	
	/* Wrap the DBus message in a Message object without taking another reference: */
	return Message(result,0);
	}

Message::ReadIterator Message::getReadIterator(void)
	{
	/* Initialize a low-level iterator: */
	DBusMessageIter iter;
	int currentType=DBUS_TYPE_INVALID;
	if(dbus_message_iter_init(message,&iter))
		{
		/* Query the iterator's initial type: */
		currentType=dbus_message_iter_get_arg_type(&iter);
		}
	
	return ReadIterator(iter,currentType);
	}

void Message::setReplySerial(Serial newReplySerial)
	{
	/* Set this message's reply serial and check for errors: */
	if(!dbus_message_set_reply_serial(message,newReplySerial))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set reply serial number");
	}

void Message::setDestination(const char* newDestination)
	{
	/* Set this message's destination and check for errors: */
	if(!dbus_message_set_destination(message,newDestination))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set destination");
	}

void Message::setPath(const char* newPath)
	{
	/* Set this message's path and check for errors: */
	if(!dbus_message_set_path(message,newPath))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set path");
	}

void Message::setInterface(const char* newInterface)
	{
	/* Set this message's interface and check for errors: */
	if(!dbus_message_set_interface(message,newInterface))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set interface");
	}

void Message::setMember(const char* newMember)
	{
	/* Set this message's member and check for errors: */
	if(!dbus_message_set_member(message,newMember))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set member");
	}

void Message::setErrorName(const char* newErrorName)
	{
	/* Set this message's error name and check for errors: */
	if(!dbus_message_set_error_name(message,newErrorName))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set error name");
	}

/*************************************************
Specializations of appendArgument template method:
*************************************************/

template <>
void Message::appendArgument<bool>(const bool& argument)
	{
	/* Convert the argument to DBus's internal type, append it, and check for errors: */
	dbus_bool_t internalArg=argument;
	if(!dbus_message_append_args(message,DBUS_TYPE_BOOLEAN,&internalArg,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<unsigned char>(const unsigned char& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_BYTE,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<dbus_int16_t>(const dbus_int16_t& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_INT16,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<dbus_uint16_t>(const dbus_uint16_t& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_UINT16,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<dbus_int32_t>(const dbus_int32_t& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_INT32,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<dbus_uint32_t>(const dbus_uint32_t& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_UINT32,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<dbus_int64_t>(const dbus_int64_t& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_INT64,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<dbus_uint64_t>(const dbus_uint64_t& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_UINT64,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<float>(const float& argument)
	{
	/* Convert the argument to DBus's internal type, append it, and check for errors: */
	double internalArg=argument;
	if(!dbus_message_append_args(message,DBUS_TYPE_DOUBLE,&internalArg,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

template <>
void Message::appendArgument<double>(const double& argument)
	{
	/* Append the argument and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_DOUBLE,&argument,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

void Message::appendString(const char* string)
	{
	/* Append the string and check for errors: */
	if(!dbus_message_append_args(message,DBUS_TYPE_STRING,&string,DBUS_TYPE_INVALID))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot append argument");
	}

}

/***********************************************************************
JsonEntityTypes - Classes for concrete entities parsed from JSON
(JavaScript Object Notation) texts, as defined by IETF RFC 8259.
Copyright (c) 2018-2026 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <IO/JsonEntityTypes.h>

namespace IO {

/****************************
Methods of class JsonBoolean:
****************************/

JsonEntity::EntityType JsonBoolean::getType(void) const
	{
	return BOOLEAN;
	}

std::string JsonBoolean::getTypeName(void) const
	{
	return "Boolean";
	}

void JsonBoolean::print(std::ostream& os) const
	{
	os<<(value?"true":"false");
	}

/***************************
Methods of class JsonNumber:
***************************/

JsonEntity::EntityType JsonNumber::getType(void) const
	{
	return NUMBER;
	}

std::string JsonNumber::getTypeName(void) const
	{
	return "Number";
	}

void JsonNumber::print(std::ostream& os) const
	{
	os<<number;
	}

/***************************
Methods of class JsonString:
***************************/

JsonEntity::EntityType JsonString::getType(void) const
	{
	return STRING;
	}

std::string JsonString::getTypeName(void) const
	{
	return "String";
	}

namespace {

/****************
Helper functions:
****************/

inline char toHex(int hexDigit)
	{
	hexDigit&=0x0f;
	return hexDigit<10?hexDigit+'0':(hexDigit-10)+'A';
	}

}

void JsonString::print(std::ostream& os) const
	{
	/* Print the opening double quote: */
	os<<'"';
	
	/* Print the string one character at a time, escaping special characters as needed: */
	for(std::string::const_iterator sIt=string.begin();sIt!=string.end();++sIt)
		{
		/* Check if the character can be printed as-is: */
		unsigned char s=*sIt;
		if(s>=0x20&&s!='"'&&s!='\\')
			os<<*sIt;
		else
			{
			/* Escape the character: */
			switch(*sIt)
				{
				case '\b': // Backspace
					os<<"\\b";
					break;
				
				case '\t': // Tab
					os<<"\\t";
					break;
				
				case '\n': // Line feed
					os<<"\\n";
					break;
				
				case '\f': // Form feed
					os<<"\\f";
					break;
				
				case '\r': // Carriage return
					os<<"\\r";
					break;
				
				case '"': // Quotation mark
					os<<"\\\"";
					break;
				
				case '\\': // Reverse solidus
					os<<"\\\\";
					break;
				
				default:
					/* Print a control character that doesn't have a short escape sequence as a four-digit hexadecimal number, but with two leading zeros because it's always <0x20: */
					os<<"\\u00"<<toHex(int(s)>>4)<<toHex(int(s));
				}
			}
		}
	
	/* Print the closing double quote: */
	os<<'"';
	}

/**************************
Methods of class JsonArray:
**************************/

JsonEntity::EntityType JsonArray::getType(void) const
	{
	return ARRAY;
	}

std::string JsonArray::getTypeName(void) const
	{
	return "Array";
	}

void JsonArray::print(std::ostream& os) const
	{
	os<<'[';
	
	/* Print all array items in order: */
	bool needComma=false;
	for(Array::const_iterator aIt=array.begin();aIt!=array.end();++aIt)
		{
		if(needComma)
			os<<',';
		
		/* Check for null items: */
		if(*aIt!=0)
			(*aIt)->print(os);
		else
			os<<"null";
		
		needComma=true;
		}
	
	os<<']';
	}

/**************************
Methods of class JsonObject:
**************************/

JsonEntity::EntityType JsonObject::getType(void) const
	{
	return OBJECT;
	}

std::string JsonObject::getTypeName(void) const
	{
	return "Object";
	}

void JsonObject::print(std::ostream& os) const
	{
	os<<'{';
	
	/* Print all object properties in some random order: */
	bool needComma=false;
	for(Map::ConstIterator mIt=map.begin();!mIt.isFinished();++mIt)
		{
		if(needComma)
			os<<',';
		
		/* Check for null property values: */
		os<<'"'<<mIt->getSource()<<'"'<<':';
		if(mIt->getDest()!=0)
			mIt->getDest()->print(os);
		else
			os<<"null";
		
		needComma=true;
		}
	
	os<<'}';
	}

}

/***********************************************************************
JsonSource - Class to retrieve JSON entities from JSON files.
Copyright (c) 2018-2024 Oliver Kreylos

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

#include <IO/JsonSource.h>

#include <Misc/StdError.h>
#include <IO/OpenFile.h>
#include <IO/JsonEntityTypes.h>

namespace IO {

/***************************
Methods of class JsonSource:
***************************/

namespace {

/****************
Helper functions:
****************/

inline bool isHex(int c)
	{
	if(c>='A')
		{
		if(c>='a')
			return c<='f';
		else
			return c<='F';
		}
	else
		return c>='0'&&c<='9';
	}

inline int fromHex(int c)
	{
	/* Check whether the character is an uppercase or lowercase character or a decimal digit: */
	if(c>='A')
		{
		/* Check if it's uppercase or lowercase: */
		if(c>='a')
			return (c-'a')+10;
		else
			return (c-'A')+10;
		}
	else
		return c-'0';
	}

}

std::string JsonSource::parseJsonString(void)
	{
	std::string result;
	
	/* Skip the opening quote: */
	file.getChar();
	
	/* Read until end-of-file or the closing quote: */
	while(!file.eof()&&file.peekc()!='"')
		{
		/* Check for a regular character: */
		if(file.peekc()!='\\')
			{
			/* Read the character as-is: */
			result.push_back(file.getChar());
			}
		else
			{
			/* Skip the escape character: */
			file.getChar();
			
			/* Handle and skip the escape sequence: */
			switch(file.peekc())
				{
				case 'b': // Backspace
					result.push_back('\b');
					file.getChar();
					break;
				
				case 't': // Tab
					result.push_back('\t');
					file.getChar();
					break;
				
				case 'n': // Line feed
					result.push_back('\n');
					file.getChar();
					break;
				
				case 'f': // Form feed
					result.push_back('\f');
					file.getChar();
					break;
				
				case 'r': // Carriage return
					result.push_back('\r');
					file.getChar();
					break;
				
				case '"': // Quotation mark
					result.push_back('"');
					file.getChar();
					break;
				
				case '/': // Solidus -- why is there an escape sequence for the solidus? It's a valid regular character!
					result.push_back('/');
					file.getChar();
					break;
				
				case '\\': // Reverse solidus
					result.push_back('\\');
					file.getChar();
					break;
				
				case 'u': // Four-digit hexadeximal number
					{
					/* Skip the u tag: */
					file.getChar();
					
					/* Parse the hexadecimal character code: */
					int charCode=0;
					int i;
					for(i=0;i<4&&isHex(file.peekc());++i)
						charCode=(charCode<<4)|fromHex(file.getChar());
					if(i<4)
						throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Illegal digit %c in \\u escape sequence",file.peekc());
					
					/* Encode the character code as UTF-8: */
					if(charCode>=0x800)
						{
						/* Encode the character as a three-byte sequence: */
						result.push_back(0xe0|(charCode>>12));
						result.push_back(0x80|((charCode>>6)&0x3f));
						result.push_back(0x80|(charCode&0x3f));
						}
					else if(charCode>=0x80)
						{
						/* Encode the character as a two-byte sequence: */
						result.push_back(0xc0|(charCode>>6));
						result.push_back(0x80|(charCode&0x3f));
						}
					else
						{
						/* Encode the character as a one-byte sequence: */
						result.push_back(charCode);
						}
					
					break;
					}
				
				default:
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Illegal escape sequence \\%c",file.peekc());
				}
			}
		}
	
	/* Check for and skip the closing quote and whitespace: */
	if(!file.isLiteral('"'))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unterminated string");
	
	return result;
	}

JsonSource::JsonSource(const char* fileName)
	:file(openFile(fileName))
	{
	/* Set up the JSON file syntax: */
	file.setWhitespace('\n',true);
	file.setWhitespace('\r',true);
	file.setPunctuation("{}[]:,\"");
	
	/* Prepare for reading: */
	file.skipWs();
	}

JsonSource::JsonSource(FilePtr sFile)
	:file(sFile)
	{
	/* Set up the JSON file syntax: */
	file.setWhitespace('\n',true);
	file.setWhitespace('\r',true);
	file.setPunctuation("{}[]:,\"");
	
	/* Prepare for reading: */
	file.skipWs();
	}

JsonSource::JsonSource(File& sFile)
	:file(sFile)
	{
	/* Set up the JSON file syntax: */
	file.setWhitespace('\n',true);
	file.setWhitespace('\r',true);
	file.setPunctuation("{}[]:,\"");
	
	/* Prepare for reading: */
	file.skipWs();
	}

JsonPointer JsonSource::parseEntity(void)
	{
	/* Check for end-of-file (not necessary, but gives more descriptive error message): */
	if(file.eof())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unexpected end-of-file");
	
	/* Determine the type of the next entity: */
	switch(file.peekc())
		{
		case '"': // String
			{
			/* Parse a JSON string: */
			// return new JsonString(std::move(parseJsonString()));
			return new JsonString(parseJsonString());
			}
		
		case '[': // Array
			{
			/* Skip the opening bracket and whitespace: */
			file.readChar();
			
			/* Create a new array entity: */
			JsonArray* array=new JsonArray;
			JsonPointer result(array);
			
			/* Parse array items until the closing bracket: */
			bool needComma=false;
			while(!file.eof()&&file.peekc()!=']')
				{
				/* Check for and skip a comma and whitespace if there was a previous array item: */
				if(needComma&&!file.isLiteral(','))
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing comma in array");
				
				/* Check for a missing array item (not necessary, but gives more descriptive error message): */
				if(file.eof()||file.peekc()==','||file.peekc()==']')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Extra comma in array");
				
				/* Parse the next array item: */
				JsonPointer item=parseEntity();
				array->getArray().push_back(item);
				
				needComma=true;
				}
			
			/* Check for and skip the closing bracket and whitespace: */
			if(!file.isLiteral(']'))
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unterminated array");
			
			return result;
			}
		
		case '{': // Object
			{
			/* Skip the opening brace and whitespace: */
			file.readChar();
			
			/* Create a new object entity: */
			JsonObject* object=new JsonObject;
			JsonPointer result(object);
			
			/* Parse object properties until the closing brace: */
			bool needComma=false;
			while(!file.eof()&&file.peekc()!='}')
				{
				/* Check for and skip a comma and whitespace if there was a previous object property: */
				if(needComma&&!file.isLiteral(','))
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing comma in object");
				
				/* Check for a missing property (not necessary, but gives more descriptive error message): */
				if(file.eof()||file.peekc()==','||file.peekc()=='}')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Extra comma in object");
				
				/* Parse the next property name: */
				if(file.peekc()!='"')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing opening quote in object property name");
				std::string name=parseJsonString();
				
				/* Check for the name/value separator: */
				if(!file.isLiteral(':'))
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing colon in object item");
				
				/* Check for a missing value (not necessary, but gives more descriptive error message): */
				if(file.eof()||file.peekc()=='}')
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing object property value");
					
				/* Parse the next property value: */
				JsonPointer value=parseEntity();
				
				/* Store the association: */
				object->getMap()[name]=value;
				
				needComma=true;
				}
			
			/* Check for and skip the closing brace and whitespace: */
			if(!file.isLiteral('}'))
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unterminated object");
			
			return result;
			}
		
		case 'F': // Boolean literal
		case 'f':
		case 'T':
		case 't':
			{
			std::string value=file.readString();
			if(strcmp(value.c_str(),"true")==0)
				return new JsonBoolean(true);
			else if(strcmp(value.c_str(),"false")==0)
				return new JsonBoolean(false);
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Illegal literal %s",value.c_str());
			}
		
		case 'n': // NULL literal
		case 'N':
			{
			std::string null=file.readString();
			if(strcmp(null.c_str(),"null")==0)
				return 0;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Illegal literal %s",null.c_str());
			}
		
		case '+': // Number
		case '-':
		case '.':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
			/* Parse a number: */
			double number=file.readNumber();
			return new JsonNumber(number);
			}
		
		case ',':
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Comma outside array or object");
		
		case ']':
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing opening bracket in array");
		
		case '}':
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing opening brace in object");
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Illegal character %c",file.peekc());
		}
	}

}

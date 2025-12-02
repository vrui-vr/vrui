/***********************************************************************
CSVSource - Class to read tabular data from input streams in generalized
comma-separated value (CSV) format.
Copyright (c) 2010-2025 Oliver Kreylos

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

#include <IO/CSVSource.h>

#include <ctype.h>
#include <math.h>
#include <string>
#include <Misc/StdError.h>

namespace IO {

namespace {

/*********************************************
Helper class to give readable names for types:
*********************************************/

template <class TypeParam>
class TypeName
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "unkown";
		}
	};

template <>
class TypeName<unsigned int>
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "unsigned int";
		}
	};

template <>
class TypeName<int>
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "int";
		}
	};

template <>
class TypeName<unsigned long>
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "unsigned long";
		}
	};

template <>
class TypeName<long>
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "long";
		}
	};

template <>
class TypeName<float>
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "float";
		}
	};

template <>
class TypeName<double>
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "double";
		}
	};

template <>
class TypeName<std::string>
	{
	/* Methods: */
	public:
	static const char* getName(void)
		{
		return "std::string";
		}
	};

}

/***************************************
Methods of class CSVSource::FormatError:
***************************************/

CSVSource::FormatError::FormatError(const char* source,unsigned int fieldIndex,size_t recordIndex)
	:std::runtime_error(Misc::makeStdErrMsg(source,"Format error in field %u of record %u",fieldIndex,(unsigned int)recordIndex))
	{
	}

/*******************************************
Methods of class CSVSource::ConversionError:
*******************************************/

CSVSource::ConversionError::ConversionError(const char* source,unsigned int fieldIndex,size_t recordIndex,const char* dataTypeName)
	:std::runtime_error(Misc::makeStdErrMsg(source,"Could not convert field %u of record %u to type %s",fieldIndex,(unsigned int)recordIndex,dataTypeName))
	{
	}

/*******************************************
Declaration of class CSVSource::FieldReader:
*******************************************/

class CSVSource::FieldReader // Helper class to read quoted or unquoted fields
	{
	/* Elements: */
	private:
	const char* sourceFunction; // Name of the function that created this object
	CSVSource& csvSource; // Reference to the CSV source object
	File& source; // Direct reference to the CSV source's character source
	unsigned char* cc; // The array of character classes
	int quote; // Quote character for a quoted field or -1 for unquoted fields
	unsigned char fieldMask; // Bit mask for characters valid in the current field
	int lastChar; // The last read character
	
	/* Constructors and destructors: */
	public:
	FieldReader(const char* sSourceFunction,CSVSource& sCsvSource)
		:sourceFunction(sSourceFunction),
		 csvSource(sCsvSource),
		 source(*csvSource.source),cc(csvSource.cc),
		 quote(-1),fieldMask(FIELD),
		 lastChar(csvSource.lastChar)
		{
		/* Check if the field is quoted: */
		if((cc[lastChar]&QUOTE)!=0x0U)
			{
			/* Remember the quote character and remove it from the set of valid quoted field characters: */
			quote=lastChar;
			cc[quote]&=~QUOTEDFIELD;
			
			/* Read a quoted field: */
			fieldMask=QUOTEDFIELD;
			
			/* Skip the quote character: */
			lastChar=source.getChar();
			}
		}
	~FieldReader(void)
		{
		/* Check if this was a quoted field: */
		if(quote>=0)
			{
			/* Re-insert the quote character into the set of valid quoted field characters: */
			cc[quote]|=QUOTEDFIELD;
			}
		
		/* Update the CSV source's last read character: */
		csvSource.lastChar=lastChar;
		}
	
	/* Methods: */
	int getChar(void) // Returns the next character from the current field, or -1 at the end of the field
		{
		/* Remember the character to be returned: */
		int result=lastChar;
		
		/* Check if the character is a valid field character or something else: */
		if((cc[result]&fieldMask)!=0x0U)
			{
			/* Read the next character: */
			lastChar=source.getChar();
			}
		else if(result<0)
			{
			/* Check if the field was quoted: */
			if(quote>=0)
				{
				/* Signal a format error: */
				throw FormatError(sourceFunction,csvSource.fieldIndex,csvSource.recordIndex);
				}
			else
				{
				/* Field is finished: */
				result=-1;
				}
			}
		else if(result==quote)
			{
			/* Skip the quote character and check for a double (quoted) quote: */
			lastChar=source.getChar();
			if(lastChar==quote)
				{
				/* Skip the second quote as well: */
				lastChar=source.getChar();
				}
			else
				{
				/* Field is finished: */
				result=-1;
				}
			}
		else
			{
			/* Field is finished: */
			result=-1;
			}
		
		return result;
		}
	void skipWhitespace(void) // Skips whitespace
		{
		/* Skip valid characters for the current field type that are also whitespace: */
		unsigned char wsMask=fieldMask|WHITESPACE;
		while((cc[lastChar]&wsMask)==wsMask)
			lastChar=source.getChar();
		}
	bool readValue(unsigned int& result) // Read a numeric value in unsigned int format
		{
		/* Check for an optional plus sign: */
		if(lastChar=='+')
			lastChar=source.getChar();
		
		/* Read the first digit: */
		if((cc[lastChar]&DIGIT)==0x0U)
			return false;
		result=(unsigned int)(lastChar-'0');
		lastChar=source.getChar();
		
		/* Read all following digits: */
		while((cc[lastChar]&DIGIT)!=0x0U)
			{
			result=result*10U+(unsigned int)(lastChar-'0');
			lastChar=source.getChar();
			}
		
		return true;
		}
	bool readValue(int& result) // Read a numeric value in signed int format
		{
		/* Check for an optional plus or minus sign: */
		bool negative=false;
		if(lastChar=='+')
			lastChar=source.getChar();
		else if(lastChar=='-')
			{
			negative=true;
			lastChar=source.getChar();
			}
		
		/* Read the first digit: */
		if((cc[lastChar]&DIGIT)==0x0U)
			return false;
		result=int(lastChar-'0');
		lastChar=source.getChar();
		
		/* Read all following digits: */
		while((cc[lastChar]&DIGIT)!=0x0U)
			{
			result=result*10+int(lastChar-'0');
			lastChar=source.getChar();
			}
		
		/* Negate the result if there was a minus sign: */
		if(negative)
			result=-result;
		
		return true;
		}
	bool readValue(unsigned long& result) // Read a numeric value in unsigned long format
		{
		/* Check for an optional plus sign: */
		if(lastChar=='+')
			lastChar=source.getChar();
		
		/* Read the first digit: */
		if((cc[lastChar]&DIGIT)==0x0U)
			return false;
		result=(unsigned long)(lastChar-'0');
		lastChar=source.getChar();
		
		/* Read all following digits: */
		while((cc[lastChar]&DIGIT)!=0x0U)
			{
			result=result*10UL+(unsigned long)(lastChar-'0');
			lastChar=source.getChar();
			}
		
		return true;
		}
	bool readValue(long& result) // Read a numeric value in signed long format
		{
		/* Check for an optional plus or minus sign: */
		bool negative=false;
		if(lastChar=='+')
			lastChar=source.getChar();
		else if(lastChar=='-')
			{
			negative=true;
			lastChar=source.getChar();
			}
		
		/* Read the first digit: */
		if((cc[lastChar]&DIGIT)==0x0U)
			return false;
		result=long(lastChar-'0');
		lastChar=source.getChar();
		
		/* Read all following digits: */
		while((cc[lastChar]&DIGIT)!=0x0U)
			{
			result=result*10L+long(lastChar-'0');
			lastChar=source.getChar();
			}
		
		/* Negate the result if there was a minus sign: */
		if(negative)
			result=-result;
		
		return true;
		}
	bool readValue(float& result) // Read a numeric value in float format
		{
		/* Simply use the double-valued method: */
		double tempResult;
		bool valid=readValue(tempResult);
		result=float(tempResult);
		return valid;
		}
	bool readValue(double& result) // Read a numeric value in double format
		{
		/* Check for an optional plus or minus sign: */
		bool negative=false;
		if(lastChar=='+')
			lastChar=source.getChar();
		else if(lastChar=='-')
			{
			negative=true;
			lastChar=source.getChar();
			}
		
		/* Keep track if any digits have been read: */
		bool haveDigit=false;
		
		/* Read an integral number part: */
		result=0.0;
		while((cc[lastChar]&DIGIT)!=0x0U)
			{
			haveDigit=true;
			result=result*10.0+double(lastChar-'0');
			lastChar=source.getChar();
			}
		
		/* Check for a period: */
		if(lastChar=='.')
			{
			lastChar=source.getChar();
			
			/* Read a fractional number part: */
			double fraction=0.0;
			double fractionBase=1.0;
			while((cc[lastChar]&DIGIT)!=0x0U)
				{
				haveDigit=true;
				fraction=fraction*10.0+double(lastChar-'0');
				fractionBase*=10.0;
				lastChar=source.getChar();
				}
			
			result+=fraction/fractionBase;
			}
		
		/* Signal a conversion error if no digits were read in the integral or fractional part: */
		if(!haveDigit)
			return false;
		
		/* Negate the result if a minus sign was read: */
		if(negative)
			result=-result;
		
		/* Check for an exponent indicator: */
		if(lastChar=='e'||lastChar=='E')
			{
			lastChar=source.getChar();
			
			/* Read a plus or minus sign: */
			bool exponentNegative=false;
			if(lastChar=='+')
				lastChar=source.getChar();
			else if(lastChar=='-')
				{
				exponentNegative=true;
				lastChar=source.getChar();
				}
			
			/* Read the first exponent digit: */
			if((cc[lastChar]&DIGIT)==0x0U)
				return false;
			double exponent=double(lastChar-'0');
			lastChar=source.getChar();
			
			/* Read the rest of the exponent digits: */
			while((cc[lastChar]&DIGIT)!=0x0U)
				{
				exponent=exponent*10.0+double(lastChar-'0');
				lastChar=source.getChar();
				}
			
			/* Multiply the mantissa with the exponent: */
			if(exponentNegative)
				exponent=-exponent;
			result*=pow(10.0,exponent);
			}
		
		return true;
		}
	void finishField(void) // Finishes reading the current field
		{
		/* Check whether the next character is a record or field separator: */
		if((cc[lastChar]&RECORDSEP)!=0x0U)
			{
			/* Remember and skip the separator character: */
			int separator=lastChar;
			lastChar=source.getChar();
			
			/* If this is a CR/LF pair, skip the LF character as well: */
			if(separator=='\r'&&(cc[lastChar]&RECORDSEP)!=0x0U&&lastChar=='\n')
				lastChar=source.getChar();
			
			/* Increase the record index and reset the field index: */
			++csvSource.recordIndex;
			csvSource.fieldIndex=0;
			}
		else if((cc[lastChar]&FIELDSEP)!=0x0U)
			{
			/* Skip the field separator: */
			lastChar=source.getChar();
			
			/* Increase the field index: */
			++csvSource.fieldIndex;
			}
		else
			{
			/* Signal a format error: */
			throw FormatError(sourceFunction,csvSource.fieldIndex,csvSource.recordIndex);
			}
		}
	};

/**************************
Methods of class CSVSource:
**************************/

#if 0

template <>
bool CSVSource::convertNumber(int& nextChar,double& value)
	{
	/* Check for optional sign: */
	bool negated=false;
	if(nextChar=='-')
		{
		negated=true;
		nextChar=source->getChar();
		}
	else if(nextChar=='+')
		nextChar=source->getChar();
	
	/* Keep track if any digits have been read: */
	bool haveDigit=false;
	
	/* Read an integral number part: */
	value=0.0;
	while(nextChar>='0'&&nextChar<='9')
		{
		haveDigit=true;
		value=value*10.0+double(nextChar-'0');
		nextChar=source->getChar();
		}
	
	/* Check for a period: */
	if(nextChar=='.')
		{
		nextChar=source->getChar();
		
		/* Read a fractional number part: */
		double fraction=0.0;
		double fractionBase=1.0;
		while(nextChar>='0'&&nextChar<='9')
			{
			haveDigit=true;
			fraction=fraction*10.0+double(nextChar-'0');
			fractionBase*=10.0;
			nextChar=source->getChar();
			}
		
		value+=fraction/fractionBase;
		}
	
	/* Signal a conversion error if no digits were read in the integral or fractional part: */
	if(!haveDigit)
		return false;
	
	/* Negate the result if a minus sign was read: */
	if(negated)
		value=-value;
	
	/* Check for an exponent indicator: */
	if(nextChar=='e'||nextChar=='E')
		{
		nextChar=source->getChar();
		
		/* Read a plus or minus sign: */
		bool exponentNegated=false;
		if(nextChar=='-')
			{
			exponentNegated=true;
			nextChar=source->getChar();
			}
		else if(nextChar=='+')
			nextChar=source->getChar();
		
		/* Signal a conversion error if the next character is not a digit: */
		if(nextChar<'0'||nextChar>'9')
			return false;
		
		/* Read the first exponent digit: */
		double exponent=double(nextChar-'0');
		nextChar=source->getChar();
		
		/* Read the rest of the exponent digits: */
		while(nextChar>='0'&&nextChar<='9')
			{
			exponent=exponent*10.0+double(nextChar-'0');
			nextChar=source->getChar();
			}
		
		/* Multiply the mantissa with the exponent: */
		if(exponentNegated)
			exponent=-exponent;
		value*=pow(10.0,exponent);
		}
	
	return true;
	}

template <>
bool CSVSource::convertNumber(int& nextChar,float& value)
	{
	/* Use the double conversion method internally: */
	double tempValue;
	if(!convertNumber<double>(nextChar,tempValue))
		return false;
	value=float(tempValue);
	return true;
	}

#endif

void CSVSource::setFieldCharacter(int character)
	{
	/* Check if the character is valid in an unquoted field: */
	if((cc[character]&(RECORDSEP|FIELDSEP|QUOTE))==0x0)
		cc[character]|=FIELD;
	
	/* Everything is valid in a quoted field until an actual quote is encountered: */
	cc[character]|=QUOTEDFIELD;
	}

void CSVSource::updateCharacterClasses(void)
	{
	/* Reset all characters: */
	for(int i=-1;i<256;++i)
		cc[i]&=~(FIELD|QUOTEDFIELD|WHITESPACE|DIGIT);
	
	/* These are not in RFC 4180, but there is really no reason to forbid them: */
	setFieldCharacter('\t');
	setFieldCharacter('\n');
	setFieldCharacter('\v');
	setFieldCharacter('\f');
	setFieldCharacter('\r');
	
	/* Core RFC 4180 characters: */
	for(int i=32;i<128;++i)
		setFieldCharacter(i);
	
	/* These aren't in RFC 4180, either, but again no reason to forbid them and thus break UTF-8: */
	for(int i=128;i<256;++i)
		setFieldCharacter(i);
	
	/* Mark whitespace: */
	cc['\t']|=WHITESPACE;
	cc['\n']|=WHITESPACE;
	cc['\v']|=WHITESPACE;
	cc['\f']|=WHITESPACE;
	cc['\r']|=WHITESPACE;
	cc[' ']|=WHITESPACE;
	
	/* Mark digits: */
	for(int i='0';i<='9';++i)
		cc[i]|=DIGIT;
	}

CSVSource::CSVSource(FilePtr sSource)
	:source(sSource),
	 cc(characterClasses+1),
	 recordIndex(0),fieldIndex(0)
	{
	/* Set up character classes for RFC 4180-style CSV: */
	cc[-1]=RECORDSEP; // End-of-file implicitly terminates the current record
	for(int i=0;i<256;++i)
		cc[i]=NONE;
	cc['\r']|=RECORDSEP;
	cc['\n']|=RECORDSEP;
	cc[',']|=FIELDSEP;
	cc['"']|=QUOTE;
	updateCharacterClasses();
	
	/* Read the first character from the character source: */
	lastChar=source->getChar();
	}

CSVSource::~CSVSource(void)
	{
	/* Put the last read character back into the character source: */
	if(lastChar>=0)
		source->ungetChar(lastChar);
	}

void CSVSource::setRecordSeparator(int newRecordSeparator)
	{
	/* Throw an error if the given character is invalid: */
	if(newRecordSeparator<0||newRecordSeparator>=256)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid record separator character");
	
	/* Clear the record separator flag from all characters: */
	for(int i=-1;i<256;++i)
		cc[i]&=~RECORDSEP;
	
	/* Set the record separator flag for the given character: */
	cc[newRecordSeparator]|=RECORDSEP;
	
	/* Update the rest of the character classes: */
	updateCharacterClasses();
	}

void CSVSource::setRecordSeparatorCRLF(void)
	{
	/* Clear the record separator flag from all characters: */
	for(int i=-1;i<256;++i)
		cc[i]&=~RECORDSEP;
	
	/* Set the record separator flag for the CR/LF sequence: */
	cc['\r']|=RECORDSEP;
	cc['\n']|=RECORDSEP;
	
	/* Update the rest of the character classes: */
	updateCharacterClasses();
	}

void CSVSource::setFieldSeparator(int newFieldSeparator)
	{
	/* Throw an error if the given character is invalid: */
	if(newFieldSeparator<0||newFieldSeparator>=256)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid field separator character");
	
	/* Clear the field separator flag from all characters: */
	for(int i=-1;i<256;++i)
		cc[i]&=~FIELDSEP;
	
	/* Set the field separator flag for the given character: */
	cc[newFieldSeparator]|=FIELDSEP;
	
	/* Update the rest of the character classes: */
	updateCharacterClasses();
	}

void CSVSource::setQuote(int newQuote)
	{
	/* Throw an error if the given character is invalid: */
	if(newQuote<0||newQuote>=256)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid quote character");
	
	/* Clear the quote flag from all characters: */
	for(int i=-1;i<256;++i)
		cc[i]&=~QUOTE;
	
	/* Set the quote flag for the given character: */
	cc[newQuote]|=QUOTE;
	
	/* Update the rest of the character classes: */
	updateCharacterClasses();
	}

bool CSVSource::skipField(void)
	{
	/* Create a field reader object for the current field: */
	FieldReader reader(__PRETTY_FUNCTION__,*this);
	
	/* Skip the field's contents: */
	bool hadContent=false;
	while(reader.getChar()>=0)
		hadContent=true;
	
	/* Finish reading the field: */
	reader.finishField();
	
	return hadContent;
	}

template <class ValueParam>
ValueParam CSVSource::readField(void)
	{
	/* Create a field reader object for the current field: */
	FieldReader reader(__PRETTY_FUNCTION__,*this);
	
	/* Skip leading whitespace: */
	reader.skipWhitespace();
	
	/* Parse a numeric value: */
	ValueParam result(0);
	bool valid=reader.readValue(result);
	
	/* Skip trailing whitespace: */
	reader.skipWhitespace();
	
	/* Invalidate the result if there is anything else left in the field, then skip that stuff: */
	while(reader.getChar()>=0)
		valid=false;
	
	/* Finish reading the field: */
	reader.finishField();
	
	/* Check for conversion errors: */
	if(!valid)
		throw ConversionError(__PRETTY_FUNCTION__,fieldIndex,recordIndex,TypeName<ValueParam>::getName());
	
	/* Return the result: */
	return result;
	}

template <>
std::string CSVSource::readField(void)
	{
	std::string result;
	
	/* Check if the field is quoted: */
	if((cc[lastChar]&QUOTE)!=0x0U)
		{
		/* Temporarily remove the quote character from the set of valid quoted field characters: */
		int quote=lastChar;
		cc[quote]&=~QUOTEDFIELD;
		
		try
			{
			/* Skip the quote character: */
			lastChar=source->getChar();
			
			/* Read characters into the result string until the end of the field: */
			while(true)
				{
				/* Check for the end of the field: */
				if((cc[lastChar]&QUOTEDFIELD)==0x0U)
					{
					/* Check for a potential closing quote: */
					if(lastChar==quote)
						{
						/* Skip the quote character and check for a double (quoted) quote: */
						lastChar=source->getChar();
						if(lastChar!=quote)
							{
							/* Stop reading: */
							break;
							}
						}
					else
						{
						/* Signal a format error: */
						throw FormatError(__PRETTY_FUNCTION__,fieldIndex,recordIndex);
						}
					}
				
				/* Append the character to the result string and read the next one: */
				result.push_back(lastChar);
				lastChar=source->getChar();
				}
			}
		catch(...)
			{
			/* Re-add the quote character to the set of valid quoted field characters and re-throw the exception: */
			cc[quote]|=QUOTEDFIELD;
			throw;
			}
		
		/* Re-add the quote character to the set of valid quoted field characters: */
		cc[quote]|=QUOTEDFIELD;
		}
	else
		{
		/* Read characters into the result string until the end of the field: */
		while((cc[lastChar]&FIELD)!=0x0U)
			{
			/* Append the character to the result string and read the next one: */
			result.push_back(lastChar);
			lastChar=source->getChar();
			}
		}
	
	/* Check whether the next character is a record or field separator: */
	if((cc[lastChar]&RECORDSEP)!=0x0U)
		{
		/* Remember and skip the separator character: */
		int separator=lastChar;
		lastChar=source->getChar();
		
		/* If this is a CR/LF pair, skip the LF character as well: */
		if(separator=='\r'&&(cc[lastChar]&RECORDSEP)!=0x0U&&lastChar=='\n')
			lastChar=source->getChar();
		
		/* Increase the record index and reset the field index: */
		++recordIndex;
		fieldIndex=0;
		}
	else if((cc[lastChar]&FIELDSEP)!=0x0U)
		{
		/* Skip the field separator: */
		lastChar=source->getChar();
		
		/* Increase the field index: */
		++fieldIndex;
		}
	else
		{
		/* Signal a format error: */
		throw FormatError(__PRETTY_FUNCTION__,fieldIndex,recordIndex);
		}
	
	return result;
	}

/************************************************************************
Force instantiation of standard versions of CSVSource::readField methods:
************************************************************************/

template unsigned int CSVSource::readField<unsigned int>(void);
template int CSVSource::readField<int>(void);
template unsigned long CSVSource::readField<unsigned long>(void);
template long CSVSource::readField<long>(void);
template float CSVSource::readField<float>(void);
template double CSVSource::readField<double>(void);
template std::string CSVSource::readField<std::string>(void);

}

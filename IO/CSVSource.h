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

#ifndef IO_CSVSOURCE_INCLUDED
#define IO_CSVSOURCE_INCLUDED

#include <stdexcept>
#include <IO/File.h>

namespace IO {

class CSVSource
	{
	/* Embedded classes: */
	public:
	class FormatError:public std::runtime_error // Class to signal format errors in the CSV source's structure; continuing to read after this error occurs has undefined results
		{
		/* Constructors and destructors: */
		public:
		FormatError(const char* source,unsigned int fieldIndex,size_t recordIndex);
		};
	
	class ConversionError:public std::runtime_error // Class to signal conversion errors while reading fields
		{
		/* Constructors and destructors: */
		public:
		ConversionError(const char* source,unsigned int fieldIndex,size_t recordIndex,const char* dataTypeName);
		};
	
	private:
	enum CharacterClasses // Enumerated type for character class bit masks to speed up tokenization
		{
		NONE=0x0,
		RECORDSEP=0x1, // Class for characters that separate records
		FIELDSEP=0x2, // Class for characters that separate fields
		QUOTE=0x4, // Class for characters that start / end quoted fields
		FIELD=0x8, // Class for characters allowed in unquoted fields
		QUOTEDFIELD=0x10, // Class for characters allowed in unquoted fields
		WHITESPACE=0x20, // Class for characters ignored in numeric fields
		DIGIT=0x40 // Class for digits
		};
	
	class FieldReader; // Helper class to read quoted or unquoted fields
	friend class FieldReader;
	
	/* Elements: */
	private:
	FilePtr source; // Data source for CSV source
	unsigned char characterClasses[257]; // Array of character type bit flags for quicker classification, with extra space for EOF
	unsigned char* cc; // Pointer into character classes array to account for EOF==-1
	size_t recordIndex; // Zero-based index of the currently read record; increments before field read on the last field in a record returns
	unsigned int fieldIndex; // Zero-based index of the currently read field; increments before field read returns; resets to zero before field read on the last field in a record returns
	int lastChar; // Last character read from character source
	char* fieldBuffer; // An internal buffer to efficiently read fields as NUL-terminated strings
	char* fbEnd; // Pointer to the end of the current field buffer
	size_t fieldLength; // Length of the current string in the field buffer
	
	/* Private methods: */
	void setFieldCharacter(int character); // Marks the given character as potentially valid in a field
	void updateCharacterClasses(void); // Updates character classes after a change to the parser's parameters
	
	/* Constructors and destructors: */
	public:
	CSVSource(FilePtr sSource); // Creates a default CSV source for the given character source
	~CSVSource(void); // Destroys the CSV source
	
	/* Methods: */
	
	/* Parser configuration methods: */
	void setRecordSeparator(int newRecordSeparator); // Sets the given character as the only record separator
	void setRecordSeparatorCRLF(void); // Sets the CR/LF sequence as the only record separator
	void setFieldSeparator(int newFieldSeparator); // Sets the given character as the only field separator
	void setQuote(int newQuote); // Sets the given character as the only quote character
	
	/* Parser status query methods: */
	size_t getRecordIndex(void) const // Returns the current record index
		{
		return recordIndex;
		}
	unsigned int getFieldIndex(void) const // Returns the current field index
		{
		return fieldIndex;
		}
	bool eof(void) const // Returns true when the entire character source was read
		{
		/* We are at end-of-file if the last read character was EOF: */
		return lastChar<0;
		}
	bool eor(void) const // Returns true when the last read field terminated a record; returns true before the first field is read
		{
		return fieldIndex==0;
		}
	
	/* Field reading methods: */
	bool skipField(void); // Skips the current field; returns true if the field was non-empty after unquoting; throws exception if the end of the field cannot be determined reliably
	void skipRecord(void) // Skips the rest of the current record
		{
		/* Simply skip fields until the field index resets to zero: */
		do
			{
			skipField();
			}
		while(fieldIndex!=0);
		}
	const char* readFieldIntoBuffer(void); // Reads the next field as a string into an internal buffer as a NUL-terminated string; returns pointer to start of that buffer
	const char* getFieldString(void) const // Returns the string currently in the field buffer
		{
		return fieldBuffer;
		}
	size_t getFieldLength(void) const // Returns the length of the string currently in the internal buffer
		{
		return fieldLength;
		}
	template <class ValueParam>
	ValueParam readField(void); // Reads the next field as the given data type; throws exception if the field contents cannot be fully converted, or the end of the field cannot be determined reliably
	};

/**********************************************
Specializations of CSVSource::readField method:
**********************************************/

}

#endif

/***********************************************************************
LZWDecompressor - Filter class to decompress a source stream using the
Lempel-Ziv-Welch compression algorithm used in GIF files.
Copyright (c) 1995-2024 Oliver Kreylos

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

#include <IO/LZWDecompressor.h>

#include <IO/File.h>

namespace IO {

/********************************
Methods of class LZWDecompressor:
********************************/

LZWDecompressor::LZWDecompressor(unsigned int sNumCharBits,unsigned int sTableSize)
	:numCharBits(sNumCharBits),cc(1U<<numCharBits),eoi(cc+1U),
	 tableSize(sTableSize),table(new TableEntry[tableSize+1]),
	 firstFree(eoi+1),numCodeBits(numCharBits+1),maxCode(Code(1)<<numCodeBits),
	 stringBuffer(new Char[tableSize+1]),stringEnd(stringBuffer+(tableSize+1)),
	 firstCode(true),lastCode(0),lastPrefix(0)
	{
	}

LZWDecompressor::~LZWDecompressor(void)
	{
	/* Release allocated resources: */
	delete[] table;
	delete[] stringBuffer;
	}

void LZWDecompressor::reset(void)
	{
	/* Clear the decompression table: */
	firstFree=eoi+1;
	
	/* Reset the code size: */
	numCodeBits=numCharBits+1;
	maxCode=Code(1)<<numCodeBits;
	
	/* The next code is the first code after a table reset: */
	firstCode=true;
	}

const LZWDecompressor::Char* LZWDecompressor::decompress(LZWDecompressor::Code code)
	{
	Char* result=stringEnd;
	
	/* Check if the code encodes a string or is a special marker: */
	if(code!=cc&&code!=eoi)
		{
		/* Prepare to put a new code into the decompression table: */
		TableEntry& newEntry=table[firstFree];
		newEntry.prefix=lastCode;
		lastCode=code;
		
		/* Put the read code into the decompression table if it isn't in yet: */
		if(lastCode>=firstFree)
			newEntry.suffix=lastPrefix;
		
		/* Generate the string represented by the code: */
		while(code>eoi)
			{
			*(--result)=table[code].suffix;
			code=table[code].prefix;
			}
		*(--result)=Char(code);
		lastPrefix=*result;
		
		/* Put a new code into the decompression table if the read code was already in it: */
		if(lastCode<firstFree)
			newEntry.suffix=lastPrefix;
		
		/* Finish adding the new code to the decompression table if it wasn't the first code and there is room: */
		if(!firstCode&&firstFree<tableSize)
			{
			++firstFree;
			if(firstFree==maxCode&&maxCode<tableSize)
				{
				++numCodeBits;
				maxCode<<=1;
				}
			}
		firstCode=false;
		}
	else if(code==cc)
		{
		/* Reset the decompression table: */
		reset();
		}
	else
		{
		/* Code is end of image marker; bail out: */
		result=0;
		}
	
	return result;
	}

void LZWDecompressor::decompress(IO::File& source,IO::File& dest)
	{
	/* Read codes from the source file until the end of image marker: */
	Code bitBuffer(0x0U);
	unsigned int numBufferBits(0);
	while(true)
		{
		/* Add more bits to the bit buffer until there are enough to read a code: */
		while(numBufferBits<numCodeBits)
			{
			/* Add 8 more bits on the left of the existing bits: */
			bitBuffer|=Code(source.getChar())<<numBufferBits;
			numBufferBits+=8;
			}
		
		/* Extract the next code from the right of the bit buffer: */
		Code code=bitBuffer&((Code(1)<<numCodeBits)-1);
		bitBuffer>>=numCodeBits;
		numBufferBits-=numCodeBits;
		
		/* Check if the code encodes a string or is a special marker: */
		if(code!=cc&&code!=eoi)
			{
			/* Prepare to put a new code into the decompression table: */
			TableEntry& newEntry=table[firstFree];
			newEntry.prefix=lastCode;
			lastCode=code;
			
			/* Put the read code into the decompression table if it isn't in yet: */
			if(code>=firstFree)
				newEntry.suffix=lastPrefix;
			
			/* Generate the string represented by the code: */
			Char* stringPtr=stringEnd;
			while(code>eoi)
				{
				*(--stringPtr)=table[code].suffix;
				code=table[code].prefix;
				}
			*(--stringPtr)=Char(code);
			lastPrefix=*stringPtr;
			
			/* Write the generated string to the destination: */
			dest.writeRaw(stringPtr,stringEnd-stringPtr);
			
			/* Put a new code into the decompression table if the read code was already in it: */
			if(code<firstFree)
				newEntry.suffix=lastPrefix;
			
			/* Finish adding the new code to the decompression table if it wasn't the first code and there is room: */
			if(!firstCode&&firstFree<tableSize)
				{
				++firstFree;
				if(firstFree==maxCode)
					{
					++numCodeBits;
					maxCode<<=1;
					}
				}
			firstCode=false;
			}
		else if(code==cc)
			{
			/* Reset the decompression table: */
			reset();
			}
		else
			{
			/* Code is end of image marker; bail out: */
			break;
			}
		}
	}

}

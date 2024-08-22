/***********************************************************************
LZWDecompressor - Filter class to decompress a source stream using the
Lempel-Ziv-Welch compression algorithm used in GIF files.
Copyright (c) 1995-2024 Oliver Kreylos
*
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
**********************************************************************/

#ifndef IO_LZWDECOMPRESSOR_INCLUDED
#define IO_LZWDECOMPRESSOR_INCLUDED

#include <Misc/SizedTypes.h>

/* Forward declarations: */
namespace IO {
class File;
}

namespace IO {

class LZWDecompressor
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt8 Char; // Type for uncompressed data values
	typedef Misc::UInt32 Code; // Type for compressed data values
	
	struct TableEntry // Structure for decompression table entries
		{
		/* Elements: */
		public:
		Code prefix; // Table entry prefix, as index into the code table
		Char suffix; // Table entry suffix
		};
	
	/* Elements: */
	private:
	unsigned int numCharBits; // Number of bits in uncomressed data values
	Code cc,eoi; // The special "clear table" and "end of image" codes
	unsigned int tableSize; // Size of the LZW decompression table
	TableEntry* table; // The LZW decompression table
	Code firstFree; // Index of the first free decompression table entry
	unsigned int numCodeBits; // Number of bits in the next code to be read
	Code maxCode; // Code value at which the number of bits in the code must be increased
	Char* stringBuffer; // Buffer to hold the decompressed string for the current code
	Char* stringEnd; // Pointer to the end of the string buffer
	bool firstCode; // Flag if the next code is the first one after a decompression table reset
	Code lastCode; // The last code read from the compressed source
	Char lastPrefix; // The prefix of the last decompressed string
	
	/* Constructors and destructors: */
	public:
	LZWDecompressor(unsigned int sNumCharBits,unsigned int sTableSize); // Creates an LZW decompressor for the given uncompressed data size and table size
	~LZWDecompressor(void); // Destroys the decompressor
	
	/* Methods: */
	void reset(void); // Resets the decompressor
	const Char* getStringEnd(void) const // Returns the end of the most recently decompressed string
		{
		return stringEnd;
		}
	unsigned int getNumCodeBits(void) const // Returns the number of bits in the next code to be read from the compressed data stream
		{
		return numCodeBits;
		}
	const Char* decompress(Code code); // Decompresses a single LZW code and returns a pointer to the beginning of the potentially empty uncompressed string; returns a null pointer when the data stream is finished
	void decompress(IO::File& source,IO::File& dest); // Decompresses codes read from the source file to the destination file
	};

}

#endif

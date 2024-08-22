/***********************************************************************
CRC32 - Class implementing the standard CRC-32 cyclic redundancy check
used in many applications, including PNG image files.
Copyright (c) 2022 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_CRC32_INCLUDED
#define MISC_CRC32_INCLUDED

#include <stddef.h>
#include <Misc/SizedTypes.h>

namespace Misc {

class CRC32
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt8 Byte; // Type for bytes over which the CRC is run
	typedef Misc::UInt32 Code; // Type for returned CRC codes
	
	/* Elements: */
	public:
	static const Code initialCode=0xffffffffU; // Code value to start CRC calculation
	private:
	static const CRC32 theCrc32; // Static CRC-32 calculation object
	Code table[256]; // Table of pre-computed coefficients to process input blocks one byte at a time
	
	/* Private methods: */
	Code operator()(const void* buffer,size_t bufferSize,Code crc) const; // Updates the given CRC code by running CRC-32 on the given memory block
	
	/* Constructors and destructors: */
	CRC32(void); // Creates a CRC-32 calculation object by initializing the code table
	
	/* Methods: */
	public:
	static Code calc(const void* buffer,size_t bufferSize,Code crc =initialCode) // Updates the given CRC code by running CRC-32 on the given memory block
		{
		/* Delegate to the static object's private method: */
		return theCrc32(buffer,bufferSize,crc);
		}
	};

}

#endif

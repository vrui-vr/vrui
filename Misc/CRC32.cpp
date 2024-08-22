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

#include <Misc/CRC32.h>

namespace Misc {

/******************************
Static elements of class CRC32:
******************************/

const CRC32 CRC32::theCrc32;

/**********************
Methods of class CRC32:
**********************/

CRC32::CRC32(void)
	{
	/* Initialize the CRC calculation table: */
	for(Code tableIndex=0;tableIndex<256;++tableIndex)
		{
		Code code=tableIndex;
		for(int bit=0;bit<8;++bit)
			{
			if((code&0x1U)!=0x0U)
				code=(code>>1)^0xedb88320U;
			else
				code=(code>>1);
			}
		table[tableIndex]=code;
		}
	}

CRC32::Code CRC32::operator()(const void* buffer,size_t bufferSize,CRC32::Code crc) const
	{
	/* Process the buffer byte-by-byte: */
	const Byte* bufferPtr=static_cast<const Byte*>(buffer);
	const Byte* bufferEnd=bufferPtr+bufferSize;
	for(;bufferPtr!=bufferEnd;++bufferPtr)
		crc=(crc>>8)^table[(crc^Code(*bufferPtr))&0xffU];
	
	return crc;
	}

}

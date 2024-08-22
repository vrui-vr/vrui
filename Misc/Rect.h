/***********************************************************************
Rect - Class for n-dimensional half-open rectangles represented as
offsets and sizes.
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

#ifndef MISC_RECT_INCLUDED
#define MISC_RECT_INCLUDED

#include <stddef.h>
#include <Misc/Utility.h>
#include <Misc/Offset.h>
#include <Misc/Size.h>

namespace Misc {

template <unsigned int numComponentsParam>
class Rect
	{
	/* Embedded classes: */
	public:
	static const unsigned int numComponents=numComponentsParam;
	typedef Misc::Offset<numComponentsParam> Offset; // Type for rectangle offsets
	typedef Misc::Size<numComponentsParam> Size; // Type for rectangle sizes
	
	/* Elements: */
	Offset offset; // Rectangle offset relative to parent coordinate system
	Size size; // Rectangle size
	
	/* Constructors and destructors: */
	Rect(void) // Dummy constructor
		{
		}
	Rect(const Offset& sOffset,const Size& sSize) // Element-wise constructor
		:offset(sOffset),size(sSize)
		{
		}
	Rect(const Size& sSize) // Creates rectangle with zero offset
		:offset(0),size(sSize)
		{
		}
	
	/* Methods: */
	bool operator==(const Rect& other) // Equality operator
		{
		return offset==other.offset&&size==other.size;
		}
	bool operator!=(const Rect& other) // Inequality operator
		{
		return offset!=other.offset||size!=other.size;
		}
	unsigned long volume(void) const // Returns the rectangle's n-dimensional volume
		{
		return size.volume();
		}
	bool contains(const Offset& position) const // Returns true if the rectangle contains the given position
		{
		bool result=true;
		for(unsigned int i=0;i<numComponents&&result;++i)
			result=position[i]>=offset[i]&&(unsigned int)(position[i]-offset[i])<size[i];
		return result;
		}
	bool contains(const Rect& other) const // Returns true if the rectangle contains the given other rectangle
		{
		bool result=true;
		for(unsigned int i=0;i<numComponents&&result;++i)
			result=offset[i]<=other.offset[i]&&offset[i]+size[i]>=other.offset[i]+other.size[i];
		return result;
		}
	Rect& intersect(const Rect& other) // Sets this rectangle to its intersection with the given other rectangle
		{
		for(unsigned int i=0;i<numComponents;++i)
			{
			int upper=offset[i]+int(size[i]);
			offset[i]=Misc::max(offset[i],other.offset[i]);
			size[i]=Misc::min(upper,other.offset[i]+int(other.size[i]))-offset[i];
			}
		return *this;
		}
	Rect& unite(const Rect& other) // Sets this rectangle to its union with the given other rectangle
		{
		for(unsigned int i=0;i<numComponents;++i)
			{
			int upper=offset[i]+int(size[i]);
			offset[i]=Misc::min(offset[i],other.offset[i]);
			size[i]=Misc::max(upper,other.offset[i]+int(other.size[i]))-offset[i];
			}
		return *this;
		}
	
	static size_t rawHash(const Rect& source) // Raw hash function
		{
		return Offset::rawHash(source.offset)+Size::rawHash(source.size)*size_t(23);
		}
	static size_t hash(const Rect& source,size_t tableSize) // Hash function compatible with use in HashTable class
		{
		return rawHash(source)%tableSize;
		}
	};

}

#endif

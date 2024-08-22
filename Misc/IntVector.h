/***********************************************************************
IntVector - Class for n-dimensional vectors of generic integer types,
to be used as base class for multi-dimensional array indices, offsets,
sizes, etc.
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

#ifndef MISC_INTVECTOR_INCLUDED
#define MISC_INTVECTOR_INCLUDED

#include <stddef.h>

namespace Misc {

template <class ComponentParam,unsigned int numComponentsParam>
class IntVector
	{
	/* Embedded classes: */
	public:
	typedef ComponentParam Component; // Vector component type
	static const unsigned int numComponents=numComponentsParam; // Number of vector components
	
	/* Elements: */
	protected:
	Component components[numComponentsParam]; // Array of vector components
	
	/* Private methods: */
	private:
	static inline long comp(const IntVector& iv1,const IntVector& iv2) // Compares two vectors lexicographically in order of index; returns result <0, ==0, or >0
		{
		long result=long(iv1.components[0])-long(iv2.components[0]);
		for(unsigned int i=1;i<numComponentsParam&&result==0;++i)
			result=long(iv1.components[i])-long(iv2.components[i]);
		return result;
		}
	
	/* Protected methods: */
	protected:
	inline void fillComponents(ComponentParam c) // Fills components with single value
		{
		for(unsigned int i=0;i<numComponentsParam;++i)
			components[i]=c;
		}
	inline void copyComponents(const ComponentParam* sComponents) // Copies components from the given array
		{
		for(unsigned int i=0;i<numComponentsParam;++i)
			components[i]=sComponents[i];
		}
	
	/* Constructors and destructors: */
	protected:
	inline IntVector(void) // Dummy constructor
		{
		}
	inline IntVector(ComponentParam c0) // Component-wise constructor; must only be called for one-component vectors
		{
		components[0]=c0;
		}
	inline IntVector(ComponentParam c0,ComponentParam c1) // Component-wise constructor; must only be called for two-component vectors
		{
		components[0]=c0;
		components[1]=c1;
		}
	inline IntVector(ComponentParam c0,ComponentParam c1,ComponentParam c2) // Component-wise constructor; must only be called for three-component vectors
		{
		components[0]=c0;
		components[1]=c1;
		components[2]=c2;
		}
	inline IntVector(ComponentParam c0,ComponentParam c1,ComponentParam c2,ComponentParam c3) // Component-wise constructor; must only be called for four-component vectors
		{
		components[0]=c0;
		components[1]=c1;
		components[2]=c2;
		components[3]=c3;
		}
	
	/* Methods: */
	public:
	inline const ComponentParam* getComponents(void) const // Returns vector components as array
		{
		return components;
		}
	inline ComponentParam* getComponents(void) // Ditto
		{
		return components;
		}
	inline ComponentParam operator[](unsigned int index) const // Returns a single vector component
		{
		return components[index];
		}
	inline ComponentParam& operator[](unsigned int index) // Ditto
		{
		return components[index];
		}
	
	/* Comparison operators using per-component comparison: */
	inline friend bool operator==(const IntVector& iv1,const IntVector& iv2)
		{
		return comp(iv1,iv2)==0;
		}
	inline friend bool operator!=(const IntVector& iv1,const IntVector& iv2)
		{
		return comp(iv1,iv2)!=0;
		}
	
	/* Comparison operators using lexicographical comparison in order of component index: */
	inline friend bool operator<(const IntVector& iv1,const IntVector& iv2)
		{
		return comp(iv1,iv2)<0;
		}
	inline friend bool operator<=(const IntVector& iv1,const IntVector& iv2)
		{
		return comp(iv1,iv2)<=0;
		}
	inline friend bool operator>=(const IntVector& iv1,const IntVector& iv2)
		{
		return comp(iv1,iv2)>=0;
		}
	inline friend bool operator>(const IntVector& iv1,const IntVector& iv2)
		{
		return comp(iv1,iv2)>0;
		}
	
	inline void min(const IntVector& other) // Component-wise minimum
		{
		for(unsigned int i=0;i<numComponentsParam;++i)
			if(components[i]>other.components[i])
				components[i]=other.components[i];
		}
	inline void max(const IntVector& other) // Component-wise maximum
		{
		for(unsigned int i=0;i<numComponentsParam;++i)
			if(components[i]<other.components[i])
				components[i]=other.components[i];
		}
	
	static size_t rawHash(const IntVector& source) // Raw hash function
		{
		size_t result(source.components[0]);
		for(unsigned int i=1;i<numComponentsParam;++i)
			result=result*size_t(257)+size_t(source.components[i]);
		return result;
		}
	static size_t hash(const IntVector& source,size_t tableSize) // Hash function compatible with use in HashTable class
		{
		return rawHash(source)%tableSize;
		}
	};

}

#endif

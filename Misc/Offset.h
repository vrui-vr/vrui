/***********************************************************************
Offset - Class for n-dimensional offsets.
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

#ifndef MISC_OFFSET_INCLUDED
#define MISC_OFFSET_INCLUDED

#include <Misc/IntVector.h>

namespace Misc {

/*************************************
Dummy generic version of Offset class:
*************************************/

template <unsigned int numComponentsParam>
class Offset
	{
	};

/************************************
Specialized versions of Offset class:
************************************/

template <>
class Offset<1>:public IntVector<int,1>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<int,1> BaseClass;
	
	/* Declarations of inherited types/elements: */
	using IntVector<int,1>::components;
	
	/* Constructors and destructors: */
	public:
	Offset(void)
		{
		}
	explicit Offset(int sC0)
		:BaseClass(sC0)
		{
		}
	Offset(const Offset& source)
		{
		copyComponents(source.components);
		}
	Offset& operator=(int sC0)
		{
		components[0]=sC0;
		return *this;
		}
	Offset& operator=(const Offset& source)
		{
		copyComponents(source.components);
		return *this;
		}
	};

template <>
class Offset<2>:public IntVector<int,2>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<int,2> BaseClass;
	
	/* Declarations of inherited types/elements: */
	using IntVector<int,2>::components;
	
	/* Constructors and destructors: */
	public:
	Offset(void)
		{
		}
	explicit Offset(int sC)
		{
		fillComponents(sC);
		}
	Offset(int sC0,int sC1)
		:BaseClass(sC0,sC1)
		{
		}
	Offset(const Offset& source)
		{
		copyComponents(source.components);
		}
	Offset& operator=(const Offset& source)
		{
		copyComponents(source.components);
		return *this;
		}
	};

template <>
class Offset<3>:public IntVector<int,3>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<int,3> BaseClass;
	
	/* Declarations of inherited types/elements: */
	using IntVector<int,3>::components;
	
	/* Constructors and destructors: */
	public:
	Offset(void)
		{
		}
	explicit Offset(int sC)
		{
		fillComponents(sC);
		}
	Offset(int sC0,int sC1,int sC2)
		:BaseClass(sC0,sC1,sC2)
		{
		}
	Offset(const Offset& source)
		{
		copyComponents(source.components);
		}
	Offset& operator=(const Offset& source)
		{
		copyComponents(source.components);
		return *this;
		}
	};

template <>
class Offset<4>:public IntVector<int,4>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<int,4> BaseClass;
	
	/* Declarations of inherited types/elements: */
	using IntVector<int,4>::components;
	
	/* Constructors and destructors: */
	public:
	Offset(void)
		{
		}
	explicit Offset(int sC)
		{
		fillComponents(sC);
		}
	Offset(int sC0,int sC1,int sC2,int sC3)
		:BaseClass(sC0,sC1,sC2,sC3)
		{
		}
	Offset(const Offset& source)
		{
		copyComponents(source.components);
		}
	Offset& operator=(const Offset& source)
		{
		copyComponents(source.components);
		return *this;
		}
	};

/***************************
Functions on Offset objects:
***************************/

template <unsigned int numComponentsParam>
inline
Offset<numComponentsParam>&
operator+=(
	Offset<numComponentsParam>& o1,
	const Offset<numComponentsParam>& o2)
	{
	for(unsigned int i=0;i<numComponentsParam;++i)
		o1[i]+=o2[i];
	return o1;
	}

template <unsigned int numComponentsParam>
inline
Offset<numComponentsParam>
operator+(
	const Offset<numComponentsParam>& o1,
	const Offset<numComponentsParam>& o2)
	{
	Offset<numComponentsParam> result=o1;
	for(unsigned int i=0;i<numComponentsParam;++i)
		result[i]+=o2[i];
	return result;
	}

template <unsigned int numComponentsParam>
inline
Offset<numComponentsParam>&
operator-=(
	Offset<numComponentsParam>& o1,
	const Offset<numComponentsParam>& o2)
	{
	for(unsigned int i=0;i<numComponentsParam;++i)
		o1[i]-=o2[i];
	return o1;
	}

template <unsigned int numComponentsParam>
inline
Offset<numComponentsParam>
operator-(
	const Offset<numComponentsParam>& o1,
	const Offset<numComponentsParam>& o2)
	{
	Offset<numComponentsParam> result=o1;
	for(unsigned int i=0;i<numComponentsParam;++i)
		result[i]-=o2[i];
	return result;
	}

}

#endif

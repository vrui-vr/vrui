/***********************************************************************
Size - Class for n-dimensional sizes.
Copyright (c) 2022-2023 Oliver Kreylos

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

#ifndef MISC_SIZE_INCLUDED
#define MISC_SIZE_INCLUDED

#include <Misc/IntVector.h>
#include <Misc/Offset.h>

namespace Misc {

/***********************************
Dummy generic version of Size class:
***********************************/

template <unsigned int numComponentsParam>
class Size
	{
	};

/**********************************
Specialized versions of Size class:
**********************************/

template <>
class Size<1>:public IntVector<unsigned int,1>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<unsigned int,1> BaseClass;
	public:
	typedef Misc::Offset<1> Offset; // Compatible offset type
	
	/* Declarations of inherited types/elements: */
	private:
	using IntVector<unsigned int,1>::components;
	
	/* Constructors and destructors: */
	public:
	Size(void)
		{
		}
	explicit Size(unsigned int sC0)
		:BaseClass(sC0)
		{
		}
	Size(const Size& source)
		{
		copyComponents(source.components);
		}
	Size& operator=(unsigned int sC0)
		{
		components[0]=sC0;
		return *this;
		}
	Size& operator=(const Size& source)
		{
		copyComponents(source.components);
		return *this;
		}
	
	/* Methods: */
	unsigned long volume(void) const // Returns the size's n-dimensional volume
		{
		return (unsigned long)(components[0]);
		}
	};

template <>
class Size<2>:public IntVector<unsigned int,2>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<unsigned int,2> BaseClass;
	public:
	typedef Misc::Offset<2> Offset; // Compatible offset type
	
	/* Declarations of inherited types/elements: */
	private:
	using IntVector<unsigned int,2>::components;
	
	/* Constructors and destructors: */
	public:
	Size(void)
		{
		}
	explicit Size(unsigned int sC)
		{
		fillComponents(sC);
		}
	Size(unsigned int sC0,unsigned int sC1)
		:BaseClass(sC0,sC1)
		{
		}
	Size(const Size& source)
		{
		copyComponents(source.components);
		}
	Size& operator=(const Size& source)
		{
		copyComponents(source.components);
		return *this;
		}
	
	/* Methods: */
	unsigned long volume(void) const // Returns the size's n-dimensional volume
		{
		return (unsigned long)(components[1])*(unsigned long)(components[0]);
		}
	};

template <>
class Size<3>:public IntVector<unsigned int,3>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<unsigned int,3> BaseClass;
	public:
	typedef Misc::Offset<3> Offset; // Compatible offset type
	
	/* Declarations of inherited types/elements: */
	private:
	using IntVector<unsigned int,3>::components;
	
	/* Constructors and destructors: */
	public:
	Size(void)
		{
		}
	explicit Size(unsigned int sC)
		{
		fillComponents(sC);
		}
	Size(unsigned int sC0,unsigned int sC1,unsigned int sC2)
		:BaseClass(sC0,sC1,sC2)
		{
		}
	Size(const Size& source)
		{
		copyComponents(source.components);
		}
	Size& operator=(const Size& source)
		{
		copyComponents(source.components);
		return *this;
		}
	
	/* Methods: */
	unsigned long volume(void) const // Returns the size's n-dimensional volume
		{
		return (unsigned long)(components[2])*(unsigned long)(components[1])*(unsigned long)(components[0]);
		}
	};

template <>
class Size<4>:public IntVector<unsigned int,4>
	{
	/* Embedded classes: */
	private:
	typedef IntVector<unsigned int,4> BaseClass;
	public:
	typedef Misc::Offset<4> Offset; // Compatible offset type
	
	/* Declarations of inherited types/elements: */
	private:
	using IntVector<unsigned int,4>::components;
	
	/* Constructors and destructors: */
	public:
	Size(void)
		{
		}
	explicit Size(unsigned int sC)
		{
		fillComponents(sC);
		}
	Size(unsigned int sC0,unsigned int sC1,unsigned int sC2,unsigned int sC3)
		:BaseClass(sC0,sC1,sC2,sC3)
		{
		}
	Size(const Size& source)
		{
		copyComponents(source.components);
		}
	Size& operator=(const Size& source)
		{
		copyComponents(source.components);
		return *this;
		}
	
	/* Methods: */
	unsigned long volume(void) const // Returns the size's n-dimensional volume
		{
		return (unsigned long)(components[3])*(unsigned long)(components[2])*(unsigned long)(components[1])*(unsigned long)(components[0]);
		}
	};

/*************************
Functions on Size objects:
*************************/

template <unsigned int numComponentsParam>
inline
Size<numComponentsParam>&
operator+=(
	Size<numComponentsParam>& s,
	const Offset<numComponentsParam>& o)
	{
	for(unsigned int i=0;i<numComponentsParam;++i)
		s[i]+=o[i];
	return s;
	}

template <unsigned int numComponentsParam>
inline
Size<numComponentsParam>
operator+(
	const Size<numComponentsParam>& s,
	const Offset<numComponentsParam>& o)
	{
	Size<numComponentsParam> result;
	for(unsigned int i=0;i<numComponentsParam;++i)
		result[i]=s[i]+o[i];
	return result;
	}

template <unsigned int numComponentsParam>
inline
Size<numComponentsParam>&
operator-=(
	Size<numComponentsParam>& s,
	const Offset<numComponentsParam>& o)
	{
	for(unsigned int i=0;i<numComponentsParam;++i)
		s[i]-=o[i];
	return s;
	}

template <unsigned int numComponentsParam>
inline
Size<numComponentsParam>
operator-(
	const Size<numComponentsParam>& s,
	const Offset<numComponentsParam>& o)
	{
	Size<numComponentsParam> result;
	for(unsigned int i=0;i<numComponentsParam;++i)
		result[i]=s[i]-o[i];
	return result;
	}

template <unsigned int numComponentsParam>
inline
Size<numComponentsParam>
min(
	const Size<numComponentsParam>& s1,
	const Size<numComponentsParam>& s2)
	{
	Size<numComponentsParam> result;
	for(unsigned int i=0;i<numComponentsParam;++i)
		result[i]=s1[i]<=s2[i]?s1[i]:s2[i];
	return result;
	}

template <unsigned int numComponentsParam>
inline
Size<numComponentsParam>
max(
	const Size<numComponentsParam>& s1,
	const Size<numComponentsParam>& s2)
	{
	Size<numComponentsParam> result;
	for(unsigned int i=0;i<numComponentsParam;++i)
		result[i]=s1[i]>=s2[i]?s1[i]:s2[i];
	return result;
	}

}

#endif

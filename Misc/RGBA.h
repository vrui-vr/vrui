/***********************************************************************
RGBA - Class to represent colors in the RGB color space with opacity
components.
Copyright (c) 2020-2023 Oliver Kreylos

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

#ifndef MISC_RGBA_INCLUDED
#define MISC_RGBA_INCLUDED

#include <Misc/ColorComponentTraits.h>
#include <Misc/RGB.h>

namespace Misc {

/* Declarations of friend functions of class RGBA: */
template <class ScalarParam>
class RGBA;
template <class ScalarParam>
RGBA<ScalarParam> blend(const RGBA<ScalarParam>& c1,const RGBA<ScalarParam>& c2,double blend);

template <class ScalarParam>
class RGBA
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Type for RGBA color components
	static const int numComponents=4; // Number of color components
	
	/* Elements: */
	private:
	Scalar components[4]; // Array of RGBA color components
	
	/* Private methods: */
	template <int numComponentsParam>
	void copy(const Scalar* sComponents) // Copies an array of color components
		{
		for(int i=0;i<numComponentsParam;++i)
			components[i]=sComponents[i];
		}
	template <int numComponentsParam,class SourceScalarParam>
	void convertAndCopy(const SourceScalarParam* sComponents); // Converts and copies an array of color components
	
	/* Constructors and destructors: */
	public:
	RGBA(void) // Dummy constructor
		{
		}
	RGBA(Scalar sRed,Scalar sGreen,Scalar sBlue,Scalar sAlpha =ColorComponentTraits<Scalar>::one) // Component-wise initialization
		{
		/* Set color components: */
		components[0]=sRed;
		components[1]=sGreen;
		components[2]=sBlue;
		components[3]=sAlpha;
		}
	RGBA(const Scalar sComponents[4]) // Component-wise initialization from array
		{
		/* Copy color components: */
		copy<4>(sComponents);
		}
	template <class SourceScalarParam>
	RGBA(const Scalar sComponents[4]) // Component-wise initialization from array with type conversion
		{
		/* Convert and copy color components from the source: */
		convertAndCopy<4>(sComponents);
		}
	template <class SourceScalarParam>
	RGBA(const RGBA<SourceScalarParam>& source) // Copy constructor with type conversion
		{
		/* Convert and copy color components from the source: */
		convertAndCopy<4>(source.getComponents());
		}
	RGBA(const RGB<Scalar>& source,Scalar sAlpha =ColorComponentTraits<Scalar>::one) // Copy constructor with opacity extension
		{
		/* Copy color components from the source: */
		copy<3>(source.getComponents());
		
		/* Assign opacity: */
		components[3]=sAlpha;
		}
	template <class SourceScalarParam>
	RGBA(const RGB<SourceScalarParam>& source,Scalar sAlpha =ColorComponentTraits<Scalar>::one) // Copy constructor with type conversion and opacity extension
		{
		/* Convert and copy color components from the source: */
		convertAndCopy<3>(source.getComponents());
		
		/* Assign opacity: */
		components[3]=sAlpha;
		}
	
	/* Methods: */
	explicit operator RGB<Scalar>(void) const // Conversion operator to RGB color
		{
		return RGB<Scalar>(components);
		}
	template <class SourceScalarParam>
	RGBA& operator=(const RGBA<SourceScalarParam>& source) // Assignment operator with type conversion
		{
		/* Convert and copy color components from the source (no need to check for aliasing): */
		convertAndCopy<4>(source.getComponents());
		
		return *this;
		}
	RGBA& operator=(const RGB<Scalar>& source) // Assignment operator with opacity extension
		{
		/* Copy color components from the source (no need to check for aliasing): */
		copy<3>(source.getComponents());
		
		/* Assign opacity: */
		components[3]=ColorComponentTraits<Scalar>::one;
		
		return *this;
		}
	template <class SourceScalarParam>
	RGBA& operator=(const RGB<SourceScalarParam>& source) // Assignment operator with type conversion and opacity extension
		{
		/* Convert and copy color components from the source (no need to check for aliasing): */
		convertAndCopy<3>(source.getComponents());
		
		/* Assign opacity: */
		components[3]=ColorComponentTraits<Scalar>::one;
		
		return *this;
		}
	const Scalar* getComponents(void) const // Returns the array of color components
		{
		return components;
		}
	Scalar* getComponents(void) // Ditto
		{
		return components;
		}
	Scalar operator[](int index) const // Returns one color component as an rvalue
		{
		return components[index];
		}
	Scalar& operator[](int index) // Returns one color component as an lvalue
		{
		return components[index];
		}
	friend RGBA blend<>(const RGBA& c1,const RGBA& c2,double blend); // Blends two colors with a blending factor in the [0, 1] interval
	};

}

#endif

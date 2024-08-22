/***********************************************************************
GLColor - Class to represent color values in RGBA format.
Copyright (c) 2003-2023 Oliver Kreylos

This file is part of the OpenGL C++ Wrapper Library (GLWrappers).

The OpenGL C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLCOLOR_INCLUDED
#define GLCOLOR_INCLUDED

#include <GL/gl.h>
#include <GL/GLScalarLimits.h>

/**************************************
Dummy generic version of GLColor class:
**************************************/

template <class ScalarParam,GLsizei numComponentsParam>
class GLColor
	{
	};

/****************************************************
Specialized versions of GLColor class for RGB colors:
****************************************************/

template <class ScalarParam>
class GLColor<ScalarParam,3>
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type
	static const GLsizei numComponents=3; // Number of stored color components
	
	/* Elements: */
	private:
	Scalar rgba[3]; // RGBA components without the A
	
	/* Private methods: */
	void copy(const Scalar* sRgba) // Copies an array of color components
		{
		for(int i=0;i<3;++i)
			rgba[i]=sRgba[i];
		}
	template <class SourceScalarParam>
	void convertAndCopy(const SourceScalarParam* sRgba); // Converts and copies an array of color components
	
	/* Constructors and destructors: */
	public:
	GLColor(void) // Dummy constructor
		{
		}
	GLColor(Scalar sRed,Scalar sGreen,Scalar sBlue) // Component-wise initialization
		{
		rgba[0]=sRed;
		rgba[1]=sGreen;
		rgba[2]=sBlue;
		}
	GLColor(const Scalar sRgba[3]) // Component-wise initialization from array
		{
		/* Copy color components: */
		copy(sRgba);
		}
	template <class SourceScalarParam>
	GLColor(const SourceScalarParam sRgba[3]) // Component-wise initialization from array with type conversion
		{
		/* Convert and copy color components from the source: */
		convertAndCopy(sRgba);
		}
	template <class SourceScalarParam>
	GLColor(const GLColor<SourceScalarParam,3>& source) // Copy constructor with type conversion
		{
		/* Convert and copy color components from the source: */
		convertAndCopy(source.getRgba());
		}
	
	/* Methods: */
	template <class SourceScalarParam>
	GLColor& operator=(const GLColor<SourceScalarParam,3>& source) // Assignment operator with type conversion
		{
		/* Convert and copy color components from the source (no need to check for aliasing): */
		convertAndCopy(source.getRgba());
		
		return *this;
		}
	const Scalar* getRgba(void) const // Returns the array of color components
		{
		return rgba;
		}
	Scalar* getRgba(void) // Ditto
		{
		return rgba;
		}
	Scalar operator[](int index) const // Returns one color component as an rvalue
		{
		return rgba[index];
		}
	Scalar& operator[](int index) // Returns one color component as an lvalue
		{
		return rgba[index];
		}
	};

/*****************************************************************
Specialized versions of GLColor class for RGB colors with opacity:
*****************************************************************/

template <class ScalarParam>
class GLColor<ScalarParam,4>
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type
	static const GLsizei numComponents=4; // Number of stored color components
	
	/* Elements: */
	private:
	Scalar rgba[4]; // RGBA components
	
	/* Private methods: */
	template <GLsizei numComponentsParam>
	void copy(const Scalar* sRgba) // Copies an array of color components
		{
		for(int i=0;i<numComponentsParam;++i)
			rgba[i]=sRgba[i];
		}
	template <GLsizei numComponentsParam,class SourceScalarParam>
	void convertAndCopy(const SourceScalarParam* sRgba); // Converts and copies an array of color components
	
	/* Constructors and destructors: */
	public:
	GLColor(void) // Dummy constructor
		{
		}
	GLColor(Scalar sRed,Scalar sGreen,Scalar sBlue,Scalar sAlpha =GLScalarLimits<Scalar>::max) // Component-wise initialization
		{
		rgba[0]=sRed;
		rgba[1]=sGreen;
		rgba[2]=sBlue;
		rgba[3]=sAlpha;
		}
	GLColor(const Scalar sRgba[4]) // Component-wise initialization from array
		{
		/* Copy color components: */
		copy<4>(sRgba);
		}
	template <class SourceScalarParam>
	GLColor(const SourceScalarParam sRgba[4]) // Component-wise initialization from array with type conversion
		{
		/* Convert and copy color components from the source: */
		convertAndCopy<4>(sRgba);
		}
	template <class SourceScalarParam>
	GLColor(const GLColor<SourceScalarParam,4>& source) // Copy constructor with type conversion
		{
		/* Convert and copy color components from the source: */
		convertAndCopy<4>(source.getRgba());
		}
	GLColor(const GLColor<Scalar,3>& source,Scalar sAlpha =GLScalarLimits<Scalar>::max) // Copy constructor with opacity extension
		{
		/* Copy color components from the source: */
		copy<3>(source.getRgba());
		
		/* Assign opacity: */
		rgba[3]=sAlpha;
		}
	template <class SourceScalarParam>
	GLColor(const GLColor<SourceScalarParam,3>& source,Scalar sAlpha =GLScalarLimits<Scalar>::max) // Copy constructor with type conversion and opacity extension
		{
		/* Convert and copy color components from the source: */
		convertAndCopy<3>(source.getRgba());
		
		/* Assign opacity: */
		rgba[3]=sAlpha;
		}
	
	/* Methods: */
	explicit operator GLColor<Scalar,3>(void) const // Conversion operator to RGB color
		{
		return GLColor<Scalar,3>(rgba);
		}
	template <class SourceScalarParam>
	GLColor& operator=(const GLColor<SourceScalarParam,4>& source) // Assignment operator with type conversion
		{
		/* Convert and copy color components from the source (no need to check for aliasing): */
		convertAndCopy<4>(source.getRgba());
		
		return *this;
		}
	GLColor& operator=(const GLColor<Scalar,3>& source) // Assignment operator with opacity extension
		{
		/* Copy color components from the source (no need to check for aliasing): */
		copy<3>(source.getRgba());
		
		/* Assign opacity: */
		rgba[3]=GLScalarLimits<Scalar>::max;
		
		return *this;
		}
	template <class SourceScalarParam>
	GLColor& operator=(const GLColor<SourceScalarParam,3>& source) // Assignment operator with type conversion and opacity extension
		{
		/* Convert and copy color components from the source (no need to check for aliasing): */
		convertAndCopy<3>(source.getRgba());
		
		/* Assign opacity: */
		rgba[3]=GLScalarLimits<Scalar>::max;
		
		return *this;
		}
	const Scalar* getRgba(void) const // Returns the array of color components
		{
		return rgba;
		}
	Scalar* getRgba(void) // Ditto
		{
		return rgba;
		}
	Scalar operator[](int index) const // Returns one color component as an rvalue
		{
		return rgba[index];
		}
	Scalar& operator[](int index) // Returns one color component as an lvalue
		{
		return rgba[index];
		}
	};

/* Comparison operators: */

template <class ScalarParam,GLsizei numComponentsParam>
inline bool operator==(const GLColor<ScalarParam,numComponentsParam>& c1,const GLColor<ScalarParam,numComponentsParam>& c2)
	{
	bool result=true;
	for(GLsizei i=0;i<numComponentsParam&&result;++i)
		result=c1[i]==c2[i];
	return result;
	}

template <class ScalarParam,GLsizei numComponentsParam>
inline bool operator!=(const GLColor<ScalarParam,numComponentsParam>& c1,const GLColor<ScalarParam,numComponentsParam>& c2)
	{
	bool result=false;
	for(GLsizei i=0;i<numComponentsParam&&!result;++i)
		result=c1[i]!=c2[i];
	return result;
	}

#endif

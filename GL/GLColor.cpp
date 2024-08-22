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

#include <GL/GLColor.h>

#include <GL/GLScalarConverter.h>

/***************************************
Methods of class GLColor<ScalarParam,3>:
***************************************/

template <class ScalarParam>
template <class SourceScalarParam>
void
GLColor<ScalarParam,3>::convertAndCopy(
	const SourceScalarParam* sRgba)
	{
	for(GLsizei i=0;i<3;++i)
		rgba[i]=glConvertScalar<Scalar>(sRgba[i]);
	}

/***************************************
Methods of class GLColor<ScalarParam,4>:
***************************************/

template <class ScalarParam>
template <GLsizei numComponentsParam,class SourceScalarParam>
void
GLColor<ScalarParam,4>::convertAndCopy(
	const SourceScalarParam* sRgba)
	{
	for(GLsizei i=0;i<numComponentsParam;++i)
		rgba[i]=glConvertScalar<Scalar>(sRgba[i]);
	}

/******************************************
Force instantiation of all GLColor classes:
******************************************/

template class GLColor<GLbyte,3>;
template void GLColor<GLbyte,3>::convertAndCopy<GLubyte>(const GLubyte*);
template void GLColor<GLbyte,3>::convertAndCopy<GLshort>(const GLshort*);
template void GLColor<GLbyte,3>::convertAndCopy<GLushort>(const GLushort*);
template void GLColor<GLbyte,3>::convertAndCopy<GLint>(const GLint*);
template void GLColor<GLbyte,3>::convertAndCopy<GLuint>(const GLuint*);
template void GLColor<GLbyte,3>::convertAndCopy<GLfloat>(const GLfloat*);
template void GLColor<GLbyte,3>::convertAndCopy<GLdouble>(const GLdouble*);

template class GLColor<GLubyte,3>;
template void GLColor<GLubyte,3>::convertAndCopy<GLbyte>(const GLbyte*);
template void GLColor<GLubyte,3>::convertAndCopy<GLshort>(const GLshort*);
template void GLColor<GLubyte,3>::convertAndCopy<GLushort>(const GLushort*);
template void GLColor<GLubyte,3>::convertAndCopy<GLint>(const GLint*);
template void GLColor<GLubyte,3>::convertAndCopy<GLuint>(const GLuint*);
template void GLColor<GLubyte,3>::convertAndCopy<GLfloat>(const GLfloat*);
template void GLColor<GLubyte,3>::convertAndCopy<GLdouble>(const GLdouble*);

template class GLColor<GLshort,3>;
template void GLColor<GLshort,3>::convertAndCopy<GLbyte>(const GLbyte*);
template void GLColor<GLshort,3>::convertAndCopy<GLubyte>(const GLubyte*);
template void GLColor<GLshort,3>::convertAndCopy<GLushort>(const GLushort*);
template void GLColor<GLshort,3>::convertAndCopy<GLint>(const GLint*);
template void GLColor<GLshort,3>::convertAndCopy<GLuint>(const GLuint*);
template void GLColor<GLshort,3>::convertAndCopy<GLfloat>(const GLfloat*);
template void GLColor<GLshort,3>::convertAndCopy<GLdouble>(const GLdouble*);

template class GLColor<GLushort,3>;
template void GLColor<GLushort,3>::convertAndCopy<GLbyte>(const GLbyte*);
template void GLColor<GLushort,3>::convertAndCopy<GLubyte>(const GLubyte*);
template void GLColor<GLushort,3>::convertAndCopy<GLshort>(const GLshort*);
template void GLColor<GLushort,3>::convertAndCopy<GLint>(const GLint*);
template void GLColor<GLushort,3>::convertAndCopy<GLuint>(const GLuint*);
template void GLColor<GLushort,3>::convertAndCopy<GLfloat>(const GLfloat*);
template void GLColor<GLushort,3>::convertAndCopy<GLdouble>(const GLdouble*);

template class GLColor<GLint,3>;
template void GLColor<GLint,3>::convertAndCopy<GLbyte>(const GLbyte*);
template void GLColor<GLint,3>::convertAndCopy<GLubyte>(const GLubyte*);
template void GLColor<GLint,3>::convertAndCopy<GLshort>(const GLshort*);
template void GLColor<GLint,3>::convertAndCopy<GLushort>(const GLushort*);
template void GLColor<GLint,3>::convertAndCopy<GLuint>(const GLuint*);
template void GLColor<GLint,3>::convertAndCopy<GLfloat>(const GLfloat*);
template void GLColor<GLint,3>::convertAndCopy<GLdouble>(const GLdouble*);

template class GLColor<GLuint,3>;
template void GLColor<GLuint,3>::convertAndCopy<GLbyte>(const GLbyte*);
template void GLColor<GLuint,3>::convertAndCopy<GLubyte>(const GLubyte*);
template void GLColor<GLuint,3>::convertAndCopy<GLshort>(const GLshort*);
template void GLColor<GLuint,3>::convertAndCopy<GLushort>(const GLushort*);
template void GLColor<GLuint,3>::convertAndCopy<GLint>(const GLint*);
template void GLColor<GLuint,3>::convertAndCopy<GLfloat>(const GLfloat*);
template void GLColor<GLuint,3>::convertAndCopy<GLdouble>(const GLdouble*);

template class GLColor<GLfloat,3>;
template void GLColor<GLfloat,3>::convertAndCopy<GLbyte>(const GLbyte*);
template void GLColor<GLfloat,3>::convertAndCopy<GLubyte>(const GLubyte*);
template void GLColor<GLfloat,3>::convertAndCopy<GLshort>(const GLshort*);
template void GLColor<GLfloat,3>::convertAndCopy<GLushort>(const GLushort*);
template void GLColor<GLfloat,3>::convertAndCopy<GLint>(const GLint*);
template void GLColor<GLfloat,3>::convertAndCopy<GLuint>(const GLuint*);
template void GLColor<GLfloat,3>::convertAndCopy<GLdouble>(const GLdouble*);

template class GLColor<GLdouble,3>;
template void GLColor<GLdouble,3>::convertAndCopy<GLbyte>(const GLbyte*);
template void GLColor<GLdouble,3>::convertAndCopy<GLubyte>(const GLubyte*);
template void GLColor<GLdouble,3>::convertAndCopy<GLshort>(const GLshort*);
template void GLColor<GLdouble,3>::convertAndCopy<GLushort>(const GLushort*);
template void GLColor<GLdouble,3>::convertAndCopy<GLint>(const GLint*);
template void GLColor<GLdouble,3>::convertAndCopy<GLuint>(const GLuint*);
template void GLColor<GLdouble,3>::convertAndCopy<GLfloat>(const GLfloat*);

template class GLColor<GLbyte,4>;
template void GLColor<GLbyte,4>::convertAndCopy<3,GLubyte>(const GLubyte*);
template void GLColor<GLbyte,4>::convertAndCopy<3,GLshort>(const GLshort*);
template void GLColor<GLbyte,4>::convertAndCopy<3,GLushort>(const GLushort*);
template void GLColor<GLbyte,4>::convertAndCopy<3,GLint>(const GLint*);
template void GLColor<GLbyte,4>::convertAndCopy<3,GLuint>(const GLuint*);
template void GLColor<GLbyte,4>::convertAndCopy<3,GLfloat>(const GLfloat*);
template void GLColor<GLbyte,4>::convertAndCopy<3,GLdouble>(const GLdouble*);
template void GLColor<GLbyte,4>::convertAndCopy<4,GLubyte>(const GLubyte*);
template void GLColor<GLbyte,4>::convertAndCopy<4,GLshort>(const GLshort*);
template void GLColor<GLbyte,4>::convertAndCopy<4,GLushort>(const GLushort*);
template void GLColor<GLbyte,4>::convertAndCopy<4,GLint>(const GLint*);
template void GLColor<GLbyte,4>::convertAndCopy<4,GLuint>(const GLuint*);
template void GLColor<GLbyte,4>::convertAndCopy<4,GLfloat>(const GLfloat*);
template void GLColor<GLbyte,4>::convertAndCopy<4,GLdouble>(const GLdouble*);

template class GLColor<GLubyte,4>;
template void GLColor<GLubyte,4>::convertAndCopy<3,GLbyte>(const GLbyte*);
template void GLColor<GLubyte,4>::convertAndCopy<3,GLshort>(const GLshort*);
template void GLColor<GLubyte,4>::convertAndCopy<3,GLushort>(const GLushort*);
template void GLColor<GLubyte,4>::convertAndCopy<3,GLint>(const GLint*);
template void GLColor<GLubyte,4>::convertAndCopy<3,GLuint>(const GLuint*);
template void GLColor<GLubyte,4>::convertAndCopy<3,GLfloat>(const GLfloat*);
template void GLColor<GLubyte,4>::convertAndCopy<3,GLdouble>(const GLdouble*);
template void GLColor<GLubyte,4>::convertAndCopy<4,GLbyte>(const GLbyte*);
template void GLColor<GLubyte,4>::convertAndCopy<4,GLshort>(const GLshort*);
template void GLColor<GLubyte,4>::convertAndCopy<4,GLushort>(const GLushort*);
template void GLColor<GLubyte,4>::convertAndCopy<4,GLint>(const GLint*);
template void GLColor<GLubyte,4>::convertAndCopy<4,GLuint>(const GLuint*);
template void GLColor<GLubyte,4>::convertAndCopy<4,GLfloat>(const GLfloat*);
template void GLColor<GLubyte,4>::convertAndCopy<4,GLdouble>(const GLdouble*);

template class GLColor<GLshort,4>;
template void GLColor<GLshort,4>::convertAndCopy<3,GLbyte>(const GLbyte*);
template void GLColor<GLshort,4>::convertAndCopy<3,GLubyte>(const GLubyte*);
template void GLColor<GLshort,4>::convertAndCopy<3,GLushort>(const GLushort*);
template void GLColor<GLshort,4>::convertAndCopy<3,GLint>(const GLint*);
template void GLColor<GLshort,4>::convertAndCopy<3,GLuint>(const GLuint*);
template void GLColor<GLshort,4>::convertAndCopy<3,GLfloat>(const GLfloat*);
template void GLColor<GLshort,4>::convertAndCopy<3,GLdouble>(const GLdouble*);
template void GLColor<GLshort,4>::convertAndCopy<4,GLbyte>(const GLbyte*);
template void GLColor<GLshort,4>::convertAndCopy<4,GLubyte>(const GLubyte*);
template void GLColor<GLshort,4>::convertAndCopy<4,GLushort>(const GLushort*);
template void GLColor<GLshort,4>::convertAndCopy<4,GLint>(const GLint*);
template void GLColor<GLshort,4>::convertAndCopy<4,GLuint>(const GLuint*);
template void GLColor<GLshort,4>::convertAndCopy<4,GLfloat>(const GLfloat*);
template void GLColor<GLshort,4>::convertAndCopy<4,GLdouble>(const GLdouble*);

template class GLColor<GLushort,4>;
template void GLColor<GLushort,4>::convertAndCopy<3,GLbyte>(const GLbyte*);
template void GLColor<GLushort,4>::convertAndCopy<3,GLubyte>(const GLubyte*);
template void GLColor<GLushort,4>::convertAndCopy<3,GLshort>(const GLshort*);
template void GLColor<GLushort,4>::convertAndCopy<3,GLint>(const GLint*);
template void GLColor<GLushort,4>::convertAndCopy<3,GLuint>(const GLuint*);
template void GLColor<GLushort,4>::convertAndCopy<3,GLfloat>(const GLfloat*);
template void GLColor<GLushort,4>::convertAndCopy<3,GLdouble>(const GLdouble*);
template void GLColor<GLushort,4>::convertAndCopy<4,GLbyte>(const GLbyte*);
template void GLColor<GLushort,4>::convertAndCopy<4,GLubyte>(const GLubyte*);
template void GLColor<GLushort,4>::convertAndCopy<4,GLshort>(const GLshort*);
template void GLColor<GLushort,4>::convertAndCopy<4,GLint>(const GLint*);
template void GLColor<GLushort,4>::convertAndCopy<4,GLuint>(const GLuint*);
template void GLColor<GLushort,4>::convertAndCopy<4,GLfloat>(const GLfloat*);
template void GLColor<GLushort,4>::convertAndCopy<4,GLdouble>(const GLdouble*);

template class GLColor<GLint,4>;
template void GLColor<GLint,4>::convertAndCopy<3,GLbyte>(const GLbyte*);
template void GLColor<GLint,4>::convertAndCopy<3,GLubyte>(const GLubyte*);
template void GLColor<GLint,4>::convertAndCopy<3,GLshort>(const GLshort*);
template void GLColor<GLint,4>::convertAndCopy<3,GLushort>(const GLushort*);
template void GLColor<GLint,4>::convertAndCopy<3,GLuint>(const GLuint*);
template void GLColor<GLint,4>::convertAndCopy<3,GLfloat>(const GLfloat*);
template void GLColor<GLint,4>::convertAndCopy<3,GLdouble>(const GLdouble*);
template void GLColor<GLint,4>::convertAndCopy<4,GLbyte>(const GLbyte*);
template void GLColor<GLint,4>::convertAndCopy<4,GLubyte>(const GLubyte*);
template void GLColor<GLint,4>::convertAndCopy<4,GLshort>(const GLshort*);
template void GLColor<GLint,4>::convertAndCopy<4,GLushort>(const GLushort*);
template void GLColor<GLint,4>::convertAndCopy<4,GLuint>(const GLuint*);
template void GLColor<GLint,4>::convertAndCopy<4,GLfloat>(const GLfloat*);
template void GLColor<GLint,4>::convertAndCopy<4,GLdouble>(const GLdouble*);

template class GLColor<GLuint,4>;
template void GLColor<GLuint,4>::convertAndCopy<3,GLbyte>(const GLbyte*);
template void GLColor<GLuint,4>::convertAndCopy<3,GLubyte>(const GLubyte*);
template void GLColor<GLuint,4>::convertAndCopy<3,GLshort>(const GLshort*);
template void GLColor<GLuint,4>::convertAndCopy<3,GLushort>(const GLushort*);
template void GLColor<GLuint,4>::convertAndCopy<3,GLint>(const GLint*);
template void GLColor<GLuint,4>::convertAndCopy<3,GLfloat>(const GLfloat*);
template void GLColor<GLuint,4>::convertAndCopy<3,GLdouble>(const GLdouble*);
template void GLColor<GLuint,4>::convertAndCopy<4,GLbyte>(const GLbyte*);
template void GLColor<GLuint,4>::convertAndCopy<4,GLubyte>(const GLubyte*);
template void GLColor<GLuint,4>::convertAndCopy<4,GLshort>(const GLshort*);
template void GLColor<GLuint,4>::convertAndCopy<4,GLushort>(const GLushort*);
template void GLColor<GLuint,4>::convertAndCopy<4,GLint>(const GLint*);
template void GLColor<GLuint,4>::convertAndCopy<4,GLfloat>(const GLfloat*);
template void GLColor<GLuint,4>::convertAndCopy<4,GLdouble>(const GLdouble*);

template class GLColor<GLfloat,4>;
template void GLColor<GLfloat,4>::convertAndCopy<3,GLbyte>(const GLbyte*);
template void GLColor<GLfloat,4>::convertAndCopy<3,GLubyte>(const GLubyte*);
template void GLColor<GLfloat,4>::convertAndCopy<3,GLshort>(const GLshort*);
template void GLColor<GLfloat,4>::convertAndCopy<3,GLushort>(const GLushort*);
template void GLColor<GLfloat,4>::convertAndCopy<3,GLint>(const GLint*);
template void GLColor<GLfloat,4>::convertAndCopy<3,GLuint>(const GLuint*);
template void GLColor<GLfloat,4>::convertAndCopy<3,GLdouble>(const GLdouble*);
template void GLColor<GLfloat,4>::convertAndCopy<4,GLbyte>(const GLbyte*);
template void GLColor<GLfloat,4>::convertAndCopy<4,GLubyte>(const GLubyte*);
template void GLColor<GLfloat,4>::convertAndCopy<4,GLshort>(const GLshort*);
template void GLColor<GLfloat,4>::convertAndCopy<4,GLushort>(const GLushort*);
template void GLColor<GLfloat,4>::convertAndCopy<4,GLint>(const GLint*);
template void GLColor<GLfloat,4>::convertAndCopy<4,GLuint>(const GLuint*);
template void GLColor<GLfloat,4>::convertAndCopy<4,GLdouble>(const GLdouble*);

template class GLColor<GLdouble,4>;
template void GLColor<GLdouble,4>::convertAndCopy<3,GLbyte>(const GLbyte*);
template void GLColor<GLdouble,4>::convertAndCopy<3,GLubyte>(const GLubyte*);
template void GLColor<GLdouble,4>::convertAndCopy<3,GLshort>(const GLshort*);
template void GLColor<GLdouble,4>::convertAndCopy<3,GLushort>(const GLushort*);
template void GLColor<GLdouble,4>::convertAndCopy<3,GLint>(const GLint*);
template void GLColor<GLdouble,4>::convertAndCopy<3,GLuint>(const GLuint*);
template void GLColor<GLdouble,4>::convertAndCopy<3,GLfloat>(const GLfloat*);
template void GLColor<GLdouble,4>::convertAndCopy<4,GLbyte>(const GLbyte*);
template void GLColor<GLdouble,4>::convertAndCopy<4,GLubyte>(const GLubyte*);
template void GLColor<GLdouble,4>::convertAndCopy<4,GLshort>(const GLshort*);
template void GLColor<GLdouble,4>::convertAndCopy<4,GLushort>(const GLushort*);
template void GLColor<GLdouble,4>::convertAndCopy<4,GLint>(const GLint*);
template void GLColor<GLdouble,4>::convertAndCopy<4,GLuint>(const GLuint*);
template void GLColor<GLdouble,4>::convertAndCopy<4,GLfloat>(const GLfloat*);

/***********************************************************************
GLGetMiscTemplates - Overloaded versions of many OpenGL get... API
calls.
Copyright (c) 2004-2022 Oliver Kreylos

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

#ifndef GLGETMISCTEMPLATES_INCLUDED
#define GLGETMISCTEMPLATES_INCLUDED

#include <Misc/Rect.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLGetTemplates.h>

inline GLboolean glGetBlend(void)
	{
	return glGet<GLboolean>(GL_BLEND);
	}

inline void glGetBlend(GLboolean& param)
	{
	glGet(GL_BLEND,&param);
	}

template <class ScalarParam>
inline void glGetBlendColor(ScalarParam params[4])
	{
	glGet(GL_BLEND_COLOR,params);
	}

template <class ScalarParam>
inline GLColor<ScalarParam,4> glGetBlendColor(void)
	{
	GLColor<ScalarParam,4> result;
	glGet(GL_BLEND,result.getRgba());
	return result;
	}

template <class ScalarParam>
inline void glGetBlendColor(GLColor<ScalarParam,4>& param)
	{
	glGet(GL_BLEND,param.getRgba());
	}

inline GLenum glGetBlendEquation(void)
	{
	return GLenum(glGet<GLint>(GL_BLEND_EQUATION));
	}

inline void glGetBlendEquation(GLenum& param)
	{
	GLint intParam;
	glGet(GL_BLEND_EQUATION,&intParam);
	param=GLenum(intParam);
	}

inline GLenum glGetBlendSrc(void)
	{
	return GLenum(glGet<GLint>(GL_BLEND_SRC));
	}

inline void glGetBlendSrc(GLenum& param)
	{
	GLint intParam;
	glGet(GL_BLEND_SRC,&intParam);
	param=GLenum(intParam);
	}

inline GLenum glGetBlendDst(void)
	{
	return GLenum(glGet<GLint>(GL_BLEND_DST));
	}

inline void glGetBlendDst(GLenum& param)
	{
	GLint intParam;
	glGet(GL_BLEND_DST,&intParam);
	param=GLenum(intParam);
	}

inline GLboolean glGetColorArray(void)
	{
	return glGet<GLboolean>(GL_COLOR_ARRAY);
	}

inline void glGetColorArray(GLboolean& param)
	{
	glGet(GL_COLOR_ARRAY,&param);
	}

inline GLint glGetColorArraySize(void)
	{
	return glGet<GLint>(GL_COLOR_ARRAY_SIZE);
	}

inline void glGetColorArraySize(GLint& param)
	{
	return glGet(GL_COLOR_ARRAY_SIZE,&param);
	}

template <class ScalarParam>
inline void glGetColorClearValue(ScalarParam params[4])
	{
	glGet(GL_COLOR_CLEAR_VALUE,params);
	}

template <class ScalarParam>
inline GLColor<ScalarParam,4> glGetColorClearValue(void)
	{
	GLColor<ScalarParam,4> result;
	glGet(GL_COLOR_CLEAR_VALUE,result.getRgba());
	return result;
	}

template <class ScalarParam>
inline void glGetColorClearValue(GLColor<ScalarParam,4>& param)
	{
	glGet(GL_COLOR_CLEAR_VALUE,param.getRgba());
	}

inline Misc::Rect<2> glGetViewport(void)
	{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT,viewport);
	return Misc::Rect<2>(Misc::Rect<2>::Offset(viewport[0],viewport[1]),Misc::Rect<2>::Size(viewport[2],viewport[3]));
	}

inline Misc::Rect<2> glGetScissor(void)
	{
	GLint scissorBox[4];
	glGetIntegerv(GL_SCISSOR_BOX,scissorBox);
	return Misc::Rect<2>(Misc::Rect<2>::Offset(scissorBox[0],scissorBox[1]),Misc::Rect<2>::Size(scissorBox[2],scissorBox[3]));
	}

#endif

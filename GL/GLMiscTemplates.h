/***********************************************************************
GLMiscTemplates - Overloaded versions of many OpenGL API calls.
Copyright (c) 2003-2022 Oliver Kreylos

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

#ifndef GLMISCTEMPLATES_INCLUDED
#define GLMISCTEMPLATES_INCLUDED

#include <Misc/Offset.h>
#include <Misc/Size.h>
#include <Misc/Rect.h>
#include <GL/gl.h>

inline void glTexImage2D(GLenum target,GLint level,GLint internalFormat,const Misc::Size<2>& size,GLint border,GLenum format,GLenum type,const GLvoid* data)
	{
	glTexImage2D(target,level,internalFormat,size[0],size[1],border,format,type,data);
	}

inline void glCopyTexImage2D(GLenum target,GLint level,GLint internalFormat,const Misc::Rect<2>& rect,GLint border)
	{
	glCopyTexImage2D(target,level,internalFormat,rect.offset[0],rect.offset[1],rect.size[0],rect.size[1],border);
	}

inline void glTexSubImage2D(GLenum target,GLint level,const Misc::Rect<2>& rect,GLenum format,GLenum type,const GLvoid* pixels)
	{
	glTexSubImage2D(target,level,rect.offset[0],rect.offset[1],rect.size[0],rect.size[1],format,type,pixels);
	}

inline void glCopyTexSubImage2D(GLenum target,GLint level,const Misc::Offset<2>& offset,const Misc::Rect<2>& rect)
	{
	glCopyTexSubImage2D(target,level,offset[0],offset[1],rect.offset[0],rect.offset[1],rect.size[0],rect.size[1]);
	}

inline void glReadPixels(const Misc::Rect<2>& rect,GLenum format,GLenum type,GLvoid* data)
	{
	glReadPixels(rect.offset[0],rect.offset[1],rect.size[0],rect.size[1],format,type,data);
	}

inline void glDrawPixels(const Misc::Size<2>& size,GLenum format,GLenum type,const GLvoid* pixels)
	{
	glDrawPixels(size[0],size[1],format,type,pixels);
	}

inline void glViewport(const Misc::Rect<2>& viewport)
	{
	glViewport(GLint(viewport.offset[0]),GLint(viewport.offset[1]),GLsizei(viewport.size[0]),GLsizei(viewport.size[1]));
	}

inline void glScissor(const Misc::Rect<2>& scissorBox)
	{
	glScissor(GLint(scissorBox.offset[0]),GLint(scissorBox.offset[1]),GLsizei(scissorBox.size[0]),GLsizei(scissorBox.size[1]));
	}

#endif

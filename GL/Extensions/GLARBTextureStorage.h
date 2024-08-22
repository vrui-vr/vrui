/***********************************************************************
GLARBTextureStorage - OpenGL extension class for the
GL_ARB_texture_storage extension.
Copyright (c) 2022 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLEXTENSIONS_GLARBTEXTURESTORAGE_INCLUDED
#define GLEXTENSIONS_GLARBTEXTURESTORAGE_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_ARB_texture_storage
#define GL_ARB_texture_storage 1

/* Extension-specific functions: */
typedef void (APIENTRY * PFNGLTEXSTORAGE1DPROC) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
typedef void (APIENTRY * PFNGLTEXSTORAGE2DPROC) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY * PFNGLTEXSTORAGE3DPROC) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

/* Extension-specific constants: */
#define GL_TEXTURE_IMMUTABLE_FORMAT_ARB 0x912F

#endif

/* Forward declarations of friend functions: */
void glTexStorage1D(GLenum target,GLsizei levels,GLenum internalformat,GLsizei width);
void glTexStorage2D(GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height);
void glTexStorage3D(GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth);

class GLARBTextureStorage:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLARBTextureStorage*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLTEXSTORAGE1DPROC glTexStorage1DProc;
	PFNGLTEXSTORAGE2DPROC glTexStorage2DProc;
	PFNGLTEXSTORAGE3DPROC glTexStorage3DProc;
	
	/* Constructors and destructors: */
	private:
	GLARBTextureStorage(void);
	public:
	virtual ~GLARBTextureStorage(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glTexStorage1D(GLenum target,GLsizei levels,GLenum internalformat,GLsizei width)
		{
		GLARBTextureStorage::current->glTexStorage1DProc(target,levels,internalformat,width);
		}
	inline friend void glTexStorage2D(GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height)
		{
		GLARBTextureStorage::current->glTexStorage2DProc(target,levels,internalformat,width,height);
		}
	inline friend void glTexStorage3D(GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth)
		{
		GLARBTextureStorage::current->glTexStorage3DProc(target,levels,internalformat,width,height,depth);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif

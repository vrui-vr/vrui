/***********************************************************************
GLEXTFramebufferMultisample - OpenGL extension class for the
GL_EXT_framebuffer_multisample extension.
Copyright (c) 2013-2022 Oliver Kreylos

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

#ifndef GLEXTENSIONS_GLEXTFRAMEBUFFERMULTISAMPLE_INCLUDED
#define GLEXTENSIONS_GLEXTFRAMEBUFFERMULTISAMPLE_INCLUDED

#include <Misc/Size.h>
#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_framebuffer_multisample
#define GL_EXT_framebuffer_multisample 1

/* Extension-specific functions: */
typedef void (APIENTRY * PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

/* Extension-specific constants: */
#define GL_RENDERBUFFER_SAMPLES_EXT       0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT 0x8D56
#define GL_MAX_SAMPLES_EXT                0x8D57

#endif

/* Forward declarations of friend functions: */
void glRenderbufferStorageMultisampleEXT(GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height);

class GLEXTFramebufferMultisample:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTFramebufferMultisample*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXTProc;
	
	/* Constructors and destructors: */
	private:
	GLEXTFramebufferMultisample(void);
	public:
	virtual ~GLEXTFramebufferMultisample(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glRenderbufferStorageMultisampleEXT(GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height)
		{
		GLEXTFramebufferMultisample::current->glRenderbufferStorageMultisampleEXTProc(target,samples,internalformat,width,height);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

inline void glRenderbufferStorageMultisampleEXT(GLenum target,GLsizei samples,GLenum internalformat,const Misc::Size<2>& size)
	{
	glRenderbufferStorageMultisampleEXT(target,samples,internalformat,size[0],size[1]);
	}

#endif

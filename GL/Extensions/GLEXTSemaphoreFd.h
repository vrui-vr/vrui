/***********************************************************************
GLEXTSemaphoreFd - OpenGL extension class for the GL_EXT_semaphore_fd
extension.
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

#ifndef GLEXTENSIONS_GLEXTSEMAPHOREFD_INCLUDED
#define GLEXTENSIONS_GLEXTSEMAPHOREFD_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_semaphore_fd
#define GL_EXT_semaphore_fd 1

/* Extension-specific functions: */
typedef void (APIENTRY* PFNGLIMPORTSEMAPHOREFDEXTPROC)(GLuint semaphore,GLenum handleType,GLint fd);

/* Extension-specific constants: */

#endif

/* Forward declarations of friend functions: */
void glImportSemaphoreFdEXT(GLuint semaphore,GLenum handleType,GLint fd);

class GLEXTSemaphoreFd:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTSemaphoreFd*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLIMPORTSEMAPHOREFDEXTPROC glImportSemaphoreFdEXTProc;
	
	/* Constructors and destructors: */
	private:
	GLEXTSemaphoreFd(void);
	public:
	virtual ~GLEXTSemaphoreFd(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glImportSemaphoreFdEXT(GLuint semaphore,GLenum handleType,GLint fd)
		{
		GLEXTSemaphoreFd::current->glImportSemaphoreFdEXTProc(semaphore,handleType,fd);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif

/***********************************************************************
GLARBDepthClamp - OpenGL extension class for the GL_ARB_depth_clamp
extension.
Copyright (c) 2023 Oliver Kreylos

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

#ifndef GLEXTENSIONS_GLARBDEPTHCLAMP_INCLUDED
#define GLEXTENSIONS_GLARBDEPTHCLAMP_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_ARB_depth_clamp
#define GL_ARB_depth_clamp 1

/* Extension-specific constants: */
#define GL_DEPTH_CLAMP 0x864F

#endif

class GLARBDepthClamp:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLARBDepthClamp*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	
	/* Constructors and destructors: */
	private:
	GLARBDepthClamp(void);
	public:
	virtual ~GLARBDepthClamp(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif

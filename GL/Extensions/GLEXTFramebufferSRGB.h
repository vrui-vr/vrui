/***********************************************************************
GLEXTFramebufferSRGB - OpenGL extension class for the
GL_EXT_framebuffer_sRGB extension.
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

#ifndef GLEXTENSIONS_GLEXTFRAMEBUFFERSRGB_INCLUDED
#define GLEXTENSIONS_GLEXTFRAMEBUFFERSRGB_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/*********************************
Extension-specific parts of glx.h:
*********************************/

#ifndef GLX_EXT_framebuffer_sRGB
#define GLX_EXT_framebuffer_sRGB 1

/* Extension-specific constants: */
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT  0x20B2

#endif

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_framebuffer_sRGB
#define GL_EXT_framebuffer_sRGB 1

/* Extension-specific constants: */
#define GL_FRAMEBUFFER_SRGB_EXT         0x8DB9
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x8DBA

#endif

/* Forward declarations of friend functions: */

class GLEXTFramebufferSRGB:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTFramebufferSRGB*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	
	/* Constructors and destructors: */
	private:
	GLEXTFramebufferSRGB(void);
	public:
	virtual ~GLEXTFramebufferSRGB(void);
	
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

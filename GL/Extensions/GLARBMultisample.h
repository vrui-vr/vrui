/***********************************************************************
GLARBMultisample - OpenGL extension class for the GL_ARB_multisample
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

#ifndef GLEXTENSIONS_GLARBMULTISAMPLE_INCLUDED
#define GLEXTENSIONS_GLARBMULTISAMPLE_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/*********************************
Extension-specific parts of glx.h:
*********************************/

#ifndef GLX_ARB_multisample
#define GLX_ARB_multisample 1

#define GLX_SAMPLE_BUFFERS_ARB 100000
#define GLX_SAMPLES_ARB        100001

#endif

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_ARB_multisample
#define GL_ARB_multisample 1

/* Extension-specific functions: */
typedef void (APIENTRY* PFNGLSAMPLECOVERAGEARBPROC)(GLfloat value,GLboolean invert);

/* Extension-specific constants: */

#define GL_MULTISAMPLE_ARB              0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB      0x809F
#define GL_SAMPLE_COVERAGE_ARB          0x80A0
#define GL_SAMPLE_BUFFERS_ARB           0x80A8
#define GL_SAMPLES_ARB                  0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB    0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB   0x80AB
#define GL_MULTISAMPLE_BIT_ARB          0x20000000

#endif

/* Forward declarations of friend functions: */
void glSampleCoverageARB(GLfloat value,GLboolean invert);

class GLARBMultisample:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLARBMultisample*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLSAMPLECOVERAGEARBPROC glSampleCoverageARBProc;
	
	/* Constructors and destructors: */
	private:
	GLARBMultisample(void);
	public:
	virtual ~GLARBMultisample(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glSampleCoverageARB(GLfloat value,GLboolean invert)
		{
		GLARBMultisample::current->glSampleCoverageARBProc(value,invert);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif

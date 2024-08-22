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

#include <GL/Extensions/GLARBMultisample.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

/*****************************************
Static elements of class GLARBMultisample:
*****************************************/

GL_THREAD_LOCAL(GLARBMultisample*) GLARBMultisample::current=0;
const char* GLARBMultisample::name="GL_ARB_multisample";

/*********************************
Methods of class GLARBMultisample:
*********************************/

GLARBMultisample::GLARBMultisample(void)
	:glSampleCoverageARBProc(GLExtensionManager::getFunction<PFNGLSAMPLECOVERAGEARBPROC>("glSampleCoverageARB"))
	{
	}

GLARBMultisample::~GLARBMultisample(void)
	{
	}

const char* GLARBMultisample::getExtensionName(void) const
	{
	return name;
	}

void GLARBMultisample::activate(void)
	{
	current=this;
	}

void GLARBMultisample::deactivate(void)
	{
	current=0;
	}

bool GLARBMultisample::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLARBMultisample::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLARBMultisample* newExtension=new GLARBMultisample;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}

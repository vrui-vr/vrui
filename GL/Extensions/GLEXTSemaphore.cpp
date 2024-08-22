/***********************************************************************
GLEXTSemaphore - OpenGL extension class for the GL_EXT_semaphore
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

#include <GL/Extensions/GLEXTSemaphore.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

/***************************************
Static elements of class GLEXTSemaphore:
***************************************/

GL_THREAD_LOCAL(GLEXTSemaphore*) GLEXTSemaphore::current=0;
const char* GLEXTSemaphore::name="GL_EXT_semaphore";

/*******************************
Methods of class GLEXTSemaphore:
*******************************/

GLEXTSemaphore::GLEXTSemaphore(void)
	:glGenSemaphoresEXTProc(GLExtensionManager::getFunction<PFNGLGENSEMAPHORESEXTPROC>("glGenSemaphoresEXT")),
	 glDeleteSemaphoresEXTProc(GLExtensionManager::getFunction<PFNGLDELETESEMAPHORESEXTPROC>("glDeleteSemaphoresEXT")),
	 glIsSemaphoreEXTProc(GLExtensionManager::getFunction<PFNGLISSEMAPHOREEXTPROC>("glIsSemaphoreEXT")),
	 glSemaphoreParameterui64vEXTProc(GLExtensionManager::getFunction<PFNGLSEMAPHOREPARAMETERUI64VEXTPROC>("glSemaphoreParameterui64vEXT")),
	 glGetSemaphoreParameterui64vEXTProc(GLExtensionManager::getFunction<PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC>("glGetSemaphoreParameterui64vEXT")),
	 glWaitSemaphoreEXTProc(GLExtensionManager::getFunction<PFNGLWAITSEMAPHOREEXTPROC>("glWaitSemaphoreEXT")),
	 glSignalSemaphoreEXTProc(GLExtensionManager::getFunction<PFNGLSIGNALSEMAPHOREEXTPROC>("glSignalSemaphoreEXT"))
	{
	}

GLEXTSemaphore::~GLEXTSemaphore(void)
	{
	}

const char* GLEXTSemaphore::getExtensionName(void) const
	{
	return name;
	}

void GLEXTSemaphore::activate(void)
	{
	current=this;
	}

void GLEXTSemaphore::deactivate(void)
	{
	current=0;
	}

bool GLEXTSemaphore::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLEXTSemaphore::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLEXTSemaphore* newExtension=new GLEXTSemaphore;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}

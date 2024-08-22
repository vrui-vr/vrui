/***********************************************************************
GLARBSync - OpenGL extension class for the GL_ARB_sync extension.
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

#include <GL/Extensions/GLARBSync.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

/**********************************
Static elements of class GLARBSync:
**********************************/

GL_THREAD_LOCAL(GLARBSync*) GLARBSync::current=0;
const char* GLARBSync::name="GL_ARB_sync";

/**************************
Methods of class GLARBSync:
**************************/

GLARBSync::GLARBSync(void)
	:glFenceSyncProc(GLExtensionManager::getFunction<PFNGLFENCESYNCPROC>("glFenceSync")),
	 glIsSyncProc(GLExtensionManager::getFunction<PFNGLISSYNCPROC>("glIsSync")),
	 glDeleteSyncProc(GLExtensionManager::getFunction<PFNGLDELETESYNCPROC>("glDeleteSync")),
	 glClientWaitSyncProc(GLExtensionManager::getFunction<PFNGLCLIENTWAITSYNCPROC>("glClientWaitSync")),
	 glWaitSyncProc(GLExtensionManager::getFunction<PFNGLWAITSYNCPROC>("glWaitSync")),
	 glGetInteger64vProc(GLExtensionManager::getFunction<PFNGLGETINTEGER64VPROC>("glGetInteger64v")),
	 glGetSyncivProc(GLExtensionManager::getFunction<PFNGLGETSYNCIVPROC>("glGetSynciv")),
	 glGetInteger64i_vProc(GLExtensionManager::getFunction<PFNGLGETINTEGER64I_VPROC>("glGetInteger64i_v"))
	{
	}

GLARBSync::~GLARBSync(void)
	{
	}

const char* GLARBSync::getExtensionName(void) const
	{
	return name;
	}

void GLARBSync::activate(void)
	{
	current=this;
	}

void GLARBSync::deactivate(void)
	{
	current=0;
	}

bool GLARBSync::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLARBSync::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLARBSync* newExtension=new GLARBSync;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}

/***********************************************************************
GLNVVdpauInterop - OpenGL extension class for the GL_NV_vdpau_interop
extension.
Copyright (c) 2025 Oliver Kreylos

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

#include <GL/Extensions/GLNVVdpauInterop.h>

#include <GL/gl.h>
#include <GL/GLExtensionManager.h>

/*****************************************
Static elements of class GLNVVdpauInterop:
*****************************************/

GL_THREAD_LOCAL(GLNVVdpauInterop*) GLNVVdpauInterop::current=0;
const char* GLNVVdpauInterop::name="GL_NV_vdpau_interop";

/*********************************
Methods of class GLNVVdpauInterop:
*********************************/

GLNVVdpauInterop::GLNVVdpauInterop(void)
	{
	/* Get all function pointers: */
	glVDPAUInitNVProc=GLExtensionManager::getFunction<PFNGLVDPAUINITNVPROC>("glVDPAUInitNV");
	glVDPAUFiniNVProc=GLExtensionManager::getFunction<PFNGLVDPAUFININVPROC>("glVDPAUFiniNV");
	glVDPAURegisterVideoSurfaceNVProc=GLExtensionManager::getFunction<PFNGLVDPAUREGISTERVIDEOSURFACENVPROC>("glVDPAURegisterVideoSurfaceNV");
	glVDPAURegisterOutputSurfaceNVProc=GLExtensionManager::getFunction<PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC>("glVDPAURegisterOutputSurfaceNV");
	glVDPAUIsSurfaceNVProc=GLExtensionManager::getFunction<PFNGLVDPAUISSURFACENVPROC>("glVDPAUIsSurfaceNV");
	glVDPAUUnregisterSurfaceNVProc=GLExtensionManager::getFunction<PFNGLVDPAUUNREGISTERSURFACENVPROC>("glVDPAUUnregisterSurfaceNV");
	glVDPAUGetSurfaceivNVProc=GLExtensionManager::getFunction<PFNGLVDPAUGETSURFACEIVNVPROC>("glVDPAUGetSurfaceivNV");
	glVDPAUSurfaceAccessNVProc=GLExtensionManager::getFunction<PFNGLVDPAUSURFACEACCESSNVPROC>("glVDPAUSurfaceAccessNV");
	glVDPAUMapSurfacesNVProc=GLExtensionManager::getFunction<PFNGLVDPAUMAPSURFACESNVPROC>("glVDPAUMapSurfacesNV");
	glVDPAUUnmapSurfacesNVProc=GLExtensionManager::getFunction<PFNGLVDPAUUNMAPSURFACESNVPROC>("glVDPAUUnmapSurfacesNV");
	}

GLNVVdpauInterop::~GLNVVdpauInterop(void)
	{
	}

const char* GLNVVdpauInterop::getExtensionName(void) const
	{
	return name;
	}

void GLNVVdpauInterop::activate(void)
	{
	current=this;
	}

void GLNVVdpauInterop::deactivate(void)
	{
	current=0;
	}

bool GLNVVdpauInterop::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLNVVdpauInterop::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLNVVdpauInterop* newExtension=new GLNVVdpauInterop;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}

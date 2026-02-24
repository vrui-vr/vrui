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

#ifndef GLEXTENSIONS_GLNVVDPAUINTEROP_INCLUDED
#define GLEXTENSIONS_GLNVVDPAUINTEROP_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_NV_vdpau_interop
#define GL_NV_vdpau_interop 1

/* Extension-specific types: */
typedef GLintptr GLvdpauSurfaceNV;

/* Extension-specific functions: */
typedef void (APIENTRY * PFNGLVDPAUINITNVPROC)(const void*,const void*);
typedef void (APIENTRY * PFNGLVDPAUFININVPROC)(void);
typedef GLvdpauSurfaceNV (APIENTRY * PFNGLVDPAUREGISTERVIDEOSURFACENVPROC)(const void*,GLenum,GLsizei,const GLuint*);
typedef GLvdpauSurfaceNV (APIENTRY * PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC)(const void*,GLenum,GLsizei,const GLuint*);
typedef GLboolean (APIENTRY * PFNGLVDPAUISSURFACENVPROC)(GLvdpauSurfaceNV);
typedef void (APIENTRY * PFNGLVDPAUUNREGISTERSURFACENVPROC)(GLvdpauSurfaceNV);
typedef void (APIENTRY * PFNGLVDPAUGETSURFACEIVNVPROC)(GLvdpauSurfaceNV,GLenum,GLsizei,GLsizei*,GLint*);
typedef void (APIENTRY * PFNGLVDPAUSURFACEACCESSNVPROC)(GLvdpauSurfaceNV,GLenum);
typedef void (APIENTRY * PFNGLVDPAUMAPSURFACESNVPROC)(GLsizei,const GLvdpauSurfaceNV*);
typedef void (APIENTRY * PFNGLVDPAUUNMAPSURFACESNVPROC)(GLsizei,const GLvdpauSurfaceNV*);

/* Extension-specific constants: */
#define GL_SURFACE_STATE_NV      0x86EB
#define GL_SURFACE_REGISTERED_NV 0x86FD
#define SURFACE_MAPPED_NV        0x8700
#define WRITE_DISCARD_NV         0x88BE

#endif

/* Forward declarations of friend functions: */
void glVDPAUInitNV(const void* vdpDevice,const void* getProcAddress);
void glVDPAUFiniNV(void);
GLvdpauSurfaceNV glVDPAURegisterVideoSurfaceNV(const void* vdpSurface,GLenum target,GLsizei numTextureNames,const GLuint* textureNames);
GLvdpauSurfaceNV glVDPAURegisterOutputSurfaceNV(const void* vdpSurface,GLenum target,GLsizei numTextureNames,const GLuint *textureNames);
GLboolean glVDPAUIsSurfaceNV(GLvdpauSurfaceNV surface);
void glVDPAUUnregisterSurfaceNV(GLvdpauSurfaceNV surface);
void glVDPAUGetSurfaceivNV(GLvdpauSurfaceNV surface,GLenum pname,GLsizei bufSize,GLsizei* length,GLint* values);
void glVDPAUSurfaceAccessNV(GLvdpauSurfaceNV surface,GLenum access);
void glVDPAUMapSurfacesNV(GLsizei numSurfaces,const GLvdpauSurfaceNV* surfaces);
void glVDPAUUnmapSurfacesNV(GLsizei numSurface,const GLvdpauSurfaceNV* surfaces);

class GLNVVdpauInterop:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLNVVdpauInterop*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLVDPAUINITNVPROC glVDPAUInitNVProc;
	PFNGLVDPAUFININVPROC glVDPAUFiniNVProc;
	PFNGLVDPAUREGISTERVIDEOSURFACENVPROC glVDPAURegisterVideoSurfaceNVProc;
	PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC glVDPAURegisterOutputSurfaceNVProc;
	PFNGLVDPAUISSURFACENVPROC glVDPAUIsSurfaceNVProc;
	PFNGLVDPAUUNREGISTERSURFACENVPROC glVDPAUUnregisterSurfaceNVProc;
	PFNGLVDPAUGETSURFACEIVNVPROC glVDPAUGetSurfaceivNVProc;
	PFNGLVDPAUSURFACEACCESSNVPROC glVDPAUSurfaceAccessNVProc;
	PFNGLVDPAUMAPSURFACESNVPROC glVDPAUMapSurfacesNVProc;
	PFNGLVDPAUUNMAPSURFACESNVPROC glVDPAUUnmapSurfacesNVProc;
	
	/* Constructors and destructors: */
	private:
	GLNVVdpauInterop(void);
	public:
	virtual ~GLNVVdpauInterop(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void);
	static void initExtension(void);
	
	/* Extension entry points: */
	inline friend void glVDPAUInitNV(const void* vdpDevice,const void* getProcAddress)
		{
		GLNVVdpauInterop::current->glVDPAUInitNVProc(vdpDevice,getProcAddress);
		}
	inline friend void glVDPAUFiniNV(void)
		{
		GLNVVdpauInterop::current->glVDPAUFiniNVProc();
		}
	inline friend GLvdpauSurfaceNV glVDPAURegisterVideoSurfaceNV(const void* vdpSurface,GLenum target,GLsizei numTextureNames,const GLuint* textureNames)
		{
		return GLNVVdpauInterop::current->glVDPAURegisterVideoSurfaceNVProc(vdpSurface,target,numTextureNames,textureNames);
		}
	inline friend GLvdpauSurfaceNV glVDPAURegisterOutputSurfaceNV(const void* vdpSurface,GLenum target,GLsizei numTextureNames,const GLuint *textureNames)
		{
		return GLNVVdpauInterop::current->glVDPAURegisterOutputSurfaceNVProc(vdpSurface,target,numTextureNames,textureNames);
		}
	inline friend GLboolean glVDPAUIsSurfaceNV(GLvdpauSurfaceNV surface)
		{
		return GLNVVdpauInterop::current->glVDPAUIsSurfaceNVProc(surface);
		}
	inline friend void glVDPAUUnregisterSurfaceNV(GLvdpauSurfaceNV surface)
		{
		GLNVVdpauInterop::current->glVDPAUUnregisterSurfaceNVProc(surface);
		}
	inline friend void glVDPAUGetSurfaceivNV(GLvdpauSurfaceNV surface,GLenum pname,GLsizei bufSize,GLsizei* length,GLint* values)
		{
		GLNVVdpauInterop::current->glVDPAUGetSurfaceivNVProc(surface,pname,bufSize,length,values);
		}
	inline friend void glVDPAUSurfaceAccessNV(GLvdpauSurfaceNV surface,GLenum access)
		{
		GLNVVdpauInterop::current->glVDPAUSurfaceAccessNVProc(surface,access);
		}
	inline friend void glVDPAUMapSurfacesNV(GLsizei numSurfaces,const GLvdpauSurfaceNV* surfaces)
		{
		GLNVVdpauInterop::current->glVDPAUMapSurfacesNVProc(numSurfaces,surfaces);
		}
	inline friend void glVDPAUUnmapSurfacesNV(GLsizei numSurface,const GLvdpauSurfaceNV* surfaces)
		{
		GLNVVdpauInterop::current->glVDPAUUnmapSurfacesNVProc(numSurface,surfaces);
		}
	};

#endif

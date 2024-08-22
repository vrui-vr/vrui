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

#ifndef GLEXTENSIONS_GLEXTSEMAPHORE_INCLUDED
#define GLEXTENSIONS_GLEXTSEMAPHORE_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_semaphore
#define GL_EXT_semaphore 1

/* Extension-specific functions: */
typedef void (APIENTRY* PFNGLGENSEMAPHORESEXTPROC)(GLsizei n,GLuint* semaphores);
typedef void (APIENTRY* PFNGLDELETESEMAPHORESEXTPROC)(GLsizei n,const GLuint* semaphores);
typedef GLboolean (APIENTRY* PFNGLISSEMAPHOREEXTPROC)(GLuint semaphore);
typedef void (APIENTRY* PFNGLSEMAPHOREPARAMETERUI64VEXTPROC)(GLuint semaphore,GLenum pname,const GLuint64* params);
typedef void (APIENTRY* PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC)(GLuint semaphore,GLenum pname,GLuint64* params);
typedef void (APIENTRY* PFNGLWAITSEMAPHOREEXTPROC)(GLuint semaphore,GLuint numBufferBarriers,const GLuint* buffers,GLuint numTextureBarriers,const GLuint* textures,const GLenum* srcLayouts);
typedef void (APIENTRY* PFNGLSIGNALSEMAPHOREEXTPROC)(GLuint semaphore,GLuint numBufferBarriers,const GLuint* buffers,GLuint numTextureBarriers,const GLuint* textures,const GLenum* dstLayouts);

/* Extension-specific constants: */
#define GL_LAYOUT_GENERAL_EXT                            0x958D
#define GL_LAYOUT_COLOR_ATTACHMENT_EXT                   0x958E
#define GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT           0x958F
#define GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT            0x9590
#define GL_LAYOUT_SHADER_READ_ONLY_EXT                   0x9591
#define GL_LAYOUT_TRANSFER_SRC_EXT                       0x9592
#define GL_LAYOUT_TRANSFER_DST_EXT                       0x9593
#define GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT 0x9530
#define GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT 0x9531

#endif

/* Forward declarations of friend functions: */
void glGenSemaphoresEXT(GLsizei n,GLuint* semaphores);
void glDeleteSemaphoresEXT(GLsizei n,const GLuint* semaphores);
GLboolean glIsSemaphoreEXT(GLuint semaphore);
void glSemaphoreParameterui64vEXT(GLuint semaphore,GLenum pname,const GLuint64* params);
void glGetSemaphoreParameterui64vEXT(GLuint semaphore,GLenum pname,GLuint64* params);
void glWaitSemaphoreEXT(GLuint semaphore,GLuint numBufferBarriers,const GLuint* buffers,GLuint numTextureBarriers,const GLuint* textures,const GLenum* srcLayouts);
void glSignalSemaphoreEXT(GLuint semaphore,GLuint numBufferBarriers,const GLuint* buffers,GLuint numTextureBarriers,const GLuint* textures,const GLenum* dstLayouts);

class GLEXTSemaphore:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTSemaphore*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLGENSEMAPHORESEXTPROC glGenSemaphoresEXTProc;
	PFNGLDELETESEMAPHORESEXTPROC glDeleteSemaphoresEXTProc;
	PFNGLISSEMAPHOREEXTPROC glIsSemaphoreEXTProc;
	PFNGLSEMAPHOREPARAMETERUI64VEXTPROC glSemaphoreParameterui64vEXTProc;
	PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC glGetSemaphoreParameterui64vEXTProc;
	PFNGLWAITSEMAPHOREEXTPROC glWaitSemaphoreEXTProc;
	PFNGLSIGNALSEMAPHOREEXTPROC glSignalSemaphoreEXTProc;
	
	/* Constructors and destructors: */
	private:
	GLEXTSemaphore(void);
	public:
	virtual ~GLEXTSemaphore(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glGenSemaphoresEXT(GLsizei n,GLuint* semaphores)
		{
		GLEXTSemaphore::current->glGenSemaphoresEXTProc(n,semaphores);
		}
	inline friend void glDeleteSemaphoresEXT(GLsizei n,const GLuint* semaphores)
		{
		GLEXTSemaphore::current->glDeleteSemaphoresEXTProc(n,semaphores);
		}
	inline friend GLboolean glIsSemaphoreEXT(GLuint semaphore)
		{
		return GLEXTSemaphore::current->glIsSemaphoreEXTProc(semaphore);
		}
	inline friend void glSemaphoreParameterui64vEXT(GLuint semaphore,GLenum pname,const GLuint64* params)
		{
		GLEXTSemaphore::current->glSemaphoreParameterui64vEXTProc(semaphore,pname,params);
		}
	inline friend void glGetSemaphoreParameterui64vEXT(GLuint semaphore,GLenum pname,GLuint64* params)
		{
		GLEXTSemaphore::current->glGetSemaphoreParameterui64vEXTProc(semaphore,pname,params);
		}
	inline friend void glWaitSemaphoreEXT(GLuint semaphore,GLuint numBufferBarriers,const GLuint* buffers,GLuint numTextureBarriers,const GLuint* textures,const GLenum* srcLayouts)
		{
		GLEXTSemaphore::current->glWaitSemaphoreEXTProc(semaphore,numBufferBarriers,buffers,numTextureBarriers,textures,srcLayouts);
		}
	inline friend void glSignalSemaphoreEXT(GLuint semaphore,GLuint numBufferBarriers,const GLuint* buffers,GLuint numTextureBarriers,const GLuint* textures,const GLenum* dstLayouts)
		{
		GLEXTSemaphore::current->glSignalSemaphoreEXTProc(semaphore,numBufferBarriers,buffers,numTextureBarriers,textures,dstLayouts);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif

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

#ifndef GLEXTENSIONS_GLARBSYNC_INCLUDED
#define GLEXTENSIONS_GLARBSYNC_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_ARB_sync
#define GL_ARB_sync 1

/* Extension-specific types: */
#include <inttypes.h>
typedef int64_t GLint64;
typedef uint64_t GLuint64;
typedef struct __GLsync* GLsync;

/* Extension-specific functions: */
typedef GLsync (APIENTRY* PFNGLFENCESYNCPROC)(GLenum condition,GLbitfield flags);
typedef GLboolean (APIENTRY* PFNGLISSYNCPROC)(GLsync sync);
typedef void (APIENTRY* PFNGLDELETESYNCPROC)(GLsync sync);
typedef GLenum (APIENTRY* PFNGLCLIENTWAITSYNCPROC)(GLsync sync,GLbitfield flags,GLuint64 timeout);
typedef void (APIENTRY* PFNGLWAITSYNCPROC)(GLsync sync,GLbitfield flags,GLuint64 timeout);
typedef void (APIENTRY* PFNGLGETINTEGER64VPROC)(GLenum pname,GLint64* data);
typedef void (APIENTRY* PFNGLGETSYNCIVPROC)(GLsync sync,GLenum pname,GLsizei bufSize,GLsizei* length,GLint* values);
typedef void (APIENTRY* PFNGLGETINTEGER64I_VPROC)(GLenum target,GLuint index,GLint64* data);

/* Extension-specific constants: */
#define GL_MAX_SERVER_WAIT_TIMEOUT    0x9111
#define GL_OBJECT_TYPE                0x9112
#define GL_SYNC_CONDITION             0x9113
#define GL_SYNC_STATUS                0x9114
#define GL_SYNC_FLAGS                 0x9115
#define GL_SYNC_FENCE                 0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_UNSIGNALED                 0x9118
#define GL_SIGNALED                   0x9119
#define GL_ALREADY_SIGNALED           0x911A
#define GL_TIMEOUT_EXPIRED            0x911B
#define GL_CONDITION_SATISFIED        0x911C
#define GL_WAIT_FAILED                0x911D
#define GL_TIMEOUT_IGNORED            0xFFFFFFFFFFFFFFFFull
#define GL_SYNC_FLUSH_COMMANDS_BIT    0x00000001

#endif

/* Forward declarations of friend functions: */
GLsync glFenceSync(GLenum condition,GLbitfield flags);
GLboolean glIsSync(GLsync sync);
void glDeleteSync(GLsync sync);
GLenum glClientWaitSync(GLsync sync,GLbitfield flags,GLuint64 timeout);
void glWaitSync(GLsync sync,GLbitfield flags,GLuint64 timeout);
void glGetInteger64v(GLenum pname,GLint64* data);
void glGetSynciv(GLsync sync,GLenum pname,GLsizei bufSize,GLsizei* length,GLint* values);
void glGetInteger64i_v(GLenum target,GLuint index,GLint64* data);

class GLARBSync:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLARBSync*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLFENCESYNCPROC glFenceSyncProc;
	PFNGLISSYNCPROC glIsSyncProc;
	PFNGLDELETESYNCPROC glDeleteSyncProc;
	PFNGLCLIENTWAITSYNCPROC glClientWaitSyncProc;
	PFNGLWAITSYNCPROC glWaitSyncProc;
	PFNGLGETINTEGER64VPROC glGetInteger64vProc;
	PFNGLGETSYNCIVPROC glGetSyncivProc;
	PFNGLGETINTEGER64I_VPROC glGetInteger64i_vProc;
	
	/* Constructors and destructors: */
	private:
	GLARBSync(void);
	public:
	virtual ~GLARBSync(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend GLsync glFenceSync(GLenum condition,GLbitfield flags)
		{
		return GLARBSync::current->glFenceSyncProc(condition,flags);
		}
	inline friend GLboolean glIsSync(GLsync sync)
		{
		return GLARBSync::current->glIsSyncProc(sync);
		}
	inline friend void glDeleteSync(GLsync sync)
		{
		GLARBSync::current->glDeleteSyncProc(sync);
		}
	inline friend GLenum glClientWaitSync(GLsync sync,GLbitfield flags,GLuint64 timeout)
		{
		return GLARBSync::current->glClientWaitSyncProc(sync,flags,timeout);
		}
	inline friend void glWaitSync(GLsync sync,GLbitfield flags,GLuint64 timeout)
		{
		GLARBSync::current->glWaitSyncProc(sync,flags,timeout);
		}
	inline friend void glGetInteger64v(GLenum pname,GLint64* data)
		{
		GLARBSync::current->glGetInteger64vProc(pname,data);
		}
	inline friend void glGetSynciv(GLsync sync,GLenum pname,GLsizei bufSize,GLsizei* length,GLint* values)
		{
		GLARBSync::current->glGetSyncivProc(sync,pname,bufSize,length,values);
		}
	inline friend void glGetInteger64i_v(GLenum target,GLuint index,GLint64* data)
		{
		GLARBSync::current->glGetInteger64i_vProc(target,index,data);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif

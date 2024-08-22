/***********************************************************************
GLEXTMemoryObject - OpenGL extension class for the GL_EXT_memory_object
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

#ifndef GLEXTENSIONS_GLEXTMEMORYOBJECT_INCLUDED
#define GLEXTENSIONS_GLEXTMEMORYOBJECT_INCLUDED

#include <Misc/Size.h>
#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_memory_object
#define GL_EXT_memory_object 1

/* Extension-specific functions: */
typedef void (APIENTRY* PFNGLGETUNSIGNEDBYTEVEXTPROC)(GLenum pname,GLubyte* data);
typedef void (APIENTRY* PFNGLGETUNSIGNEDBYTEI_VEXTPROC)(GLenum target,GLuint index,GLubyte* data);
typedef void (APIENTRY* PFNGLDELETEMEMORYOBJECTSEXTPROC)(GLsizei n,const GLuint* memoryObjects);
typedef GLboolean (APIENTRY* PFNGLISMEMORYOBJECTEXTPROC)(GLuint memoryObject);
typedef void (APIENTRY* PFNGLCREATEMEMORYOBJECTSEXTPROC)(GLsizei n,GLuint* memoryObjects);
typedef void (APIENTRY* PFNGLMEMORYOBJECTPARAMETERIVEXTPROC)(GLuint memoryObject,GLenum pname,const GLint* params);
typedef void (APIENTRY* PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC)(GLuint memoryObject,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLTEXSTORAGEMEM2DEXTPROC)(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC)(GLenum target,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXSTORAGEMEM3DEXTPROC)(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC)(GLenum target,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLBUFFERSTORAGEMEMEXTPROC)(GLenum target,GLsizeiptr size,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXTURESTORAGEMEM2DEXTPROC)(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC)(GLuint texture,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXTURESTORAGEMEM3DEXTPROC)(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC)(GLuint texture,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC)(GLuint buffer,GLsizeiptr size,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXSTORAGEMEM1DEXTPROC)(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLuint memory,GLuint64 offset);
typedef void (APIENTRY* PFNGLTEXTURESTORAGEMEM1DEXTPROC)(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLuint memory,GLuint64 offset);

/* Extension-specific constants: */
#define GL_TEXTURE_TILING_EXT          0x9580
#define GL_DEDICATED_MEMORY_OBJECT_EXT 0x9581
#define GL_PROTECTED_MEMORY_OBJECT_EXT 0x959B
#define GL_NUM_TILING_TYPES_EXT        0x9582
#define GL_TILING_TYPES_EXT            0x9583
#define GL_OPTIMAL_TILING_EXT          0x9584
#define GL_LINEAR_TILING_EXT           0x9585
#define GL_NUM_DEVICE_UUIDS_EXT        0x9596
#define GL_DEVICE_UUID_EXT             0x9597
#define GL_DRIVER_UUID_EXT             0x9598
#define GL_UUID_SIZE_EXT               16

#endif

/* Forward declarations of friend functions: */
void glGetUnsignedBytevEXT(GLenum pname,GLubyte* data);
void glGetUnsignedBytei_vEXT(GLenum target,GLuint index,GLubyte* data);
void glDeleteMemoryObjectsEXT(GLsizei n,const GLuint* memoryObjects);
GLboolean glIsMemoryObjectEXT(GLuint memoryObject);
void glCreateMemoryObjectsEXT(GLsizei n,GLuint* memoryObjects);
void glMemoryObjectParameterivEXT(GLuint memoryObject,GLenum pname,const GLint* params);
void glGetMemoryObjectParameterivEXT(GLuint memoryObject,GLenum pname,GLint* params);
void glTexStorageMem2DEXT(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLuint memory,GLuint64 offset);
void glTexStorageMem2DMultisampleEXT(GLenum target,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
void glTexStorageMem3DEXT(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLuint memory,GLuint64 offset);
void glTexStorageMem3DMultisampleEXT(GLenum target,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
void glBufferStorageMemEXT(GLenum target,GLsizeiptr size,GLuint memory,GLuint64 offset);
void glTextureStorageMem2DEXT(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLuint memory,GLuint64 offset);
void glTextureStorageMem2DMultisampleEXT(GLuint texture,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
void glTextureStorageMem3DEXT(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLuint memory,GLuint64 offset);
void glTextureStorageMem3DMultisampleEXT(GLuint texture,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset);
void glNamedBufferStorageMemEXT(GLuint buffer,GLsizeiptr size,GLuint memory,GLuint64 offset);
void glTexStorageMem1DEXT(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLuint memory,GLuint64 offset);
void glTextureStorageMem1DEXT(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLuint memory,GLuint64 offset);

class GLEXTMemoryObject:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTMemoryObject*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLGETUNSIGNEDBYTEVEXTPROC glGetUnsignedBytevEXTProc;
	PFNGLGETUNSIGNEDBYTEI_VEXTPROC glGetUnsignedBytei_vEXTProc;
	PFNGLDELETEMEMORYOBJECTSEXTPROC glDeleteMemoryObjectsEXTProc;
	PFNGLISMEMORYOBJECTEXTPROC glIsMemoryObjectEXTProc;
	PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXTProc;
	PFNGLMEMORYOBJECTPARAMETERIVEXTPROC glMemoryObjectParameterivEXTProc;
	PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC glGetMemoryObjectParameterivEXTProc;
	PFNGLTEXSTORAGEMEM2DEXTPROC glTexStorageMem2DEXTProc;
	PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC glTexStorageMem2DMultisampleEXTProc;
	PFNGLTEXSTORAGEMEM3DEXTPROC glTexStorageMem3DEXTProc;
	PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC glTexStorageMem3DMultisampleEXTProc;
	PFNGLBUFFERSTORAGEMEMEXTPROC glBufferStorageMemEXTProc;
	PFNGLTEXTURESTORAGEMEM2DEXTPROC glTextureStorageMem2DEXTProc;
	PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC glTextureStorageMem2DMultisampleEXTProc;
	PFNGLTEXTURESTORAGEMEM3DEXTPROC glTextureStorageMem3DEXTProc;
	PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC glTextureStorageMem3DMultisampleEXTProc;
	PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC glNamedBufferStorageMemEXTProc;
	PFNGLTEXSTORAGEMEM1DEXTPROC glTexStorageMem1DEXTProc;
	PFNGLTEXTURESTORAGEMEM1DEXTPROC glTextureStorageMem1DEXTProc;
	
	/* Constructors and destructors: */
	private:
	GLEXTMemoryObject(void);
	public:
	virtual ~GLEXTMemoryObject(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glGetUnsignedBytevEXT(GLenum pname,GLubyte* data)
		{
		GLEXTMemoryObject::current->glGetUnsignedBytevEXTProc(pname,data);
		}
	inline friend void glGetUnsignedBytei_vEXT(GLenum target,GLuint index,GLubyte* data)
		{
		GLEXTMemoryObject::current->glGetUnsignedBytei_vEXTProc(target,index,data);
		}
	inline friend void glDeleteMemoryObjectsEXT(GLsizei n,const GLuint* memoryObjects)
		{
		GLEXTMemoryObject::current->glDeleteMemoryObjectsEXTProc(n,memoryObjects);
		}
	inline friend GLboolean glIsMemoryObjectEXT(GLuint memoryObject)
		{
		return GLEXTMemoryObject::current->glIsMemoryObjectEXTProc(memoryObject);
		}
	inline friend void glCreateMemoryObjectsEXT(GLsizei n,GLuint* memoryObjects)
		{
		GLEXTMemoryObject::current->glCreateMemoryObjectsEXTProc(n,memoryObjects);
		}
	inline friend void glMemoryObjectParameterivEXT(GLuint memoryObject,GLenum pname,const GLint* params)
		{
		GLEXTMemoryObject::current->glMemoryObjectParameterivEXTProc(memoryObject,pname,params);
		}
	inline friend void glGetMemoryObjectParameterivEXT(GLuint memoryObject,GLenum pname,GLint* params)
		{
		GLEXTMemoryObject::current->glGetMemoryObjectParameterivEXTProc(memoryObject,pname,params);
		}
	inline friend void glTexStorageMem2DEXT(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTexStorageMem2DEXTProc(target,levels,internalFormat,width,height,memory,offset);
		}
	inline friend void glTexStorageMem2DMultisampleEXT(GLenum target,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTexStorageMem2DMultisampleEXTProc(target,samples,internalFormat,width,height,fixedSampleLocations,memory,offset);
		}
	inline friend void glTexStorageMem3DEXT(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTexStorageMem3DEXTProc(target,levels,internalFormat,width,height,depth,memory,offset);
		}
	inline friend void glTexStorageMem3DMultisampleEXT(GLenum target,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTexStorageMem3DMultisampleEXTProc(target,samples,internalFormat,width,height,depth,fixedSampleLocations,memory,offset);
		}
	inline friend void glBufferStorageMemEXT(GLenum target,GLsizeiptr size,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glBufferStorageMemEXTProc(target,size,memory,offset);
		}
	inline friend void glTextureStorageMem2DEXT(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTextureStorageMem2DEXTProc(texture,levels,internalFormat,width,height,memory,offset);
		}
	inline friend void glTextureStorageMem2DMultisampleEXT(GLuint texture,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTextureStorageMem2DMultisampleEXTProc(texture,samples,internalFormat,width,height,fixedSampleLocations,memory,offset);
		}
	inline friend void glTextureStorageMem3DEXT(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTextureStorageMem3DEXTProc(texture,levels,internalFormat,width,height,depth,memory,offset);
		}
	inline friend void glTextureStorageMem3DMultisampleEXT(GLuint texture,GLsizei samples,GLenum internalFormat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTextureStorageMem3DMultisampleEXTProc(texture,samples,internalFormat,width,height,depth,fixedSampleLocations,memory,offset);
		}
	inline friend void glNamedBufferStorageMemEXT(GLuint buffer,GLsizeiptr size,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glNamedBufferStorageMemEXTProc(buffer,size,memory,offset);
		}
	inline friend void glTexStorageMem1DEXT(GLenum target,GLsizei levels,GLenum internalFormat,GLsizei width,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTexStorageMem1DEXTProc(target,levels,internalFormat,width,memory,offset);
		}
	inline friend void glTextureStorageMem1DEXT(GLuint texture,GLsizei levels,GLenum internalFormat,GLsizei width,GLuint memory,GLuint64 offset)
		{
		GLEXTMemoryObject::current->glTextureStorageMem1DEXTProc(texture,levels,internalFormat,width,memory,offset);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

inline void glTexStorageMem2DEXT(GLenum target,GLsizei levels,GLenum internalFormat,const Misc::Size<2>& size,GLuint memory,GLuint64 offset)
	{
	glTexStorageMem2DEXT(target,levels,internalFormat,size[0],size[1],memory,offset);
	}

inline void glTexStorageMem2DMultisampleEXT(GLenum target,GLsizei samples,GLenum internalFormat,const Misc::Size<2>& size,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset)
	{
	glTexStorageMem2DMultisampleEXT(target,samples,internalFormat,size[0],size[1],fixedSampleLocations,memory,offset);
	}

void glTextureStorageMem2DEXT(GLuint texture,GLsizei levels,GLenum internalFormat,const Misc::Size<2>& size,GLuint memory,GLuint64 offset)
	{
	glTextureStorageMem2DEXT(texture,levels,internalFormat,size[0],size[1],memory,offset);
	}

void glTextureStorageMem2DMultisampleEXT(GLuint texture,GLsizei samples,GLenum internalFormat,const Misc::Size<2>& size,GLboolean fixedSampleLocations,GLuint memory,GLuint64 offset)
	{
	glTextureStorageMem2DMultisampleEXT(texture,samples,internalFormat,size[0],size[1],fixedSampleLocations,memory,offset);
	}

#endif

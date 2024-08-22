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

#include <GL/Extensions/GLEXTMemoryObject.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

/******************************************
Static elements of class GLEXTMemoryObject:
******************************************/

GL_THREAD_LOCAL(GLEXTMemoryObject*) GLEXTMemoryObject::current=0;
const char* GLEXTMemoryObject::name="GL_EXT_memory_object";

/**********************************
Methods of class GLEXTMemoryObject:
**********************************/

GLEXTMemoryObject::GLEXTMemoryObject(void)
	:glGetUnsignedBytevEXTProc(GLExtensionManager::getFunction<PFNGLGETUNSIGNEDBYTEVEXTPROC>("glGetUnsignedBytevEXT")),
	 glGetUnsignedBytei_vEXTProc(GLExtensionManager::getFunction<PFNGLGETUNSIGNEDBYTEI_VEXTPROC>("glGetUnsignedBytei_vEXT")),
	 glDeleteMemoryObjectsEXTProc(GLExtensionManager::getFunction<PFNGLDELETEMEMORYOBJECTSEXTPROC>("glDeleteMemoryObjectsEXT")),
	 glIsMemoryObjectEXTProc(GLExtensionManager::getFunction<PFNGLISMEMORYOBJECTEXTPROC>("glIsMemoryObjectEXT")),
	 glCreateMemoryObjectsEXTProc(GLExtensionManager::getFunction<PFNGLCREATEMEMORYOBJECTSEXTPROC>("glCreateMemoryObjectsEXT")),
	 glMemoryObjectParameterivEXTProc(GLExtensionManager::getFunction<PFNGLMEMORYOBJECTPARAMETERIVEXTPROC>("glMemoryObjectParameterivEXT")),
	 glGetMemoryObjectParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC>("glGetMemoryObjectParameterivEXT")),
	 glTexStorageMem2DEXTProc(GLExtensionManager::getFunction<PFNGLTEXSTORAGEMEM2DEXTPROC>("glTexStorageMem2DEXT")),
	 glTexStorageMem2DMultisampleEXTProc(GLExtensionManager::getFunction<PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC>("glTexStorageMem2DMultisampleEXT")),
	 glTexStorageMem3DEXTProc(GLExtensionManager::getFunction<PFNGLTEXSTORAGEMEM3DEXTPROC>("glTexStorageMem3DEXT")),
	 glTexStorageMem3DMultisampleEXTProc(GLExtensionManager::getFunction<PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC>("glTexStorageMem3DMultisampleEXT")),
	 glBufferStorageMemEXTProc(GLExtensionManager::getFunction<PFNGLBUFFERSTORAGEMEMEXTPROC>("glBufferStorageMemEXT")),
	 glTextureStorageMem2DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGEMEM2DEXTPROC>("glTextureStorageMem2DEXT")),
	 glTextureStorageMem2DMultisampleEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC>("glTextureStorageMem2DMultisampleEXT")),
	 glTextureStorageMem3DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGEMEM3DEXTPROC>("glTextureStorageMem3DEXT")),
	 glTextureStorageMem3DMultisampleEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC>("glTextureStorageMem3DMultisampleEXT")),
	 glNamedBufferStorageMemEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC>("glNamedBufferStorageMemEXT")),
	 glTexStorageMem1DEXTProc(GLExtensionManager::getFunction<PFNGLTEXSTORAGEMEM1DEXTPROC>("glTexStorageMem1DEXT")),
	 glTextureStorageMem1DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGEMEM1DEXTPROC>("glTextureStorageMem1DEXT"))
	{
	}

GLEXTMemoryObject::~GLEXTMemoryObject(void)
	{
	}

const char* GLEXTMemoryObject::getExtensionName(void) const
	{
	return name;
	}

void GLEXTMemoryObject::activate(void)
	{
	current=this;
	}

void GLEXTMemoryObject::deactivate(void)
	{
	current=0;
	}

bool GLEXTMemoryObject::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLEXTMemoryObject::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLEXTMemoryObject* newExtension=new GLEXTMemoryObject;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}

/***********************************************************************
GLEXTFramebufferObject - OpenGL extension class for the
GL_EXT_framebuffer_object extension.
Copyright (c) 2007-2024 Oliver Kreylos

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

#include <GL/Extensions/GLEXTFramebufferObject.h>

#include <string>
#include <stdexcept>
#include <Misc/ParsePrettyFunction.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

/***********************************************
Static elements of class GLEXTFramebufferObject:
***********************************************/

GL_THREAD_LOCAL(GLEXTFramebufferObject*) GLEXTFramebufferObject::current=0;
const char* GLEXTFramebufferObject::name="GL_EXT_framebuffer_object";

/***************************************
Methods of class GLEXTFramebufferObject:
***************************************/

GLEXTFramebufferObject::GLEXTFramebufferObject(void)
	{
	/* Get all function pointers: */
	glIsRenderbufferEXTProc=GLExtensionManager::getFunction<PFNGLISRENDERBUFFEREXTPROC>("glIsRenderbufferEXT");
	glBindRenderbufferEXTProc=GLExtensionManager::getFunction<PFNGLBINDRENDERBUFFEREXTPROC>("glBindRenderbufferEXT");
	glDeleteRenderbuffersEXTProc=GLExtensionManager::getFunction<PFNGLDELETERENDERBUFFERSEXTPROC>("glDeleteRenderbuffersEXT");
	glGenRenderbuffersEXTProc=GLExtensionManager::getFunction<PFNGLGENRENDERBUFFERSEXTPROC>("glGenRenderbuffersEXT");
	glRenderbufferStorageEXTProc=GLExtensionManager::getFunction<PFNGLRENDERBUFFERSTORAGEEXTPROC>("glRenderbufferStorageEXT");
	glGetRenderbufferParameterivEXTProc=GLExtensionManager::getFunction<PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC>("glGetRenderbufferParameterivEXT");
	glIsFramebufferEXTProc=GLExtensionManager::getFunction<PFNGLISFRAMEBUFFEREXTPROC>("glIsFramebufferEXT");
	glBindFramebufferEXTProc=GLExtensionManager::getFunction<PFNGLBINDFRAMEBUFFEREXTPROC>("glBindFramebufferEXT");
	glDeleteFramebuffersEXTProc=GLExtensionManager::getFunction<PFNGLDELETEFRAMEBUFFERSEXTPROC>("glDeleteFramebuffersEXT");
	glGenFramebuffersEXTProc=GLExtensionManager::getFunction<PFNGLGENFRAMEBUFFERSEXTPROC>("glGenFramebuffersEXT");
	glCheckFramebufferStatusEXTProc=GLExtensionManager::getFunction<PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC>("glCheckFramebufferStatusEXT");
	glFramebufferTexture1DEXTProc=GLExtensionManager::getFunction<PFNGLFRAMEBUFFERTEXTURE1DEXTPROC>("glFramebufferTexture1DEXT");
	glFramebufferTexture2DEXTProc=GLExtensionManager::getFunction<PFNGLFRAMEBUFFERTEXTURE2DEXTPROC>("glFramebufferTexture2DEXT");
	glFramebufferTexture3DEXTProc=GLExtensionManager::getFunction<PFNGLFRAMEBUFFERTEXTURE3DEXTPROC>("glFramebufferTexture3DEXT");
	glFramebufferRenderbufferEXTProc=GLExtensionManager::getFunction<PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC>("glFramebufferRenderbufferEXT");
	glGetFramebufferAttachmentParameterivEXTProc=GLExtensionManager::getFunction<PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC>("glGetFramebufferAttachmentParameterivEXT");
	glGenerateMipmapEXTProc=GLExtensionManager::getFunction<PFNGLGENERATEMIPMAPEXTPROC>("glGenerateMipmapEXT");
	}

GLEXTFramebufferObject::~GLEXTFramebufferObject(void)
	{
	}

const char* GLEXTFramebufferObject::getExtensionName(void) const
	{
	return name;
	}

void GLEXTFramebufferObject::activate(void)
	{
	current=this;
	}

void GLEXTFramebufferObject::deactivate(void)
	{
	current=0;
	}

bool GLEXTFramebufferObject::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLEXTFramebufferObject::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLEXTFramebufferObject* newExtension=new GLEXTFramebufferObject;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}

namespace {

/****************
Helper functions:
****************/

const char* glGetFramebufferStatusErrorEXT(GLenum status)
	{
	switch(status)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			return "has an incomplete attachment";
		
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			return "is missing an attachment";
		
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			return "has attachments with mismatching sizes";
		
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			return "has an attachment with an invalid format";
		
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			return "is missing a draw buffer attachment";
		
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			return "is missing a read buffer attachment";
		
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			return "has unsupported configuration";
		
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			return "is complete";
		
		default:
			return "is incomplete for unknown reasons";
		}
	}

}

void glPrintFramebufferStatusEXT(std::ostream& stream,const char* tag)
	{
	GLenum status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status!=GL_FRAMEBUFFER_COMPLETE_EXT)
		stream<<tag<<' '<<glGetFramebufferStatusErrorEXT(status)<<std::endl;
	}

void glPrintFramebufferStatusEXT(std::ostream& stream,const char* prettyFunction,const char* tag)
	{
	GLenum status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status!=GL_FRAMEBUFFER_COMPLETE_EXT)
		stream<<Misc::parsePrettyFunction(prettyFunction)<<": "<<tag<<' '<<glGetFramebufferStatusErrorEXT(status)<<std::endl;
	}

void glThrowFramebufferStatusExceptionEXT(const char* tag)
	{
	GLenum status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status!=GL_FRAMEBUFFER_COMPLETE_EXT)
		{
		std::string msg(tag);
		msg.push_back(' ');
		msg.append(glGetFramebufferStatusErrorEXT(status));
		throw std::runtime_error(msg);
		}
	}

void glThrowFramebufferStatusExceptionEXT(const char* prettyFunction,const char* tag)
	{
	GLenum status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status!=GL_FRAMEBUFFER_COMPLETE_EXT)
		{
		std::string msg(Misc::parsePrettyFunction(prettyFunction));
		msg.append(": ");
		msg.append(tag);
		msg.push_back(' ');
		msg.append(glGetFramebufferStatusErrorEXT(status));
		throw std::runtime_error(msg);
		}
	}

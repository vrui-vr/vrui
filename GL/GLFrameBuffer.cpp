/***********************************************************************
GLFrameBuffer - Simple class to encapsulate the state of and operations
on OpenGL frame buffer objects.
Copyright (c) 2012-2024 Oliver Kreylos

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

#include <GL/GLFrameBuffer.h>

#include <Misc/StdError.h>
#include <GL/gl.h>
#include <GL/GLMiscTemplates.h>

// DEBUGGING
// #include <GL/GLPrintError.h>

/******************************
Methods of class GLFrameBuffer:
******************************/

void GLFrameBuffer::deleteDepthAttachment(void)
	{
	/* Check if there is a depth attachment: */
	if(depthBufferId!=0)
		{
		/* Detach and destroy the depth buffer: */
		if(depthIsTexture)
			glDeleteTextures(1,&depthBufferId);
		else
			glDeleteRenderbuffersEXT(1,&depthBufferId);
		}
	}

void GLFrameBuffer::deleteColorAttachment(GLint colorAttachmentIndex)
	{
	/* Check if there is a color attachment in the given attachment slot: */
	if(colorBufferIds[colorAttachmentIndex]!=0)
		{
		/* Detach and destroy the color buffer: */
		if(colorIsTextures[colorAttachmentIndex])
			glDeleteTextures(1,&colorBufferIds[colorAttachmentIndex]);
		else
			glDeleteRenderbuffersEXT(1,&colorBufferIds[colorAttachmentIndex]);
		}
	}

void GLFrameBuffer::bindAttachments(void)
	{
	/* Attach all requested buffers: */
	if(depthBufferId!=0)
		{
		/* Attach the depth buffer to the frame buffer: */
		if(depthIsTexture)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,textureTarget,depthBufferId,0);
		else
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthBufferId);
		}
	else
		{
		/* Reset the depth attachment point: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,0);
		}
	// DEBUGGING
	// glPrintError("BA1");
	for(GLint colorAttachmentIndex=0;colorAttachmentIndex<numColorAttachments;++colorAttachmentIndex)
		{
		if(colorBufferIds[colorAttachmentIndex]!=0)
			{
			/* Attach the color buffer to the frame buffer: */
			if(colorIsTextures[colorAttachmentIndex])
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+colorAttachmentIndex,textureTarget,colorBufferIds[colorAttachmentIndex],0);
			else
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+colorAttachmentIndex,GL_RENDERBUFFER_EXT,colorBufferIds[colorAttachmentIndex]);
			}
		else
			{
			/* Reset the color attachment point: */
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+colorAttachmentIndex,GL_RENDERBUFFER_EXT,0);
			}
		}
	// DEBUGGING
	// glPrintError("BA2");
	if(stencilBufferId!=0)
		{
		/* Attach the stencil buffer to the frame buffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,stencilBufferId);
		}
	else
		{
		/* Reset the stencil attachment point: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,0);
		}
	// DEBUGGING
	// glPrintError("BA3");
	}

void GLFrameBuffer::checkConsistency(void)
	{
	/* Check the frame buffer for consistency: */
	GLenum status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	
	if(status!=GL_FRAMEBUFFER_COMPLETE_EXT)
		{
		/* Throw an exception with a descriptive message: */
		static const GLenum statusCodes[7]=
			{
			GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT,
			GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT,
			GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT,
			GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT,
			GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT,
			GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT,
			GL_FRAMEBUFFER_UNSUPPORTED_EXT
			};
		static const char* statusMessages[8]=
			{
			"Attachment",
			"Missing attachment",
			"Dimensions",
			"Formats",
			"Draw buffer",
			"Read buffer",
			"Unsupported",
			"Unknown issue"
			};
		int i;
		for(i=0;i<7&&status!=statusCodes[i];++i)
			;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,statusMessages[i]);
		}
	}

GLFrameBuffer::GLFrameBuffer(const Size& sSize,bool pad)
	:size(sSize),
	 haveTextureRectangles(GLARBTextureRectangle::isSupported()),
	 paddedSize(sSize),
	 textureTarget(pad||!haveTextureRectangles?GL_TEXTURE_2D:GL_TEXTURE_RECTANGLE_ARB),
	 frameBufferId(0),
	 haveDepthTextures(GLARBDepthTexture::isSupported()),
	 depthIsTexture(false),depthBufferId(0),
	 numColorAttachments(0),colorIsTextures(0),colorBufferIds(0),
	 stencilBufferId(0),
	 modified(true)
	{
	/* Initialize the required extensions: */
	GLEXTFramebufferObject::initExtension();
	if(haveDepthTextures)
		GLARBDepthTexture::initExtension();
	if(haveTextureRectangles)
		GLARBTextureRectangle::initExtension();
	
	/* Check if texture attachments need to be padded to powers of two: */
	if(pad||!haveTextureRectangles)
		{
		/* Pad the sizes for all frame buffer attachments: */
		for(int i=0;i<2;++i)
			for(paddedSize[i]=1;paddedSize[i]<size[i];paddedSize[1]<<=1)
				;
		}
	
	/* Create the frame buffer object: */
	glGenFramebuffersEXT(1,&frameBufferId);
	
	/* Query the number of supported color attachments: */
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT,&numColorAttachments);
	
	/* Initialize the color buffer attachment arrays: */
	colorIsTextures=new bool[numColorAttachments];
	colorBufferIds=new GLuint[numColorAttachments];
	for(GLsizei i=0;i<numColorAttachments;++i)
		{
		colorIsTextures[i]=false;
		colorBufferIds[i]=0;
		}
	}

GLFrameBuffer::~GLFrameBuffer(void)
	{
	/* Destroy the frame buffer object: */
	glDeleteFramebuffersEXT(1,&frameBufferId);
	
	/* Destroy all render buffer objects: */
	deleteDepthAttachment();
	for(GLint colorAttachmentIndex=0;colorAttachmentIndex<numColorAttachments;++colorAttachmentIndex)
		deleteColorAttachment(colorAttachmentIndex);
	if(stencilBufferId!=0)
		glDeleteRenderbuffersEXT(1,&stencilBufferId);
	
	/* Destroy the color buffer attachment arrays: */
	delete[] colorIsTextures;
	delete[] colorBufferIds;
	}

bool GLFrameBuffer::isSupported(void)
	{
	return GLEXTFramebufferObject::isSupported();
	}

void GLFrameBuffer::attachDepthBuffer(void)
	{
	/* Delete any current depth attachments: */
	deleteDepthAttachment();
	
	/* Create a new render buffer: */
	depthIsTexture=false;
	glGenRenderbuffersEXT(1,&depthBufferId);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthBufferId);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,paddedSize);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	
	modified=true;
	}

void GLFrameBuffer::attachDepthTexture(GLenum pixelFormat,GLenum filterMode)
	{
	/* Check for the required OpenGL extensions: */
	if(!haveDepthTextures)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"GL_ARB_depth_texture extension not supported");
	
	/* Delete any current depth attachments: */
	deleteDepthAttachment();
	
	/* Create a new depth texture object: */
	depthIsTexture=true;
	glGenTextures(1,&depthBufferId);
	glBindTexture(textureTarget,depthBufferId);
	glTexParameteri(textureTarget,GL_TEXTURE_MIN_FILTER,filterMode);
	glTexParameteri(textureTarget,GL_TEXTURE_MAG_FILTER,filterMode);
	glTexParameteri(textureTarget,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(textureTarget,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(textureTarget,GL_DEPTH_TEXTURE_MODE_ARB,GL_INTENSITY);
	glTexImage2D(textureTarget,0,pixelFormat,paddedSize,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);
	glBindTexture(textureTarget,0);
	
	modified=true;
	}

void GLFrameBuffer::attachColorBuffer(GLint colorAttachmentIndex,GLenum pixelFormat)
	{
	/* Delete any current color attachments in the given attachment slot: */
	deleteColorAttachment(colorAttachmentIndex);
	
	/* Create a new render buffer: */
	colorIsTextures[colorAttachmentIndex]=false;
	glGenRenderbuffersEXT(1,&colorBufferIds[colorAttachmentIndex]);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,colorBufferIds[colorAttachmentIndex]);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,pixelFormat,paddedSize);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	
	modified=true;
	}

void GLFrameBuffer::attachColorTexture(GLint colorAttachmentIndex,GLenum pixelFormat,GLenum filterMode)
	{
	/* Delete any current color attachments in the given attachment slot: */
	deleteColorAttachment(colorAttachmentIndex);
	
	/* Create a new color texture object: */
	colorIsTextures[colorAttachmentIndex]=true;
	glGenTextures(1,&colorBufferIds[colorAttachmentIndex]);
	glBindTexture(textureTarget,colorBufferIds[colorAttachmentIndex]);
	glTexParameteri(textureTarget,GL_TEXTURE_MIN_FILTER,filterMode);
	glTexParameteri(textureTarget,GL_TEXTURE_MAG_FILTER,filterMode);
	glTexParameteri(textureTarget,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(textureTarget,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexImage2D(textureTarget,0,pixelFormat,paddedSize,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,0);
	glBindTexture(textureTarget,0);
	
	modified=true;
	}

void GLFrameBuffer::attachColorTexture(GLint colorAttachmentIndex,GLuint textureObjectId)
	{
	/* Delete any current color attachments in the given attachment slot: */
	deleteColorAttachment(colorAttachmentIndex);
	
	/* Attach the given color texture: */
	colorIsTextures[colorAttachmentIndex]=true;
	colorBufferIds[colorAttachmentIndex]=textureObjectId;
	
	modified=true;
	}

GLuint GLFrameBuffer::detachColorTexture(GLint colorAttachmentIndex)
	{
	/* Check if the given attachment is actually a texture: */
	if(!colorIsTextures[colorAttachmentIndex])
		return 0;
	
	/* Detach and return the given attachment: */
	GLuint result=colorBufferIds[colorAttachmentIndex];
	colorIsTextures[colorAttachmentIndex]=false;
	colorBufferIds[colorAttachmentIndex]=0;
	modified=true;
	return result;
	}

void GLFrameBuffer::attachStencilBuffer(GLenum pixelFormat)
	{
	/* Delete any current stencil attachments: */
	if(stencilBufferId!=0)
		glDeleteRenderbuffersEXT(1,&stencilBufferId);
	
	/* Create a new render buffer: */
	glGenRenderbuffersEXT(1,&stencilBufferId);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,stencilBufferId);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,pixelFormat,paddedSize);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	
	modified=true;
	}

void GLFrameBuffer::bind(void) const
	{
	/* Bind the frame buffer object: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBufferId);
	}

void GLFrameBuffer::selectBuffers(GLenum readAttachment,GLenum writeAttachment)
	{
	/* Check if the frame buffer was modified: */
	if(modified)
		{
		/* Bind all attachments: */
		// DEBUGGING
		// glPrintError("A");
		bindAttachments();
		
		modified=false;
		}
	
	/* Set the read and write attachment points: */
	// DEBUGGING
	// glPrintError("B");
	glReadBuffer(readAttachment);
	// DEBUGGING
	// glPrintError("C");
	glDrawBuffer(writeAttachment);
	
	/* Check the frame buffer for consistency: */
	// DEBUGGING
	// glPrintError("D");
	checkConsistency();
	}

void GLFrameBuffer::unbind(void)
	{
	/* Unbind all frame buffer objects: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	}

/***********************************************************************
VRWindowAnaglyph2 - Class for OpenGL windows that render an anaglyph 
stereoscopic view with saturation and cross-talk reduction.
Copyright (c) 2025 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Internal/VRWindowAnaglyph2.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Matrix.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBVertexProgram.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLEXTFramebufferBlit.h>
#include <GL/Extensions/GLEXTFramebufferMultisample.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTPackedDepthStencil.h>
#include <GL/Extensions/GLEXTTextureSRGB.h>
#include <Vrui/Viewer.h>
#include <Vrui/DisplayState.h>

#include <iostream>

namespace Vrui {
extern bool vruiVerbose;
}

namespace Vrui {

/**********************************
Methods of class VRWindowAnaglyph2:
**********************************/

void VRWindowAnaglyph2::drawInner(bool canDraw)
	{
	if(canDraw)
		{
		/* Bind the per-eye rendering framebuffer: */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBufferId);
		
		/* Check if the window size changed: */
		if(frameBufferSize!=getWindowSize())
			{
			/* Update the frame buffer size: */
			frameBufferSize=getWindowSize();
			
			/* Reallocate all buffers and textures: */
			GLenum texturePixelFormat=getContext().isNonlinear()?GL_SRGB8_ALPHA8_EXT:GL_RGBA8;
			for(int i=0;i<2;++i)
				{
				glBindTexture(GL_TEXTURE_2D,colorBufferIds[i]);
				glTexImage2D(GL_TEXTURE_2D,0,texturePixelFormat,frameBufferSize[0],frameBufferSize[1],0,GL_RGBA,GL_UNSIGNED_BYTE,0);
				}
			glBindTexture(GL_TEXTURE_2D,0);
			
			if(multisamplingLevel>1)
				{
				glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,multisamplingColorBufferId);
				GLenum framebufferPixelFormat=getContext().isNonlinear()?GL_SRGB8_EXT:GL_RGB8;
				glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,framebufferPixelFormat,frameBufferSize);
				glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
				}
			
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthStencilBufferId);
			bool haveStencil=(clearBufferMask&GL_STENCIL_BUFFER_BIT)!=0x0U;
			GLenum depthPixelFormat=haveStencil?GL_DEPTH24_STENCIL8_EXT:GL_DEPTH_COMPONENT;
			if(multisamplingLevel>1)
				glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,depthPixelFormat,frameBufferSize);
			else
				glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,depthPixelFormat,frameBufferSize);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
			}
		
		if(multisamplingLevel>1)
			{
			/* Draw into the multisampling image buffer: */
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
			}
		
		/* Render the left and right views: */
		for(int eye=0;eye<2;++eye)
			{
			/* Set up rendering state: */
			displayState->eyeIndex=eye;
			displayState->eyePosition=viewer->getEyePosition(eye==0?Viewer::LEFT:Viewer::RIGHT);
			
			if(multisamplingLevel==1)
				{
				/* Draw directly into the left or right buffers: */
				glReadBuffer(GL_COLOR_ATTACHMENT0_EXT+eye);
				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+eye);
				}
			
			/* Render the view: */
			render();
			
			if(multisamplingLevel>1)
				{
				/* Blit the multisampling color buffer containing the per-eye image into the "fixing" framebuffer: */
				glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+eye);
				glBlitFramebufferEXT(frameBufferSize,frameBufferSize,GL_COLOR_BUFFER_BIT,GL_NEAREST);
				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
				glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,frameBufferId);
				}
			}
		
		/* Unbind the per-eye rendering framebuffer: */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
		
		/* Combine the rendered per-eye views into the on-screen window: */
		glUseProgramObjectARB(combiningShader);
		
		/* Bind the left and right per-eye textures: */
		for(int i=0;i<2;++i)
			{
			glActiveTextureARB(GL_TEXTURE0_ARB+i);
			glBindTexture(GL_TEXTURE_2D,colorBufferIds[i]);
			glUniform1iARB(combiningShaderUniforms[i],i);
			}
		
		/* Upload the color desaturation matrix: */
		glUniformMatrix3fvARB(combiningShaderUniforms[2],1,GL_FALSE,&colorMatrix[0][0]);
		
		/* Draw a quad filling the entire window: */
		glBegin(GL_QUADS);
		glVertex2f(-1.0f,-1.0f);
		glVertex2f( 1.0f,-1.0f);
		glVertex2f( 1.0f, 1.0f);
		glVertex2f(-1.0f, 1.0f);
		glEnd();
		
		/* Protect the combining shader: */
		glUseProgramObjectARB(0);
		}
	else
		{
		/* Clear the window's color buffer: */
		glClearColor(disabledColor);
		glClear(GL_COLOR_BUFFER_BIT);
		}
	}

VRWindowAnaglyph2::VRWindowAnaglyph2(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindowSingleViewport(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection)
	{
	/* Create the color de-saturation matrix: */
	Math::Matrix cc(3,3);
	cc(0,0)=0.299;
	cc(0,1)=0.587;
	cc(0,2)=0.114;
	cc(1,0)=-0.299;
	cc(1,1)=-0.587;
	cc(1,2)=0.886;
	cc(2,0)=0.701;
	cc(2,1)=-0.587;
	cc(2,2)=-0.114;
	
	Math::Matrix icc=cc.inverseFullPivot();
	
	double desaturation=configFileSection.retrieveValue<double>("./desaturation",0.0);
	Math::Matrix ds(3,3,1.0);
	ds(1,1)=Math::clamp(1.0-desaturation,0.0,1.0);
	ds(2,2)=Math::clamp(1.0-desaturation,0.0,1.0);
	
	Math::Matrix a=icc*ds*cc;
	
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			colorMatrix[j][i]=GLfloat(a(i,j));
	}

VRWindowAnaglyph2::~VRWindowAnaglyph2(void)
	{
	}

void VRWindowAnaglyph2::setDisplayState(DisplayState* newDisplayState,const Misc::ConfigurationFileSection& configFileSection)
	{
	VRWindowSingleViewport::setDisplayState(newDisplayState,configFileSection);
	
	/* Initialize the required OpenGL extensions: */
	GLARBFragmentShader::initExtension();
	GLARBMultitexture::initExtension();
	GLARBVertexProgram::initExtension();
	GLARBVertexShader::initExtension();
	GLEXTFramebufferObject::initExtension();
	if(clearBufferMask&GL_STENCIL_BUFFER_BIT)
		GLEXTPackedDepthStencil::initExtension();
	if(multisamplingLevel>1)
		{
		GLEXTFramebufferBlit::initExtension();
		GLEXTFramebufferMultisample::initExtension();
		}
	
	/* Create the per-eye rendering framebuffer: */
	glGenFramebuffersEXT(1,&frameBufferId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBufferId);
	frameBufferSize=getWindowSize();
	
	/* Create the per-eye rendering color textures: */
	glGenTextures(2,colorBufferIds);
	GLenum texturePixelFormat=getContext().isNonlinear()?GL_SRGB8_ALPHA8_EXT:GL_RGBA8;
	for(int i=0;i<2;++i)
		{
		glBindTexture(GL_TEXTURE_2D,colorBufferIds[i]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,0,texturePixelFormat,frameBufferSize[0],frameBufferSize[1],0,GL_RGBA,GL_UNSIGNED_BYTE,0);
		}
	glBindTexture(GL_TEXTURE_2D,0);
	
	if(multisamplingLevel>1)
		{
		/* Create the per-eye rendering multisampling color buffer: */
		glGenRenderbuffersEXT(1,&multisamplingColorBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,multisamplingColorBufferId);
		GLenum framebufferPixelFormat=getContext().isNonlinear()?GL_SRGB8_EXT:GL_RGB8;
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,framebufferPixelFormat,frameBufferSize);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the per-eye rendering multisampling color buffer to the framebuffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_RENDERBUFFER_EXT,multisamplingColorBufferId);
		}
	else
		{
		/* Directly attach the per-eye rendering color textures to the framebuffer: */
		for(GLenum i=0;i<2;++i)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+i,GL_TEXTURE_2D,colorBufferIds[i],0);
		}
	
	/* Create the per-eye rendering depth buffer with optional interleaved stencil buffer: */
	glGenRenderbuffersEXT(1,&depthStencilBufferId);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthStencilBufferId);
	bool haveStencil=(clearBufferMask&GL_STENCIL_BUFFER_BIT)!=0x0U;
	GLenum depthPixelFormat=haveStencil?GL_DEPTH24_STENCIL8_EXT:GL_DEPTH_COMPONENT;
	if(multisamplingLevel>1)
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,depthPixelFormat,frameBufferSize);
	else
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,depthPixelFormat,frameBufferSize);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	
	/* Attach the per-eye rendering depth buffer and the optional stencil buffer to the framebuffer: */
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthStencilBufferId);
	if(haveStencil)
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthStencilBufferId);
	
	/* Set up pixel sources and destinations: */
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	
	/* Check the status of the per-eye rendering framebuffer: */
	glThrowFramebufferStatusExceptionEXT(__PRETTY_FUNCTION__,"Per-eye rendering framebuffer");
	
	if(multisamplingLevel>1)
		{
		/* Create the multisample "fixing" framebuffer: */
		glGenFramebuffersEXT(1,&multisamplingFrameBufferId);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
		if(getContext().isNonlinear())
			glEnable(GL_FRAMEBUFFER_SRGB_EXT);
		
		/* Attach the per-eye rendering color image textures to the "fixing" framebuffer: */
		for(GLenum i=0;i<2;++i)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+i,GL_TEXTURE_2D,colorBufferIds[i],0);
		
		/* Check the status of the multisample "fixing" framebuffer: */
		glThrowFramebufferStatusExceptionEXT(__PRETTY_FUNCTION__,"Multisampling fixing framebuffer");
		}
	
	/* Protect the created framebuffer(s): */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	
	/* Create the combining shader: */
	combiningShader=glCreateProgramObjectARB();
	
	/* Compile the combining vertex shader: */
	std::string vertexShaderSource="\
void main()\n\
	{\n\
	/* Pass through the vertex position in clip coordinates: */\n\
	gl_Position=gl_Vertex;\n\
	\n\
	/* Convert the clip-coordinate vertex position to texture coordinates: */\n\
	gl_TexCoord[0]=vec4((gl_Vertex.x+1.0)*0.5,(gl_Vertex.y+1.0)*0.5,0.0,1.0);\n\
	}\n";
	
	GLhandleARB vertexShader=glCompileVertexShaderFromString(vertexShaderSource.c_str());
	glAttachObjectARB(combiningShader,vertexShader);
	glDeleteObjectARB(vertexShader);
	
	/* Compile the combining fragment shader: */
	std::string fragmentShaderSource="\
uniform sampler2D leftSampler;\n\
uniform sampler2D rightSampler;\n\
uniform mat3 colorMatrix;\n\
\n\
void main()\n\
	{\n\
	vec3 left=colorMatrix*texture2D(leftSampler,gl_TexCoord[0].st).rgb;\n\
	vec3 right=colorMatrix*texture2D(rightSampler,gl_TexCoord[0].st).rgb;\n\
	gl_FragColor=vec4(left.r,right.g,right.b,1.0);\n\
	}\n";
	
	GLhandleARB fragmentShader=glCompileFragmentShaderFromString(fragmentShaderSource.c_str());
	glAttachObjectARB(combiningShader,fragmentShader);
	glDeleteObjectARB(fragmentShader);
	
	/* Link the combining shader: */
	glLinkAndTestShader(combiningShader);
	
	/* Retrieve the combining shader's uniform variable locations: */
	combiningShaderUniforms[0]=glGetUniformLocationARB(combiningShader,"leftSampler");
	combiningShaderUniforms[1]=glGetUniformLocationARB(combiningShader,"rightSampler");
	combiningShaderUniforms[2]=glGetUniformLocationARB(combiningShader,"colorMatrix");
	}

void VRWindowAnaglyph2::releaseGLState(void)
	{
	/* Release all allocated OpenGL resources: */
	glDeleteFramebuffersEXT(1,&frameBufferId);
	glDeleteTextures(2,colorBufferIds);
	if(multisamplingLevel>1)
		glDeleteRenderbuffersEXT(1,&multisamplingColorBufferId);
	glDeleteRenderbuffersEXT(1,&depthStencilBufferId);
	if(multisamplingLevel>1)
		glDeleteFramebuffersEXT(1,&multisamplingFrameBufferId);
	glDeleteObjectARB(combiningShader);
	
	VRWindow::releaseGLState();
	}

int VRWindowAnaglyph2::getNumViews(void) const
	{
	return 2;
	}

VRWindow::View VRWindowAnaglyph2::getView(int index)
	{
	/* Create a view structure: */
	View result;
	result.viewport=getWindowSize();
	result.viewer=viewer;
	result.eye=viewer->getDeviceEyePosition(index==0?Viewer::LEFT:Viewer::RIGHT);
	result.screen=screen;
	writePanRect(screen,result.screenRect);
	
	return result;
	}

}

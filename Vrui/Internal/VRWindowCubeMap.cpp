/***********************************************************************
VRWindowCubeMap - Class for OpenGL windows that use off-screen rendering
into a cube map to create pre-distorted panoramic display images for
planetarium projectors, panoramic video, etc.
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

#include <Vrui/Internal/VRWindowCubeMap.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLMiscTemplates.h>
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
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/DisplayState.h>

// DEBUGGING
#include <iostream>
#include <Geometry/OutputOperators.h>

namespace Vrui {

/********************************
Methods of class VRWindowCubeMap:
********************************/

ISize VRWindowCubeMap::getViewportSize(void) const
	{
	/* Return the size of each cube map face: */
	return cubeMapSize;
	}

ISize VRWindowCubeMap::getFramebufferSize(void) const
	{
	/* Return the size of each cube map face: */
	return cubeMapSize;
	}

VRWindowCubeMap::VRWindowCubeMap(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindow(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection),
	 viewer(0),
	 cubeSize(configFileSection.retrieveValue<Scalar>("./cubeSize",getMeterFactor())),
	 cubeMapSize(configFileSection.retrieveValue<ISize>("./cubeMapSize",ISize(1024,1024))),
	 frameBufferId(0),colorBufferId(0),multisamplingColorBufferId(0),depthStencilBufferId(0),multisamplingFrameBufferId(0),
	 reprojectionShader(0)
	{
	for(int i=0;i<6;++i)
		screens[i]=0;
	
	/* Find the window's viewer: */
	std::string viewerName=configFileSection.retrieveString("viewerName");
	viewer=findViewer(viewerName.c_str());
	if(viewer==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot find viewer %s",viewerName.c_str());
	
	/* Retrieve a potential input device to which the viewer is attached: */
	InputDevice* headDevice=const_cast<InputDevice*>(viewer->getHeadDevice()); // Oof, sorry, this is fine
	
	/* Create a normalized coordinate frame around the viewer's mono eye position: */
	Vector viewerZ=viewer->getDeviceUpDirection();
	Vector viewerX=viewer->getDeviceViewDirection()^viewerZ;
	Vector viewerY=viewerZ^viewerX;
	ONTransform viewerTrans=ONTransform::translateFromOriginTo(viewer->getDeviceEyePosition(Viewer::MONO));
	viewerTrans*=ONTransform::rotate(Rotation::fromBaseVectors(viewerX,viewerY));
	
	/* Transform the coordinate frame to physical space if the viewer is not attached to a device: */
	if(headDevice==0)
		viewerTrans.leftMultiply(viewer->getHeadTransformation());
	
	/* Create the window's screens: */
	for(int screenIndex=0;screenIndex<6;++screenIndex)
		{
		/* Create a new screen of the appropriate size: */
		VRScreen* screen=screens[screenIndex]=new VRScreen;
		screen->setSize(cubeSize,cubeSize);
		
		/* Attach the screen to the same input device as the viewer: */
		screen->attachToDevice(headDevice);
		}
	
	/* Position the window's screens: */
	Scalar cr=Math::div2(cubeSize);
	screens[0]->setTransform(viewerTrans*ONTransform(Vector(cr,cr,cr),Rotation::fromBaseVectors(Vector(0,0,-1),Vector(0,-1,0)))); // +X
	screens[1]->setTransform(viewerTrans*ONTransform(Vector(-cr,cr,-cr),Rotation::fromBaseVectors(Vector(0,0,1),Vector(0,-1,0)))); // -X
	screens[2]->setTransform(viewerTrans*ONTransform(Vector(-cr,cr,-cr),Rotation::fromBaseVectors(Vector(1,0,0),Vector(0,0,1)))); // +Y
	screens[3]->setTransform(viewerTrans*ONTransform(Vector(-cr,-cr,cr),Rotation::fromBaseVectors(Vector(1,0,0),Vector(0,0,-1)))); // -Y
	screens[4]->setTransform(viewerTrans*ONTransform(Vector(-cr,cr,cr),Rotation::fromBaseVectors(Vector(1,0,0),Vector(0,-1,0)))); // +Z
	screens[5]->setTransform(viewerTrans*ONTransform(Vector(cr,cr,-cr),Rotation::fromBaseVectors(Vector(-1,0,0),Vector(0,-1,0)))); // -Z
	
	/* Configure the window: */
	configFileSection.updateValue("./cubeMapSize",cubeMapSize);
	}

VRWindowCubeMap::~VRWindowCubeMap(void)
	{
	/* Destroy the created screens: */
	for(int screenIndex=0;screenIndex<6;++screenIndex)
		delete screens[screenIndex];
	
	/* Release allocated resources: */
	}

void VRWindowCubeMap::setDisplayState(DisplayState* newDisplayState,const Misc::ConfigurationFileSection& configFileSection)
	{
	VRWindow::setDisplayState(newDisplayState,configFileSection);
	
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
	
	/* Create the cube map rendering framebuffer: */
	glGenFramebuffersEXT(1,&frameBufferId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBufferId);
	
	/* Create the cube map texture: */
	glGenTextures(1,&colorBufferId);
	glBindTexture(GL_TEXTURE_CUBE_MAP,colorBufferId);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	GLenum texturePixelFormat=getContext().isNonlinear()?GL_SRGB8_ALPHA8_EXT:GL_RGBA8;
	for(int i=0;i<6;++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,texturePixelFormat,cubeMapSize[0],cubeMapSize[1],0,GL_RGBA,GL_UNSIGNED_BYTE,0);
	glBindTexture(GL_TEXTURE_CUBE_MAP,0);
	
	if(multisamplingLevel>1)
		{
		/* Create the cube map rendering multisampling color buffer: */
		glGenRenderbuffersEXT(1,&multisamplingColorBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,multisamplingColorBufferId);
		GLenum framebufferPixelFormat=getContext().isNonlinear()?GL_SRGB8_EXT:GL_RGB8;
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,framebufferPixelFormat,cubeMapSize);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the cube map rendering multisampling color buffer to the framebuffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_RENDERBUFFER_EXT,multisamplingColorBufferId);
		}
	else
		{
		/* Directly attach the cube map face textures to the framebuffer: */
		for(GLenum i=0;i<6;++i)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+i,GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,colorBufferId,0);
		}
	
	/* Create the cube map rendering depth buffer: */
	if(clearBufferMask&GL_STENCIL_BUFFER_BIT)
		{
		/* Create an interleaved depth+stencil render buffer: */
		glGenRenderbuffersEXT(1,&depthStencilBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthStencilBufferId);
		if(multisamplingLevel>1)
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,GL_DEPTH24_STENCIL8_EXT,cubeMapSize);
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH24_STENCIL8_EXT,cubeMapSize);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the cube map rendering interleaved depth and stencil buffer to the framebuffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthStencilBufferId);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthStencilBufferId);
		}
	else
		{
		/* Create a depth-only render buffer: */
		glGenRenderbuffersEXT(1,&depthStencilBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthStencilBufferId);
		if(multisamplingLevel>1)
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,GL_DEPTH_COMPONENT,cubeMapSize);
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,cubeMapSize);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the cube map rendering depth buffer to the framebuffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthStencilBufferId);
		}
	
	/* Set up pixel sources and destinations: */
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	
	/* Check the status of the cube map rendering framebuffer: */
	glThrowFramebufferStatusExceptionEXT(__PRETTY_FUNCTION__,"Cube map rendering framebuffer");
	
	if(multisamplingLevel>1)
		{
		/* Create the multisample "fixing" framebuffer: */
		glGenFramebuffersEXT(1,&multisamplingFrameBufferId);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
		if(getContext().isNonlinear())
			glEnable(GL_FRAMEBUFFER_SRGB_EXT);
		
		/* Attach the cube map rendering color image textures to the "fixing" framebuffer: */
		for(GLenum i=0;i<6;++i)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+i,GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,colorBufferId,0);
		
		/* Check the status of the multisample "fixing" framebuffer: */
		glThrowFramebufferStatusExceptionEXT(__PRETTY_FUNCTION__,"Multisampling fixing framebuffer");
		}
	
	/* Protect the created framebuffer(s): */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	
	/* Create the reprojection shader: */
	reprojectionShader=glCreateProgramObjectARB();
	
	/* Compile the reprojection vertex shader: */
	std::string vertexShaderSource="\
void main()\n\
	{\n\
	/* Pass through the vertex position in clip coordinates: */\n\
	gl_Position=gl_Vertex;\n\
	\n\
	/* Convert the clip-coordinate vertex position to lat/long: */\n\
	const float pi=3.141592653590;\n\
	gl_TexCoord[0]=vec4(gl_Vertex.x*pi,gl_Vertex.y*pi*0.5,0.0,1.0);\n\
	}\n";
	
	GLhandleARB vertexShader=glCompileVertexShaderFromString(vertexShaderSource.c_str());
	glAttachObjectARB(reprojectionShader,vertexShader);
	glDeleteObjectARB(vertexShader);
	
	/* Compile the reprojection fragment shader: */
	std::string fragmentShaderSource="\
uniform samplerCube cubeMapSampler;\n\
\n\
void main()\n\
	{\n\
	vec3 v=vec3(sin(gl_TexCoord[0].s)*cos(gl_TexCoord[0].t),cos(gl_TexCoord[0].s)*cos(gl_TexCoord[0].t),sin(gl_TexCoord[0].t));\n\
	gl_FragColor=texture(cubeMapSampler,v);\n\
	}\n";
	
	GLhandleARB fragmentShader=glCompileFragmentShaderFromString(fragmentShaderSource.c_str());
	glAttachObjectARB(reprojectionShader,fragmentShader);
	glDeleteObjectARB(fragmentShader);
	
	/* Link the reprojection shader: */
	glLinkAndTestShader(reprojectionShader);
	
	/* Retrieve the reprojection shader's uniform variable locations: */
	reprojectionShaderUniforms[0]=glGetUniformLocationARB(reprojectionShader,"cubeMapSampler");
	}

void VRWindowCubeMap::init(const Misc::ConfigurationFileSection& configFileSection)
	{
	VRWindow::init(configFileSection);
	}

void VRWindowCubeMap::releaseGLState(void)
	{
	/* Release all allocated OpenGL resources: */
	glDeleteFramebuffersEXT(1,&frameBufferId);
	glDeleteTextures(1,&colorBufferId);
	if(multisamplingLevel>1)
		glDeleteRenderbuffersEXT(1,&multisamplingColorBufferId);
	glDeleteRenderbuffersEXT(1,&depthStencilBufferId);
	if(multisamplingLevel>1)
		glDeleteFramebuffersEXT(1,&multisamplingFrameBufferId);
	glDeleteObjectARB(reprojectionShader);
	
	VRWindow::releaseGLState();
	}

int VRWindowCubeMap::getNumVRScreens(void) const
	{
	return 6;
	}

VRScreen* VRWindowCubeMap::getVRScreen(int index)
	{
	return screens[index];
	}

VRScreen* VRWindowCubeMap::replaceVRScreen(int index,VRScreen* newScreen)
	{
	VRScreen* result=screens[index];
	screens[index]=newScreen;
	
	return result;
	}

int VRWindowCubeMap::getNumViewers(void) const
	{
	return 1;
	}

Viewer* VRWindowCubeMap::getViewer(int index)
	{
	return viewer;
	}

Viewer* VRWindowCubeMap::replaceViewer(int index,Viewer* newViewer)
	{
	Viewer* result=viewer;
	viewer=newViewer;
	
	return result;
	}

VRWindow::InteractionRectangle VRWindowCubeMap::getInteractionRectangle(void)
	{
	/* Calculate a coordinate frame for the viewer: */
	Point monoEyePos=viewer->getEyePosition(Viewer::MONO);
	Vector headY=viewer->getUpDirection();
	Vector headZ=-viewer->getViewDirection();
	Vector headX=headY^headZ;
	Rotation headRot=Rotation::fromBaseVectors(headX,headY);
	
	/* Place the interaction plane on the cube's +Y plane: */
	Scalar cr=Math::div2(cubeSize);
	InteractionRectangle result;
	result.transformation=ONTransform::translateFromOriginTo(monoEyePos);
	result.transformation*=ONTransform::rotate(headRot);
	result.transformation*=ONTransform::translate(Vector(-cr,-cr,-cr));
	result.transformation.renormalize();
	result.size[0]=cubeSize;
	result.size[1]=cubeSize;
	
	// DEBUGGING
	std::cout<<result.transformation<<std::endl;
	
	return result;
	}

int VRWindowCubeMap::getNumViews(void) const
	{
	return 6;
	}

VRWindow::View VRWindowCubeMap::getView(int index)
	{
	/* Create a view structure: */
	View result;
	result.viewport=IRect(IOffset(0,0),cubeMapSize);
	result.viewer=viewer;
	result.eye=viewer->getDeviceEyePosition(Viewer::MONO);
	result.screen=screens[index];
	writePanRect(screens[index],result.screenRect);
	
	return result;
	}

void VRWindowCubeMap::updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const
	{
	/* Delegate to the common method using the full-window viewport, monoscopic eye, and forward-facing screen: */
	updateScreenDeviceCommon(windowPos,getWindowSize(),viewer->getEyePosition(Viewer::MONO),screens[2],device);
	}

void VRWindowCubeMap::draw(void)
	{
	/* Check whether this window can be drawn at this time: */
	if(enabled&&viewer->isEnabled())
		{
		/* Update the shared display state for this window: */
		displayState->frameSize=cubeMapSize;
		displayState->viewer=viewer;
		displayState->viewport=IRect(IOffset(0,0),cubeMapSize);
		displayState->context.setViewport(displayState->viewport);
		displayState->eyeIndex=0;
		displayState->eyePosition=viewer->getEyePosition(Viewer::MONO);
		
		/* Prepare for rendering: */
		prepareRender();
		
		/* Bind the cube mape rendering framebuffer: */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBufferId);
		if(multisamplingLevel>1)
			{
			/* Draw into the multisampling image buffer: */
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
			}
		
		/* Draw the six cube map faces in sequence: */
		for(int screenIndex=0;screenIndex<6;++screenIndex)
			{
			/* Set up the display state: */
			displayState->screen=screens[screenIndex];
			
			if(multisamplingLevel==1)
				{
				/* Draw directly into the cube face's color image buffer: */
				glReadBuffer(GL_COLOR_ATTACHMENT0_EXT+screenIndex);
				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+screenIndex);
				}
			
			/* Project the virtual environment into the window: */
			render();
			
			if(multisamplingLevel>1)
				{
				/* Blit the multisampling color buffer containing the cube face image into the "fixing" framebuffer: */
				glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+screenIndex);
				glBlitFramebufferEXT(cubeMapSize,cubeMapSize,GL_COLOR_BUFFER_BIT,GL_NEAREST);
				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
				glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,frameBufferId);
				}
			}
		
		/* Unbind the cube map rendering framebuffer: */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
		
		/* Reproject the rendered cube map into the on-screen window: */
		displayState->context.setViewport(getWindowSize());
		
		/* Enable the reprojection shader: */
		glUseProgramObjectARB(reprojectionShader);
		
		/* Bind the cube map texture: */
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP,colorBufferId);
		glUniform1iARB(reprojectionShaderUniforms[0],0);
		
		/* Draw a quad filling the entire window: */
		glBegin(GL_QUADS);
		glVertex2f(-1.0f,-1.0f);
		glVertex2f( 1.0f,-1.0f);
		glVertex2f( 1.0f, 1.0f);
		glVertex2f(-1.0f, 1.0f);
		glEnd();
		
		/* Protect the reprojection shader: */
		glUseProgramObjectARB(0);
		}
	else
		{
		/* Just clear the window to grey: */
		glClearColor(0.5f,0.5f,0.5f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		}
	
	/* If supported, insert a fence into the OpenGL command stream to wait for completion of this draw() call: */
	if(haveSync)
		drawFence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0x0);
	}

void VRWindowCubeMap::waitComplete(void)
	{
	/* Wait until all OpenGL operations from the most recent draw() call have completed: */
	if(haveSync)
		{
		glClientWaitSync(drawFence,0x0,~GLuint64(0));
		glDeleteSync(drawFence);
		drawFence=0;
		}
	else
		glFinish();
	
	/* Tell the base class that rendering is done: */
	renderComplete();
	}

void VRWindowCubeMap::present(void)
	{
	/* Present the back buffer: */
	swapBuffers();
	
	/* In synchronized or low-latency mode, wait until vsync actually happened: */
	if(synchronize)
		{
		glFinish();
		
		/* Update the Vrui kernel's frame synchronization state: */
		// FIX ME!!!
		}
	else if(vsync&&lowLatency)
		glFinish();
	}

}

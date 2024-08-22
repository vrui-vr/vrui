/***********************************************************************
VRWindowCompositorClient - Class for OpenGL windows that drive head-
mounted displays via an external VR compositing client.
Copyright (c) 2004-2024 Oliver Kreylos

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

#include <Vrui/Internal/VRWindowCompositorClient.h>

#include <stddef.h>
#include <unistd.h>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/FunctionCalls.h>
#include <GL/gl.h>
#include <GL/GLMiscTemplates.h>
#include <GL/GLContext.h>
#include <GL/Extensions/GLEXTFramebufferBlit.h>
#include <GL/Extensions/GLEXTFramebufferMultisample.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTMemoryObject.h>
#include <GL/Extensions/GLEXTMemoryObjectFd.h>
#include <GL/Extensions/GLEXTPackedDepthStencil.h>
#include <GL/Extensions/GLEXTTextureSRGB.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Internal/Vrui.h>
#include <Vrui/Internal/HMDConfiguration.h>
#include <Vrui/Internal/HMDConfigurationUpdater.h>

// DEBUGGING
#include <iostream>

namespace Vrui {

/*********************************************************
Methods of class VRWindowCompositorClient::CompositorInfo:
*********************************************************/

VRWindowCompositorClient::CompositorInfo::CompositorInfo(const char* source,Comm::UNIXPipe& pipe)
	:sharedMemoryBlockFd(-1),imageMemoryBlockFd(-1)
	{
	/* Read the server's shared memory file descriptors: */
	sharedMemoryBlockFd=pipe.readFd();
	imageMemoryBlockFd=pipe.readFd();
	
	/* Read and check the server's protocol version number: */
	unsigned int serverProtocolVersion=pipe.read<unsigned int>();
	if(serverProtocolVersion!=VRCompositorProtocol::protocolVersion)
		{
		/* Close the shared memory files and throw an exception: */
		close(sharedMemoryBlockFd);
		close(imageMemoryBlockFd);
		throw Misc::makeStdErr(source,"VR compositing server's UNIX domain pipe has wrong protocol version");
		}
	
	/* Read the rest of the server's connection information: */
	imageMemoryBlockSize=pipe.read<size_t>();
	pipe.read(imageMemorySizes,3);
	pipe.read(imageMemoryOffsets,3);
	}

/*****************************************
Methods of class VRWindowCompositorClient:
*****************************************/

ISize VRWindowCompositorClient::getViewportSize(void) const
	{
	/* Return a size encompassing both the pre-distortion viewports: */
	return Misc::max(hmdConfiguration.eyeRects[0].size,hmdConfiguration.eyeRects[1].size);
	}

ISize VRWindowCompositorClient::getFramebufferSize(void) const
	{
	/* Return the size of the pre-distortion framebuffer: */
	return hmdConfiguration.frameSize;
	}

void VRWindowCompositorClient::hmdConfigurationUpdatedCallback(const Vrui::HMDConfiguration& hmdConfiguration)
	{
	/* Update the viewer based on the updated HMD configuration: */
	Point eyes[2];
	for(int eye=0;eye<2;++eye)
		eyes[eye]=hmdConfiguration.getEyePosition(eye);
	viewer->setEyes(viewer->getDeviceViewDirection(),Geometry::mid(eyes[0],eyes[1]),(eyes[1]-eyes[0])*Scalar(0.5));
	
	/* Update the screens based on the updated HMD configuration: */
	Scalar virtualScreenDist=getMeterFactor(); // Distance from eye to virtual screen (completely arbitrary)
	for(int eye=0;eye<2;++eye)
		{
		/* Get the eye's rendered FoV: */
		const Scalar* eyeFov=hmdConfiguration.getFov(eye);
		
		/* Configure the eye's screen so that its calculated FoV will match the HMD's configured FoV: */
		Scalar w=(eyeFov[1]-eyeFov[0])*virtualScreenDist;
		Scalar h=(eyeFov[3]-eyeFov[2])*virtualScreenDist;
		screens[eye]->setSize(w,h);
		ONTransform screenT=ONTransform::translateFromOriginTo(eyes[eye]);
		screenT*=ONTransform::rotate(hmdConfiguration.getEyeRotation(eye));
		screenT*=ONTransform::translate(Vector(eyeFov[0]*virtualScreenDist,eyeFov[2]*virtualScreenDist,-virtualScreenDist));
		screenT.renormalize();
		screens[eye]->setTransform(screenT);
		}
	}

VRWindowCompositorClient::VRWindowCompositorClient(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindow(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection),
	 compositorPipe("VRCompositingServer.socket",true),
	 compositorInfo(__PRETTY_FUNCTION__,compositorPipe),
	 sharedMemory(compositorInfo.sharedMemoryBlockFd,true),
	 sharedMemorySegment(sharedMemory.getValue<SharedMemorySegment>(0)),
	 viewer(0),hmdConfigurationUpdater(0),
	 mirrorHmd(false),mirrorEyeIndex(1),mirrorFov(90),mirrorFollowAzimuth(false),mirrorFollowElevation(false),
	 mirrorModeKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./mirrorModeKey","Super+m"))),
	 mirrorFollowModeKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./mirrorFollowModeKey","Super+Shift+m")))
	{
	screens[1]=screens[0]=0;
	
	/* Check the protocol version of the compositor's shared memory segment: */
	if(sharedMemorySegment->protocolVersion!=protocolVersion)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"VR compositing server's shared memory block has wrong protocol version");
	
	/* Read the compositor's current HMD configuration: */
	sharedMemorySegment->hmdConfiguration.read(hmdConfiguration);
	
	/* Find the window's viewer: */
	std::string viewerName=configFileSection.retrieveString("viewerName");
	viewer=findViewer(viewerName.c_str());
	if(viewer==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot find viewer %s",viewerName);
	
	/* Find the window's screens: */
	std::string leftScreenName=configFileSection.retrieveString("leftScreenName");
	screens[0]=findScreen(leftScreenName.c_str());
	if(screens[0]==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot find screen %s",leftScreenName);
	std::string rightScreenName=configFileSection.retrieveString("rightScreenName");
	screens[1]=findScreen(rightScreenName.c_str());
	if(screens[1]==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot find screen %s",rightScreenName);
	
	/* Update the viewer based on the initial HMD configuration: */
	Point eyes[2];
	for(int eye=0;eye<2;++eye)
		eyes[eye]=Point(hmdConfiguration.eyePositions[eye]);
	viewer->setEyes(viewer->getDeviceViewDirection(),Geometry::mid(eyes[0],eyes[1]),(eyes[1]-eyes[0])*Scalar(0.5));
	
	/* Update the screens based on the initial HMD configuration: */
	Scalar virtualScreenDist=getMeterFactor(); // Distance from eye to virtual screen (completely arbitrary)
	for(int eye=0;eye<2;++eye)
		{
		/* Get the eye's rendered FoV: */
		Scalar* eyeFov=hmdConfiguration.eyeFovs[eye];
		
		/* Configure the eye's screen so that its calculated FoV will match the HMD's configured FoV: */
		Scalar w=(eyeFov[1]-eyeFov[0])*virtualScreenDist;
		Scalar h=(eyeFov[3]-eyeFov[2])*virtualScreenDist;
		screens[eye]->setSize(w,h);
		ONTransform screenT=ONTransform::translateFromOriginTo(eyes[eye]);
		screenT*=ONTransform::rotate(hmdConfiguration.eyeRotations[eye]);
		screenT*=ONTransform::translate(Vector(eyeFov[0]*virtualScreenDist,eyeFov[2]*virtualScreenDist,-virtualScreenDist));
		screenT.renormalize();
		screens[eye]->setTransform(screenT);
		}
	
	/* Create an HMD configuration updater: */
	hmdConfigurationUpdater=new HMDConfigurationUpdater(viewer,*Threads::createFunctionCall(this,&VRWindowCompositorClient::hmdConfigurationUpdatedCallback));
	
	/* Check if the pre-distortion image should be mirrored to the window: */
	configFileSection.updateValue("mirrorHmd",mirrorHmd);
	configFileSection.updateValue("mirrorEyeIndex",mirrorEyeIndex);
	if(mirrorEyeIndex<0||mirrorEyeIndex>1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid mirror eye index %d",mirrorEyeIndex);
	configFileSection.updateValue("mirrorFov",mirrorFov);
	if(mirrorFov<=Scalar(0)||mirrorFov>=Scalar(180))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid mirror field of view %f",mirrorFov);
	configFileSection.updateValue("mirrorFollowAzimuth",mirrorFollowAzimuth);
	configFileSection.updateValue("mirrorFollowElevation",mirrorFollowElevation);
	
	/* Convert the mirror field of view to tangent space: */
	mirrorFov=Math::tan(Math::rad(mirrorFov)*Scalar(0.5));
	}

VRWindowCompositorClient::~VRWindowCompositorClient(void)
	{
	/* Release allocated resources: */
	delete hmdConfigurationUpdater;
	}

void VRWindowCompositorClient::setDisplayState(DisplayState* newDisplayState,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Disable vsync for this window, no matter what was configured: */
	vsync=false;
	
	VRWindow::setDisplayState(newDisplayState,configFileSection);
	
	/* Initialize the required OpenGL extensions: */
	GLEXTFramebufferObject::initExtension();
	GLEXTMemoryObject::initExtension();
	GLEXTMemoryObjectFd::initExtension();
	if(clearBufferMask&GL_STENCIL_BUFFER_BIT)
		GLEXTPackedDepthStencil::initExtension();
	if(multisamplingLevel>1)
		{
		GLEXTFramebufferBlit::initExtension();
		GLEXTFramebufferMultisample::initExtension();
		}
	
	/* Create the pre-distortion rendering framebuffer: */
	glGenFramebuffersEXT(1,&predistortionFrameBufferId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,predistortionFrameBufferId);
	
	/* Import the VR compositor's GPU memory object: */
	glCreateMemoryObjectsEXT(1,&memoryObjectId);
	glImportMemoryFdEXT(memoryObjectId,compositorInfo.imageMemoryBlockSize,GL_HANDLE_TYPE_OPAQUE_FD_EXT,compositorInfo.imageMemoryBlockFd);
	if(!glIsMemoryObjectEXT(memoryObjectId))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to import VR compositor's shared GPU memory object");
	
	/* Create three pre-distortion color buffers from the VR compositor's shared textures: */
	GLenum texturePixelFormat=getContext().isNonlinear()?GL_SRGB8_ALPHA8_EXT:GL_RGBA8;
	glGenTextures(3,predistortionColorBufferIds);
	for(int i=0;i<3;++i)
		{
		glBindTexture(GL_TEXTURE_2D,predistortionColorBufferIds[i]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_TILING_EXT,GL_OPTIMAL_TILING_EXT);
		glTexStorageMem2DEXT(GL_TEXTURE_2D,1,texturePixelFormat,hmdConfiguration.frameSize,memoryObjectId,compositorInfo.imageMemoryOffsets[i]);
		}
	glBindTexture(GL_TEXTURE_2D,0);
	
	if(multisamplingLevel>1)
		{
		/* Create the pre-distortion multisampling color buffer: */
		glGenRenderbuffersEXT(1,&predistortionMultisamplingColorBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,predistortionMultisamplingColorBufferId);
		GLenum framebufferPixelFormat=getContext().isNonlinear()?GL_SRGB8_EXT:GL_RGB8;
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,framebufferPixelFormat,hmdConfiguration.frameSize);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the pre-distortion multisampling color buffer to the framebuffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_RENDERBUFFER_EXT,predistortionMultisamplingColorBufferId);
		}
	else
		{
		/* Attach the pre-distortion color image textures to the framebuffer: */
		for(GLenum i=0;i<3;++i)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+i,GL_TEXTURE_2D,predistortionColorBufferIds[i],0);
		}
	
	/* Create the pre-distortion depth buffer: */
	if(clearBufferMask&GL_STENCIL_BUFFER_BIT)
		{
		/* Create an interleaved depth+stencil render buffer: */
		glGenRenderbuffersEXT(1,&predistortionDepthStencilBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		if(multisamplingLevel>1)
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,GL_DEPTH24_STENCIL8_EXT,hmdConfiguration.frameSize);
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH24_STENCIL8_EXT,hmdConfiguration.frameSize);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the pre-distortion interleaved depth and stencil buffer to the framebuffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		}
	else
		{
		/* Create a depth-only render buffer: */
		glGenRenderbuffersEXT(1,&predistortionDepthStencilBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		if(multisamplingLevel>1)
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,multisamplingLevel,GL_DEPTH_COMPONENT,hmdConfiguration.frameSize);
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,hmdConfiguration.frameSize);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the pre-distortion depth buffer to the framebuffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		}
	
	/* Set up pixel sources and destinations: */
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	
	/* Check the status of the pre-distortion rendering framebuffer: */
	glThrowFramebufferStatusExceptionEXT(__PRETTY_FUNCTION__,"Lens correction framebuffer");
	
	if(multisamplingLevel>1)
		{
		/* Create the multisample "fixing" framebuffer: */
		glGenFramebuffersEXT(1,&multisamplingFrameBufferId);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
		
		/* Attach the pre-distortion color image textures to the "fixing" framebuffer: */
		for(GLenum i=0;i<3;++i)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT+i,GL_TEXTURE_2D,predistortionColorBufferIds[i],0);
		
		/* Check the status of the multisample "fixing" framebuffer: */
		glThrowFramebufferStatusExceptionEXT(__PRETTY_FUNCTION__,"Multisampling fixing framebuffer");
		}
	
	/* Protect the created framebuffer(s): */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	}

void VRWindowCompositorClient::init(const Misc::ConfigurationFileSection& configFileSection)
	{
	VRWindow::init(configFileSection);
	}

void VRWindowCompositorClient::releaseGLState(void)
	{
	/* Release all allocated OpenGL resources: */
	glDeleteFramebuffersEXT(1,&predistortionFrameBufferId);
	glDeleteTextures(3,predistortionColorBufferIds);
	glDeleteMemoryObjectsEXT(1,&memoryObjectId);
	if(multisamplingLevel>1)
		glDeleteRenderbuffersEXT(1,&predistortionMultisamplingColorBufferId);
	glDeleteRenderbuffersEXT(1,&predistortionDepthStencilBufferId);
	if(multisamplingLevel>1)
		glDeleteFramebuffersEXT(1,&multisamplingFrameBufferId);
	
	VRWindow::releaseGLState();
	}

int VRWindowCompositorClient::getNumVRScreens(void) const
	{
	return 2;
	}

VRScreen* VRWindowCompositorClient::getVRScreen(int index)
	{
	return screens[index];
	}

VRScreen* VRWindowCompositorClient::replaceVRScreen(int index,VRScreen* newScreen)
	{
	VRScreen* result=screens[index];
	screens[index]=newScreen;
	
	return result;
	}

int VRWindowCompositorClient::getNumViewers(void) const
	{
	return 1;
	}

Viewer* VRWindowCompositorClient::getViewer(int index)
	{
	return viewer;
	}

Viewer* VRWindowCompositorClient::replaceViewer(int index,Viewer* newViewer)
	{
	Viewer* result=viewer;
	viewer=newViewer;
	
	return result;
	}

VRWindow::InteractionRectangle VRWindowCompositorClient::getInteractionRectangle(void)
	{
	/* Calculate a coordinate frame for the viewer: */
	Point monoEyePos=viewer->getDeviceEyePosition(Viewer::MONO);
	Vector headY=viewer->getUpDirection();
	Vector headZ=-viewer->getViewDirection();
	Vector headX=headY^headZ;
	Rotation headRot=Rotation::fromBaseVectors(headX,headY);
	
	/* Calculate the bottom-leftmost and top-rightmost visible vectors in viewer space: */
	Vector bottomLeft=headRot.inverseTransform(screens[0]->getScreenTransformation().transform(Point(0,0,0))-monoEyePos);
	Vector topRight=headRot.inverseTransform(screens[1]->getScreenTransformation().transform(Point(screens[1]->getWidth(),screens[1]->getHeight(),0))-monoEyePos);
	
	/* Select an interaction plane distance and intersect the view vectors with that plane: */
	Scalar planeDist=Scalar(-1.5)*getMeterFactor(); // 1.5m away seems reasonable
	Scalar l=planeDist*bottomLeft[0]/bottomLeft[2];
	Scalar b=planeDist*bottomLeft[1]/bottomLeft[2];
	Scalar r=planeDist*topRight[0]/topRight[2];
	Scalar t=planeDist*topRight[1]/topRight[2];
	
	/* Calculate the interaction rectangle transformation: */
	InteractionRectangle result;
	result.transformation=ONTransform::translateFromOriginTo(monoEyePos);
	result.transformation*=ONTransform::rotate(headRot);
	result.transformation*=ONTransform::translate(Vector(l,b,-planeDist));
	result.transformation.renormalize();
	result.size[0]=r-l;
	result.size[1]=t-b;
	
	return result;
	}

int VRWindowCompositorClient::getNumViews(void) const
	{
	return 2;
	}

VRWindow::View VRWindowCompositorClient::getView(int index)
	{
	/* Create a view structure: */
	View result;
	result.viewport=hmdConfiguration.eyeRects[index];
	result.viewer=viewer;
	result.eye=viewer->getDeviceEyePosition(index==0?Viewer::LEFT:Viewer::RIGHT);
	result.screen=screens[index];
	writePanRect(screens[index],result.screenRect);
	
	return result;
	}

bool VRWindowCompositorClient::processEvent(const XEvent& event)
	{
	/* Intercept key events related to HMD view display: */
	bool intercepted=false;
	if(event.type==KeyPress||event.type==KeyRelease)
		{
		/* Convert event key index to keysym: */
		const XKeyEvent& keyEvent=event.xkey;
		KeySym keySym=XLookupKeysym(const_cast<XKeyEvent*>(&keyEvent),0);
		
		/* Check against control keys: */
		if(mirrorModeKey.matches(keySym,keyEvent.state))
			{
			if(event.type==KeyPress)
				{
				/* Cycle through mirroring modes: no mirroring, mirror left eye, mirror right eye */
				if(!mirrorHmd)
					{
					mirrorHmd=true;
					mirrorEyeIndex=0;
					}
				else
					{
					if(mirrorEyeIndex<2)
						++mirrorEyeIndex;
					else
						mirrorHmd=false;
					}
				}
			
			intercepted=true;
			}
		else if(mirrorFollowModeKey.matches(keySym,keyEvent.state))
			{
			/* Cycle through mirror following modes: no following, follow azimuth only, follow azimuth and elevation */
			if(event.type==KeyPress)
				{
				if(!mirrorFollowAzimuth)
					{
					mirrorFollowAzimuth=true;
					mirrorFollowElevation=false;
					}
				else
					{
					if(!mirrorFollowElevation)
						mirrorFollowElevation=true;
					else
						{
						mirrorFollowAzimuth=false;
						mirrorFollowElevation=false;
						}
					}
				}
			
			intercepted=true;
			}
		}
	
	/* If the event was not intercepted, delegate to the base class: */
	if(!intercepted)
		return VRWindow::processEvent(event);
	else
		return true;
	}

void VRWindowCompositorClient::updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const
	{
	/* No idea yet how to handle this... */
	}

void VRWindowCompositorClient::draw(void)
	{
	/* Check whether this window can be drawn at this time: */
	if(enabled&&viewer->isEnabled()&&screens[0]->isEnabled()&&screens[1]->isEnabled())
		{
		/* Update the shared display state for this window: */
		displayState->frameSize=hmdConfiguration.frameSize;
		displayState->viewer=viewer;
		
		/* Prepare for rendering: */
		prepareRender();
		
		/* Prepare the next rendering result in the VR compositor's input triple buffer: */
		RenderResult& renderResult=sharedMemorySegment->renderResults.startNewValue();
		
		/* Measure the current rendering time: */
		renderResult.renderTime.set();
		
		/* Store the head transformation used for rendering: */
		renderResult.headDeviceTransform=viewer->getHeadTransformation();
		
		/* Bind the pre-distortion framebuffer: */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,predistortionFrameBufferId);
		if(multisamplingLevel>1)
			{
			/* Draw into the multisampling image buffer: */
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
			}
		else
			{
			/* Draw directly into the next color image buffer to be submitted to the VR compositor: */
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT+renderResult.imageIndex);
			glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+renderResult.imageIndex);
			}
		
		/* Draw the left- and right-eye views: */
		glEnable(GL_SCISSOR_TEST);
		for(int eyeIndex=0;eyeIndex<2;++eyeIndex)
			{
			displayState->viewport=hmdConfiguration.eyeRects[eyeIndex];
			displayState->context.setViewport(hmdConfiguration.eyeRects[eyeIndex]);
			glScissor(hmdConfiguration.eyeRects[eyeIndex]);
			displayState->eyeIndex=eyeIndex;
			displayState->eyePosition=viewer->getEyePosition(eyeIndex==0?Viewer::LEFT:Viewer::RIGHT);
			displayState->screen=screens[eyeIndex];
			
			/* Project the virtual environment into the window: */
			render();
			}
		glDisable(GL_SCISSOR_TEST);
		
		if(multisamplingLevel>1)
			{
			/* Blit the multisampling color buffer containing the pre-distortion image into the "fixing" framebuffer: */
			glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
			if(getContext().isNonlinear())
				glEnable(GL_FRAMEBUFFER_SRGB_EXT);
			glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+renderResult.imageIndex);
			glBlitFramebufferEXT(hmdConfiguration.frameSize,hmdConfiguration.frameSize,GL_COLOR_BUFFER_BIT,GL_NEAREST);
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,0);
			}
		
		/* Unbind the pre-distortion framebuffer: */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
		
		/* If supported, insert a fence into the OpenGL command stream to wait for completion of this draw() call: */
		if(haveSync)
			drawFence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0x0);
		
		if(mirrorHmd)
			{
			/* Draw the pre-distortion image's right-eye view into the window: */
			glViewport(getWindowSize());
			
			/* Set up OpenGL state: */
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			
			/* Clear the window's color buffer: */
			glClearColor(0.0f,0.0f,0.0f,1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			/* Set up OpenGL matrices for a fixed camera with a given horizontal field of view: */
			glPushMatrix();
			glLoadIdentity();
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			Scalar vertFov=mirrorFov*Scalar(getWindowSize()[1])/Scalar(getWindowSize()[0]);
			Scalar near(0.5);
			Scalar far(4.0);
			glFrustum(-mirrorFov*near,mirrorFov*near,-vertFov*near,vertFov*near,near,far);
			
			/* Bind the pre-distortion texture: */
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,predistortionColorBufferIds[renderResult.imageIndex]);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
			
			/* Define the four corners of the mirrored eye's FoV: */
			const Scalar* eyeFov=hmdConfiguration.eyeFovs[mirrorEyeIndex];
			Point bl(eyeFov[0],eyeFov[2],-1);
			Point br(eyeFov[1],eyeFov[2],-1);
			Point tr(eyeFov[1],eyeFov[3],-1);
			Point tl(eyeFov[0],eyeFov[3],-1);
			
			/* Get the viewer's head rotation: */
			Rotation headRot=viewer->getHeadTransformation().getRotation();
			
			if(mirrorFollowAzimuth)
				{
				/* Rotate the viewer's view direction into the (y, z) plane: */
				Vector viewHorz=headRot.getDirection(2).orthogonalize(getUpDirection());
				headRot.leftMultiply(Rotation::rotateFromTo(viewHorz,Vector(0,0,1)));
				
				if(mirrorFollowElevation)
					{
					/* Rotate the viewer's view direction into the negatize z axis: */
					headRot.leftMultiply(Rotation::rotateFromTo(headRot.getDirection(2),Vector(0,0,1)));
					}
				}
			
			/* Render the pre-distortion image: */
			GLfloat left=mirrorEyeIndex==0?0.0f:0.5f;
			GLfloat right=mirrorEyeIndex==0?0.5f:1.0f;
			glBegin(GL_QUADS);
			glTexCoord2f(left,0.0f);
			glVertex(headRot.transform(bl));
			glTexCoord2f(right,0.0f);
			glVertex(headRot.transform(br));
			glTexCoord2f(right,1.0f);
			glVertex(headRot.transform(tr));
			glTexCoord2f(left,1.0f);
			glVertex(headRot.transform(tl));
			glEnd();
			
			/* Protect the pre-distortion texture: */
			glBindTexture(GL_TEXTURE_2D,0);
			glDisable(GL_TEXTURE_2D);
			
			/* Reset OpenGL matrices: */
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
			
			/* Reset OpenGL state: */
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_LIGHTING);
			}
		}
	else
		{
		/* If supported, insert a fence into the OpenGL command stream to wait for completion of this draw() call: */
		if(haveSync)
			drawFence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0x0);
		
		if(mirrorHmd)
			{
			/* Just clear the window to grey: */
			glClearColor(0.5f,0.5f,0.5f,1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			}
		}
	}

void VRWindowCompositorClient::waitComplete(void)
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
	
	/* Submit the rendered frame to the VR compositor: */
	sharedMemorySegment->renderResults.postNewValue();
	
	/* Tell the base class that rendering is done: */
	renderComplete();
	}

void VRWindowCompositorClient::present(void)
	{
	if(mirrorHmd)
		{
		/* Present the back buffer: */
		swapBuffers();
		}
	
	/* Wait for a vsync signal from the compositor, and read any that have been queued up due to missed frames: */
	unsigned char signals[64]; // This ought to be sufficient
	compositorPipe.readUpTo(signals,sizeof(signals));
	
	/* Check if this window is responsible for Vrui's frame synchronization: */
	if(synchronize)
		{
		/* Read the compositor's new vblank estimates: */
		VblankTimer vblankTimer;
		sharedMemorySegment->vblankTimer.read(vblankTimer);
		
		/* Update the Vrui kernel: */
		Vrui::vsync(vblankTimer.nextVblankTime,vblankTimer.vblankPeriod,hmdConfiguration.exposeOffset);
		}
	}

}

/***********************************************************************
VRWindowSingleViewport - Abstract base class for OpenGL windows that use
a single viewer/VR screen pair and a full-size viewport.
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

#include <Vrui/Internal/VRWindowSingleViewport.h>

#include <string>
#include <iostream>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <GL/gl.h>
#include <GL/GLContext.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Internal/Vrui.h>

namespace Vrui {

/***************************************
Methods of class VRWindowSingleViewport:
***************************************/

ISize VRWindowSingleViewport::getViewportSize(void) const
	{
	/* By default, windows render to the entire window: */
	return getWindowSize();
	}

ISize VRWindowSingleViewport::getFramebufferSize(void) const
	{
	/* By default, windows render to the entire window: */
	return getWindowSize();
	}

VRWindowSingleViewport::VRWindowSingleViewport(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindow(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection),
	 viewer(0),screen(0)
	{
	/* Find the window's viewer: */
	std::string viewerName=configFileSection.retrieveString("./viewerName");
	viewer=findViewer(viewerName.c_str());
	if(viewer==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot find viewer %s",viewerName);
	
	/* Find the window's screen: */
	std::string screenName=configFileSection.retrieveString("./screenName");
	screen=findScreen(screenName.c_str());
	if(screen==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot find screen %s",screenName);
	
	/* Check if the size of the screen should be determined automatically: */
	if(configFileSection.retrieveValue<bool>("./autoScreenSize",false))
		{
		/* Calculate the screen's current size: */
		Scalar oldSize=Math::sqrt(Math::sqr(screen->getWidth())+Math::sqr(screen->getHeight()));
		
		/* Convert the output's advertised display size from mm to physical units: */
		Scalar w=Scalar(outputConfiguration.sizeMm[0])*getMeterFactor()*Scalar(0.001);
		Scalar h=Scalar(outputConfiguration.sizeMm[1])*getMeterFactor()*Scalar(0.001);
		
		if(vruiVerbose)
			std::cout<<"\tAuto-detecting screen size as "<<outputConfiguration.sizeMm[0]<<"mm x "<<outputConfiguration.sizeMm[1]<<"mm"<<std::endl;
		
		/* Adjust the size of the screen used by this window, scaling around its center: */
		screen->setSize(w,h);
		Scalar newSize=Math::sqrt(Math::sqr(screen->getWidth())+Math::sqr(screen->getHeight()));
		
		/* Adjust the configured display size based on the screen's changed size: */
		setDisplayCenter(getDisplayCenter(),getDisplaySize()*newSize/oldSize);
		}
	
	if(vruiVerbose)
		std::cout<<"\tScreen size "<<screen->getWidth()<<'x'<<screen->getHeight()<<", aspect ratio "<<screen->getWidth()/screen->getHeight()<<std::endl;
	}

VRWindowSingleViewport::~VRWindowSingleViewport(void)
	{
	}

int VRWindowSingleViewport::getNumVRScreens(void) const
	{
	return 1;
	}

VRScreen* VRWindowSingleViewport::getVRScreen(int index)
	{
	return screen;
	}

VRScreen* VRWindowSingleViewport::replaceVRScreen(int index,VRScreen* newScreen)
	{
	VRScreen* result=screen;
	screen=newScreen;
	
	return result;
	}

int VRWindowSingleViewport::getNumViewers(void) const
	{
	return 1;
	}

Viewer* VRWindowSingleViewport::getViewer(int index)
	{
	return viewer;
	}

Viewer* VRWindowSingleViewport::replaceViewer(int index,Viewer* newViewer)
	{
	Viewer* result=viewer;
	viewer=newViewer;
	
	return result;
	}

VRWindow::InteractionRectangle VRWindowSingleViewport::getInteractionRectangle(void)
	{
	/* Create an interaction rectangle representing the current panning rectangle on the single screen: */
	InteractionRectangle result;
	result.transformation=screen->getScreenTransformation();
	Scalar screenRect[4];
	writePanRect(screen,screenRect);
	Vector origin;
	for(int i=0;i<2;++i)
		{
		result.size[i]=screenRect[2*i+1]-screenRect[2*i+0];
		origin[i]=screenRect[2*i+0];
		}
	origin[2]=Scalar(0);
	result.transformation*=ONTransform::translate(origin);
	
	return result;
	}

void VRWindowSingleViewport::updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const
	{
	/* Delegate to the common method using the full-window viewport, monoscopic eye, and only screen: */
	updateScreenDeviceCommon(windowPos,getWindowSize(),viewer->getEyePosition(Viewer::MONO),screen,device);
	}

void VRWindowSingleViewport::draw(void)
	{
	/* Set up the viewport: */
	displayState->viewport=getWindowSize();
	displayState->context.setViewport(displayState->viewport);
	
	/* Check whether this window can be drawn at this time: */
	if(enabled&&viewer->isEnabled()&&screen->isEnabled())
		{
		/* Continue updating the shared display state for this window: */
		displayState->frameSize=getWindowSize();
		displayState->viewer=viewer;
		displayState->screen=screen;
		
		/* Prepare for rendering: */
		prepareRender();
		
		/* Call the inner draw method: */
		drawInner(true);
		}
	else
		{
		/* Call the inner draw method: */
		drawInner(false);
		}
	
	/* If supported, insert a fence into the OpenGL command stream to wait for completion of this draw() call: */
	if(haveSync)
		drawFence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0x0);
	}

void VRWindowSingleViewport::waitComplete(void)
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

void VRWindowSingleViewport::present(void)
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

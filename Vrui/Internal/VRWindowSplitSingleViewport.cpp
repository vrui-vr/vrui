/***********************************************************************
VRWindowSplitSingleViewport - Class for OpenGL windows that render side-
by-side stereoscopic views using a single viewer and screen.
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

#include <Vrui/Internal/VRWindowSplitSingleViewport.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <GL/gl.h>
#include <GL/GLMiscTemplates.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContext.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/DisplayState.h>

namespace Vrui {

/********************************************
Methods of class VRWindowSplitSingleViewport:
********************************************/

ISize VRWindowSplitSingleViewport::getViewportSize(void) const
	{
	/* Return a size that encompasses both viewports: */
	return Misc::max(viewports[0].size,viewports[1].size);
	}

void VRWindowSplitSingleViewport::drawInner(bool canDraw)
	{
	/* Is not called */
	}

VRWindowSplitSingleViewport::VRWindowSplitSingleViewport(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindowSingleViewport(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection)
	{
	/* Read the left and right viewport rectangles: */
	viewports[0]=configFileSection.retrieveValue<Rect>("./leftViewportPos");
	viewports[1]=configFileSection.retrieveValue<Rect>("./rightViewportPos");
	}

VRWindowSplitSingleViewport::~VRWindowSplitSingleViewport(void)
	{
	}

int VRWindowSplitSingleViewport::getNumViews(void) const
	{
	return 2;
	}

VRWindow::View VRWindowSplitSingleViewport::getView(int index)
	{
	/* Create a view structure: */
	View result;
	result.viewport=viewports[index];
	result.viewer=viewer;
	result.eye=viewer->getDeviceEyePosition(index==0?Viewer::LEFT:Viewer::RIGHT);
	result.screen=screen;
	writePanRect(screen,result.screenRect);
	
	return result;
	}

void VRWindowSplitSingleViewport::updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const
	{
	/* Find the viewport that contains the given window position: */
	Rect::Offset wPos;
	for(int i=0;i<2;++i)
		wPos[i]=int(windowPos[i]);
	if(viewports[0].contains(wPos))
		{
		/* Delegate to the common method using the left-eye viewport, monoscopic eye, and only screen: */
		updateScreenDeviceCommon(windowPos,viewports[0],viewer->getEyePosition(Viewer::MONO),screen,device);
		}
	else if(viewports[1].contains(wPos))
		{
		/* Delegate to the common method using the right-eye viewport, monoscopic eye, and only screen: */
		updateScreenDeviceCommon(windowPos,viewports[1],viewer->getEyePosition(Viewer::MONO),screen,device);
		}
	}

void VRWindowSplitSingleViewport::draw(void)
	{
	/* Check whether this window can be drawn at this time: */
	if(enabled&&viewer->isEnabled()&&screen->isEnabled())
		{
		/* Update the shared display state for this window: */
		displayState->frameSize=getWindowSize();
		displayState->viewer=viewer;
		displayState->screen=screen;
		
		/* Prepare for rendering: */
		prepareRender();
		
		/* Set up buffers: */
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);
		
		/* Clear the entire window if it has been resized: */
		if(resized)
			{
			displayState->context.setViewport(getWindowSize());
			glClearColor(disabledColor);
			if(clearBufferMask&GL_STENCIL_BUFFER_BIT)
				glStencilMask(~0x0U);
			glClear(clearBufferMask);
			}
		
		/* Draw the left- and right-eye views: */
		glEnable(GL_SCISSOR_TEST);
		for(int eyeIndex=0;eyeIndex<2;++eyeIndex)
			{
			displayState->viewport=viewports[eyeIndex];
			displayState->context.setViewport(viewports[eyeIndex]);
			glScissor(viewports[eyeIndex]);
			displayState->eyeIndex=eyeIndex;
			displayState->eyePosition=viewer->getEyePosition(eyeIndex==0?Viewer::LEFT:Viewer::RIGHT);
			
			/* Project the virtual environment into the window: */
			render();
			}
		glDisable(GL_SCISSOR_TEST);
		}
	else
		{
		/* Set the viewport to the entire window: */
		displayState->context.setViewport(getWindowSize());
		
		/* Set up buffers: */
		glDrawBuffer(GL_BACK);
		
		/* Clear the window's color buffer: */
		glClearColor(disabledColor);
		glClear(GL_COLOR_BUFFER_BIT);
		}
	
	/* If supported, insert a fence into the OpenGL command stream to wait for completion of this draw() call: */
	if(haveSync)
		drawFence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0x0);
	}

}

/***********************************************************************
VRWindowQuadbuffer - Class for OpenGL windows that render stereoscopic
views using OpenGL quadbuffers.
Copyright (c) 2004-2023 Oliver Kreylos

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

#include <Vrui/Internal/VRWindowQuadbuffer.h>

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <Vrui/Viewer.h>
#include <Vrui/DisplayState.h>

namespace Vrui {
extern bool vruiVerbose;
}

namespace Vrui {

/***********************************
Methods of class VRWindowQuadbuffer:
***********************************/

void VRWindowQuadbuffer::drawInner(bool canDraw)
	{
	if(canDraw)
		{
		/* Render the left-eye view into the left back buffer: */
		glDrawBuffer(GL_BACK_LEFT);
		glReadBuffer(GL_BACK_LEFT);
		displayState->eyeIndex=0;
		displayState->eyePosition=viewer->getEyePosition(Viewer::LEFT);
		render();
		
		/* Render the right-eye view into the right back buffer: */
		glDrawBuffer(GL_BACK_RIGHT);
		glReadBuffer(GL_BACK_RIGHT);
		displayState->eyeIndex=1;
		displayState->eyePosition=viewer->getEyePosition(Viewer::RIGHT);
		render();
		}
	else
		{
		/* Clear the left and right back buffers: */
		glClearColor(disabledColor);
		glDrawBuffer(GL_BACK_LEFT);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawBuffer(GL_BACK_RIGHT);
		glClear(GL_COLOR_BUFFER_BIT);
		}
	}

VRWindowQuadbuffer::VRWindowQuadbuffer(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindowSingleViewport(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection)
	{
	}

VRWindowQuadbuffer::~VRWindowQuadbuffer(void)
	{
	}

int VRWindowQuadbuffer::getNumViews(void) const
	{
	return 2;
	}

VRWindow::View VRWindowQuadbuffer::getView(int index)
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

/***********************************************************************
VRWindowMono - Class for OpenGL windows that render a monoscopic view.
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

#include <Vrui/Internal/VRWindowMono.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/ConfigurationFile.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <Vrui/DisplayState.h>

namespace Vrui {

/*****************************
Methods of class VRWindowMono:
*****************************/

void VRWindowMono::drawInner(bool canDraw)
	{
	if(canDraw)
		{
		/* Update the shared display state for this window: */
		displayState->eyeIndex=0;
		displayState->eyePosition=viewer->getEyePosition(eye);
		
		/* Set up buffers: */
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);
		
		/* Project the virtual environment into the window: */
		render();
		}
	else
		{
		/* Set up buffers: */
		glDrawBuffer(GL_BACK);
		
		/* Clear the window's color buffer: */
		glClearColor(disabledColor);
		glClear(GL_COLOR_BUFFER_BIT);
		}
	}

VRWindowMono::VRWindowMono(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindowSingleViewport(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection),
	 eye(Viewer::MONO)
	{
	/* Determine which of the viewer's eyes to use to render this window: */
	std::string windowType=configFileSection.retrieveString("./windowType");
	if(windowType=="Mono")
		eye=Viewer::MONO;
	else if(windowType=="LeftEye")
		eye=Viewer::LEFT;
	else if(windowType=="RightEye")
		eye=Viewer::RIGHT;
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unkown window type %s",windowType.c_str());
	}

VRWindowMono::~VRWindowMono(void)
	{
	}

int VRWindowMono::getNumViews(void) const
	{
	return 1;
	}

VRWindow::View VRWindowMono::getView(int index)
	{
	/* Create a view structure: */
	View result;
	result.viewport=getWindowSize();
	result.viewer=viewer;
	result.eye=viewer->getDeviceEyePosition(eye);
	result.screen=screen;
	writePanRect(screen,result.screenRect);
	
	return result;
	}

}

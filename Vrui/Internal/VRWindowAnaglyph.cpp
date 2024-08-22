/***********************************************************************
VRWindowAnaglyph - Class for OpenGL windows that render a anaglyph 
tereoscopic view.
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

#include <Vrui/Internal/VRWindowAnaglyph.h>

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <Vrui/Viewer.h>
#include <Vrui/DisplayState.h>

namespace Vrui {
extern bool vruiVerbose;
}

namespace Vrui {

/*********************************
Methods of class VRWindowAnaglyph:
*********************************/

void VRWindowAnaglyph::drawInner(bool canDraw)
	{
	if(canDraw)
		{
		/* Set up buffers: */
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);
		
		/* Render the left-eye view into the red color channel: */
		displayState->eyeIndex=0;
		displayState->eyePosition=viewer->getEyePosition(Viewer::LEFT);
		glColorMask(GL_TRUE,GL_FALSE,GL_FALSE,GL_TRUE);
		render();
		
		/* Render the right-eye view into the green and blue color channels: */
		displayState->eyeIndex=1;
		displayState->eyePosition=viewer->getEyePosition(Viewer::RIGHT);
		glColorMask(GL_FALSE,GL_TRUE,GL_TRUE,GL_TRUE);
		render();
		
		/* Reset the default color mask: */
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
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

VRWindowAnaglyph::VRWindowAnaglyph(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:VRWindowSingleViewport(sContext,sOutputConfiguration,windowName,initialRect,decorate,configFileSection)
	{
	}

VRWindowAnaglyph::~VRWindowAnaglyph(void)
	{
	}

int VRWindowAnaglyph::getNumViews(void) const
	{
	return 2;
	}

VRWindow::View VRWindowAnaglyph::getView(int index)
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

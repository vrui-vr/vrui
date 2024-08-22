/***********************************************************************
VRWindowSplitSingleViewport - Class for OpenGL windows that render side-
by-side stereoscopic views using a single viewer and screen.
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

#ifndef VRUI_VRWINDOWSPLITSINGLEVIEWPORT_INCLUDED
#define VRUI_VRWINDOWSPLITSINGLEVIEWPORT_INCLUDED

#include <Vrui/Viewer.h>
#include <Vrui/Internal/VRWindowSingleViewport.h>

namespace Vrui {

class VRWindowSplitSingleViewport:public VRWindowSingleViewport
	{
	/* Elements: */
	private:
	Rect viewports[2]; // The left- and right-view viewports in window coordinates
	
	/* Protected methods from class VRWindow: */
	virtual ISize getViewportSize(void) const;
	
	/* Protected methods from class VRWindowSingleViewport: */
	protected:
	virtual void drawInner(bool canDraw);
	
	/* Constructors and destructors: */
	public:
	VRWindowSplitSingleViewport(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~VRWindowSplitSingleViewport(void);
	
	/* Methods from class VRWindow: */
	virtual int getNumViews(void) const;
	virtual View getView(int index);
	virtual void updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const;
	virtual void draw(void);
	};

}

#endif

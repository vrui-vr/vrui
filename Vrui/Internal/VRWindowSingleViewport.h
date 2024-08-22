/***********************************************************************
VRWindowSingleViewport - Abstract base class for OpenGL windows that use
a single viewer/VR screen pair and a full-size viewport.
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

#ifndef VRUI_VRWINDOWSINGLEVIEWPORT_INCLUDED
#define VRUI_VRWINDOWSINGLEVIEWPORT_INCLUDED

#include <Vrui/VRWindow.h>

/* Forward declarations: */
namespace Vrui {
class Viewer;
class VRScreen;
}

namespace Vrui {

class VRWindowSingleViewport:public VRWindow
	{
	/* Elements: */
	protected:
	Viewer* viewer; // Viewer from which to project the virtual environment
	VRScreen* screen; // Screen onto which to project the virtual environment
	
	/* Protected methods from class VRWindow: */
	protected:
	virtual ISize getViewportSize(void) const;
	virtual ISize getFramebufferSize(void) const;
	
	/* New protected methods: */
	virtual void drawInner(bool canDraw) =0; // Does the actual rendering after display state and OpenGL have been set up
	
	/* Constructors and destructors: */
	public:
	VRWindowSingleViewport(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~VRWindowSingleViewport(void);
	
	/* Methods from class VRWindow: */
	virtual int getNumVRScreens(void) const;
	virtual VRScreen* getVRScreen(int index);
	virtual VRScreen* replaceVRScreen(int index,VRScreen* newVRScreen);
	virtual int getNumViewers(void) const;
	virtual Viewer* getViewer(int index);
	virtual Viewer* replaceViewer(int index,Viewer* newViewer);
	virtual InteractionRectangle getInteractionRectangle(void);
	virtual void updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const;
	virtual void draw(void);
	virtual void waitComplete(void);
	virtual void present(void);
	};

}

#endif

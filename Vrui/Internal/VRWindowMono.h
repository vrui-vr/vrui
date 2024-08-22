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

#ifndef VRUI_VRWINDOWMONO_INCLUDED
#define VRUI_VRWINDOWMONO_INCLUDED

#include <Vrui/Viewer.h>
#include <Vrui/Internal/VRWindowSingleViewport.h>

namespace Vrui {

class VRWindowMono:public VRWindowSingleViewport
	{
	/* Elements: */
	protected:
	Viewer::Eye eye; // Which of the viewer's eyes to use for projection
	
	/* Protected methods from class VRWindowSingleViewport: */
	virtual void drawInner(bool canDraw);
	
	/* Constructors and destructors: */
	public:
	VRWindowMono(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~VRWindowMono(void);
	
	/* Methods from class VRWindow: */
	virtual int getNumViews(void) const;
	virtual View getView(int index);
	};

}

#endif

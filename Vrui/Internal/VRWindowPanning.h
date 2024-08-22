/***********************************************************************
VRWindowPanning - Abstract base class for VR windows that update their
associated screens when being moved or resized.
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

#ifndef VRUI_VRWINDOWPANNING_INCLUDED
#define VRUI_VRWINDOWPANNING_INCLUDED

#include <Vrui/VRWindow.h>

namespace Vrui {

class VRWindowPanning:public VRWindow
	{
	/* Elements: */
	protected:
	ONTransform domainTransform; // Transformation representing the window's panning domain in physical space, with the origin in the lower-left corner
	Scalar domainSize[2]; // Width and height of the window's panning domain in physical coordinate units
	bool navigate; // Flag if the window should drag navigation space along when it is moved/resized
	bool movePrimaryWidgets; // Flag if the window should drag along primary popped-up widgets when it is moved/resized
	bool trackToolKillZone; // Flag if the tool manager's tool kill zone should follow the window when moved/resized
	Scalar toolKillZonePos[2]; // Position of tool kill zone in relative window coordinates (0.0-1.0 in both directions)
	
	/* Constructors and destructors: */
	public:
	VRWindowPanning(VRWindow(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,const Misc::ConfigurationFileSection& configFileSection,VruiState* sVruiState,InputDeviceAdapterMouse* sMouseAdapter);
	};

}

#endif

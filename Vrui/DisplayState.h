/***********************************************************************
DisplayState - Class to store Vrui rendering state in a GLContextData
object so it can be queried by applications from inside their display
methods. Workaround until a "proper" method to pass display state into
applications is found.
Copyright (c) 2009-2023 Oliver Kreylos

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

#ifndef VRUI_DISPLAYSTATE_INCLUDED
#define VRUI_DISPLAYSTATE_INCLUDED

#include <Misc/Size.h>
#include <Misc/Rect.h>
#include <Geometry/Point.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <Vrui/Types.h>

/* Forward declarations: */
class GLContext;
namespace Vrui {
class VRWindow;
class Viewer;
class VRScreen;
}

namespace Vrui {

class DisplayState
	{
	/* Elements: */
	public:
	GLContext& context; // The shared OpenGL context
	const VRWindow* window; // The VR window being rendered to
	int windowIndex; // The index of the above VR window in the environment's complete window list
	IRect viewport; // The window's current viewport
	ISize frameSize; // The window's current frame buffer size
	ISize maxViewportSize; // The maximum viewport size of all windows in the current window's window group
	ISize maxFrameSize; // The maximum frame buffer size of all windows in the current window's window group
	bool resized; // Flag whether the VR window has changed size since the last redraw
	const Viewer* viewer; // The viewer whose view is currently rendered
	int eyeIndex; // Index of the eye currently projected from
	Point eyePosition; // Exact eye position used for projection
	const VRScreen* screen; // The screen onto which the viewer's view is projected
	PTransform projection; // Projection transformation
	NavTransform modelviewPhysical; // Model view transformation for physical coordinates
	Scalar mvpGl[4*4]; // Model view transformation for physical coordinates as 4x4 column-major matrix for OpenGL
	NavTransform modelviewNavigational; // Model view transformation for navigational coordinates
	Scalar mvnGl[4*4]; // Model view transformation for navigational coordinates as 4x4 column-major matrix for OpenGL
	
	/* Constructors and destructors: */
	DisplayState(GLContext& sContext)
		:context(sContext)
		{
		}
	};

}

#endif

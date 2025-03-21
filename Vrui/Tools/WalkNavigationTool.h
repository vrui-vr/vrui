/***********************************************************************
WalkNavigationTool - Class to navigate in a VR environment by walking
around a fixed center position.
Copyright (c) 2007-2025 Oliver Kreylos

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

#ifndef VRUI_WALKNAVIGATIONTOOL_INCLUDED
#define VRUI_WALKNAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/ONTransformNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/NavigationTool.h>

/* Forward declarations: */
class GLContextData;

namespace Vrui {

class WalkNavigationTool;

class WalkNavigationToolFactory:public ToolFactory
	{
	friend class WalkNavigationTool;
	
	/* Elements: */
	private:
	bool centerOnActivation; // Flag whether to center navigation on the head position when the tool is activated
	Point centerPoint; // Center point of movement circles on floor
	Scalar moveSpeed; // Maximum movement speed
	Scalar innerRadius; // Radius of circle of no motion around center point
	Scalar outerRadius; // Radius where maximum movement speed is reached
	Vector centerViewDirection; // Central view direction
	Scalar rotateSpeed; // Maximum rotation speed in radians per second
	Scalar innerAngle; // Angle of no rotation around central view direction
	Scalar outerAngle; // Angle where maximum rotation speed is reached
	bool drawMovementCircles; // Flag whether to draw the movement circles
	Color movementCircleColor; // Color for drawing movement circles
	
	/* Constructors and destructors: */
	public:
	WalkNavigationToolFactory(ToolManager& toolManager);
	virtual ~WalkNavigationToolFactory(void);
	
	/* Methods from class ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class WalkNavigationTool:public NavigationTool
	{
	friend class WalkNavigationToolFactory;
	
	/* Elements: */
	private:
	static WalkNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	SceneGraph::ONTransformNodePointer circleRoot; // Pointer to the root transform node for the tool's movement circles
	
	/* Transient navigation state: */
	Point centerPoint; // Center point of movement circle while the navigation tool is active
	Vector centerViewDirection; // Central view direction while the navigation tool is active
	NavTransform preScale; // Previous navigation transformation
	Vector translation; // Total accumulated translation
	Scalar azimuth; // Total accumulated rotation around up axis
	
	/* Private methods: */
	void showMovementCircles(void);
	
	/* Constructors and destructors: */
	public:
	WalkNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from class Tool: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif

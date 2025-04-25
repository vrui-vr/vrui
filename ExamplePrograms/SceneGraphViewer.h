/***********************************************************************
SceneGraphViewer - Viewer for one or more scene graphs loaded from VRML
2.0 or binary scene graph files.
Copyright (c) 2010-2025 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPHVIEWER_INCLUDED
#define SCENEGRAPHVIEWER_INCLUDED

#include <string>
#include <GLMotif/ToggleButton.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/SceneGraphList.h>
#include <Vrui/Application.h>
#include <Vrui/ToolManager.h>
#include <Vrui/SurfaceNavigationTool.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class PopupMenu;
}

class SceneGraphViewer:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef SceneGraph::Scalar Scalar;
	typedef SceneGraph::Point Point;
	typedef SceneGraph::Vector Vector;
	
	struct AlignmentState:public Vrui::SurfaceNavigationTool::AlignmentState // Structure to keep track of continuing surface-aligned navigation sequences
		{
		public:
		Scalar height; // User height in navigational space from previous frame
		Scalar floorLift; // Amount by which the floor was artificially lifted on the previous frame to correct lack of headroom
		};
	
	class WalkNavigationToolFactory;
	class WalkNavigationTool;
	
	class TransformToolFactory;
	class TransformTool;
	
	class SurfaceTouchTransformToolFactory;
	class SurfaceTouchTransformTool;
	
	/* Elements: */
	private:
	SceneGraph::SceneGraphList physicalSceneGraphs; // List of scene graphs in physical space
	SceneGraph::SceneGraphList navigationalSceneGraphs; // List of scene graphs in navigational space
	GLMotif::PopupMenu* mainMenu;
	
	/* Private methods: */
	void goToPhysicalSpaceCallback(Misc::CallbackData* cbData);
	void showPhysicalSceneGraphListCallback(Misc::CallbackData* cbData);
	void showNavigationalSceneGraphListCallback(Misc::CallbackData* cbData);
	void alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData);
	
	/* Constructors and destructors: */
	public:
	SceneGraphViewer(int& argc,char**& argv);
	~SceneGraphViewer(void);
	
	/* Methods: */
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

#endif

/***********************************************************************
SceneGraphList - Helper class to manage a dynamic list of scene graphs
collected under a common root node.
Copyright (c) 2025 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_SCENEGRAPHLIST_INCLUDED
#define SCENEGRAPH_SCENEGRAPHLIST_INCLUDED

#include <string>
#include <vector>
#include <IO/Directory.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/ListBox.h>
#include <SceneGraph/GraphNode.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/NodeCreator.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class WidgetManager;
class PopupWindow;
}

namespace SceneGraph {

class SceneGraphList
	{
	/* Embedded classes: */
	private:
	struct SGItem // Structure holding state pertaining to a scene graph managed by this class
		{
		/* Elements: */
		public:
		std::string fileName; // Name of the file from which the scene graph was loaded
		GraphNodePointer sceneGraph; // Pointer to the scene graph's root node
		bool enabled; // Flag if the scene graph is currently enabled, i.e., a child of the common root node
		
		/* Constructors and destructors: */
		SGItem(const std::string& sFileName,GraphNode& sSceneGraph,bool sEnabled) // Element-wise constructor
			:fileName(sFileName),sceneGraph(&sSceneGraph),enabled(sEnabled)
			{
			}
		};
	
	/* Elements: */
	private:
	GroupNodePointer root; // Common root node of all scene graphs managed by this object
	NodeCreator nodeCreator; // A node creator to load scene graph files
	std::vector<SGItem> sceneGraphs; // List of scene graphs currently managed by this object
	IO::DirectoryPtr currentDirectory; // Last directory from which a scene graph file was loaded
	GLMotif::PopupWindow* sceneGraphDialog; // Dialog window to manipulate the list of scene graphs
	GLMotif::ListBox* sceneGraphList; // List box containing the scene graph file names
	GLMotif::ToggleButton* enableToggle; // Toggle button to enable/disable individual scene graphs
	GLMotif::Button* removeSceneGraphButton; // Button to remove a scene graph from the list
	
	/* Private methods: */
	void sceneGraphListValueChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData); // Callback called when the scene graph list box changes
	void sceneGraphListItemSelectedCallback(GLMotif::ListBox::ItemSelectedCallbackData* cbData); // Callback called when a scene graph list box item is selected
	void addSceneGraphButtonSelectedCallback(Misc::CallbackData* cbData); // Callback called when the "Add Scene Graph" button is selected
	void enableToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData); // Callback called when the enable/disable toggle changes
	void removeSceneGraphButtonSelectedCallback(Misc::CallbackData* cbData); // Callback called when the "Remove Scene Graph" button is selected
	
	/* Constructors and destructors: */
	public:
	SceneGraphList(GroupNode& sRoot); // Creates a scene graph viewer managing its scene graphs under the given root node
	~SceneGraphList(void);
	
	/* Methods: */
	IO::Directory& getCurrentDirectory(void) // Returns the directory from which the last scene graph file was loaded
		{
		return *currentDirectory;
		}
	GraphNodePointer addSceneGraph(IO::Directory& directory,const char* fileName); // Adds a new scene graph to the viewer by reading the file of the given name relative to the given directory; remembers the given directory as the current; returns the loaded scene graph
	GraphNodePointer addSceneGraph(const char* fileName) // Adds a new scene graph to the viewer by reading the file of the given name relative to the current directory
		{
		/* Delegate to the other method: */
		return addSceneGraph(*currentDirectory,fileName);
		}
	GLMotif::PopupWindow* createSceneGraphDialog(GLMotif::WidgetManager& widgetManager); // Returns a dialog window to manipulate the list of scene graphs
	void destroySceneGraphDialog(void); // Destroys a previously created scene graph list dialog window
	};

}

#endif

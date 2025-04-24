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

#include <SceneGraph/SceneGraphList.h>

#include <string.h>
#include <Misc/StdError.h>
#include <Misc/FileNameExtensions.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Button.h>
#include <GLMotif/ScrolledListBox.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>

namespace SceneGraph {

/*******************************
Methods of class SceneGraphList:
*******************************/

void SceneGraphList::sceneGraphListValueChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData)
	{
	}

void SceneGraphList::sceneGraphListItemSelectedCallback(GLMotif::ListBox::ItemSelectedCallbackData* cbData)
	{
	}

void SceneGraphList::addSceneGraphButtonSelectedCallback(Misc::CallbackData* cbData)
	{
	}

void SceneGraphList::enableToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	}

void SceneGraphList::removeSceneGraphButtonSelectedCallback(Misc::CallbackData* cbData)
	{
	}

SceneGraphList::SceneGraphList(SceneGraph::GroupNode& sRoot)
	:root(&sRoot),
	 sceneGraphDialog(0)
	{
	}

SceneGraphList::~SceneGraphList(void)
	{
	/* Remove all currently enabled scene graphs from the common root node: */
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt)
		if(sgIt->enabled)
			root->removeChild(*sgIt->sceneGraph);
	
	/* Destroy the scene graph list dialog: */
	delete sceneGraphDialog;
	}

GraphNodePointer SceneGraphList::addSceneGraph(IO::Directory& directory,const char* fileName)
	{
	GraphNodePointer sceneGraph;
	
	/* Check if the scene graph file is a binary file or a VRML 2.0 file: */
	if(Misc::hasCaseExtension(fileName,".bwrl"))
		{
		/* Load a binary scene graph file: */
		SceneGraphReader reader(directory.openFile(fileName),nodeCreator);
		sceneGraph=reader.readTypedNode<GraphNode>();
		}
	else if(Misc::hasCaseExtension(fileName,".wrl"))
		{
		/* Create a new group node as the root for the VRML 2.0 scene graph file: */
		GroupNodePointer root=new GroupNode;
		sceneGraph=root;
		
		/* Open and parse the VRML v2.0 scene graph file: */
		VRMLFile vrmlFile(directory,fileName,nodeCreator);
		vrmlFile.parse(*root);
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scene graph file name has unrecognized extension %s",Misc::getExtension(fileName));
	
	/* Add the scene graph to the common root node: */
	root->addChild(*sceneGraph);
	
	/* Add the scene graph to the list of scene graphs: */
	sceneGraphs.push_back(SGItem(fileName,*sceneGraph,true));
	
	/* Add the scene graph to the scene graph dialog: */
	if(sceneGraphDialog!=0)
		{
		/* Sort the scene graph into the list by file name in alphabetical order: */
		int insertPos=sceneGraphList->getNumItems();
		while(insertPos>0&&strcasecmp(sceneGraphList->getItem(insertPos-1),fileName)>0)
			--insertPos;
		sceneGraphList->insertItem(insertPos,fileName);
		
		/* Select the just-inserted item and bring it to the visible page: */
		sceneGraphList->selectItem(insertPos,true);
		
		/* Update the rest of the dialog box: */
		...
		}
	
	/* Update the current directory: */
	currentDirectory=&directory;
	
	return sceneGraph;
	}

GLMotif::PopupWindow* SceneGraphList::createSceneGraphDialog(GLMotif::WidgetManager& widgetManager)
	{
	/* If the scene graph dialog already exists, return it: */
	if(sceneGraphDialog!=0)
		return sceneGraphDialog;
	
	/* Retrieve the GLMotif style sheet: */
	const GLMotif::StyleSheet& ss=*widgetManager.getStyleSheet();
	
	/* Create the scene graph dialog window pop-up: */
	sceneGraphDialog=new GLMotif::PopupWindow("SceneGraphDialog",&widgetManager,"Scene Graph List");
	sceneGraphDialog->setHideButton(true);
	sceneGraphDialog->setCloseButton(true);
	sceneGraphDialog->setResizableFlags(true,true);
	
	/* Create the main dialog panel with a scrolled list box on the left and a button panel on the right: */
	GLMotif::RowColumn* dialog=new GLMotif::RowColumn("Dialog",sceneGraphDialog,false);
	dialog->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	dialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	dialog->setNumMinorWidgets(1);
	
	/* Create the scene graph list box: */
	GLMotif::ScrolledListBox* sceneGraphScrolledList=new GLMotif::ScrolledListBox("SceneGraphScrolledList",dialog,GLMotif::ListBox::ALWAYS_ONE,40,10);
	sceneGraphList=sceneGraphScrolledList->getListBox();
	sceneGraphList->getValueChangedCallbacks().add(this,&SceneGraphList::sceneGraphListValueChangedCallback);
	sceneGraphList->getItemSelectedCallbacks().add(this,&SceneGraphList::sceneGraphListItemSelectedCallback);
	
	/* Add all currently managed scene graphs to the list box: */
	int numListItems=0;
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt)
		{
		/* Sort the scene graph into the list by file name in alphabetical order: */
		int insertPos=numListItems;
		while(insertPos>0&&strcasecmp(sceneGraphList->getItem(insertPos-1),sgIt->fileName.c_str())>0)
			--insertPos;
		sceneGraphList->insertItem(insertPos,sgIt->fileName.c_str());
		}
	
	/* Create the button panel: */
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",dialog,false);
	buttonMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::TOP));
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
	buttonBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	buttonBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	buttonBox->setNumMinorWidgets(1);
	
	/* Create a button to add another scene graph: */
	GLMotif::Button* addSceneGraphButton=new GLMotif::Button("AddSceneGraphButton",buttonBox,"Add Scene Graph...");
	addSceneGraphButton->getSelectCallbacks().add(this,&SceneGraphList::addSceneGraphButtonSelectedCallback);
	
	/* Create the enable/disable toggle: */
	enableToggle=new GLMotif::ToggleButton("EnableToggle",buttonBox,"Enabled");
	enableToggle->setToggle(true);
	enableToggle->getValueChangedCallbacks().add(this,&SceneGraphList::enableToggleValueChangedCallback);
	if(sceneGraphs.empty())
		enableToggle->setEnabled(false);
	
	/* Create a button to remove the selected scene graph: */
	removeSceneGraphButton=new GLMotif::Button("RemoveSceneGraphButton",buttonBox,"Remove Scene Graph");
	removeSceneGraphButton->getSelectCallbacks().add(this,&SceneGraphList::removeSceneGraphButtonSelectedCallback);
	if(sceneGraphs.empty())
		removeSceneGraphButton->setEnabled(false);
	
	buttonBox->manageChild();
	
	buttonMargin->manageChild();
	
	dialog->manageChild();
	
	return sceneGraphDialog;
	}

void SceneGraphList::destroySceneGraphDialog(void)
	{
	if(sceneGraphDialog!=0)
		delete sceneGraphDialog;
	sceneGraphDialog=0;
	}

}

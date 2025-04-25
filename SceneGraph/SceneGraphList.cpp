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
#include <stdexcept>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/MessageLogger.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Separator.h>
#include <GLMotif/Button.h>
#include <GLMotif/ScrolledListBox.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>

namespace SceneGraph {

/*******************************
Methods of class SceneGraphList:
*******************************/

SceneGraph::GraphNodePointer SceneGraphList::loadSceneGraph(IO::Directory& directory,const char* fileName)
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
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scene graph file name %s has unrecognized extension",fileName);
	
	return sceneGraph;
	}

void SceneGraphList::sceneGraphListValueChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData)
	{
	/* Ignore this callback if it is not due to direct user action: */
	if(cbData->interactive)
		{
		/* Select the affected scene graph list item: */
		SGItem& item=sceneGraphs[cbData->newSelectedItem];
		
		/* Update the rest of the dialog to reflect the newly-selected list item: */
		enableToggle->setToggle(item.enabled);
		}
	}

void SceneGraphList::sceneGraphListItemSelectedCallback(GLMotif::ListBox::ItemSelectedCallbackData* cbData)
	{
	/* Select the affected scene graph list item: */
	SGItem& item=sceneGraphs[cbData->selectedItem];
	
	/* Toggle the selected item's enabled state: */
	item.enabled=!item.enabled;
	if(item.enabled)
		{
		/* Add the list item's scene graph to the common root node: */
		root->addChild(*item.sceneGraph);
		}
	else
		{
		/* Remove the list item's scene graph from the common root node: */
		root->removeChild(*item.sceneGraph);
		}
	
	/* Update the rest of the dialog: */
	if(cbData->listBox->getSelectedItem()==cbData->selectedItem)
		enableToggle->setToggle(item.enabled);
	}

void SceneGraphList::addSceneGraphOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Re-enable the "Add Scene Graph..." button: */
	addSceneGraphButton->setEnabled(true);
	
	try
		{
		/* Add the selected scene graph: */
		addSceneGraph(*cbData->selectedDirectory,cbData->selectedFileName);
		}
	catch(const std::runtime_error& err)
		{
		/* Show an error message: */
		Misc::formattedUserError(__PRETTY_FUNCTION__,"Cannot open a file selection dialog for directory %s due to exception %s",currentDirectory->getPath().c_str(),err.what());
		}
	
	/* Close the file selection dialog: */
	cbData->fileSelectionDialog->close();
	}

void SceneGraphList::addSceneGraphCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
	{
	/* Re-enable the "Add Scene Graph..." button: */
	addSceneGraphButton->setEnabled(true);
	
	/* Close the file selection dialog: */
	cbData->fileSelectionDialog->close();
	}

void SceneGraphList::addSceneGraphButtonSelectedCallback(Misc::CallbackData* cbData)
	{
	try
		{
		/* Get a handle to the widget manager: */
		GLMotif::WidgetManager* widgetManager=sceneGraphDialog->getManager();
		
		/* Open a file selection dialog to select a binary or VRML V2.0 scene graph file: */
		Misc::SelfDestructPointer<GLMotif::FileSelectionDialog> fileSelectionDialog(new GLMotif::FileSelectionDialog(widgetManager,"Load Scene Graph",currentDirectory,".bwrl;.wrl"));
		fileSelectionDialog->getOKCallbacks().add(this,&SceneGraphList::addSceneGraphOKCallback);
		fileSelectionDialog->getCancelCallbacks().add(this,&SceneGraphList::addSceneGraphCancelCallback);
		widgetManager->popupPrimaryWidget(fileSelectionDialog.releaseTarget());
		
		/* Disable the "Add Scene Graph..." button until the user finishes the current task: */
		addSceneGraphButton->setEnabled(false);
		}
	catch(const std::runtime_error& err)
		{
		/* Show an error message: */
		Misc::formattedUserError(__PRETTY_FUNCTION__,"Cannot open a file selection dialog for directory %s due to exception %s",currentDirectory->getPath().c_str(),err.what());
		}
	}

void SceneGraphList::enableToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Select the affected scene graph list item: */
	SGItem& item=sceneGraphs[sceneGraphList->getSelectedItem()];
	
	/* Update the selected item's enabled state: */
	if(item.enabled!=cbData->set)
		{
		item.enabled=cbData->set;
		if(item.enabled)
			{
			/* Add the list item's scene graph to the common root node: */
			root->addChild(*item.sceneGraph);
			}
		else
			{
			/* Remove the list item's scene graph from the common root node: */
			root->removeChild(*item.sceneGraph);
			}
		}
	}

void SceneGraphList::reloadSceneGraphButtonSelectedCallback(Misc::CallbackData* cbData)
	{
	/* Select the affected scene graph list item: */
	int itemIndex=sceneGraphList->getSelectedItem();
	SGItem& item=sceneGraphs[itemIndex];
	
	try
		{
		/* Load a new copy of the scene graph from the original file: */
		SceneGraph::GraphNodePointer newSceneGraph=loadSceneGraph(*item.directory,item.fileName.c_str());
		
		/* Remove the item's original scene graph from the common root node if it is currently enabled: */
		if(item.enabled)
			root->removeChild(*item.sceneGraph);
		
		/* Replace the item's scene graph with the new version: */
		item.sceneGraph=newSceneGraph;
		
		/* Insert the item's new scene graph into the common root node if it is currently enabled: */
		if(item.enabled)
			root->addChild(*item.sceneGraph);
		}
	catch(const std::runtime_error& err)
		{
		/* Show an error message: */
		Misc::formattedUserError(__PRETTY_FUNCTION__,"Cannot reload the scene graph due to exception %s",err.what());
		}
	}

void SceneGraphList::removeSceneGraphButtonSelectedCallback(Misc::CallbackData* cbData)
	{
	/* Select the affected scene graph list item: */
	int itemIndex=sceneGraphList->getSelectedItem();
	SGItem& item=sceneGraphs[itemIndex];
	
	/* Remove the item's scene graph from the common root node if it is currently enabled: */
	if(item.enabled)
		root->removeChild(*item.sceneGraph);
	
	/* Remove the item from the scene graph list: */
	sceneGraphs.erase(sceneGraphs.begin()+itemIndex);
	
	/* Remove the item from the list box: */
	sceneGraphList->removeItem(itemIndex);
	
	/* Update the rest of the dialog box: */
	if(sceneGraphList->getNumItems()>0)
		{
		/* Update the state of the "enabled" toggle: */
		enableToggle->setToggle(sceneGraphs[sceneGraphList->getSelectedItem()].enabled);
		}
	else
		{
		/* Disable buttons that require a selected item: */
		enableToggle->setToggle(false);
		enableToggle->setEnabled(false);
		reloadSceneGraphButton->setEnabled(false);
		removeSceneGraphButton->setEnabled(false);
		}
	}

SceneGraphList::SceneGraphList(SceneGraph::GroupNode& sRoot,IO::Directory& sCurrentDirectory)
	:root(&sRoot),currentDirectory(&sCurrentDirectory),
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

GraphNodePointer SceneGraphList::addSceneGraph(IO::Directory& directory,const char* fileName,bool enable)
	{
	/* Load a scene graph from the given file: */
	GraphNodePointer sceneGraph=loadSceneGraph(directory,fileName);
	
	/* Get the directory containing the scene graph file: */
	IO::DirectoryPtr sceneGraphDirectory=directory.openFileDirectory(fileName);
	
	/* Sort the scene graph into the list of scene graphs alphabetically by file name: */
	unsigned int insertPos=sceneGraphs.size();
	while(insertPos>0&&strcasecmp(sceneGraphs[insertPos-1].fileName.c_str(),fileName)>0)
		--insertPos;
	sceneGraphs.insert(sceneGraphs.begin()+insertPos,SGItem(*sceneGraphDirectory,Misc::getFileName(fileName),*sceneGraph,enable));
	
	/* Add the scene graph to the common root node if it is to be enabled: */
	if(enable)
		root->addChild(*sceneGraph);
	
	/* Add the scene graph to the scene graph dialog: */
	if(sceneGraphDialog!=0)
		{
		/* Insert the scene graph's file name into the list box: */
		sceneGraphList->insertItem(insertPos,fileName);
		
		/* Select the just-inserted item and bring it to the visible page: */
		sceneGraphList->selectItem(insertPos,true);
		
		/* Update the rest of the dialog box: */
		enableToggle->setEnabled(true);
		enableToggle->setToggle(enable);
		reloadSceneGraphButton->setEnabled(true);
		removeSceneGraphButton->setEnabled(true);
		}
	
	/* Update the current directory: */
	currentDirectory=sceneGraphDirectory;
	
	return sceneGraph;
	}

GLMotif::PopupWindow* SceneGraphList::createSceneGraphDialog(GLMotif::WidgetManager& widgetManager,const char* title)
	{
	/* If the scene graph dialog already exists, return it: */
	if(sceneGraphDialog!=0)
		return sceneGraphDialog;
	
	/* Retrieve the GLMotif style sheet: */
	const GLMotif::StyleSheet& ss=*widgetManager.getStyleSheet();
	
	/* Create the scene graph dialog window pop-up: */
	sceneGraphDialog=new GLMotif::PopupWindow("SceneGraphDialog",&widgetManager,title!=0?title:"Scene Graph List");
	sceneGraphDialog->setHideButton(true);
	sceneGraphDialog->setCloseButton(true);
	sceneGraphDialog->setResizableFlags(true,true);
	
	/* Create the main dialog panel with a scrolled list box on the left and a button panel on the right: */
	GLMotif::RowColumn* dialog=new GLMotif::RowColumn("Dialog",sceneGraphDialog,false);
	dialog->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	dialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	dialog->setNumMinorWidgets(1);
	
	/* Create the scene graph list box: */
	GLMotif::ScrolledListBox* sceneGraphScrolledList=new GLMotif::ScrolledListBox("SceneGraphScrolledList",dialog,GLMotif::ListBox::ALWAYS_ONE,20,10);
	sceneGraphScrolledList->showHorizontalScrollBar(true);
	sceneGraphList=sceneGraphScrolledList->getListBox();
	sceneGraphList->getValueChangedCallbacks().add(this,&SceneGraphList::sceneGraphListValueChangedCallback);
	sceneGraphList->getItemSelectedCallbacks().add(this,&SceneGraphList::sceneGraphListItemSelectedCallback);
	
	/* Add all currently managed scene graphs to the list box: */
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt)
		sceneGraphList->addItem(sgIt->fileName.c_str());
	
	/* Create the button panel: */
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",dialog,false);
	buttonMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::TOP));
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
	buttonBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	buttonBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	buttonBox->setNumMinorWidgets(1);
	
	/* Create a button to add another scene graph: */
	addSceneGraphButton=new GLMotif::Button("AddSceneGraphButton",buttonBox,"Add Scene Graph...");
	addSceneGraphButton->getSelectCallbacks().add(this,&SceneGraphList::addSceneGraphButtonSelectedCallback);
	
	/* Create the enable/disable toggle: */
	enableToggle=new GLMotif::ToggleButton("EnableToggle",buttonBox,"Enabled");
	enableToggle->getValueChangedCallbacks().add(this,&SceneGraphList::enableToggleValueChangedCallback);
	if(sceneGraphs.empty())
		enableToggle->setEnabled(false);
	else
		enableToggle->setToggle(sceneGraphs[sceneGraphList->getSelectedItem()].enabled);
	
	/* Create a button to reload the selected scene graph: */
	reloadSceneGraphButton=new GLMotif::Button("ReloadSceneGraphButton",buttonBox,"Reload Scene Graph");
	reloadSceneGraphButton->getSelectCallbacks().add(this,&SceneGraphList::reloadSceneGraphButtonSelectedCallback);
	if(sceneGraphs.empty())
		reloadSceneGraphButton->setEnabled(false);
	
	/* Add a separator: */
	new GLMotif::Separator("Separator1",buttonBox,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
	
	/* Create a button to remove the selected scene graph: */
	removeSceneGraphButton=new GLMotif::Button("RemoveSceneGraphButton",buttonBox,"Remove Scene Graph");
	removeSceneGraphButton->getSelectCallbacks().add(this,&SceneGraphList::removeSceneGraphButtonSelectedCallback);
	if(sceneGraphs.empty())
		removeSceneGraphButton->setEnabled(false);
	
	buttonBox->manageChild();
	
	buttonMargin->manageChild();
	
	dialog->setColumnWeight(0,1.0f);
	dialog->setColumnWeight(1,0.0f);
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

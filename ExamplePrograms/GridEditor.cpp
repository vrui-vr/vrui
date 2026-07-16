/***********************************************************************
GridEditor - Vrui application for interactive virtual clay modeling
using a density grid and interactive isosurface extraction.
Copyright (c) 2006-2024 Oliver Kreylos

This file is part of the Virtual Clay Editing Package.

The Virtual Clay Editing Package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Clay Editing Package is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Clay Editing Package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include "GridEditor.h"

#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <Misc/Utility.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/MessageLogger.h>
#include <Misc/CommandLineParser.h>
#include <Realtime/Time.h>
#include <IO/ValueSource.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Geometry/Box.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/OutputOperators.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLVertex.h>
#include <GL/GLMaterial.h>
#include <GL/GLContextData.h>
#include <GL/GLModels.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/TextField.h>
#include <GLMotif/ToggleButton.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/MeshFileNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/DisplayState.h>

#include "TriangleKdTree.h"

/*************************************
Methods of class GridEditor::DataItem:
*************************************/

GridEditor::DataItem::DataItem(void)
	:influenceSphereDisplayListId(glGenLists(1)),
	 domainBoxDisplayListId(glGenLists(1))
	{
	}

GridEditor::DataItem::~DataItem(void)
	{
	glDeleteLists(influenceSphereDisplayListId,1);
	glDeleteLists(domainBoxDisplayListId,1);
	}

/*********************************************
Static elements of class GridEditor::EditTool:
*********************************************/

GridEditor::EditToolFactory* GridEditor::EditTool::factory=0;

/*************************************
Methods of class GridEditor::EditTool:
*************************************/

void GridEditor::EditTool::brushSizeSliderCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Get the new brush size: */
	influenceRadius=Vrui::Scalar(cbData->value);
	}

void GridEditor::EditTool::fudgeSizeSliderCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Get the new fudge size: */
	fudgeSize=float(cbData->value);
	}

void GridEditor::EditTool::changeEditModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	switch(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle))
		{
		case 0:
			editMode=ADD;
			break;
		
		case 1:
			editMode=SUBTRACT;
			break;
		
		case 2:
			editMode=SMOOTH;
			break;
		
		case 3:
			editMode=DRAG;
			break;
		}
	}

GridEditor::EditToolFactory* GridEditor::EditTool::initClass(Vrui::ToolManager& toolManager)
	{
	/* Create the tool factory: */
	factory=new EditToolFactory("EditTool","Edit Grid",0,toolManager);
	
	/* Set up the tool class' input layout: */
	factory->setNumButtons(1,true);
	factory->setButtonFunction(0,"Edit");
	factory->setButtonFunction(1,"Set Add Mode");
	factory->setButtonFunction(2,"Set Subtract Mode");
	factory->setButtonFunction(3,"Set Smooth Mode");
	factory->setButtonFunction(4,"Set Drag Mode");
	
	/* Register and return the class: */
	toolManager.addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	return factory;
	}

GridEditor::EditTool::EditTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 grid(0),
	 influenceRadius(Vrui::getGlyphRenderer()->getGlyphSize()*Vrui::Scalar(2.5)),
	 editMode(ADD),
	 active(false),
	 settingsDialog(0),editModeBox(0)
	{
	}

GridEditor::EditTool::~EditTool(void)
	{
	}

void GridEditor::EditTool::initialize(void)
	{
	/* Access the application's editable grid: */
	grid=application->grid;
	
	/* Resize the temporary grid storage: */
	newValues.resize(grid->getNumVertices());
	
	/* Calculate the fudge size: */
	fudgeSize=0.0f;
	for(int i=0;i<3;++i)
		fudgeSize+=Math::sqr(grid->getCellSize(i));
	fudgeSize=Math::sqrt(fudgeSize)*2.0f;
	
	const GLMotif::StyleSheet& ss=*Vrui::getUiStyleSheet();
	
	/* Create the settings dialog: */
	settingsDialog=new GLMotif::PopupWindow("SettingsDialog",Vrui::getWidgetManager(),"Edit Tool Settings");
	
	GLMotif::RowColumn* settings=new GLMotif::RowColumn("Settings",settingsDialog,false);
	settings->setNumMinorWidgets(2);
	
	/* Create a slider to change the marker sphere size: */
	new GLMotif::Label("BrushSizeLabel",settings,"Brush Size");
	
	GLMotif::TextFieldSlider* brushSizeSlider=new GLMotif::TextFieldSlider("BrushSizeSlider",settings,8,ss.fontHeight*10.0f);
	brushSizeSlider->getTextField()->setFieldWidth(7);
	brushSizeSlider->getTextField()->setPrecision(3);
	brushSizeSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	brushSizeSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	brushSizeSlider->setValueRange(double(influenceRadius)*0.1,double(influenceRadius)*5.0,double(influenceRadius)*0.01);
	brushSizeSlider->setValue(double(influenceRadius));
	brushSizeSlider->getValueChangedCallbacks().add(this,&GridEditor::EditTool::brushSizeSliderCallback);
	
	/* Create a slider/textfield combo to change the fudge size: */
	new GLMotif::Label("FudgeSizeLabel",settings,"Fudge Size");
	
	GLMotif::TextFieldSlider* fudgeSizeSlider=new GLMotif::TextFieldSlider("FudgeSizeSlider",settings,8,ss.fontHeight*10.0f);
	fudgeSizeSlider->getTextField()->setFieldWidth(7);
	fudgeSizeSlider->getTextField()->setPrecision(3);
	fudgeSizeSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	fudgeSizeSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	fudgeSizeSlider->setValueRange(0.0,double(fudgeSize)*2.0,double(fudgeSize)*0.1);
	fudgeSizeSlider->setValue(double(fudgeSize));
	fudgeSizeSlider->getValueChangedCallbacks().add(this,&GridEditor::EditTool::fudgeSizeSliderCallback);
	
	/* Create a radio box to select editing modes: */
	new GLMotif::Label("EditModeLabel",settings,"Editing Mode");
	
	editModeBox=new GLMotif::RadioBox("EditModeBox",settings,false);
	editModeBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	editModeBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	editModeBox->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	editModeBox->addToggle("Add");
	editModeBox->addToggle("Subtract");
	editModeBox->addToggle("Smooth");
	editModeBox->addToggle("Drag");
	
	switch(editMode)
		{
		case ADD:
			editModeBox->setSelectedToggle(0);
			break;
		
		case SUBTRACT:
			editModeBox->setSelectedToggle(1);
			break;
		
		case SMOOTH:
			editModeBox->setSelectedToggle(2);
			break;
		
		case DRAG:
			editModeBox->setSelectedToggle(3);
			break;
		}
	editModeBox->getValueChangedCallbacks().add(this,&GridEditor::EditTool::changeEditModeCallback);
	editModeBox->manageChild();
	
	settings->manageChild();
	
	/* Pop up the settings dialog: */
	Vrui::popupPrimaryWidget(settingsDialog);
	}

void GridEditor::EditTool::deinitialize(void)
	{
	/* Pop down the settings dialog: */
	Vrui::popdownPrimaryWidget(settingsDialog);
	
	/* Delete the settings dialog: */
	delete settingsDialog;
	}

const Vrui::ToolFactory* GridEditor::EditTool::getFactory(void) const
	{
	return factory;
	}

void GridEditor::EditTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonSlotIndex==0)
		{
		/* Activate or deactivate the tool: */
		active=cbData->newButtonState;
		
		if(active)
			{
			/* Initialize the tool's position and orientation in model coordinates: */
			lastTrackerState=getButtonDeviceNavTransformation(0);
			}
		}
	else if(cbData->newButtonState)
		{
		/* Switch editing modes: */
		switch(buttonSlotIndex)
			{
			case 1:
				editMode=ADD;
				break;
			
			case 2:
				editMode=SUBTRACT;
				break;
			
			case 3:
				editMode=SMOOTH;
				break;
			
			case 4:
				editMode=DRAG;
				break;
			}
		
		/* Update the tool mode dialog: */
		editModeBox->setSelectedToggle(buttonSlotIndex-1);
		}
	}

void GridEditor::EditTool::frame(void)
	{
	/* Bail out if the tool is not active: */
	if(!active)
		return;
	
	/* Update the tool's position and radius in model coordinates: */
	Vrui::NavTrackerState newTrackerState=getButtonDeviceNavTransformation(0);
	
	/* Update the brush position and size in model coordinates: */
	modelCenter=Point(newTrackerState.getOrigin());
	for(int i=0;i<3;++i)
		modelCenter[i]-=grid->getOrigin()[i];
	modelRadius=float(influenceRadius*newTrackerState.getScaling());
	
	/* Determine the subdomain of the grid affected by the brush: */
	EditableGrid::Index min,max;
	for(int i=0;i<3;++i)
		{
		min[i]=int(Math::floor((modelCenter[i]-modelRadius-fudgeSize)/grid->getCellSize(i)));
		if(min[i]<1)
			min[i]=1;
		max[i]=int(Math::ceil((modelCenter[i]+modelRadius+fudgeSize)/grid->getCellSize(i)));
		if(max[i]>grid->getNumVertices(i)-1)
			max[i]=grid->getNumVertices(i)-1;
		}
	
	/* Update the grid: */
	float minr2=modelRadius>fudgeSize?Math::sqr(modelRadius-fudgeSize):0.0f;
	float maxr2=Math::sqr(modelRadius+fudgeSize);
	switch(editMode)
		{
		case ADD:
			{
			for(EditableGrid::Index v=min;v[0]<max[0];v.preInc(min,max))
				{
				Point p;
				float dist=0.0f;
				for(int i=0;i<3;++i)
					{
					p[i]=float(v[i])*grid->getCellSize(i);
					dist+=Math::sqr(modelCenter[i]-p[i]);
					}
				if(dist<maxr2)
					{
					float val;
					if(dist<minr2)
						val=1.0f;
					else
						val=(modelRadius+fudgeSize-Math::sqrt(dist))/(2.0f*fudgeSize);
					float oldVal=grid->getValue(v);
					if(val>oldVal)
						grid->setValue(v,val);
					}
				}
			
			grid->invalidateVertices(min,max);
			break;
			}
		
		case SUBTRACT:
			{
			for(EditableGrid::Index v=min;v[0]<max[0];v.preInc(min,max))
				{
				Point p;
				float dist=0.0f;
				for(int i=0;i<3;++i)
					{
					p[i]=float(v[i])*grid->getCellSize(i);
					dist+=Math::sqr(modelCenter[i]-p[i]);
					}
				if(dist<maxr2)
					{
					float val;
					if(dist<minr2)
						val=0.0f;
					else
						val=1.0f-(modelRadius+fudgeSize-Math::sqrt(dist))/(2.0f*fudgeSize);
					float oldVal=grid->getValue(v);
					if(val<oldVal)
						grid->setValue(v,val);
					}
				}
			
			grid->invalidateVertices(min,max);
			break;
			}
		
		case SMOOTH:
			{
			for(int i=0;i<3;++i)
				{
				if(min[i]==0)
					++min[i];
				if(max[i]==grid->getNumVertices(i))
					--max[i];
				}
			for(EditableGrid::Index v=min;v[0]<max[0];v.preInc(min,max))
				{
				Point p;
				float dist=0.0f;
				for(int i=0;i<3;++i)
					{
					p[i]=float(v[i])*grid->getCellSize(i);
					dist+=Math::sqr(modelCenter[i]-p[i]);
					}
				if(dist<maxr2)
					{
					float avgVal=0.0f;
					EditableGrid::Index i;
					for(i[0]=v[0]-1;i[0]<=v[0]+1;++i[0])
						for(i[1]=v[1]-1;i[1]<=v[1]+1;++i[1])
							for(i[2]=v[2]-1;i[2]<=v[2]+1;++i[2])
								avgVal+=grid->getValue(i);
					avgVal/=27.0f;
					if(dist<minr2)
						newValues(v)=avgVal;
					else
						{
						float w=(modelRadius+fudgeSize-Math::sqrt(dist))/(2.0f*fudgeSize);
						newValues(v)=avgVal*w+grid->getValue(v)*(1.0f-w);
						}
					}
				else
					newValues(v)=grid->getValue(v);
				}
			
			for(EditableGrid::Index v=min;v[0]<max[0];v.preInc(min,max))
				grid->setValue(v,newValues(v));
			
			grid->invalidateVertices(min,max);
			
			/* Request another frame to continue smoothing: */
			Vrui::scheduleUpdate(Vrui::getApplicationTime()+1.0/125.0);
			break;
			}
		
		case DRAG:
			{
			/* Calculate the incremental tool transformation since the last frame: */
			Vrui::NavTrackerState t=lastTrackerState;
			t*=Geometry::invert(newTrackerState);
			Geometry::OrthogonalTransformation<float,3> pt(t);
			
			float r2=Math::sqr(modelRadius);
			for(EditableGrid::Index v=min;v[0]<max[0];v.preInc(min,max))
				{
				Point p;
				float dist=0.0f;
				for(int i=0;i<3;++i)
					{
					p[i]=float(v[i])*grid->getCellSize(i);
					dist+=Math::sqr(modelCenter[i]-p[i]);
					}
				if(dist<r2)
					{
					/* Compute the dragged position: */
					Point dp=pt.transform(p);
					float w=Math::sqrt(dist)/modelRadius;
					dp=Geometry::affineCombination(dp,p,w);
					
					/* Look up the grid value at the dragged position: */
					float dragVal=grid->getValue(dp);
					newValues(v)=dragVal;
					}
				else
					newValues(v)=grid->getValue(v);
				}
			
			for(EditableGrid::Index v=min;v[0]<max[0];v.preInc(min,max))
				grid->setValue(v,newValues(v));
			
			grid->invalidateVertices(min,max);
			break;
			}
		}
	
	lastTrackerState=newTrackerState;
	}

void GridEditor::EditTool::glRenderActionTransparent(GLContextData& contextData) const
	{
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POLYGON_BIT);
	
	/* Retrieve context entry: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(application);
	
	/* Render the influence sphere: */
	glDisable(GL_LIGHTING);
	
	glPushMatrix();
	glMultMatrix(getButtonDeviceTransformation(0));
	glScale(influenceRadius);
	glCallList(dataItem->influenceSphereDisplayListId);
	glPopMatrix();
	
	glPopAttrib();
	}

/***************************
Methods of class GridEditor:
***************************/

void GridEditor::saveGridCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Write the current contents of the grid to a floating-point vol file: */
		IO::FilePtr gridFile(cbData->selectedDirectory->openFile(cbData->selectedFileName,IO::File::WriteOnly));
		gridFile->setEndianness(Misc::BigEndian);
		gridFile->write(grid->getNumVertices().getComponents(),3);
		gridFile->write<int>(0);
		float domainSize[3];
		for(int i=0;i<3;++i)
			domainSize[i]=float(grid->getNumVertices(i)-1)*grid->getCellSize(i);
		gridFile->write(domainSize,3);
		
		/* Write the grid data values: */
		for(EditableGrid::Index i(0);i[0]<grid->getNumVertices(0);i.preInc(grid->getNumVertices()))
			gridFile->write<float>(grid->getValue(i));
		}
	catch(const std::runtime_error& err)
		{
		Vrui::showErrorMessage("Save Grid...",Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Could not save grid due to exception %s",err.what()).c_str());
		}
	}

void GridEditor::exportSurfaceCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Write the PLY file: */
		IO::FilePtr plyFile(cbData->selectedDirectory->openFile(cbData->selectedFileName,IO::File::WriteOnly));
		grid->exportSurface(*plyFile);
		}
	catch(const std::runtime_error& err)
		{
		Vrui::showErrorMessage("Export Surface...",Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Could not export surface due to exception %s",err.what()).c_str());
		}
	}

GLMotif::PopupMenu* GridEditor::createMainMenu(void)
	{
	/* Create the main menu shell: */
	GLMotif::PopupMenu* mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("3D Grid Editor");
	
	/* Create a button to save the grid to a file: */
	GLMotif::Button* saveGridButton=new GLMotif::Button("SaveGridButton",mainMenu,"Save Grid...");
	saveGridHelper.addSaveCallback(saveGridButton,this,&GridEditor::saveGridCallback);
	
	/* Create a button to export the current isosurface to a mesh file: */
	GLMotif::Button* exportSurfaceButton=new GLMotif::Button("ExportSurfaceButton",mainMenu,"Export Surface...");
	exportSurfaceHelper.addSaveCallback(exportSurfaceButton,this,&GridEditor::exportSurfaceCallback);
	
	/* Calculate the main menu's proper layout: */
	mainMenu->manageMenu();
	
	/* Return the created top-level shell: */
	return mainMenu;
	}

EditableGrid* GridEditor::loadFvolFile(const std::string& fileName,const EditableGrid::Point& origin)
	{
	/* Load the grid from a float-valued vol file: */
	IO::FilePtr volFile=IO::openFile(fileName.c_str());
	volFile->setEndianness(Misc::BigEndian);
	
	/* Read the file header: */
	EditableGrid::Index numVertices;
	volFile->read(numVertices.getComponents(),3);
	int borderSize=volFile->read<int>();
	for(int i=0;i<3;++i)
		numVertices[i]+=borderSize*2;
	float domainSize[3];
	volFile->read(domainSize,3);
	EditableGrid::Size cellSize;
	for(int i=0;i<3;++i)
		cellSize[i]=domainSize[i]/float(numVertices[i]-borderSize*2-1);
	
	/* Create the grid: */
	Misc::SelfDestructPointer<EditableGrid> result(new EditableGrid(origin,numVertices,cellSize));
	EditableGrid* grid=result.getTarget();
	
	std::cout<<"Creating grid with "<<numVertices[0]<<'x'<<numVertices[1]<<'x'<<numVertices[2]<<" vertices"<<std::endl;
	std::cout<<"Cell size "<<cellSize[0]<<'x'<<cellSize[1]<<'x'<<cellSize[2]<<std::endl;
	std::cout<<"Domain box: ["<<grid->getBox().min<<", "<<grid->getBox().max<<"]"<<std::endl;
	
	/* Read all grid values: */
	for(EditableGrid::Index i(0);i[0]<grid->getNumVertices(0);i.preInc(grid->getNumVertices()))
		grid->setValue(i,volFile->read<float>());
	grid->invalidateVertices(EditableGrid::Index(0,0,0),grid->getNumVertices());
	
	return result.releaseTarget();
	}

EditableGrid* GridEditor::loadSdfFile(const std::string& fileName)
	{
	/* Load the grid from a signed distance field in ASCII format: */
	IO::ValueSource sdfFile(IO::openFile(fileName.c_str()));
	sdfFile.setWhitespace(" \t\r\n");
	sdfFile.skipWs();
	EditableGrid::Index numVertices;
	for(int i=0;i<3;++i)
		numVertices[i]=sdfFile.readUnsignedInteger();
	EditableGrid::Point origin;
	for(int i=0;i<3;++i)
		origin[i]=float(sdfFile.readNumber());
	EditableGrid::Size cellSize;
	cellSize[2]=cellSize[1]=cellSize[0]=float(sdfFile.readNumber());
	
	/* Create the grid: */
	Misc::SelfDestructPointer<EditableGrid> result(new EditableGrid(origin,numVertices,cellSize));
	EditableGrid* grid=result.getTarget();
	
	std::cout<<"Creating grid with "<<numVertices[0]<<'x'<<numVertices[1]<<'x'<<numVertices[2]<<" vertices"<<std::endl;
	std::cout<<"Cell size "<<cellSize[0]<<'x'<<cellSize[1]<<'x'<<cellSize[2]<<std::endl;
	std::cout<<"Domain box: ["<<grid->getBox().min<<", "<<grid->getBox().max<<"]"<<std::endl;
	
	/* Read all grid values: */
	float minValue=Math::Constants<float>::max;
	float maxValue=Math::Constants<float>::min;
	EditableGrid::Index i;
	for(i[2]=0;i[2]<numVertices[2];++i[2])
		for(i[1]=0;i[1]<numVertices[1];++i[1])
			for(i[0]=0;i[0]<numVertices[0];++i[0])
				{
				float value=float(sdfFile.readNumber());
				if(minValue>value)
					minValue=value;
				if(maxValue<value)
					maxValue=value;
				grid->setValue(i,value+0.5f);
				}
	grid->invalidateVertices(EditableGrid::Index(0,0,0),grid->getNumVertices());
	
	return result.releaseTarget();
	}

namespace {

/****************
Helper functions:
****************/

std::vector<SceneGraph::Scalar> intersectXray(const SceneGraph::Point& start,const std::vector<SceneGraph::Point>& triangles) // Returns all intersections between a ray along the positive x axis from the given start point with the given set of triangles, ordered by x position
	{
	std::vector<SceneGraph::Scalar> result;
	
	/* Test all triangles in the triangle set: */
	for(std::vector<SceneGraph::Point>::const_iterator tIt=triangles.begin();tIt!=triangles.end();tIt+=3)
		{
		/* Sort the triangle's vertices by their z values: */
		const SceneGraph::Point* v0=&tIt[0];
		const SceneGraph::Point* v1=&tIt[1];
		if((*v0)[2]>(*v1)[2])
			Misc::swap(v0,v1);
		const SceneGraph::Point* v2=&tIt[2];
		if((*v1)[2]>(*v2)[2])
			Misc::swap(v1,v2);
		if((*v0)[2]>(*v1)[2])
			Misc::swap(v0,v1);
		
		/* Intersect the triangle with the plane parallel to the (x, y) plane containing the ray: */
		SceneGraph::Point e0,e1;
		bool haveEdge=false;
		if((*v0)[2]<start[2]&&(*v1)[2]<start[2]&&(*v2)[2]>=start[2])
			{
			e0=Geometry::affineCombination(*v0,*v2,(start[2]-(*v0)[2])/((*v2)[2]-(*v0)[2]));
			e1=Geometry::affineCombination(*v1,*v2,(start[2]-(*v1)[2])/((*v2)[2]-(*v1)[2]));
			haveEdge=true;
			}
		else if((*v0)[2]<start[2]&&(*v1)[2]>=start[2]&&(*v2)[2]>=start[2])
			{
			e0=Geometry::affineCombination(*v0,*v1,(start[2]-(*v0)[2])/((*v1)[2]-(*v0)[2]));
			e1=Geometry::affineCombination(*v0,*v2,(start[2]-(*v0)[2])/((*v2)[2]-(*v0)[2]));
			haveEdge=true;
			}
		if(haveEdge)
			{
			/* Intersect the edge in the intersection plane with the ray: */
			if(e0[1]<start[1]&&e1[1]>=start[1])
				{
				SceneGraph::Scalar w=(start[1]-e0[1])/(e1[1]-e0[1]);
				SceneGraph::Scalar x=e0[0]*(SceneGraph::Scalar(1)-w)+e1[0]*w;
				if(x>=start[0])
					{
					/* We have an intersection: */
					result.push_back(x);
					}
				}
			else if(e0[1]>=start[1]&&e1[1]<start[1])
				{
				SceneGraph::Scalar w=(start[1]-e1[1])/(e0[1]-e1[1]);
				SceneGraph::Scalar x=e1[0]*(SceneGraph::Scalar(1)-w)+e0[0]*w;
				if(x>=start[0])
					{
					/* We have an intersection: */
					result.push_back(x);
					}
				}
			}
		}
	
	/* Sort and return the set of intersections: */
	std::sort(result.begin(),result.end());
	return result;
	}

}

EditableGrid* GridEditor::loadMeshFile(const std::string& fileName,const EditableGrid::Size& cellSize)
	{
	/* Use a scene graph mesh file node to load the mesh file: */
	Realtime::TimePointMonotonic timer1;
	SceneGraph::MeshFileNode meshFile;
	meshFile.url.setValue(fileName);
	meshFile.update();
	std::cout<<"Loaded input file in "<<double(timer1.setAndDiff())*1000.0<<" ms"<<std::endl;
	
	/* Fit a grid around the mesh: */
	SceneGraph::Box meshBox=meshFile.calcBoundingBox();
	std::cout<<"Mesh bounding box: "<<meshBox.min<<", "<<meshBox.max<<std::endl;
	EditableGrid::Point origin;
	EditableGrid::Index numVertices;
	for(int i=0;i<3;++i)
		{
		SceneGraph::Scalar min=Math::floor(meshBox.min[i]/cellSize[i])-SceneGraph::Scalar(2);
		origin[i]=min*cellSize[i];
		SceneGraph::Scalar max=Math::ceil(meshBox.max[i]/cellSize[i])+SceneGraph::Scalar(2);
		numVertices[i]=int(max-min)+1;
		}
	
	/* Create the grid: */
	Misc::SelfDestructPointer<EditableGrid> result(new EditableGrid(origin,numVertices,cellSize));
	EditableGrid* grid=result.getTarget();
	
	std::cout<<"Creating grid with "<<numVertices[0]<<'x'<<numVertices[1]<<'x'<<numVertices[2]<<" vertices"<<std::endl;
	std::cout<<"Cell size "<<cellSize[0]<<'x'<<cellSize[1]<<'x'<<cellSize[2]<<std::endl;
	std::cout<<"Domain box: ["<<grid->getBox().min<<", "<<grid->getBox().max<<"]"<<std::endl;
	
	/* Enter all shapes contained in the loaded mesh file to the grid: */
	Realtime::TimePointMonotonic timer2;
	TriangleKdTree::TriangleList triangles;
	for(std::vector<SceneGraph::ShapeNodePointer>::const_iterator sIt=meshFile.getShapes().begin();sIt!=meshFile.getShapes().end();++sIt)
		{
		/* Check if the shape node has an indexed face set as geometry: */
		const SceneGraph::IndexedFaceSetNode* faceSet=dynamic_cast<const SceneGraph::IndexedFaceSetNode*>((*sIt)->geometry.getValue().getPointer());
		if(faceSet!=0)
			{
			/* Extract the indexed face set node's triangles: */
			const SceneGraph::MFPoint::ValueList& points=faceSet->coord.getValue()->point.getValues();
			const SceneGraph::MFInt::ValueList& coordIndex=faceSet->coordIndex.getValues();
			SceneGraph::MFInt::ValueList::const_iterator ciIt=coordIndex.begin();
			while(ciIt!=coordIndex.end())
				{
				/* Find the end of the current face: */
				SceneGraph::MFInt::ValueList::const_iterator faceEnd=ciIt;
				while(faceEnd!=coordIndex.end()&&*faceEnd>=0)
					++faceEnd;
				
				/* Triangulate the current face: */
				if(faceEnd-ciIt>=3)
					{
					SceneGraph::MFInt::ValueList::const_iterator v1It=ciIt+1;
					SceneGraph::MFInt::ValueList::const_iterator v2It=ciIt+2;
					while(v2It!=faceEnd)
						{
						/* Add the triangle to the triangle set: */
						triangles.push_back(TriangleKdTree::Triangle(points[*ciIt],points[*v1It],points[*v2It]));
						
						/* Go to the next triangle: */
						v1It=v2It;
						++v2It;
						}
					}
				
				/* Go to the next face: */
				if(faceEnd!=coordIndex.end())
					++faceEnd;
				ciIt=faceEnd;
				}
			}
		}
	std::cout<<"Extracted "<<triangles.size()<<" triangles from input mesh file in "<<double(timer2.setAndDiff())*1000.0<<" ms"<<std::endl;
	
	/* Create a triangle kd-tree: */
	Realtime::TimePointMonotonic timer3;
	TriangleKdTree triangleTree(triangles);
	triangleTree.createTree(meshBox,64);
	std::cout<<"Created triangle kd-tree in "<<double(timer3.setAndDiff())*1000.0<<" ms"<<std::endl;
	
	/* Calculate inside/outside values for all grid vertices: */
	Realtime::TimePointMonotonic timer4;
	for(int z=0;z<numVertices[2];++z)
		for(int y=0;y<numVertices[1];++y)
			{
			/* Calculate all intersections of the current grid line with the triangle set: */
			TriangleKdTree::Point start=grid->getBox().min;
			start[1]+=TriangleKdTree::Scalar(SceneGraph::Scalar(y)*cellSize[1]);
			start[2]+=TriangleKdTree::Scalar(SceneGraph::Scalar(z)*cellSize[2]);
			TriangleKdTree::IntersectionResultList intersections=triangleTree.intersectXray(start);
			
			/* Process all spans between intersections: */
			EditableGrid::Index i(0,y,z);
			float value=0.0f; // Start from the outside
			TriangleKdTree::IntersectionResultList::const_iterator iIt=intersections.begin();
			int spanEnd=numVertices[0];
			if(iIt!=intersections.end())
				{
				spanEnd=int(Math::floor((iIt->lambda-start[0])/TriangleKdTree::Scalar(cellSize[0])));
				++iIt;
				}
			while(i[0]<numVertices[0])
				{
				/* Assign the current vertex value to all vertices in the span: */
				while(i[0]<spanEnd)
					{
					grid->setValue(i,value);
					++i[0];
					}
				
				/* Go to the next span: */
				value=1.0f-value; // Flip the inside/outside flag
				spanEnd=numVertices[0];
				if(iIt!=intersections.end())
					{
					spanEnd=int(Math::floor((iIt->lambda-start[0])/TriangleKdTree::Scalar(cellSize[0])));
					++iIt;
					}
				}
			}
	std::cout<<"Created grid in "<<double(timer4.setAndDiff())*1000.0<<" ms"<<std::endl;
	
	Realtime::TimePointMonotonic timer5;
	grid->invalidateVertices(EditableGrid::Index(0,0,0),grid->getNumVertices());
	std::cout<<"Extracted initial surface in "<<double(timer5.setAndDiff())*1000.0<<" ms"<<std::endl;
	
	return result.releaseTarget();
	}

GridEditor::GridEditor(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 grid(0),
	 saveGridHelper(Vrui::getWidgetManager(),"SavedGrid.fvol",".fvol",IO::openDirectory(".")),
	 exportSurfaceHelper(Vrui::getWidgetManager(),"ExportedSurface.ply",".ply",IO::openDirectory(".")),
	 mainMenu(0)
	{
	/* Parse the command line: */
	Misc::CommandLineParser parser;
	parser.setDescription("Three-dimensional sculpting application based on a virtual clay metaphor.");
	parser.setArguments("[<input file name>]","Sets the name of an input file, either a 3D distance field in .fvol or .sdf format, or a mesh file. If no input file name is given, an empty grid is created.");
	std::vector<std::string> inputFileNames;
	parser.addArgumentsToList(inputFileNames);
	EditableGrid::Point newOrigin=EditableGrid::Point::origin;
	parser.addArrayOption("origin","o",3,newOrigin.getComponents(),"<x> <y> <z>","Sets the origin of a newly-created grid. Default 0 0 0.");
	EditableGrid::Size newCellSize(1,1,1);
	parser.addArrayOption("cellSize","cs",3,newCellSize.getComponents(),"<cx> <cy> <cz>","Sets the cell size of a newly-created grid in some coordinate unit. Default 1 1 1.");
	EditableGrid::Index newGridSize(256,256,256);
	parser.addArrayOption("gridSize","gs",3,newGridSize.getComponents(),"<sx> <sy> <sz>","Sets the size of a newly-created grid. Default 256 256 256.");
	parser.parse(argv,argv+argc);
	if(parser.hadHelp())
		{
		Vrui::shutdown();
		return;
		}
	if(inputFileNames.size()>1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"More than one input file name provided");
	
	/* Create the grid: */
	if(!inputFileNames.empty())
		{
		if(Misc::hasCaseExtension(inputFileNames.front().c_str(),".fvol"))
			{
			try
				{
				/* Create a grid from a float-valued .vol file: */
				grid=loadFvolFile(inputFileNames.front(),newOrigin);
				}
			catch(const std::runtime_error& err)
				{
				Misc::formattedUserError("Cannot load float-valued .vol input file %s due to exception %s",inputFileNames.front().c_str(),err.what());
				
				/* Create a new grid instead: */
				grid=new EditableGrid(newOrigin,newGridSize,newCellSize);
				}
			}
		else if(Misc::hasCaseExtension(inputFileNames.front().c_str(),".sdf"))
			{
			try
				{
				/* Create a grid from a signed distance field in ASCII format: */
				grid=loadSdfFile(inputFileNames.front());
				}
			catch(const std::runtime_error& err)
				{
				Misc::formattedUserError("Cannot load ASCII signed distance field input file %s due to exception %s",inputFileNames.front().c_str(),err.what());
				
				/* Create a new grid instead: */
				grid=new EditableGrid(newOrigin,newGridSize,newCellSize);
				}
			}
		else
			{
			/* Create a grid from a mesh file: */
			grid=loadMeshFile(inputFileNames.front(),newCellSize);
			}
		}
	else
		{
		/* Create a new grid: */
		grid=new EditableGrid(newOrigin,newGridSize,newCellSize);
		}
	
	/* Create the program GUI: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Initialize the tool classes: */
	EditTool::initClass(*Vrui::getToolManager());
	}

GridEditor::~GridEditor(void)
	{
	delete mainMenu;
	}

void GridEditor::display(GLContextData& contextData) const
	{
	/* Get a pointer to the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Render the grid's domain box: */
	GLboolean lightingEnabled=glIsEnabled(GL_LIGHTING);
	if(lightingEnabled)
		glDisable(GL_LIGHTING);
	GLfloat lineWidth;
	glGetFloatv(GL_LINE_WIDTH,&lineWidth);
	glLineWidth(1.0f);
	glCallList(dataItem->domainBoxDisplayListId);
	if(lightingEnabled)
		glEnable(GL_LIGHTING);
	glLineWidth(lineWidth);
	
	/* Render the grid's current state: */
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.5f,0.5f,0.5f),GLMaterial::Color(0.5f,0.5f,0.5f),25.0f));
	grid->glRenderAction(contextData);
	}

void GridEditor::resetNavigation(void)
	{
	EditableGrid::Box bb=grid->getBox();
	
	/* Calculate the center and radius of the box: */
	Vrui::Point center=Geometry::mid(bb.min,bb.max);
	Vrui::Scalar radius=Vrui::Scalar(Geometry::dist(bb.min,bb.max))*Vrui::Scalar(0.5);
	Vrui::setNavigationTransformation(center,radius);
	}

void GridEditor::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the influence sphere display list: */
	glNewList(dataItem->influenceSphereDisplayListId,GL_COMPILE);
	glDisable(GL_CULL_FACE);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor4f(1.0f,1.0f,0.0f,0.5f);
	glDrawSphereIcosahedron(1.0,5);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor4f(0.5f,0.5f,0.1f,0.333f);
	glDrawSphereIcosahedron(1.0,5);
	glEndList();
	
	/* Create the domain box display list: */
	EditableGrid::Box box=grid->getBox();
	Point min=box.min;
	Point max=box.max;
	Vrui::Color fgColor=Vrui::getBackgroundColor();
	for(int i=0;i<3;++i)
		fgColor[i]=1.0f-fgColor[i];
	glNewList(dataItem->domainBoxDisplayListId,GL_COMPILE);
	glColor(fgColor);
	glBegin(GL_LINE_STRIP);
	glVertex(min[0],min[1],min[2]);
	glVertex(max[0],min[1],min[2]);
	glVertex(max[0],max[1],min[2]);
	glVertex(min[0],max[1],min[2]);
	glVertex(min[0],min[1],min[2]);
	glVertex(min[0],min[1],max[2]);
	glVertex(max[0],min[1],max[2]);
	glVertex(max[0],max[1],max[2]);
	glVertex(min[0],max[1],max[2]);
	glVertex(min[0],min[1],max[2]);
	glEnd();
	glBegin(GL_LINES);
	glVertex(max[0],min[1],min[2]);
	glVertex(max[0],min[1],max[2]);
	glVertex(max[0],max[1],min[2]);
	glVertex(max[0],max[1],max[2]);
	glVertex(min[0],max[1],min[2]);
	glVertex(min[0],max[1],max[2]);
	glEnd();
	glEndList();
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(GridEditor)

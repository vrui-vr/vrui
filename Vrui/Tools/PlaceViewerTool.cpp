/***********************************************************************
PlaceViewerTool - Class for tools that move a viewer to the tool's
current position when their button is pressed.
Copyright (c) 2022-2023 Oliver Kreylos

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

#include <Vrui/Tools/PlaceViewerTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/MessageLogger.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/******************************************************
Methods of class PlaceViewerToolFactory::Configuration:
******************************************************/

PlaceViewerToolFactory::Configuration::Configuration(void)
	:deviceOffset(Point::origin),
	 dragViewer(false)
	{
	}

void PlaceViewerToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateString("./viewerName",viewerName);
	cfs.updateValue("./deviceOffset",deviceOffset);
	cfs.updateValue("./dragViewer",dragViewer);
	}

void PlaceViewerToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeString("./viewerName",viewerName);
	cfs.storeValue("./deviceOffset",deviceOffset);
	cfs.storeValue("./dragViewer",dragViewer);
	}

/***************************************
Methods of class PlaceViewerToolFactory:
***************************************/

PlaceViewerToolFactory::PlaceViewerToolFactory(ToolManager& toolManager)
	:ToolFactory("PlaceViewerTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UtilityTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	PlaceViewerTool::factory=this;
	}

PlaceViewerToolFactory::~PlaceViewerToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	PlaceViewerTool::factory=0;
	}

const char* PlaceViewerToolFactory::getName(void) const
	{
	return "Place Viewer";
	}

const char* PlaceViewerToolFactory::getButtonFunction(int) const
	{
	return "Place Viewer";
	}

Tool* PlaceViewerToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new PlaceViewerTool(this,inputAssignment);
	}

void PlaceViewerToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolvePlaceViewerToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createPlaceViewerToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	PlaceViewerToolFactory* toolFactory=new PlaceViewerToolFactory(*toolManager);
	
	/* Return factory object: */
	return toolFactory;
	}

extern "C" void destroyPlaceViewerToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class PlaceViewerTool:
****************************************/

PlaceViewerToolFactory* PlaceViewerTool::factory=0;

/********************************
Methods of class PlaceViewerTool:
********************************/

void PlaceViewerTool::updateViewer(void)
	{
	/* Get the current tool position: */
	Point toolPos=getButtonDeviceTransformation(0).transform(configuration.deviceOffset);
	
	/* Get the viewer's current head transformation: */
	TrackerState currentHeadTransform=viewer->getHeadTransformation();
	
	/* Place the viewer's mono eye position at the tool position: */
	Vector headTranslation=toolPos-currentHeadTransform.getRotation().transform(viewer->getDeviceEyePosition(Viewer::MONO));
	TrackerState newHeadTransform(headTranslation,currentHeadTransform.getRotation());
	viewer->detachFromDevice(newHeadTransform);
	}

PlaceViewerTool::PlaceViewerTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(factory,inputAssignment),
	 configuration(PlaceViewerTool::factory->configuration),
	 viewer(0)
	{
	}

void PlaceViewerTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void PlaceViewerTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void PlaceViewerTool::initialize(void)
	{
	/* Find the viewer indicated by the configuration: */
	viewer=findViewer(configuration.viewerName.c_str());
	if(viewer==0)
		{
		/* Show an error message: */
		Misc::formattedUserError("Vrui::PlaceViewerTool: Viewer %s not found",configuration.viewerName.c_str());
		}
	else if(viewer->getHeadDevice()!=0)
		{
		/* Show an error message: */
		Misc::formattedUserError("Vrui::PlaceViewerTool: Viewer %s is head-tracked",configuration.viewerName.c_str());
		}
	}

const ToolFactory* PlaceViewerTool::getFactory(void) const
	{
	return factory;
	}

void PlaceViewerTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Update the viewer if not dragging and the button was pressed: */
	if(viewer!=0&&!configuration.dragViewer&&cbData->newButtonState)
		updateViewer();
	}

void PlaceViewerTool::frame(void)
	{
	/* Update the viewer if dragging and the button is pressed: */
	if(viewer!=0&&configuration.dragViewer&&getButtonState(0))
		updateViewer();
	}

}

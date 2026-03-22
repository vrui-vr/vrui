/***********************************************************************
RayMenuTool - Class for menu selection tools using ray selection.
Copyright (c) 2004-2026 Oliver Kreylos

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

#include <Vrui/Tools/RayMenuTool.h>

#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Vector.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/Vrui.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/VRScreen.h>
#include <Vrui/UIManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************************************
Methods of class RayMenuToolFactory::Configuration:
**********************************************************/

RayMenuToolFactory::Configuration::Configuration(void)
	:initialMenuOffset(getInchFactor()*Scalar(6)),
	 interactWithWidgets(false)
	{
	}

void RayMenuToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./initialMenuOffset",initialMenuOffset);
	cfs.updateValue("./interactWithWidgets",interactWithWidgets);
	cfs.updateString("./alignmentScreen",alignmentScreen);
	}

void RayMenuToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./initialMenuOffset",initialMenuOffset);
	cfs.storeValue("./interactWithWidgets",interactWithWidgets);
	cfs.storeString("./alignmentScreen",alignmentScreen);
	}

/***********************************
Methods of class RayMenuToolFactory:
***********************************/

RayMenuToolFactory::RayMenuToolFactory(ToolManager& toolManager)
	:ToolFactory("RayMenuTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* menuToolFactory=toolManager.loadClass("MenuTool");
	menuToolFactory->addChildClass(this);
	addParentClass(menuToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	RayMenuTool::factory=this;
	}

RayMenuToolFactory::~RayMenuToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	RayMenuTool::factory=0;
	}

const char* RayMenuToolFactory::getName(void) const
	{
	return "Free-Standing Menu";
	}

Tool* RayMenuToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new RayMenuTool(this,inputAssignment);
	}

void RayMenuToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveRayMenuToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("MenuTool");
	}

extern "C" ToolFactory* createRayMenuToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	RayMenuToolFactory* rayMenuToolFactory=new RayMenuToolFactory(*toolManager);
	
	/* Return factory object: */
	return rayMenuToolFactory;
	}

extern "C" void destroyRayMenuToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/************************************
Static elements of class RayMenuTool:
************************************/

RayMenuToolFactory* RayMenuTool::factory=0;

/****************************
Methods of class RayMenuTool:
****************************/

RayMenuTool::RayMenuTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:MenuTool(factory,inputAssignment),
	 GUIInteractor(false,0,getButtonDevice(0)),
	 configuration(RayMenuTool::factory->configuration),
	 alignmentScreen(0)
	{
	}

void RayMenuTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void RayMenuTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void RayMenuTool::initialize(void)
	{
	/* Check if popped-up menus should be aligned with a specific screen: */
	if(!configuration.alignmentScreen.empty())
		{
		/* Retrieve the alignment screen: */
		alignmentScreen=findScreen(configuration.alignmentScreen.c_str());
		
		/* Show a warning if the requested screen does not exist: */
		if(alignmentScreen==0)
			Misc::sourcedUserWarning(__PRETTY_FUNCTION__,"Alignment screen %s not found",configuration.alignmentScreen.c_str());
		}
	}

const ToolFactory* RayMenuTool::getFactory(void) const
	{
	return factory;
	}

void RayMenuTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Check if the GUI interactor refuses the event: */
		GUIInteractor::updateRay();
		if(!(configuration.interactWithWidgets&&GUIInteractor::buttonDown(false)))
			{
			/* Try activating this tool: */
			if(GUIInteractor::canActivate()&&activate())
				{
				/* Calculate the initial menu position: */
				Point rayOrigin=getButtonDevicePosition(0);
				if(!getButtonDevice(0)->isRayDevice())
					{
					/* For 6-DOF devices, offset the menu by some amount: */
					rayOrigin+=getRay().getDirection()*configuration.initialMenuOffset;
					}
				Point hotSpot=getUiManager()->projectRay(Ray(rayOrigin,getRay().getDirection()));
				
				/* Pop up the menu: */
				if(alignmentScreen!=0)
					{
					/* Align the menu with the requested screen: */
					ONTransform menuTransform=ONTransform::translateFromOriginTo(hotSpot);
					menuTransform*=ONTransform::rotate(alignmentScreen->getScreenTransformation().getRotation());
					
					/* Align the widget's hot spot with the transformation center: */
					GLMotif::Vector menuHotSpot=menu->getPopup()->calcHotSpot();
					menuTransform*=ONTransform::translate(-ONTransform::Vector(menuHotSpot.getXyzw()));
					
					/* Pop up the menu with the calculated transformation: */
					menuTransform.renormalize();
					getWidgetManager()->popupPrimaryWidget(menu->getPopup(),menuTransform);
					}
				else
					{
					/* Use a default menu alignment: */
					popupPrimaryWidget(menu->getPopup(),hotSpot,false);
					}
				
				/* Explicity grab the pointer in case the initial event misses the menu: */
				getWidgetManager()->grabPointer(menu->getPopup());
				
				/* Force the event on the GUI interactor: */
				GUIInteractor::buttonDown(true);
				}
			}
		}
	else // Button has just been released
		{
		/* Check if the GUI interactor is active: */
		if(GUIInteractor::isActive())
			{
			/* Deliver the event: */
			GUIInteractor::buttonUp();
			
			/* Check if the tool's menu is popped up: */
			if(MenuTool::isActive())
				{
				/* Release the pointer grab: */
				getWidgetManager()->releasePointer(menu->getPopup());
				
				/* Pop down the menu: */
				getWidgetManager()->popdownWidget(menu->getPopup());
				
				/* Deactivate the tool: */
				deactivate();
				}
			}
		}
	}

void RayMenuTool::frame(void)
	{
	if(configuration.interactWithWidgets||GUIInteractor::isActive())
		{
		/* Update the GUI interactor: */
		GUIInteractor::updateRay();
		GUIInteractor::move();
		}
	}

void RayMenuTool::display(GLContextData& contextData) const
	{
	if(isDrawRay()&&(configuration.interactWithWidgets||GUIInteractor::isActive()))
		{
		/* Draw the GUI interactor's state: */
		GUIInteractor::glRenderAction(getRayWidth(),getRayColor(),contextData);
		}
	}

Point RayMenuTool::calcHotSpot(void) const
	{
	/* Calculate the interaction position: */
	Point rayOrigin=getButtonDevicePosition(0);
	if(!getButtonDevice(0)->isRayDevice())
		{
		/* For 6-DOF devices, offset the menu by some amount: */
		rayOrigin+=getRay().getDirection()*configuration.initialMenuOffset;
		}
	return getUiManager()->projectRay(Ray(rayOrigin,getRay().getDirection()));
	}

}

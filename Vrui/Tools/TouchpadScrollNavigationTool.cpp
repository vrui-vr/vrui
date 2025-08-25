/***********************************************************************
TouchpadScrollNavigationTool - Class to scroll using a linear touch pad
device.
Copyright (c) 2025 Oliver Kreylos

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

#include <Vrui/Tools/TouchpadScrollNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/*******************************************************************
Methods of class TouchpadScrollNavigationToolFactory::Configuration:
*******************************************************************/

TouchpadScrollNavigationToolFactory::Configuration::Configuration(void)
	:scrollDirection(0,0,1),scrollFactor(1)
	{
	}

void TouchpadScrollNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./scrollDirection",scrollDirection);
	cfs.updateValue("./scrollFactor",scrollFactor);
	}

void TouchpadScrollNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./scrollDirection",scrollDirection);
	cfs.storeValue("./scrollFactor",scrollFactor);
	}

/****************************************************
Methods of class TouchpadScrollNavigationToolFactory:
****************************************************/

TouchpadScrollNavigationToolFactory::TouchpadScrollNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("TouchpadScrollNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(0);
	layout.setNumValuators(2);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	TouchpadScrollNavigationTool::factory=this;
	}

TouchpadScrollNavigationToolFactory::~TouchpadScrollNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	TouchpadScrollNavigationTool::factory=0;
	}

const char* TouchpadScrollNavigationToolFactory::getName(void) const
	{
	return "Touchpad Scrolling";
	}

const char* TouchpadScrollNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Enable";
		
		case 1:
			return "Scroll";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

Tool* TouchpadScrollNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new TouchpadScrollNavigationTool(this,inputAssignment);
	}

void TouchpadScrollNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveTouchpadScrollNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createTouchpadScrollNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	TouchpadScrollNavigationToolFactory* factory=new TouchpadScrollNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyTouchpadScrollNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*****************************************************
Static elements of class TouchpadScrollNavigationTool:
*****************************************************/

TouchpadScrollNavigationToolFactory* TouchpadScrollNavigationTool::factory=0;

/*********************************************
Methods of class TouchpadScrollNavigationTool:
*********************************************/

TouchpadScrollNavigationTool::TouchpadScrollNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 configuration(TouchpadScrollNavigationTool::factory->configuration),
	 lastScrollValue(0)
	{
	}

void TouchpadScrollNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void TouchpadScrollNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

const ToolFactory* TouchpadScrollNavigationTool::getFactory(void) const
	{
	return factory;
	}

void TouchpadScrollNavigationTool::valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			if(cbData->newValuatorValue>0.0)
				{
				/* Try activating this tool: */
				if(activate())
					{
					/* Initialize the scroll valuator value: */
					lastScrollValue=getValuatorState(1);
					}
				}
			else
				{
				/* Deactivate this tool: */
				deactivate();
				}
			break;
		}
	}

void TouchpadScrollNavigationTool::frame(void)
	{
	if(isActive())
		{
		/* Apply this frame's scrolling transformation: */
		double scrollValue=getValuatorState(1);
		NavTransform nav=getNavigationTransformation();
		nav.leftMultiply(NavTransform::translate(configuration.scrollDirection*((scrollValue-lastScrollValue)*configuration.scrollFactor)));
		setNavigationTransformation(nav);
		
		lastScrollValue=scrollValue;
		}
	}

}

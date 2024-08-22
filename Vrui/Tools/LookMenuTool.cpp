/***********************************************************************
LookMenuTool - Class for menu tools that show the program's main menu
when the main viewer is looking at it and allow any widget interaction
tool to select items from it.
Copyright (c) 2023 Oliver Kreylos

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

#include <Vrui/Tools/LookMenuTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/***************************************************
Methods of class LookMenuToolFactory::Configuration:
***************************************************/

LookMenuToolFactory::Configuration::Configuration(void)
	:maxActivationAngleCos(Math::cos(Math::rad(Scalar(22.5)))),
	 menuOffset(Scalar(2.5)*getInchFactor()),
	 trackDevice(true)
	{
	}

void LookMenuToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	/* Update the maximum activation angle: */
	Scalar maxActivationAngleDeg=Math::deg(Math::acos(maxActivationAngleCos));
	cfs.updateValue("./maxActivationAngle",maxActivationAngleDeg);
	maxActivationAngleCos=Math::cos(Math::rad(maxActivationAngleDeg));
	
	/* Update the rest of the settings: */
	cfs.updateValue("./menuOffset",menuOffset);
	cfs.updateValue("./trackDevice",trackDevice);
	}

void LookMenuToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	/* Write the maximum activation angle: */
	cfs.storeValue("./maxActivationAngle",Math::deg(Math::acos(maxActivationAngleCos)));
	
	/* Write the rest of the settings: */
	cfs.storeValue("./menuOffset",menuOffset);
	cfs.storeValue("./trackDevice",trackDevice);
	}

/************************************
Methods of class LookMenuToolFactory:
************************************/

LookMenuToolFactory::LookMenuToolFactory(ToolManager& toolManager)
	:ToolFactory("LookMenuTool",toolManager)
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
	LookMenuTool::factory=this;
	}

LookMenuToolFactory::~LookMenuToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	LookMenuTool::factory=0;
	}

const char* LookMenuToolFactory::getName(void) const
	{
	return "Look-Enabled Menu";
	}

const char* LookMenuToolFactory::getButtonFunction(int) const
	{
	return "Activate";
	}

Tool* LookMenuToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new LookMenuTool(this,inputAssignment);
	}

void LookMenuToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveLookMenuToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("MenuTool");
	}

extern "C" ToolFactory* createLookMenuToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	LookMenuToolFactory* factory=new LookMenuToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyLookMenuToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*************************************
Static elements of class LookMenuTool:
*************************************/

LookMenuToolFactory* LookMenuTool::factory=0;

/*****************************
Methods of class LookMenuTool:
*****************************/

LookMenuTool::LookMenuTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:MenuTool(factory,inputAssignment),
	 configuration(LookMenuTool::factory->configuration)
	{
	}

LookMenuTool::~LookMenuTool(void)
	{
	if(isActive())
		{
		/* Pop down the menu: */
		getWidgetManager()->popdownWidget(menu->getPopup());
		
		/* Deactivate the tool again: */
		deactivate();
		}
	}

void LookMenuTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Update the tool configuration: */
	configuration.read(configFileSection);
	}

void LookMenuTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write the tool configuration: */
	configuration.write(configFileSection);
	}

const ToolFactory* LookMenuTool::getFactory(void) const
	{
	return factory;
	}

void LookMenuTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		/* Activate the tool and show the menu if the source device is currently looking at the main viewer: */
		Point devicePos=getButtonDevicePosition(0);
		Vector viewDir=getMainViewer()->getEyePosition(Viewer::MONO)-devicePos;
		Vector rayDir=getButtonDeviceRayDirection(0);
		rayDir.normalize();
		if(viewDir*rayDir>=configuration.maxActivationAngleCos*viewDir.mag()&&activate())
			{
			/* Calculate the menu transformation: */
			menuTransform=ONTransform::translateFromOriginTo(devicePos+rayDir*configuration.menuOffset);
			Vector x=getUpDirection()^rayDir;
			Vector y=rayDir^x;
			menuTransform*=ONTransform::rotate(ONTransform::Rotation::fromBaseVectors(x,y));
			Point bottomLeft(menu->getPopup()->getExterior().getCorner(0).getXyzw());
			Point topRight(menu->getPopup()->getExterior().getCorner(3).getXyzw());
			menuTransform*=ONTransform::translateToOriginFrom(Geometry::mid(bottomLeft,topRight));
			menuTransform.leftMultiply(Geometry::invert(getButtonDeviceTransformation(0)));
			menuTransform.renormalize();
			
			/* Pop up the menu: */
			getWidgetManager()->popupPrimaryWidget(menu->getPopup(),GLMotif::WidgetManager::Transformation(getButtonDeviceTransformation(0)*menuTransform));
			}
		}
	else if(isActive())
		{
		/* Pop down the menu and deactivate the tool: */
		getWidgetManager()->popdownWidget(menu->getPopup());
		deactivate();
		}
	}

void LookMenuTool::frame(void)
	{
	if(isActive()&&configuration.trackDevice)
		{
		/* Update the menu's position: */
		getWidgetManager()->setPrimaryWidgetTransformation(menu->getPopup(),GLMotif::WidgetManager::Transformation(getButtonDeviceTransformation(0)*menuTransform));
		}
	}

}

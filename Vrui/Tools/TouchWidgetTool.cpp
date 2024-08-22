/***********************************************************************
TouchWidgetTool - Class for tools that can interact with GLMotif GUI
widgets by touch.
Copyright (c) 2021-2023 Oliver Kreylos

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

#include <Vrui/Tools/TouchWidgetTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Widget.h>
#include <GLMotif/Event.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/ToolManager.h>
#include <Vrui/UIManager.h>

namespace Vrui {

/******************************************************
Methods of class TouchWidgetToolFactory::Configuration:
******************************************************/

TouchWidgetToolFactory::Configuration::Configuration(void)
	:popUpMainMenu(true),alignWidgets(false)
	{
	}

void TouchWidgetToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	/* Retrieve configuration settings: */
	cfs.updateValue("./popUpMainMenu",popUpMainMenu);
	cfs.updateValue("./alignWidgets",alignWidgets);
	}

void TouchWidgetToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	/* Store configuration settings: */
	cfs.storeValue("./popUpMainMenu",popUpMainMenu);
	cfs.storeValue("./alignWidgets",alignWidgets);
	}

/***************************************
Methods of class TouchWidgetToolFactory:
***************************************/

TouchWidgetToolFactory::TouchWidgetToolFactory(ToolManager& toolManager)
	:ToolFactory("TouchWidgetTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("MenuTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	TouchWidgetTool::factory=this;
	}

TouchWidgetToolFactory::~TouchWidgetToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	TouchWidgetTool::factory=0;
	}

const char* TouchWidgetToolFactory::getName(void) const
	{
	return "Touch Interaction";
	}

const char* TouchWidgetToolFactory::getButtonFunction(int) const
	{
	return "Activate";
	}

Tool* TouchWidgetToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new TouchWidgetTool(this,inputAssignment);
	}

void TouchWidgetToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveTouchWidgetToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("MenuTool");
	}

extern "C" ToolFactory* createTouchWidgetToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	TouchWidgetToolFactory* factory=new TouchWidgetToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyTouchWidgetToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class TouchWidgetTool:
****************************************/

TouchWidgetToolFactory* TouchWidgetTool::factory=0;

/********************************
Methods of class TouchWidgetTool:
********************************/

void TouchWidgetTool::calcTouchPlane(GLMotif::Widget* widget,const Point& toolPos)
	{
	if(widget!=0&&widget!=target)
		{
		/* Find the given widget's widget transformation: */
		GLMotif::WidgetManager::Transformation widgetT=getWidgetManager()->calcWidgetTransformation(widget);
		
		/* Create the given widget's front plane and ensure it contains the current tool position: */
		Point frontP(widget->getExterior().origin.getXyzw());
		frontP[2]=Math::max(Scalar(widget->getZRange().second),widgetT.inverseTransform(toolPos)[2]);
		frontP[2]+=getUiSize()*Scalar(0.25); // Add a fudge factor
		touchPlane=Plane(widgetT.getDirection(2),widgetT.transform(frontP));
		}
	}

Ray TouchWidgetTool::calcTouchRay(const Point& toolPos) const
	{
	/* Create a ray through the tool position's projection onto the touch plane that starts a little ahead of the touch plane: */
	return Ray(touchPlane.project(toolPos)+touchPlane.getNormal()*Scalar(getUiSize()),-touchPlane.getNormal());
	}

TouchWidgetTool::TouchWidgetTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:MenuTool(factory,inputAssignment),
	 GUIInteractor(true,0,getButtonDevice(0)),
	 configuration(TouchWidgetTool::factory->configuration),
	 active(false),touching(false),target(0)
	{
	}

void TouchWidgetTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Update the tool configuration: */
	configuration.read(configFileSection);
	}

void TouchWidgetTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write the tool configuration: */
	configuration.write(configFileSection);
	}

const ToolFactory* TouchWidgetTool::getFactory(void) const
	{
	return factory;
	}

namespace {

/****************
Helper functions:
****************/

bool isCloseToWidget(const Point& eyePos,const Point& toolPos,GLMotif::WidgetManager::PoppedWidgetIterator begin,GLMotif::WidgetManager::PoppedWidgetIterator end)
	{
	/* Define slack for widget detection: */
	Scalar slack=getUiSize()*Scalar(4);
	bool result=false;
	for(GLMotif::WidgetManager::PoppedWidgetIterator pwIt=begin;!result&&pwIt!=end;++pwIt)
		if(pwIt.isVisible())
			{
			/* Transform the eye and tool positions to the widget's coordinate system: */
			OGTransform widgetTransform(pwIt.getWidgetToWorld());
			Point widgetEyePos=widgetTransform.inverseTransform(eyePos);
			Point widgetToolPos=widgetTransform.inverseTransform(toolPos);
			
			/* Check if the tool is inside the pyramid formed by the widget's extended exterior and the eye: */
			GLMotif::Box widgetBox=(*pwIt)->getExterior().outset(GLMotif::Vector(slack,slack,0));
			widgetBox.origin[2]=(*pwIt)->getZRange().second;
			Point c[4];
			for(int i=0;i<4;++i)
				c[i]=Point(widgetBox.getCorner(i).getXyzw());
			Plane p0((c[1]-c[0])^(c[0]-widgetEyePos),widgetEyePos);
			Plane p1((c[3]-c[1])^(c[1]-widgetEyePos),widgetEyePos);
			Plane p2((c[2]-c[3])^(c[3]-widgetEyePos),widgetEyePos);
			Plane p3((c[0]-c[2])^(c[2]-widgetEyePos),widgetEyePos);
			result=p0.calcDistance(widgetToolPos)>=Scalar(0)&&p1.calcDistance(widgetToolPos)>=Scalar(0)&&p2.calcDistance(widgetToolPos)>=Scalar(0)&&p3.calcDistance(widgetToolPos)>=Scalar(0);
			
			/* Recurse into the popup binding's secondary bindings if necessary: */
			result=result||isCloseToWidget(widgetEyePos,widgetToolPos,pwIt.beginSecondaryWidgets(),pwIt.endSecondaryWidgets());
			}
	return result;
	}

}

void TouchWidgetTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Try activating this tool: */
		active=getUiManager()->activateGuiInteractor(this);
		
		/* Check if the tool should pop up the main menu if not close to any existing widgets: */
		if(configuration.popUpMainMenu)
			{
			/* Check if the touch tool is close enough to any popped-up widgets not to pop open a context menu: */
			Point eyePos=getMainViewer()->getHeadPosition();
			Point toolPos=getButtonDevicePosition(0);
			if(!isCloseToWidget(eyePos,toolPos,getWidgetManager()->beginPrimaryWidgets(),getWidgetManager()->endPrimaryWidgets())&&activate())
				{
				/* Update the GUI interactor: */
				GUIInteractor::updateRay();
				
				/* Pop up the main menu some distance behind the tool position from the main viewer's point of view: */
				Vector rayDir=toolPos-eyePos;
				rayDir*=getUiSize()*Scalar(8)/rayDir.mag();
				Point hotSpot=getUiManager()->projectRay(Ray(toolPos+rayDir,rayDir));
				popupPrimaryWidget(menu->getPopup(),hotSpot,false);
				}
			}
		}
	else // Button has just been released
		{
		/* Stop an ongoing interaction: */
		if(isDragging()||touching)
			{
			/* Deliver a button-up event: */
			GLMotif::Event event(calcTouchRay(getButtonDevicePosition(0)),true);
			getWidgetManager()->pointerButtonUp(event);
			
			/* Stop dragging: */
			if(isDragging())
				stopDragging();
			}
		touching=false;
		target=0;
		
		/* Pop down the main menu and deactivate the menu tool if it was active: */
		if(MenuTool::isActive())
			{
			getWidgetManager()->popdownWidget(menu->getPopup());
			deactivate();
			}
		
		/* Deactivate this tool: */
		if(active)
			getUiManager()->deactivateGuiInteractor(this);
		active=false;
		}
	}

void TouchWidgetTool::frame(void)
	{
	if(active)
		{
		/* Update the GUI interactor: */
		GUIInteractor::updateRay();
		
		/* Check if there is an ongoing interaction: */
		if(isDragging())
			{
			/* Continue dragging: */
			drag(getButtonDeviceTransformation(0),configuration.alignWidgets);
			}
		else if(touching)
			{
			/* Check if the tool position is behind the touch plane: */
			Point toolPos=getButtonDevicePosition(0);
			GLMotif::Event event(calcTouchRay(toolPos),true);
			if(touchPlane.contains(toolPos))
				{
				/* Continue the interaction by sending a pointer motion event: */
				getWidgetManager()->pointerMotion(event);
				
				/* Update the touch plane: */
				GLMotif::Widget* newTarget=event.getTargetWidget();
				calcTouchPlane(newTarget,toolPos);
				target=newTarget;
				}
			else
				{
				/* End the current interaction: */
				getWidgetManager()->pointerButtonUp(event);
				target=0;
				touching=false;
				}
			}
		else
			{
			/* Check if any top-level widget is being touched: */
			Point toolPos=getButtonDevicePosition(0);
			GLMotif::Widget* topLevel=getWidgetManager()->findPrimaryWidget(toolPos);
			if(topLevel!=0)
				{
				/* Set up a touch plane for the found top-level widget: */
				calcTouchPlane(topLevel,toolPos);
				
				/* Start a new interaction by sending a button down event: */
				GLMotif::Event event(calcTouchRay(toolPos),false);
				if(getWidgetManager()->pointerButtonDown(event))
					{
					/* Try dragging the selected widget first, send it pointer events if it can't be dragged: */
					if(!startDragging(event,getButtonDeviceTransformation(0)))
						{
						/* Update the touch plane: */
						GLMotif::Widget* newTarget=event.getTargetWidget();
						calcTouchPlane(newTarget,toolPos);
						target=newTarget;
						
						touching=true;
						}
					}
				}
			}
		}
	}

}

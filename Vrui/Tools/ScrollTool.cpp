/***********************************************************************
ScrollTool - Class for tools that can scroll inside certain GLMotif GUI
widgets. ScrollTool objects are cascadable and prevent valuator events
if they would fall into the area of interest of scrollable widgets.
Copyright (c) 2011-2026 Oliver Kreylos

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

#include <Vrui/Tools/ScrollTool.h>

#include <Misc/StdError.h>
#include <Math/Math.h>
#include <GLMotif/TextControlEvent.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************
Methods of class ScrollToolFactory:
**********************************/

ScrollToolFactory::ScrollToolFactory(ToolManager& toolManager)
	:ToolFactory("ScrollTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(2);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UserInterfaceTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Set tool class' factory pointer: */
	ScrollTool::factory=this;
	}

ScrollToolFactory::~ScrollToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ScrollTool::factory=0;
	}

const char* ScrollToolFactory::getName(void) const
	{
	return "GUI Scrolling";
	}

const char* ScrollToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Scroll Up";
		
		case 1:
			return "Scroll Down";
		
		default:
			return 0;
		}
	}

Tool* ScrollToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ScrollTool(this,inputAssignment);
	}

void ScrollToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveScrollToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UserInterfaceTool");
	}

extern "C" ToolFactory* createScrollToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ScrollToolFactory* widgetToolFactory=new ScrollToolFactory(*toolManager);
	
	/* Return factory object: */
	return widgetToolFactory;
	}

extern "C" void destroyScrollToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************
Static elements of class ScrollTool:
***********************************/

ScrollToolFactory* ScrollTool::factory=0;

/***************************
Methods of class ScrollTool:
***************************/

ScrollTool::ScrollTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UserInterfaceTool(factory,inputAssignment),
	 GUIInteractor(false,0,getButtonDevice(0)),
	 wheelDevice(0),interceptedEvent(false)
	{
	}

void ScrollTool::initialize(void)
	{
	/* Create a virtual input device to shadow the wheel buttons: */
	InputDevice* sourceDevice=getButtonDevice(0);
	std::string wheelDeviceName=sourceDevice->getDeviceName();
	wheelDeviceName.append("-ForwardedWheel");
	wheelDevice=addVirtualInputDevice(wheelDeviceName.c_str(),2,0);
	
	/* Copy the source device's tracking type: */
	wheelDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(wheelDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(wheelDevice,this);
	
	/* Initialize the virtual input device's position: */
	wheelDevice->copyTrackingState(sourceDevice);
	}

void ScrollTool::deinitialize(void)
	{
	/* Release the virtual input device: */
	getInputGraphManager()->releaseInputDevice(wheelDevice,this);
	
	/* Destroy the virtual input device: */
	getInputDeviceManager()->destroyInputDevice(wheelDevice);
	wheelDevice=0;
	}

const ToolFactory* ScrollTool::getFactory(void) const
	{
	return factory;
	}

void ScrollTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pushed
		{
		/* Check if the GUI interactor accepts the event: */
		GUIInteractor::updateRay();
		GLMotif::TextControlEvent tce(buttonSlotIndex==0?GLMotif::TextControlEvent::CURSOR_UP:GLMotif::TextControlEvent::CURSOR_DOWN);
		interceptedEvent=GUIInteractor::textControl(tce);
		
		/* If the event was not accepted, forward the button press to the wheel device: */
		if(!interceptedEvent)
			wheelDevice->setButtonState(buttonSlotIndex,true);
		}
	else // Button has just been released
		{
		/* If the previous button press event was forwarded to the wheel device, forward this release event as well: */
		if(!interceptedEvent)
			wheelDevice->setButtonState(buttonSlotIndex,false);
		}
	}

void ScrollTool::frame(void)
	{
	/* Update the GUI interactor: */
	GUIInteractor::updateRay();
	GUIInteractor::move();
	
	/* Update the virtual input device: */
	wheelDevice->copyTrackingState(getButtonDevice(0));
	}

void ScrollTool::display(GLContextData& contextData) const
	{
	if(isDrawRay())
		{
		/* Draw the GUI interactor's state: */
		GUIInteractor::glRenderAction(getRayWidth(),getRayColor(),contextData);
		}
	}

std::vector<InputDevice*> ScrollTool::getForwardedDevices(void)
	{
	std::vector<InputDevice*> result;
	result.push_back(wheelDevice);
	return result;
	}

InputDeviceFeatureSet ScrollTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the wheel device: */
	if(forwardedFeature.getDevice()!=wheelDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on forwarded device");
	
	/* Return the source feature: */
	InputDeviceFeatureSet result;
	result.push_back(input.getButtonSlotFeature(forwardedFeature.getFeatureIndex()));
	return result;
	}

InputDevice* ScrollTool::getSourceDevice(const InputDevice* forwardedDevice)
	{
	/* Paranoia: Check if the forwarded device is the same as the wheel device: */
	if(forwardedDevice!=wheelDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Given forwarded device is not forwarded device");
	
	/* Return the designated source device: */
	return getButtonDevice(0);
	}

InputDeviceFeatureSet ScrollTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Paranoia: Check if the source feature belongs to this tool: */
	if(input.findFeature(sourceFeature)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* Return the forwarded feature: */
	InputDeviceFeatureSet result;
	result.push_back(InputDeviceFeature(wheelDevice,InputDevice::BUTTON,sourceFeature.getFeatureIndex()));
	return result;
	}

}

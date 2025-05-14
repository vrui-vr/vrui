/***********************************************************************
StylusTool - Class to represent styluses on touchscreen-like devices,
where a set of selector buttons changes the function triggered by the
activation of a main button.
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

#include <Vrui/Tools/StylusTool.h>

#include <Misc/StdError.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************
Methods of class StylusToolFactory:
**********************************/

StylusToolFactory::StylusToolFactory(ToolManager& toolManager)
	:ToolFactory("StylusTool",toolManager)
	{
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Initialize tool layout: */
	layout.setNumButtons(1,true);
	
	/* Set tool class's factory pointer: */
	StylusTool::factory=this;
	}

StylusToolFactory::~StylusToolFactory(void)
	{
	/* Reset tool class's factory pointer: */
	StylusTool::factory=0;
	}

const char* StylusToolFactory::getName(void) const
	{
	return "Stylus Adapter";
	}

const char* StylusToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	if(buttonSlotIndex==0)
		return "Main Button";
	else
		return "Selector Button";
	}

Tool* StylusToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new StylusTool(this,inputAssignment);
	}

void StylusToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveStylusToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createStylusToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	StylusToolFactory* factory=new StylusToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyStylusToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************
Static elements of class StylusTool:
***********************************/

StylusToolFactory* StylusTool::factory=0;

/***************************
Methods of class StylusTool:
***************************/

StylusTool::StylusTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 selectorButtonIndex(0)
	{
	}

StylusTool::~StylusTool(void)
	{
	}

void StylusTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	transformedDevice=addVirtualInputDevice("StylusToolTransformedDevice",input.getNumButtonSlots(),0);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	}

const ToolFactory* StylusTool::getFactory(void) const
	{
	return factory;
	}

void StylusTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Check if the main button or one of the selector buttons was pressed: */
	if(buttonSlotIndex==0)
		{
		/* Set the state of the selected virtual input device button to the state of the main button: */
		transformedDevice->setButtonState(selectorButtonIndex,cbData->newButtonState);
		}
	else
		{
		/* Determine the potentially changed selector button index: */
		int newSelectorButtonIndex=selectorButtonIndex;
		
		/* Check if the button was pressed or released: */
		if(cbData->newButtonState)
			{
			/* Check if the newly pressed selector button has higher priority than the currently pressed one: */
			if(selectorButtonIndex<buttonSlotIndex)
				newSelectorButtonIndex=buttonSlotIndex;
			}
		else
			{
			/* Check if the newly released selector button was the highest-priority one: */
			if(selectorButtonIndex==buttonSlotIndex)
				{
				/* Find the next-highest priority pressed selector button: */
				for(--newSelectorButtonIndex;newSelectorButtonIndex>0&&!getButtonState(newSelectorButtonIndex);--newSelectorButtonIndex)
					;
				}
			}
		
		/* Check if the selector button changed: */
		if(selectorButtonIndex!=newSelectorButtonIndex)
			{
			/* Reset the previously-active virtual input device button: */
			transformedDevice->setButtonState(selectorButtonIndex,cbData->newButtonState);
			
			/* Update the current selector button index: */
			selectorButtonIndex=newSelectorButtonIndex;
			
			/* Set the state of the selected virtual input device button to the state of the main button: */
			transformedDevice->setButtonState(selectorButtonIndex,getButtonState(0));
			}
		}
	}

InputDeviceFeatureSet StylusTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on transformed device");
	
	/* Return the main button slot: */
	InputDeviceFeatureSet result;
	result.push_back(input.getButtonSlotFeature(0));
	
	return result;
	}

InputDeviceFeatureSet StylusTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* Get the slot's button slot index: */
	int buttonSlotIndex=input.getButtonSlotIndex(slotIndex);
	
	/* If the source feature is the main button, return all forwarded buttons; otherwise, return the empty set: */
	InputDeviceFeatureSet result;
	if(buttonSlotIndex==0)
		for(int i=0;i<transformedDevice->getNumButtons();++i)
			result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,i));
	
	return result;
	}

}

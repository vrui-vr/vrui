/***********************************************************************
MultiShiftButtonTool - Class to switch between mulitple planes of
buttons and/or valuators by pressing one from an array of "radio
buttons."
Copyright (c) 2012-2026 Oliver Kreylos

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

#include <Vrui/Tools/MultiShiftButtonTool.h>

#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/***********************************************************
Methods of class MultiShiftButtonToolFactory::Configuration:
***********************************************************/

MultiShiftButtonToolFactory::Configuration::Configuration(void)
	:numPlanes(2),
	 forwardRadioButtons(false),
	 resetFeatures(false),
	 initialPlane(0)
	{
	}

void MultiShiftButtonToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./numPlanes",numPlanes);
	cfs.updateValue("./forwardRadioButtons",forwardRadioButtons);
	cfs.updateValue("./resetFeatures",resetFeatures);
	cfs.updateValue("./initialPlane",initialPlane);
	}

void MultiShiftButtonToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./numPlanes",numPlanes);
	cfs.storeValue("./forwardRadioButtons",forwardRadioButtons);
	cfs.storeValue("./resetFeatures",resetFeatures);
	cfs.storeValue("./initialPlane",initialPlane);
	}

/********************************************
Methods of class MultiShiftButtonToolFactory:
********************************************/

MultiShiftButtonToolFactory::MultiShiftButtonToolFactory(ToolManager& toolManager)
	:ToolFactory("MultiShiftButtonTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1,true);
	layout.setNumValuators(0,true);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	config.read(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	MultiShiftButtonTool::factory=this;
	}

MultiShiftButtonToolFactory::~MultiShiftButtonToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	MultiShiftButtonTool::factory=0;
	}

const char* MultiShiftButtonToolFactory::getName(void) const
	{
	return "Radio Buttons";
	}

const char* MultiShiftButtonToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	if(buttonSlotIndex==0)
		return "First Radio Button";
	else
		return "Additional Radio or Forwarded Button";
	}

Tool* MultiShiftButtonToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MultiShiftButtonTool(this,inputAssignment);
	}

void MultiShiftButtonToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMultiShiftButtonToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createMultiShiftButtonToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MultiShiftButtonToolFactory* multiShiftButtonToolFactory=new MultiShiftButtonToolFactory(*toolManager);
	
	/* Return factory object: */
	return multiShiftButtonToolFactory;
	}

extern "C" void destroyMultiShiftButtonToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*********************************************
Static elements of class MultiShiftButtonTool:
*********************************************/

MultiShiftButtonToolFactory* MultiShiftButtonTool::factory=0;

/*************************************
Methods of class MultiShiftButtonTool:
*************************************/

void MultiShiftButtonTool::changePlane(int nextPlane)
	{
	/* Bail out if the plane won't actually change: */
	if(currentPlane==nextPlane)
		return;
	
	/* Deactivate the currently mapped plane if it is valid: */
	if(currentPlane>=0)
		{
		/* Disable all features on the currently mapped plane: */
		int buttonBase=currentPlane*numForwardedButtons;
		int valuatorBase=currentPlane*input.getNumValuatorSlots();
		
		if(config.forwardRadioButtons)
			{
			/* Disable the forwarded radio button on the currently mapped plane: */
			transformedDevice->setButtonState(buttonBase,false);
			}
		
		if(config.resetFeatures)
			{
			/* Reset all buttons and valuators in the currently mapped plane: */
			for(int i=firstForwardedButton;i<numForwardedButtons;++i)
				transformedDevice->setButtonState(buttonBase+i,false);
			for(int i=0;i<input.getNumValuatorSlots();++i)
				transformedDevice->setValuator(valuatorBase+i,0.0);
			}
		}
	
	/* Activate the next plane if it is valid: */
	if(nextPlane>=0)
		{
		/* Copy all forwarded source features to the next mapped plane: */
		int buttonBase=nextPlane*numForwardedButtons;
		int valuatorBase=nextPlane*input.getNumValuatorSlots();
		
		if(config.forwardRadioButtons)
			{
			/* Enable the forwarded radio button on the next mapped plane: */
			transformedDevice->setButtonState(buttonBase,true);
			}
		
		/* Copy all source buttons and valuators to the next mapped plane: */
		for(int i=firstForwardedButton;i<numForwardedButtons;++i)
			transformedDevice->setButtonState(buttonBase+i,getButtonState(config.numPlanes+i-firstForwardedButton));
		for(int i=0;i<input.getNumValuatorSlots();++i)
			transformedDevice->setValuator(valuatorBase+i,getValuatorState(i));
		}
	
	/* Change the currently active plane: */
	currentPlane=nextPlane;
	}

MultiShiftButtonTool::MultiShiftButtonTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:TransformTool(sFactory,inputAssignment),
	 config(factory->config),
	 numForwardedButtons(0),firstForwardedButton(0),currentPlane(-1)
	{
	}

MultiShiftButtonTool::~MultiShiftButtonTool(void)
	{
	}

void MultiShiftButtonTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read settings: */
	config.read(configFileSection);
	}

void MultiShiftButtonTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write settings: */
	config.write(configFileSection);
	}

void MultiShiftButtonTool::initialize(void)
	{
	/* Set the transformation source device: */
	if(input.getNumButtonSlots()>config.numPlanes)
		sourceDevice=getButtonDevice(config.numPlanes);
	else if(input.getNumValuatorSlots()>0)
		sourceDevice=getValuatorDevice(0);
	else
		sourceDevice=getButtonDevice(0); // User didn't select anything to forward; let's just pretend it makes sense
	
	/* Create a virtual input device to shadow the source input device: */
	numForwardedButtons=input.getNumButtonSlots()-config.numPlanes;
	firstForwardedButton=0;
	if(config.forwardRadioButtons)
		{
		++numForwardedButtons;
		++firstForwardedButton;
		}
	transformedDevice=addVirtualInputDevice("MultiShiftButtonToolTransformedDevice",config.numPlanes*numForwardedButtons,config.numPlanes*input.getNumValuatorSlots());
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	
	/* Activate the initial button/valuator plane: */
	changePlane(config.initialPlane);
	}

void MultiShiftButtonTool::deinitialize(void)
	{
	/* Release the virtual input device: */
	getInputGraphManager()->releaseInputDevice(transformedDevice,this);
	
	/* Destroy the virtual input device: */
	getInputDeviceManager()->destroyInputDevice(transformedDevice);
	transformedDevice=0;
	}

const ToolFactory* MultiShiftButtonTool::getFactory(void) const
	{
	return factory;
	}

void MultiShiftButtonTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonSlotIndex<config.numPlanes)
		{
		/* Change the currently mapped plane if the button was pressed: */
		if(cbData->newButtonState)
			changePlane(buttonSlotIndex);
		}
	else if(currentPlane>=0)
		{
		/* Pass the button event through to the virtual input device: */
		int buttonBase=currentPlane*numForwardedButtons;
		transformedDevice->setButtonState(buttonBase+firstForwardedButton+buttonSlotIndex-config.numPlanes,cbData->newButtonState);
		}
	}

void MultiShiftButtonTool::valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	if(currentPlane>=0)
		{
		/* Pass the valuator event through to the virtual input device: */
		int valuatorBase=currentPlane*input.getNumValuatorSlots();
		transformedDevice->setValuator(valuatorBase+valuatorSlotIndex,cbData->newValuatorValue);
		}
	}

void MultiShiftButtonTool::frame(void)
	{
	/* Set the forwarded device's position and orientation: */
	resetDevice();
	}

InputDeviceFeatureSet MultiShiftButtonTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on transformed device");
	
	/* Create an empty feature set: */
	InputDeviceFeatureSet result;
	
	if(forwardedFeature.isButton())
		{
		/* Find the source button slot index: */
		int buttonSlotIndex=forwardedFeature.getIndex();
		buttonSlotIndex%=numForwardedButtons;
		
		/* Add the button slot's feature to the result set: */
		result.push_back(input.getButtonSlotFeature(buttonSlotIndex+firstForwardedButton));
		}
	
	if(forwardedFeature.isValuator())
		{
		/* Find the source valuator slot index: */
		int valuatorSlotIndex=forwardedFeature.getIndex();
		valuatorSlotIndex%=input.getNumValuatorSlots();
		
		/* Add the valuator slot's feature to the result set: */
		result.push_back(input.getValuatorSlotFeature(valuatorSlotIndex));
		}
	
	return result;
	}

InputDeviceFeatureSet MultiShiftButtonTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* Create an empty feature set: */
	InputDeviceFeatureSet result;
	
	/* Check if the feature is a button or valuator: */
	if(sourceFeature.isButton())
		{
		/* Get the slot's button slot index: */
		int buttonSlotIndex=input.getButtonSlotIndex(slotIndex);
		
		/* Check if the button is part of the forwarded subset: */
		if(buttonSlotIndex>=firstForwardedButton)
			{
			/* Add the forwarded feature for the current button plane to the result set: */
			int buttonBase=currentPlane*numForwardedButtons;
			result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,buttonBase-firstForwardedButton+buttonSlotIndex));
			}
		}
	
	if(sourceFeature.isValuator())
		{
		/* Get the slot's valuator slot index: */
		int valuatorSlotIndex=input.getValuatorSlotIndex(slotIndex);
		
		/* Add the forwarded feature for the current chamber to the result set: */
		int valuatorBase=currentPlane*input.getNumValuatorSlots();
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::VALUATOR,valuatorSlotIndex+valuatorBase));
		}
	
	return result;
	}

}

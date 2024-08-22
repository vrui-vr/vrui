/***********************************************************************
ThumbstickButtonsTool - Transform an analog stick to multiple buttons
arranged around a circle.
Copyright (c) 2021-2024 Oliver Kreylos

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

#include <Vrui/Tools/ThumbstickButtonsTool.h>

#include <Misc/StdError.h>
#include <Misc/FixedArray.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Vrui/InputGraphManager.h>

namespace Vrui {

/************************************************************
Methods of class ThumbstickButtonsToolFactory::Configuration:
************************************************************/

ThumbstickButtonsToolFactory::Configuration::Configuration(void)
	:numButtons(4)
	{
	activationThresholds[0]=0.25;
	activationThresholds[1]=0.75;
	}

void ThumbstickButtonsToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./numButtons",numButtons);
	Misc::FixedArray<double,2> ats(activationThresholds);
	cfs.updateValue("./activationThresholds",ats);
	ats.writeElements(activationThresholds);
	}

void ThumbstickButtonsToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./numButtons",numButtons);
	cfs.storeValue("./activationThresholds",Misc::FixedArray<double,2>(activationThresholds));
	}

/*********************************************
Methods of class ThumbstickButtonsToolFactory:
*********************************************/

ThumbstickButtonsToolFactory::ThumbstickButtonsToolFactory(ToolManager& toolManager)
	:ToolFactory("ThumbstickButtonsTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumValuators(2);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	ThumbstickButtonsTool::factory=this;
	}

ThumbstickButtonsToolFactory::~ThumbstickButtonsToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ThumbstickButtonsTool::factory=0;
	}

const char* ThumbstickButtonsToolFactory::getName(void) const
	{
	return "Thumbstick -> Buttons";
	}

const char* ThumbstickButtonsToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	static const char* valuatorNames[2]=
		{
		"Thumbstick X Axis","Thumbstick Y Axis"
		};
	return valuatorNames[valuatorSlotIndex];
	}

Tool* ThumbstickButtonsToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ThumbstickButtonsTool(this,inputAssignment);
	}

void ThumbstickButtonsToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveThumbstickButtonsToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createThumbstickButtonsToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ThumbstickButtonsToolFactory* factory=new ThumbstickButtonsToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyThumbstickButtonsToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**********************************************
Static elements of class ThumbstickButtonsTool:
**********************************************/

ThumbstickButtonsToolFactory* ThumbstickButtonsTool::factory=0;

/**************************************
Methods of class ThumbstickButtonsTool:
**************************************/

ThumbstickButtonsTool::ThumbstickButtonsTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 configuration(ThumbstickButtonsTool::factory->configuration),
	 anglePerButton(0),pressedButton(-1)
	{
	}

ThumbstickButtonsTool::~ThumbstickButtonsTool(void)
	{
	}

void ThumbstickButtonsTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Update the class configuration: */
	configuration.read(configFileSection);
	}

void ThumbstickButtonsTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Store the tool configuration: */
	configuration.write(configFileSection);
	}

void ThumbstickButtonsTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	transformedDevice=addVirtualInputDevice("ThumbstickButtonsToolTransformedDevice",configuration.numButtons,0);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	
	/* Calculate the angle per button: */
	anglePerButton=2.0*Math::Constants<double>::pi/double(configuration.numButtons);
	}

const ToolFactory* ThumbstickButtonsTool::getFactory(void) const
	{
	return factory;
	}

void ThumbstickButtonsTool::valuatorCallback(int buttonSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Check if one of the buttons is currently pressed: */
	double x=getValuatorState(0);
	double y=getValuatorState(1);
	double r2=x*x+y*y;
	if(pressedButton>=0)
		{
		/* Release the button if the thumbstick returned to the center position: */
		if(r2<Math::sqr(configuration.activationThresholds[0]))
			{
			transformedDevice->setButtonState(pressedButton,false);
			pressedButton=-1;
			}
		}
	else
		{
		/* Press a button if the thumbstick left the center position: */
		if(r2>Math::sqr(configuration.activationThresholds[1]))
			{
			/* Calculate the index of the selected button: */
			double angle=Math::atan2(-x,y);
			if(angle<0.0)
				angle+=2.0*Math::Constants<double>::pi;
			pressedButton=int(Math::floor(angle/anglePerButton+0.5))%configuration.numButtons;
			
			/* Press the selected button: */
			transformedDevice->setButtonState(pressedButton,true);
			}
		}
	}

InputDeviceFeatureSet ThumbstickButtonsTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on transformed device");
	
	/* Return all input feature slots: */
	InputDeviceFeatureSet result;
	for(int i=0;i<2;++i)
		result.push_back(input.getValuatorSlotFeature(i));
	
	return result;
	}

InputDeviceFeatureSet ThumbstickButtonsTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* Return the currently pressed button: */
	InputDeviceFeatureSet result;
	if(pressedButton>=0)
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,pressedButton));
	
	return result;
	}

}

/***********************************************************************
ButtonToValuatorTool - Class to convert a single button or two buttons
into a two- or three-state valuator, respectively.
Copyright (c) 2011-2024 Oliver Kreylos

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

#include <Vrui/Tools/ButtonToValuatorTool.h>

#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/***********************************************************
Methods of class ButtonToValuatorToolFactory::Configuration:
***********************************************************/

ButtonToValuatorToolFactory::Configuration::Configuration(void)
	:deviceName("ButtonToValuatorToolTransformedDevice"),
	 mode(Immediate),
	 step(1),exponent(1)
	{
	}

void ButtonToValuatorToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateString("./deviceName",deviceName);
	if(cfs.hasTag("./mode"))
		{
		std::string modeString=cfs.retrieveValue<std::string>("./mode");
		if(modeString=="Immediate")
			mode=Immediate;
		else if(modeString=="Ramped")
			mode=Ramped;
		else if(modeString=="Incremental")
			mode=Incremental;
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid mode %s",modeString.c_str());
		}
	cfs.updateValue("./step",step);
	cfs.updateValue("./exponent",exponent);
	}

void ButtonToValuatorToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeString("./deviceName",deviceName);
	switch(mode)
		{
		case Immediate:
			cfs.storeString("./mode","Immediate");
			break;
		
		case Ramped:
			cfs.storeString("./mode","Ramped");
			break;
		
		case Incremental:
			cfs.storeString("./mode","Incremental");
			break;
		}
	cfs.storeValue("./step",step);
	cfs.storeValue("./exponent",exponent);
	}

/********************************************
Methods of class ButtonToValuatorToolFactory:
********************************************/

ButtonToValuatorToolFactory::ButtonToValuatorToolFactory(ToolManager& toolManager)
	:ToolFactory("ButtonToValuatorTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1,true);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	ButtonToValuatorTool::factory=this;
	}

ButtonToValuatorToolFactory::~ButtonToValuatorToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ButtonToValuatorTool::factory=0;
	}

const char* ButtonToValuatorToolFactory::getName(void) const
	{
	return "Button -> Valuator";
	}

const char* ButtonToValuatorToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "+1";
		
		case 1:
			return "-1";
		
		case 2:
			return "Reset";
		
		default:
			return "Unused";
		}
	}

Tool* ButtonToValuatorToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ButtonToValuatorTool(this,inputAssignment);
	}

void ButtonToValuatorToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveButtonToValuatorToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createButtonToValuatorToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ButtonToValuatorToolFactory* buttonToValuatorToolFactory=new ButtonToValuatorToolFactory(*toolManager);
	
	/* Return factory object: */
	return buttonToValuatorToolFactory;
	}

extern "C" void destroyButtonToValuatorToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*********************************************
Static elements of class ButtonToValuatorTool:
*********************************************/

ButtonToValuatorToolFactory* ButtonToValuatorTool::factory=0;

/*************************************
Methods of class ButtonToValuatorTool:
*************************************/

ButtonToValuatorTool::ButtonToValuatorTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 configuration(ButtonToValuatorTool::factory->configuration),
	 rawValue(0)
	{
	/* Set the transformation source device: */
	sourceDevice=getButtonDevice(0);
	}

ButtonToValuatorTool::~ButtonToValuatorTool(void)
	{
	}

void ButtonToValuatorTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void ButtonToValuatorTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void ButtonToValuatorTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	transformedDevice=addVirtualInputDevice(configuration.deviceName.c_str(),0,1);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	}

const ToolFactory* ButtonToValuatorTool::getFactory(void) const
	{
	return factory;
	}

void ButtonToValuatorTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Calculate the button's effect: */
	double delta=0.0;
	if(buttonSlotIndex==0)
		delta=configuration.step;
	else if(buttonSlotIndex==1)
		delta=-configuration.step;
	
	switch(configuration.mode)
		{
		case ButtonToValuatorToolFactory::Configuration::Immediate:
			if(cbData->newButtonState) // Button was just pressed
				rawValue+=delta;
			else // Button was just released
				rawValue-=delta;
			break;
		
		case ButtonToValuatorToolFactory::Configuration::Ramped:
			if(!cbData->newButtonState) // Button was just released
				rawValue=0.0;
			break;
		
		case ButtonToValuatorToolFactory::Configuration::Incremental:
			if(cbData->newButtonState) // Button was just pressed
				{
				if(buttonSlotIndex<2)
					rawValue+=delta;
				else
					rawValue=0.0;
				}
			break;
		}
	
	/* Clamp the raw valuator value and update the virtual valuator: */
	rawValue=Math::clamp(rawValue,-1.0,1.0);
	transformedDevice->setValuator(0,Math::copysign(Math::pow(Math::abs(rawValue),configuration.exponent),rawValue));
	}

void ButtonToValuatorTool::frame(void)
	{
	if(configuration.mode==ButtonToValuatorToolFactory::Configuration::Ramped)
		{
		/* Adjust the valuator value: */
		double step=configuration.step*Vrui::getFrameTime();
		if(getButtonState(0))
			rawValue+=step;
		if(getButtonState(1))
			rawValue-=step;
		
		/* Clamp the raw valuator value and update the virtual valuator: */
		rawValue=Math::clamp(rawValue,-1.0,1.0);
		transformedDevice->setValuator(0,Math::copysign(Math::pow(Math::abs(rawValue),configuration.exponent),rawValue));
		}
	}

InputDeviceFeatureSet ButtonToValuatorTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on transformed device");
	
	/* The source features are all button slots: */
	InputDeviceFeatureSet result;
	for(int i=0;i<input.getNumButtonSlots();++i)
		result.push_back(input.getButtonSlotFeature(i));
	
	return result;
	}

InputDeviceFeatureSet ButtonToValuatorTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* The only forwarded feature is the valuator slot: */
	InputDeviceFeatureSet result;
	result.push_back(InputDeviceFeature(transformedDevice,InputDevice::VALUATOR,0));
	
	return result;
	}

}

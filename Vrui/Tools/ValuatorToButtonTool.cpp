/***********************************************************************
ValuatorToButtonTool - Class to convert a single valuator into a pair of
buttons.
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

#include <Vrui/Tools/ValuatorToButtonTool.h>

#include <Misc/StdError.h>
#include <Misc/FixedArray.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/***********************************************************
Methods of class ValuatorToButtonToolFactory::Configuration:
***********************************************************/

ValuatorToButtonToolFactory::Configuration::Configuration(void)
	{
	posThresholds[0]=Scalar(0.7);
	posThresholds[1]=Scalar(0.3);
	negThresholds[0]=Scalar(-0.7);
	negThresholds[1]=Scalar(-0.3);
	}

void ValuatorToButtonToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	Misc::FixedArray<Scalar,2> pt(posThresholds);
	cfs.updateValue("./posThresholds",pt);
	pt.writeElements(posThresholds);
	Misc::FixedArray<Scalar,2> nt(negThresholds);
	cfs.updateValue("./negThresholds",nt);
	nt.writeElements(negThresholds);
	}

void ValuatorToButtonToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./posThresholds",Misc::FixedArray<Scalar,2>(posThresholds));
	cfs.storeValue("./negThresholds",Misc::FixedArray<Scalar,2>(negThresholds));
	}

/********************************************
Methods of class ValuatorToButtonToolFactory:
********************************************/

ValuatorToButtonToolFactory::ValuatorToButtonToolFactory(ToolManager& toolManager)
	:ToolFactory("ValuatorToButtonTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumValuators(1,true);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	ValuatorToButtonTool::factory=this;
	}

ValuatorToButtonToolFactory::~ValuatorToButtonToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ValuatorToButtonTool::factory=0;
	}

const char* ValuatorToButtonToolFactory::getName(void) const
	{
	return "Valuator -> Button";
	}

const char* ValuatorToButtonToolFactory::getValuatorFunction(int) const
	{
	return "Button Pair";
	}

Tool* ValuatorToButtonToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ValuatorToButtonTool(this,inputAssignment);
	}

void ValuatorToButtonToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveValuatorToButtonToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createValuatorToButtonToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ValuatorToButtonToolFactory* valuatorToButtonToolFactory=new ValuatorToButtonToolFactory(*toolManager);
	
	/* Return factory object: */
	return valuatorToButtonToolFactory;
	}

extern "C" void destroyValuatorToButtonToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*********************************************
Static elements of class ValuatorToButtonTool:
*********************************************/

ValuatorToButtonToolFactory* ValuatorToButtonTool::factory=0;

/*************************************
Methods of class ValuatorToButtonTool:
*************************************/

ValuatorToButtonTool::ValuatorToButtonTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 configuration(ValuatorToButtonTool::factory->configuration)
	{
	/* Set the transformation source device: */
	sourceDevice=getValuatorDevice(0);
	}

ValuatorToButtonTool::~ValuatorToButtonTool(void)
	{
	}

void ValuatorToButtonTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void ValuatorToButtonTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void ValuatorToButtonTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	transformedDevice=addVirtualInputDevice("ValuatorToButtonToolTransformedDevice",input.getNumValuatorSlots()*2,0);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	}

const ToolFactory* ValuatorToButtonTool::getFactory(void) const
	{
	return factory;
	}

void ValuatorToButtonTool::valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Check and update the state of the button associated with the updated valuator's positive side: */
	if(transformedDevice->getButtonState(valuatorSlotIndex*2+0))
		{
		if(cbData->newValuatorValue<configuration.posThresholds[1])
			transformedDevice->setButtonState(valuatorSlotIndex*2+0,false);
		}
	else
		{
		if(cbData->newValuatorValue>configuration.posThresholds[0])
			transformedDevice->setButtonState(valuatorSlotIndex*2+0,true);
		}
	
	/* Check and update the state of the button associated with the updated valuator's negative side: */
	if(transformedDevice->getButtonState(valuatorSlotIndex*2+1))
		{
		if(cbData->newValuatorValue>configuration.negThresholds[1])
			transformedDevice->setButtonState(valuatorSlotIndex*2+1,false);
		}
	else
		{
		if(cbData->newValuatorValue<configuration.negThresholds[0])
			transformedDevice->setButtonState(valuatorSlotIndex*2+1,true);
		}
	}

InputDeviceFeatureSet ValuatorToButtonTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on transformed device");
	
	/* Return the valuator slot feeding the forwarded button slot: */
	InputDeviceFeatureSet result;
	result.push_back(input.getValuatorSlotFeature(forwardedFeature.getIndex()/2));
	
	return result;
	}

InputDeviceFeatureSet ValuatorToButtonTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* Get the slot's valuator slot index: */
	int valuatorSlotIndex=input.getValuatorSlotIndex(slotIndex);
	
	/* Return the two button slots fed by the source valuator slot: */
	InputDeviceFeatureSet result;
	for(int i=0;i<2;++i)
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,valuatorSlotIndex*2+i));
	
	return result;
	}

}

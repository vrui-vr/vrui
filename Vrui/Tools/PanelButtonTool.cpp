/***********************************************************************
PanelButtonTool - Class to map a single input device button to several
virtual input device buttons by presenting an extensible panel with GUI
buttons.
Copyright (c) 2013-2025 Oliver Kreylos

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

#include <Vrui/Tools/PanelButtonTool.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/ToggleButton.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>

namespace Vrui {

/******************************************************
Methods of class PanelButtonToolFactory::Configuration:
******************************************************/

PanelButtonToolFactory::Configuration::Configuration(void)
	:panelVertical(true),
	 dynamic(true),
	 numButtons(1)
	{
	}

void PanelButtonToolFactory::Configuration::load(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./panelVertical",panelVertical);
	cfs.updateValue("./dynamic",dynamic);
	cfs.updateValue("./numButtons",numButtons);
	if(numButtons<1)
		numButtons=1;
	}

void PanelButtonToolFactory::Configuration::save(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./panelVertical",panelVertical);
	cfs.storeValue("./dynamic",dynamic);
	cfs.storeValue("./numButtons",numButtons);
	}

/***************************************
Methods of class PanelButtonToolFactory:
***************************************/

PanelButtonToolFactory::PanelButtonToolFactory(ToolManager& toolManager)
	:ToolFactory("PanelButtonTool",toolManager)
	{
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	config.load(toolManager.getToolClassSection(getClassName()));
	
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Set tool class' factory pointer: */
	PanelButtonTool::factory=this;
	}

PanelButtonToolFactory::~PanelButtonToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	PanelButtonTool::factory=0;
	}

const char* PanelButtonToolFactory::getName(void) const
	{
	return "Panel Multi-Button";
	}

const char* PanelButtonToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	return "Forwarded Button";
	}

Tool* PanelButtonToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new PanelButtonTool(this,inputAssignment);
	}

void PanelButtonToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolvePanelButtonToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createPanelButtonToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	PanelButtonToolFactory* factory=new PanelButtonToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyPanelButtonToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class PanelButtonTool:
****************************************/

PanelButtonToolFactory* PanelButtonTool::factory=0;

/********************************
Methods of class PanelButtonTool:
********************************/

void PanelButtonTool::addDevice(void)
	{
	/* Create a new virtual device and add it to the list: */
	InputDevice* device=addVirtualInputDevice("PanelButtonToolTransformedDevice",1,0);
	devices.push_back(device);
	
	/* Copy the source device's tracking type: */
	device->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(device).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(device,this);
	
	/* Disable the virtual input device: */
	getInputGraphManager()->disable(device);
	
	/* Add a new toggle button to the radio box: */
	deviceSelector->addToggle("<unassigned>");
	}

void PanelButtonTool::addDeviceCallback(Misc::CallbackData* cbData)
	{
	/* Add a new virtual input device to the list: */
	addDevice();
	
	/* Enable the "remove device" button if there is more than one virtual input device: */
	removeDeviceButton->setEnabled(devices.size()>1);
	}

void PanelButtonTool::removeDeviceCallback(Misc::CallbackData* cbData)
	{
	/* Get the index of the currently selected virtual device toggle: */
	GLMotif::ToggleButton* current=deviceSelector->getSelectedToggle();
	int currentIndex=deviceSelector->getToggleIndex(current);
	
	/* Delete and then remove the selected virtual input device: */
	getInputGraphManager()->releaseInputDevice(transformedDevice,this);
	getInputDeviceManager()->destroyInputDevice(transformedDevice);
	devices.erase(devices.begin()+currentIndex);
	
	/* Remove and then delete the selected toggle from the radio box: */
	deviceSelector->removeToggle(current);
	delete current;
	
	/* Disable the "remove device" button if there is only one virtual input device left: */
	removeDeviceButton->setEnabled(devices.size()>1);
	
	/* Select and enable the new selected virtual input device: */
	transformedDevice=devices[deviceSelector->getToggleIndex(deviceSelector->getSelectedToggle())];
	getInputGraphManager()->enable(transformedDevice);
	
	/* Initialize the new selected input device's tracking state: */
	resetDevice();
	}

void PanelButtonTool::selectedDeviceChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Disable the currently selected input device: */
	getInputGraphManager()->disable(transformedDevice);
	
	/* Select and enable the new selected input device: */
	transformedDevice=devices[cbData->radioBox->getToggleIndex(cbData->newSelectedToggle)];
	getInputGraphManager()->enable(transformedDevice);
	
	/* Initialize the new selected input device's tracking state: */
	resetDevice();
	}

int PanelButtonTool::findDevice(Tool* tool)
	{
	int deviceIndex=-1;
	
	/* Check if the tool is bound to any of the virtual input devices: */
	const ToolInputAssignment& tia=tool->getInputAssignment();
	int numDevices(devices.size());
	for(int i=0;i<tia.getNumButtonSlots();++i)
		{
		const ToolInputAssignment::Slot& slot=tia.getButtonSlot(i);
		for(int j=0;j<numDevices;++j)
			if(slot.device==devices[j])
				{
				deviceIndex=j;
				goto found;
				}
		}
	
	found:
	return deviceIndex;
	}

void PanelButtonTool::toolCreationCallback(ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Find the index of a virtual device bound to the new tool: */
	int deviceIndex=findDevice(cbData->tool);
	if(deviceIndex>=0)
		{
		/* Set the name of the radio button associated with the device to the tool's name: */
		GLMotif::Label* deviceToggle=static_cast<GLMotif::Label*>(deviceSelector->getChild(deviceIndex));
		deviceToggle->setString(cbData->tool->getName().c_str());
		}
	}

void PanelButtonTool::toolDestructionCallback(ToolManager::ToolDestructionCallbackData* cbData)
	{
	/* Find the index of a virtual device bound to the new tool: */
	int deviceIndex=findDevice(cbData->tool);
	if(deviceIndex>=0)
		{
		/* Reset the name of the radio button associated with the device: */
		GLMotif::Label* deviceToggle=static_cast<GLMotif::Label*>(deviceSelector->getChild(deviceIndex));
		deviceToggle->setString("<unassigned>");
		}
	}

PanelButtonTool::PanelButtonTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 config(PanelButtonTool::factory->config),
	 panelPopup(0),deviceSelector(0)
	{
	/* Set the transformation source device and forwarding parameters: */
	sourceDevice=getButtonDevice(0);
	numPrivateButtons=0;
	}

PanelButtonTool::~PanelButtonTool(void)
	{
	}

void PanelButtonTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override current configuration from the configuration file section: */
	config.load(configFileSection);
	}

void PanelButtonTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Store current configuration to the configuration file section: */
	config.save(configFileSection);
	}

void PanelButtonTool::initialize(void)
	{
	/* Register tool creation/destruction callbacks with the tool manager: */
	getToolManager()->getToolCreationCallbacks().add(this,&PanelButtonTool::toolCreationCallback);
	getToolManager()->getToolDestructionCallbacks().add(this,&PanelButtonTool::toolDestructionCallback);
	
	/* Name the button panel dialog by the source input device and feature: */
	std::string titleString=sourceDevice->getDeviceName();
	titleString.append("->");
	InputDeviceFeature sourceFeature(sourceDevice,InputDevice::BUTTON,input.getButtonSlot(0).index);
	titleString.append(getInputDeviceManager()->getFeatureName(sourceFeature));
	
	/* Create the button panel dialog: */
	panelPopup=new GLMotif::PopupWindow("PanelButtonToolDialog",getWidgetManager(),titleString.c_str());
	panelPopup->setHideButton(true);
	panelPopup->setResizableFlags(false,false);
	
	/* Create the main panel: */
	GLMotif::RowColumn* panel=new GLMotif::RowColumn("Panel",panelPopup,false);
	panel->setOrientation(GLMotif::RowColumn::VERTICAL);
	panel->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	/* Create the add and remove device buttons: */
	GLMotif::Margin* addRemoveMargin=new GLMotif::Margin("AddRemoveMargin",panel,false);
	addRemoveMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::LEFT,GLMotif::Alignment::VFILL));
	
	GLMotif::RowColumn* addRemoveBox=new GLMotif::RowColumn("AddRemoveBox",addRemoveMargin,false);
	addRemoveBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	addRemoveBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	GLMotif::Button* addDeviceButton=new GLMotif::Button("AddDeviceButton",addRemoveBox,"+");
	addDeviceButton->getSelectCallbacks().add(this,&PanelButtonTool::addDeviceCallback);
	
	removeDeviceButton=new GLMotif::Button("RemoveDeviceButton",addRemoveBox,"-");
	removeDeviceButton->getSelectCallbacks().add(this,&PanelButtonTool::removeDeviceCallback);
	removeDeviceButton->setEnabled(config.numButtons>1);
	
	addRemoveBox->manageChild();
	
	addRemoveMargin->manageChild();
	
	/* Create the virtual device selector radio box: */
	deviceSelector=new GLMotif::RadioBox("DeviceSelector",panel,false);
	deviceSelector->setOrientation(GLMotif::RowColumn::VERTICAL);
	deviceSelector->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	deviceSelector->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	/* Create the initial list of virtual devices: */
	for(int i=0;i<config.numButtons;++i)
		addDevice();
	
	deviceSelector->setSelectedToggle(0);
	deviceSelector->getValueChangedCallbacks().add(this,&PanelButtonTool::selectedDeviceChangedCallback);
	deviceSelector->manageChild();
	
	panel->manageChild();
	
	/* Pop up the panel button dialog: */
	popupPrimaryWidget(panelPopup);
	
	/* Select the first virtual input device: */
	transformedDevice=devices[deviceSelector->getToggleIndex(deviceSelector->getSelectedToggle())];
	
	/* Enable and initialize the selected virtual input device's position: */
	getInputGraphManager()->enable(transformedDevice);
	resetDevice();
	}

void PanelButtonTool::deinitialize(void)
	{
	/* Pop down and delete the panel button dialog: */
	delete panelPopup;
	panelPopup=0;
	
	/* Unregister tool creation/destruction callbacks with the tool manager: */
	getToolManager()->getToolCreationCallbacks().remove(this,&PanelButtonTool::toolCreationCallback);
	getToolManager()->getToolDestructionCallbacks().remove(this,&PanelButtonTool::toolDestructionCallback);
	
	/* Delete all virtual input devices: */
	InputDeviceManager* inputDeviceManager=getInputDeviceManager();
	InputGraphManager* inputGraphManager=getInputGraphManager();
	for(std::vector<InputDevice*>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		{
		/* Release the virtual input device: */
		inputGraphManager->releaseInputDevice(*dIt,this);
		
		/* Destroy the virtual input device: */
		inputDeviceManager->destroyInputDevice(*dIt);
		}
	transformedDevice=0;
	}

const ToolFactory* PanelButtonTool::getFactory(void) const
	{
	return factory;
	}

std::vector<InputDevice*> PanelButtonTool::getForwardedDevices(void)
	{
	/* Return the list of virtual devices: */
	return devices;
	}

InputDeviceFeatureSet PanelButtonTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on one of the virtual devices: */
	std::vector<InputDevice*>::iterator dIt;
	for(dIt=devices.begin();dIt!=devices.end()&&*dIt!=forwardedFeature.getDevice();++dIt)
		;
	if(dIt==devices.end())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on a transformed device");
	
	/* Return the single source button feature: */
	InputDeviceFeatureSet result;
	result.push_back(input.getButtonSlotFeature(forwardedFeature.getIndex()));
	return result;
	}

InputDeviceFeatureSet PanelButtonTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* Create an empty feature set: */
	InputDeviceFeatureSet result;
	
	/* Check if the feature is a button: */
	if(sourceFeature.isButton())
		{
		/* Add a forwarded feature for each virtual input device to the result set: */
		int buttonSlotIndex=input.getButtonSlotIndex(slotIndex);
		for(std::vector<InputDevice*>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
			result.push_back(InputDeviceFeature(*dIt,InputDevice::BUTTON,buttonSlotIndex));
		}
	
	return result;
	}

}

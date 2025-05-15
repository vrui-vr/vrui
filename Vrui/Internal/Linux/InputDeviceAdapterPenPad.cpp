/***********************************************************************
InputDeviceAdapterPenPad - Input device adapter for pen pads or pen
displays represented by one or more component HIDs.
Copyright (c) 2023-2025 Oliver Kreylos

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

#include <Vrui/Internal/Linux/InputDeviceAdapterPenPad.h>

#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>

namespace Vrui {

/*****************************************
Methods of class InputDeviceAdapterPenPad:
*****************************************/

void InputDeviceAdapterPenPad::synReportCallback(RawHID::EventDevice::CallbackData* cbData)
	{
	/* Update the relevant HID features: */
	{
	Threads::Mutex::Lock featureLock(featureMutex);
	for(int i=0;i<2;++i)
		posAxes[i].update();
	hoverButton.update();
	touchButton.update();
	for(unsigned int i=0;i<numButtons;++i)
		buttons[i].update();
	}
	
	/* Request a new Vrui frame: */
	requestUpdate();
	}

InputDeviceAdapterPenPad::InputDeviceAdapterPenPad(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager),
	 numButtons(0),buttons(0),
	 penScreen(0),pressedButtonIndex(-1)
	{
	/* Initialize input device adapter: */
	numInputDevices=1;
	inputDevices=new InputDevice*[numInputDevices];
	inputDevices[0]=0;
	
	/* Connect to the pen pad's component HIDs: */
	devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Pen",0));
	devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Pad",0));
	// devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Touch Strip",0));
	// devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Dial",0));
	
	/* Set up relevant device features: */
	for(int i=0;i<2;++i)
		posAxes[i]=RawHID::EventDevice::AbsAxisFeature(&devices[0],i);
	hoverButton=RawHID::EventDevice::KeyFeature(&devices[0],0);
	touchButton=RawHID::EventDevice::KeyFeature(&devices[0],1);
	
	/* Set up other buttons on the device: */
	numButtons=7;
	buttons=new RawHID::EventDevice::KeyFeature[numButtons];
	buttons[0]=RawHID::EventDevice::KeyFeature(&devices[0],2);
	buttons[1]=RawHID::EventDevice::KeyFeature(&devices[0],3);
	buttons[2]=RawHID::EventDevice::KeyFeature(&devices[1],0);
	buttons[3]=RawHID::EventDevice::KeyFeature(&devices[1],1);
	buttons[4]=RawHID::EventDevice::KeyFeature(&devices[1],2);
	buttons[5]=RawHID::EventDevice::KeyFeature(&devices[1],3);
	buttons[6]=RawHID::EventDevice::KeyFeature(&devices[1],4);
	buttonStates=new bool[numButtons];
	for(unsigned int i=0;i<numButtons;++i)
		buttonStates[i]=buttons[i].getValue();
	
	/* Create the list of pen feature names: */
	featureNames.push_back("Touch");
	featureNames.push_back("Pen1");
	featureNames.push_back("Pen2");
	featureNames.push_back("Pad1");
	featureNames.push_back("Pad2");
	featureNames.push_back("Pad3");
	featureNames.push_back("Pad4");
	featureNames.push_back("Pad5");
	
	/* Create the pen input device: */
	inputDevices[0]=inputDeviceManager->createInputDevice("Pen",InputDevice::TRACK_POS|InputDevice::TRACK_DIR,8,0,true);
	
	/* Register callbacks with the component HIDs and start dispatching events on the HIDs' device nodes: */
	Threads::EventDispatcher& eventDispatcher=inputDeviceManager->acquireEventDispatcher();
	for(std::vector<RawHID::EventDevice>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		{
		/* Try grabbing the device for exclusive access: */
		if(!dIt->grabDevice())
			Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"Cannot grab device %s",dIt->getDeviceName().c_str());
		
		/* Register callbacks and start dispatching events from the device: */
		dIt->getSynReportEventCallbacks().add(this,&InputDeviceAdapterPenPad::synReportCallback);
		dIt->registerEventHandler(eventDispatcher);
		}
	}

InputDeviceAdapterPenPad::~InputDeviceAdapterPenPad(void)
	{
	/* Release allocated resources: */
	delete[] buttons;
	delete[] buttonStates;
	}

std::string InputDeviceAdapterPenPad::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Check the device is really ours: */
	if(feature.getDevice()!=inputDevices[0])
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",feature.getDevice()->getDeviceName());
	if(!feature.isButton()||feature.getIndex()>=8)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown feature");
	
	/* Return the button feature name: */
	return featureNames[feature.getIndex()];
	}

int InputDeviceAdapterPenPad::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Check the device is really ours: */
	if(device!=inputDevices[0])
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",device->getDeviceName());
	
	/* Find the feature name: */
	for(int i=0;i<8;++i)
		if(featureNames[i]==featureName)
			return device->getButtonFeatureIndex(i);
	
	return -1;
	}

void InputDeviceAdapterPenPad::prepareMainLoop(void)
	{
	/* Connect to the pen screen: */
	penScreen=findScreen("Screen");
	if(penScreen==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Screen Screen not found");
	}

void InputDeviceAdapterPenPad::updateInputDevices(void)
	{
	/* Grab relevant pen device data: */
	bool hoverState,touchState;
	Scalar pos[2];
	{
	Threads::Mutex::Lock featureLock(featureMutex);
	for(int i=0;i<2;++i)
		pos[i]=Scalar(posAxes[i].getNormalizedValueOneSide());
	hoverState=hoverButton.getValue();
	touchState=touchButton.getValue();
	for(unsigned int i=0;i<numButtons;++i)
		buttonStates[i]=buttons[i].getValue();
	}
	
	/* Check if the pen position is valid: */
	if(hoverState)
		{
		/* Calibrate the pen position: */
		// ...
		
		/* Set the pen's button and valuator states: */
		int buttonIndex=0;
		for(unsigned int i=0;i<numButtons;++i)
			if(buttonStates[i])
				{
				buttonIndex=i+1;
				break;
				}
		if(pressedButtonIndex!=-1&&pressedButtonIndex!=buttonIndex)
			inputDevices[0]->setButtonState(pressedButtonIndex,false);
		inputDevices[0]->setButtonState(buttonIndex,touchState);
		pressedButtonIndex=touchState?buttonIndex:-1;
		
		/* Enable the pen device: */
		getInputGraphManager()->enable(inputDevices[0]);
		}
	else
		{
		/* Disable the pen device: */
		getInputGraphManager()->disable(inputDevices[0]);
		}
	}

}

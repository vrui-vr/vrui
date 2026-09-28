/***********************************************************************
MultipipeDispatcher - Class to distribute input device and ancillary
data between the nodes in a multipipe VR environment.
Copyright (c) 2004-2026 Oliver Kreylos

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

#include <Vrui/Internal/MultipipeDispatcher.h>

#include <Misc/StdError.h>
#include <Misc/Marshaller.h>
#include <Misc/StringMarshaller.h>
#include <Cluster/MulticastPipe.h>
#include <Geometry/GeometryMarshallers.h>
#include <GL/GLMarshallers.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/TextEventDispatcher.h>
#include <Vrui/InputDeviceManager.h>

namespace Vrui {

/************************************
Methods of class MultipipeDispatcher:
************************************/

void MultipipeDispatcher::inputDeviceStateChangedCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData)
	{
	/* Find the changed device in the list of dispatched devices: */
	for(int i=0;i<numDispatchedInputDevices;++i)
		if(dispatchedInputDevices[i]==cbData->inputDevice)
			{
			newInputDeviceEnableds[i]=cbData->newEnabled;
			break;
			}
	}

MultipipeDispatcher::MultipipeDispatcher(InputDeviceManager* sInputDeviceManager,Cluster::MulticastPipe* sPipe)
	:InputDeviceAdapter(sInputDeviceManager),
	 inputGraphManager(inputDeviceManager->getInputGraphManager()),
	 pipe(sPipe),
	 inputDeviceEnableds(0),newInputDeviceEnableds(0),
	 numDispatchedInputDevices(0),dispatchedInputDevices(0)
	{
	/*********************************************************************
	Add this dispatcher as an input device adapter to the input device
	manager. On the master node, it won't add any input devices and will
	appear transparent, but its updateInputDevices method will be called
	immediately after all other input device adapters have updated their
	devices, meaning at exactly the right time. On the slave nodes, it
	will act as a regular input device adapter. Win-win!
	*********************************************************************/
	
	inputDeviceManager->addAdapter(this);
		
	if(pipe->isMaster())
		{
		/*******************************************************************
		Distribute the input device configuration from the input device
		manager to all slave nodes. We will leave the input device adapter
		input device states empty so that we can pretend to be a regular
		input device adapter with no devices on the master node, simplifying
		processing.
		*******************************************************************/
		
		/* Send the number of dispatched input devices: */
		numDispatchedInputDevices=inputDeviceManager->getNumInputDevices();
		pipe->write<int>(numDispatchedInputDevices);
		dispatchedInputDevices=new InputDevice*[numDispatchedInputDevices];
		
		/* Allocate the input device state tracking arrays: */
		inputDeviceEnableds=new bool[numDispatchedInputDevices];
		newInputDeviceEnableds=new bool[numDispatchedInputDevices];
		
		/* Send configuration of all input devices: */
		for(int deviceIndex=0;deviceIndex<numDispatchedInputDevices;++deviceIndex)
			{
			/* Get pointer to input device: */
			InputDevice* device=dispatchedInputDevices[deviceIndex]=inputDeviceManager->getInputDevice(deviceIndex);
			
			/* Send input device name: */
			Misc::writeCString(device->getDeviceName(),*pipe);
			
			/* Send track type: */
			pipe->write<int>(device->getTrackType());
			
			/* Send number of buttons: */
			pipe->write<int>(device->getNumButtons());
			
			/* Send number of valuators: */
			pipe->write<int>(device->getNumValuators());
			
			/* Check if the device has a handle transformation: */
			const ONTransform& handleTransform=inputDeviceManager->getHandleTransform(device);
			if(handleTransform!=ONTransform::identity)
				{
				/* Send the handle transform to the slaves: */
				pipe->write(Misc::UInt8(1));
				Misc::Marshaller<ONTransform>::write(handleTransform,*pipe);
				}
			else
				{
				/* Notify the slaves that the device does not have a handle transform: */
				pipe->write(Misc::UInt8(0));
				}
			
			/* Send device glyph: */
			Glyph& glyph=inputGraphManager->getInputDeviceGlyph(device);
			pipe->write<char>(glyph.isEnabled()?1:0);
			pipe->write<int>(glyph.getGlyphType());
			Misc::write(glyph.getGlyphMaterial(),*pipe);
			
			/* Send and remember the device's enabled state: */
			newInputDeviceEnableds[deviceIndex]=inputDeviceEnableds[deviceIndex]=inputGraphManager->isEnabled(device);
			pipe->write(Misc::UInt8(inputDeviceEnableds[deviceIndex]?1:0));
			
			/* Send all button names: */
			for(int buttonIndex=0;buttonIndex<device->getNumButtons();++buttonIndex)
				Misc::writeCppString(inputDeviceManager->getFeatureName(InputDeviceFeature(device,InputDevice::BUTTON,buttonIndex)),*pipe);
			
			/* Send all valuator names: */
			for(int valuatorIndex=0;valuatorIndex<device->getNumValuators();++valuatorIndex)
				Misc::writeCppString(inputDeviceManager->getFeatureName(InputDeviceFeature(device,InputDevice::VALUATOR,valuatorIndex)),*pipe);
			}
		
		pipe->flush();
		
		/* Register an input device state change callback with the input graph manager: */
		inputGraphManager->getInputDeviceStateChangeCallbacks().add(this,&MultipipeDispatcher::inputDeviceStateChangedCallback);
		}
	else
		{
		/*******************************************************************
		Receive the input device configuration from the master node:
		*******************************************************************/
		
		/* Read number of input devices: */
		numInputDevices=pipe->read<int>();
		inputDevices=new InputDevice*[numInputDevices];
		
		/* Allocate the input device state tracking arrays: */
		inputDeviceEnableds=new bool[numInputDevices];
		newInputDeviceEnableds=new bool[numInputDevices];
		
		/* Read configuration of all input devices: */
		for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
			{
			/* Read input device name: */
			char* name=Misc::readCString(*pipe);
			
			/* Read track type: */
			int trackType=pipe->read<int>();
			
			/* Read number of buttons: */
			int numButtons=pipe->read<int>();
			
			/* Read number of valuators: */
			int numValuators=pipe->read<int>();
			
			/* Create the input device: */
			InputDevice* device=inputDevices[deviceIndex]=inputDeviceManager->createInputDevice(name,trackType,numButtons,numValuators,true);
			delete[] name;
			
			/* Check if the device has a handle transformation: */
			if(pipe->read<Misc::UInt8>()!=0)
				{
				/* Read and set the device's handle transformation: */
				inputDeviceManager->addHandleTransform(device,Misc::Marshaller<ONTransform>::read(*pipe));
				}
			
			/* Read device glyph: */
			Glyph deviceGlyph;
			bool glyphEnabled=pipe->read<char>()!=0;
			Glyph::GlyphType glyphType=Glyph::GlyphType(pipe->read<int>());
			GLMaterial glyphMaterial=Misc::Marshaller<GLMaterial>::read(*pipe);
			if(glyphEnabled)
				deviceGlyph.enable(glyphType,glyphMaterial);
			
			/* Initialize the input device glyph: */
			inputGraphManager->getInputDeviceGlyph(device)=deviceGlyph;
			
			/* Read, remember, and set the device's enabled flag: */
			newInputDeviceEnableds[deviceIndex]=inputDeviceEnableds[deviceIndex]=pipe->read<Misc::UInt8>()!=0;
			inputGraphManager->setEnabled(device,inputDeviceEnableds[deviceIndex]);
			
			/* Receive all button names: */
			for(int buttonIndex=0;buttonIndex<device->getNumButtons();++buttonIndex)
				buttonNames.push_back(Misc::readCppString(*pipe));
			
			/* Receive all valuator names: */
			for(int valuatorIndex=0;valuatorIndex<device->getNumValuators();++valuatorIndex)
				valuatorNames.push_back(Misc::readCppString(*pipe));
			}
		}
	}

MultipipeDispatcher::~MultipipeDispatcher(void)
	{
	if(pipe->isMaster())
		{
		/* Unregister the input device state change callback from the input graph manager: */
		inputGraphManager->getInputDeviceStateChangeCallbacks().remove(this,&MultipipeDispatcher::inputDeviceStateChangedCallback);
		}
	
	/* Delete the state tracking arrays: */
	delete[] inputDeviceEnableds;
	delete[] newInputDeviceEnableds;
	delete[] dispatchedInputDevices;
	}

std::string MultipipeDispatcher::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Find the input device owning the given feature: */
	bool deviceFound=false;
	int buttonIndexBase=0;
	int valuatorIndexBase=0;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==feature.getDevice())
			{
			deviceFound=true;
			break;
			}
		
		/* Go to the next device: */
		buttonIndexBase+=inputDevices[deviceIndex]->getNumButtons();
		valuatorIndexBase+=inputDevices[deviceIndex]->getNumValuators();
		}
	if(!deviceFound)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",feature.getDevice()->getDeviceName());
	
	/* Check whether the feature is a button or a valuator: */
	std::string result;
	if(feature.isButton())
		{
		/* Return the button feature's name: */
		result=buttonNames[buttonIndexBase+feature.getIndex()];
		}
	if(feature.isValuator())
		{
		/* Return the valuator feature's name: */
		result=valuatorNames[valuatorIndexBase+feature.getIndex()];
		}
	
	return result;
	}

int MultipipeDispatcher::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Find the input device owning the given feature: */
	bool deviceFound=false;
	int buttonIndexBase=0;
	int valuatorIndexBase=0;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==device)
			{
			deviceFound=true;
			break;
			}
		
		/* Go to the next device: */
		buttonIndexBase+=inputDevices[deviceIndex]->getNumButtons();
		valuatorIndexBase+=inputDevices[deviceIndex]->getNumValuators();
		}
	if(!deviceFound)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",device->getDeviceName());
	
	/* Check if the feature names a button or a valuator: */
	for(int buttonIndex=0;buttonIndex<device->getNumButtons();++buttonIndex)
		if(buttonNames[buttonIndexBase+buttonIndex]==featureName)
			return device->getButtonFeatureIndex(buttonIndex);
	for(int valuatorIndex=0;valuatorIndex<device->getNumValuators();++valuatorIndex)
		if(valuatorNames[valuatorIndexBase+valuatorIndex]==featureName)
			return device->getValuatorFeatureIndex(valuatorIndex);
	
	return -1;
	}

void MultipipeDispatcher::updateInputDevices(void)
	{
	if(pipe->isMaster())
		{
		/* Write the change lists of all dispatched input devices to the pipe: */
		for(int i=0;i<numDispatchedInputDevices;++i)
			if(inputDeviceEnableds[i]!=newInputDeviceEnableds[i]||dispatchedInputDevices[i]->hasChanges())
				{
				/* Write the input device's index and enabled state: */
				pipe->write(Misc::UInt8(i));
				pipe->write(Misc::UInt8(newInputDeviceEnableds[i]));
				
				/* If the input device is enabled, write its change list: */
				if(newInputDeviceEnableds[i])
					dispatchedInputDevices[i]->writeChanges(*pipe);
				
				/* Mark the device's enabled state as up-to-date: */
				inputDeviceEnableds[i]=newInputDeviceEnableds[i];
				}
		
		/* Terminate the list of changes with an invalid input device index: */
		pipe->write(Misc::UInt8(-1));
		
		/* Now is the perfect time to write the text event dispatcher's events to the pipe: */
		inputDeviceManager->getTextEventDispatcher()->writeEventQueues(*pipe);
		}
	else
		{
		/* Read a sequence of input device change lists from the pipe: */
		while(true)
			{
			/* Read the index of the next input device and bail out if it's the end-of-list marker: */
			Misc::UInt8 index=pipe->read<Misc::UInt8>();
			if(index==Misc::UInt8(-1))
				break;
			
			/* Read the input device's enabled state and update the device's state if it changed: */
			newInputDeviceEnableds[index]=pipe->read<Misc::UInt8>()!=0;
			if(inputDeviceEnableds[index]!=newInputDeviceEnableds[index])
				inputGraphManager->setEnabled(inputDevices[index],newInputDeviceEnableds[index]);
			inputDeviceEnableds[index]=newInputDeviceEnableds[index];
			
			/* If the input device is enabled, read its change list: */
			if(inputDeviceEnableds[index])
				inputDevices[index]->readChanges(*pipe);
			}
		
		/* Read the text event dispatcher's event queues: */
		inputDeviceManager->getTextEventDispatcher()->readEventQueues(*pipe);
		}
	}

}

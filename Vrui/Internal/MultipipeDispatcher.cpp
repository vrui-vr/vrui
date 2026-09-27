/***********************************************************************
MultipipeDispatcher - Class to distribute input device and ancillary
data between the nodes in a multipipe VR environment.
Copyright (c) 2004-2024 Oliver Kreylos

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
#include <GL/GLMarshallers.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>

namespace Vrui {

/************************************
Methods of class MultipipeDispatcher:
************************************/

MultipipeDispatcher::MultipipeDispatcher(InputDeviceManager* sInputDeviceManager,Cluster::MulticastPipe* sPipe)
	:InputDeviceAdapter(sInputDeviceManager),
	 pipe(sPipe)
	{
	if(pipe->isMaster())
		{
		/*******************************************************************
		Distribute the input device configuration from the input device
		manager to all slave nodes:
		*******************************************************************/
		
		/* Send number of input devices: */
		numInputDevices=inputDeviceManager->getNumInputDevices();
		pipe->write<int>(numInputDevices);
		inputDevices=new InputDevice*[numInputDevices];
		
		/* Send configuration of all input devices: */
		for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
			{
			/* Get pointer to input device: */
			InputDevice* device=inputDevices[deviceIndex]=inputDeviceManager->getInputDevice(deviceIndex);
			
			/* Send input device name: */
			Misc::writeCString(device->getDeviceName(),*pipe);
			
			/* Send track type: */
			pipe->write<int>(device->getTrackType());
			
			/* Send number of buttons: */
			pipe->write<int>(device->getNumButtons());
			
			/* Send number of valuators: */
			pipe->write<int>(device->getNumValuators());
			
			/* Send device glyph: */
			Glyph& glyph=inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(device);
			pipe->write<char>(glyph.isEnabled()?1:0);
			pipe->write<int>(glyph.getGlyphType());
			Misc::write(glyph.getGlyphMaterial(),*pipe);
			
			/* Send all button names: */
			for(int buttonIndex=0;buttonIndex<device->getNumButtons();++buttonIndex)
				Misc::writeCppString(inputDeviceManager->getFeatureName(InputDeviceFeature(device,InputDevice::BUTTON,buttonIndex)),*pipe);
			
			/* Send all valuator names: */
			for(int valuatorIndex=0;valuatorIndex<device->getNumValuators();++valuatorIndex)
				Misc::writeCppString(inputDeviceManager->getFeatureName(InputDeviceFeature(device,InputDevice::VALUATOR,valuatorIndex)),*pipe);
			}
		
		pipe->flush();
		}
	else
		{
		/* Add the dispatcher as an input device adapter to the input device manager: */
		inputDeviceManager->addAdapter(this);
		
		/*******************************************************************
		Receive the input device configuration from the master node:
		*******************************************************************/
		
		/* Read number of input devices: */
		numInputDevices=pipe->read<int>();
		inputDevices=new InputDevice*[numInputDevices];
		
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
			
			/* Read device glyph: */
			Glyph deviceGlyph;
			bool glyphEnabled=pipe->read<char>()!=0;
			Glyph::GlyphType glyphType=Glyph::GlyphType(pipe->read<int>());
			GLMaterial glyphMaterial=Misc::Marshaller<GLMaterial>::read(*pipe);
			if(glyphEnabled)
				deviceGlyph.enable(glyphType,glyphMaterial);
			
			/* Create the input device: */
			InputDevice* device=inputDevices[deviceIndex]=inputDeviceManager->createInputDevice(name,trackType,numButtons,numValuators,true);
			delete[] name;
			
			/* Initialize the input device glyph: */
			inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(device)=deviceGlyph;
			
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
		/* Do not destroy input devices on the master node, since they belong to input device adapters: */
		for(int i=0;i<numInputDevices;++i)
			inputDevices[i]=0;
		}
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
		/* Write the change lists of all input devices to the pipe: */
		for(int i=0;i<numInputDevices;++i)
			if(inputDevices[i]->hasChanges())
				{
				/* Write the input device's index followed by its change list: */
				pipe->write(Misc::UInt8(i));
				inputDevices[i]->writeChanges(*pipe);
				}
		
		/* Terminate the list of changes with an invalid input device index: */
		pipe->write(Misc::UInt8(-1));
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
			
			/* Read the input device's change list: */
			inputDevices[index]->readChanges(*pipe);
			}
		}
	}

}

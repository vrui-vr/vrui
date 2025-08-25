/***********************************************************************
InputDeviceAdapter - Base class to convert from diverse "raw" input
device representations to Vrui's internal input device representation.
Copyright (c) 2004-2025 Oliver Kreylos

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

#include <Vrui/Internal/InputDeviceAdapter.h>

#include <string.h>
#include <stdio.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>

namespace Vrui {

/***********************************
Methods of class InputDeviceAdapter:
***********************************/

int InputDeviceAdapter::updateTrackType(int trackType,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Convert the given tracking type to a string: */
	std::string trackTypeString;
	switch(trackType)
		{
		case InputDevice::TRACK_NONE:
			trackTypeString="None";
			break;
		
		case InputDevice::TRACK_POS:
			trackTypeString="3D";
			break;
		
		case InputDevice::TRACK_POS|InputDevice::TRACK_DIR:
			trackTypeString="Ray";
			break;
		
		case InputDevice::TRACK_POS|InputDevice::TRACK_DIR|InputDevice::TRACK_ORIENT:
			trackTypeString="6D";
			break;
		
		default:
			trackTypeString="Invalid";
		}
	
	/* Update the tracking type from the configuration file section: */
	configFileSection.updateString("./trackType",trackTypeString);
	
	/* Parse the updated tracking type: */
	if(trackTypeString=="None")
		return InputDevice::TRACK_NONE;
	else if(trackTypeString=="3D")
		return InputDevice::TRACK_POS;
	else if(trackTypeString=="Ray")
		return InputDevice::TRACK_POS|InputDevice::TRACK_DIR;
	else if(trackTypeString=="6D")
		return InputDevice::TRACK_POS|InputDevice::TRACK_DIR|InputDevice::TRACK_ORIENT;
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid tracking type %s",trackTypeString.c_str());
	}

InputDevice* InputDeviceAdapter::createInputDevice(const char* name,int trackType,int numButtons,int numValuators,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Create the new input device as a physical device: */
	InputDevice* newDevice=inputDeviceManager->createInputDevice(name,trackType,numButtons,numValuators,true);
	
	/* Initialize the new device's ray from the configuration file section: */
	Vector deviceRayDirection=configFileSection.retrieveValue("./deviceRayDirection",Vector(0,1,0));
	Scalar deviceRayStart=configFileSection.retrieveValue("./deviceRayStart",-getInchFactor());
	newDevice->setDeviceRay(deviceRayDirection,deviceRayStart);
	
	/* Initialize the new device's glyph from the configuration file section: */
	Glyph& deviceGlyph=inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(newDevice);
	deviceGlyph.configure(configFileSection,"./deviceGlyphType","./deviceGlyphMaterial");
	
	return newDevice;
	}

InputDevice* InputDeviceAdapter::createInputDevice(const char* name,int trackType,int numButtons,int numValuators,const Misc::ConfigurationFileSection& configFileSection,std::vector<std::string>& buttonNames,std::vector<std::string>& valuatorNames)
	{
	/* Call the other method: */
	InputDevice* newDevice=createInputDevice(name,trackType,numButtons,numValuators,configFileSection);
	
	typedef std::vector<std::string> StringList;
	
	/* Read a (partial) list of button names from the configuration file section: */
	StringList tempButtonNames;
	configFileSection.updateValue("./buttonNames",tempButtonNames);
	int buttonIndex=0;
	
	/* Copy names from the beginning of the read list: */
	for(StringList::iterator bnIt=tempButtonNames.begin();bnIt!=tempButtonNames.end()&&buttonIndex<numButtons;++bnIt,++buttonIndex)
		buttonNames.push_back(*bnIt);
	
	/* Assign default names to all remaining buttons: */
	for(;buttonIndex<numButtons;++buttonIndex)
		{
		char buttonName[40];
		snprintf(buttonName,sizeof(buttonName),"Button%d",buttonIndex);
		buttonNames.push_back(buttonName);
		}
	
	/* Read a (partial) list of valuator names from the configuration file section: */
	StringList tempValuatorNames;
	configFileSection.updateValue("./valuatorNames",tempValuatorNames);
	int valuatorIndex=0;
	
	/* Copy names from the beginning of the read list: */
	for(StringList::iterator vnIt=tempValuatorNames.begin();vnIt!=tempValuatorNames.end()&&valuatorIndex<numValuators;++vnIt,++valuatorIndex)
		valuatorNames.push_back(*vnIt);
	
	/* Assign default names to all remaining valuators: */
	for(;valuatorIndex<numValuators;++valuatorIndex)
		{
		char valuatorName[40];
		snprintf(valuatorName,sizeof(valuatorName),"Valuator%d",valuatorIndex);
		valuatorNames.push_back(valuatorName);
		}
	
	return newDevice;
	}

void InputDeviceAdapter::initializeInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read input device name: */
	std::string name=configFileSection.retrieveString("./name",configFileSection.getName());
	
	/* Determine the input device's tracking type: */
	int trackType=updateTrackType(InputDevice::TRACK_NONE,configFileSection);
	
	/* Determine numbers of buttons and valuators: */
	int numButtons=configFileSection.retrieveValue<int>("./numButtons",0);
	int numValuators=configFileSection.retrieveValue<int>("./numValuators",0);
	
	/* Create and save the new input device: */
	inputDevices[deviceIndex]=createInputDevice(name.c_str(),trackType,numButtons,numValuators,configFileSection);
	}

void InputDeviceAdapter::initializeAdapter(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Allocate adapter state arrays: */
	std::vector<std::string> inputDeviceNames;
	configFileSection.updateValue("./inputDeviceNames",inputDeviceNames);
	numInputDevices=inputDeviceNames.size();
	inputDevices=new InputDevice*[numInputDevices];
	for(int i=0;i<numInputDevices;++i)
		inputDevices[i]=0;
	
	/* Initialize input devices: */
	int numIgnoredDevices=0;
	for(int i=0;i<numInputDevices;++i)
		{
		try
			{
			/* Go to device's section: */
			Misc::ConfigurationFileSection deviceSection=configFileSection.getSection(inputDeviceNames[i].c_str());
			
			/* Initialize input device: */
			initializeInputDevice(i,deviceSection);
			}
		catch(const std::runtime_error& err)
			{
			/* Print an error message: */
			Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Ignoring input device %s due to exception %s",inputDeviceNames[i].c_str(),err.what());
			
			/* Ignore the input device: */
			inputDevices[i]=0;
			++numIgnoredDevices;
			}
		}
	
	if(numIgnoredDevices!=0)
		{
		/* Remove any ignored input devices from the array: */
		InputDevice** newInputDevices=new InputDevice*[numInputDevices-numIgnoredDevices];
		int newNumInputDevices=0;
		for(int i=0;i<numInputDevices;++i)
			if(inputDevices[i]!=0)
				newInputDevices[newNumInputDevices++]=inputDevices[i];
		delete[] inputDevices;
		numInputDevices=newNumInputDevices;
		inputDevices=newInputDevices;
		}
	}

InputDeviceAdapter::~InputDeviceAdapter(void)
	{
	/* Destroy all input devices: */
	for(int i=0;i<numInputDevices;++i)
		if(inputDevices[i]!=0)
			inputDeviceManager->destroyInputDevice(inputDevices[i]);
	delete[] inputDevices;
	}

std::string InputDeviceAdapter::getDefaultFeatureName(const InputDeviceFeature& feature)
	{
	char featureName[40];
	featureName[0]='\0';
	
	/* Check if the feature is a button or a valuator: */
	if(feature.isButton())
		{
		/* Return a default button name: */
		snprintf(featureName,sizeof(featureName),"Button%d",feature.getIndex());
		}
	if(feature.isValuator())
		{
		/* Return a default valuator name: */
		snprintf(featureName,sizeof(featureName),"Valuator%d",feature.getIndex());
		}
	
	return std::string(featureName);
	}

int InputDeviceAdapter::getDefaultFeatureIndex(InputDevice* device,const char* featureName)
	{
	int result=-1;
	
	/* Check if the feature names a button or a valuator: */
	if(strncmp(featureName,"Button",6)==0)
		{
		/* Get the button index: */
		int buttonIndex=atoi(featureName+6);
		if(buttonIndex<device->getNumButtons())
			result=device->getButtonFeatureIndex(buttonIndex);
		else
			result=-1;
		}
	if(strncmp(featureName,"Valuator",8)==0)
		{
		/* Get the valuator index: */
		int valuatorIndex=atoi(featureName+8);
		if(valuatorIndex<device->getNumValuators())
			result=device->getValuatorFeatureIndex(valuatorIndex);
		else
			result=-1;
		}
	
	return result;
	}

int InputDeviceAdapter::findInputDevice(const InputDevice* device) const
	{
	/* Look for the given device in the list: */
	for(int i=0;i<numInputDevices;++i)
		if(inputDevices[i]==device)
			return i;
	return -1;
	}

std::string InputDeviceAdapter::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Return a default feature name: */
	return getDefaultFeatureName(feature);
	}

int InputDeviceAdapter::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Parse a default feature name: */
	return getDefaultFeatureIndex(device,featureName);
	}

void InputDeviceAdapter::prepareMainLoop(void)
	{
	/* Do nothing */
	}

TrackerState InputDeviceAdapter::peekTrackerState(int deviceIndex)
	{
	/* Default implementation throws an exception, for input device adapters (or devices) that don't have a tracker state: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Requested device does not have tracker states");
	}

void InputDeviceAdapter::glRenderAction(GLContextData& contextData) const
	{
	}

void InputDeviceAdapter::hapticTick(unsigned int hapticFeatureIndex,unsigned int duration,unsigned int frequency,unsigned int amplitude)
	{
	}

}


/***********************************************************************
InputDeviceManager - Class to manage physical and virtual input devices,
tools associated to input devices, and the input device update graph.
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

#include <Vrui/InputDeviceManager.h>

#define DEBUGGING 0
#if DEBUGGING
#include <iostream>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <utility>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Internal/InputDeviceAdapter.h>
#include <Vrui/Internal/InputDeviceAdapterMouse.h>
#include <Vrui/Internal/InputDeviceAdapterMultitouch.h>
#include <Vrui/Internal/InputDeviceAdapterDeviceDaemon.h>
#include <Vrui/Internal/InputDeviceAdapterTrackd.h>
#include <Vrui/Internal/InputDeviceAdapterVisBox.h>
#ifdef __linux__
#include <Vrui/Internal/Linux/InputDeviceAdapterHID.h>
#include <Vrui/Internal/Linux/InputDeviceAdapterPenPad.h>
#endif
#ifdef __APPLE__
#include <Vrui/Internal/MacOSX/InputDeviceAdapterHID.h>
#endif
#include <Vrui/Internal/InputDeviceAdapterOVRD.h>
#include <Vrui/Internal/InputDeviceAdapterPlayback.h>
#include <Vrui/Internal/InputDeviceAdapterDummy.h>

namespace Vrui {

/****************
Helper functions:
****************/

namespace {

static int getPrefixLength(const char* deviceName)
	{
	/* Find the last colon in the device name: */
	const char* colonPtr=0;
	const char* cPtr;
	bool onlyDigits=false;
	for(cPtr=deviceName;*cPtr!='\0';++cPtr)
		{
		if(*cPtr==':')
			{
			colonPtr=cPtr;
			onlyDigits=true;
			}
		else if(!isdigit(*cPtr))
			onlyDigits=false;
		}
	
	if(onlyDigits&&colonPtr[1]!='\0')
		return colonPtr-deviceName;
	else
		return cPtr-deviceName;
	}

}

/***********************************
Methods of class InputDeviceManager:
***********************************/

InputDeviceManager::InputDeviceManager(InputGraphManager* sInputGraphManager,TextEventDispatcher* sTextEventDispatcher)
	:inputGraphManager(sInputGraphManager),textEventDispatcher(sTextEventDispatcher),
	 numInputDeviceAdapters(0),inputDeviceAdapters(0),
	 hapticFeatureMap(17),handleTransformMap(17),
	 predictDeviceStates(false),predictionTime(0,0)
	{
	}

InputDeviceManager::~InputDeviceManager(void)
	{
	/* Delete all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		delete inputDeviceAdapters[i];
	delete[] inputDeviceAdapters;
	
	/* Delete all leftover input devices: */
	for(InputDevices::iterator idIt=inputDevices.begin();idIt!=inputDevices.end();++idIt)
		{
		InputDevice* device=&(*idIt);
		
		/* Call the input device destruction callbacks: */
		InputDeviceDestructionCallbackData cbData(this,device);
		inputDeviceDestructionCallbacks.call(&cbData);
		
		/* Remove the device from the input graph: */
		inputGraphManager->removeInputDevice(device);
		}
	}

void InputDeviceManager::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Retrieve the list of input device adapters: */
	typedef std::vector<std::string> StringList;
	StringList inputDeviceAdapterNames=configFileSection.retrieveValue<StringList>("./inputDeviceAdapterNames");
	
	/* Remove all duplicates from the list of input device adapters: */
	for(unsigned int i=0;i<inputDeviceAdapterNames.size()-1;++i)
		{
		for(unsigned int j=inputDeviceAdapterNames.size()-1;j>i;--j)
			{
			if(inputDeviceAdapterNames[j]==inputDeviceAdapterNames[i])
				{
				/* Remove the duplicate list entry: */
				inputDeviceAdapterNames.erase(inputDeviceAdapterNames.begin()+j);
				}
			}
		}
	
	/* Initialize the adapter array: */
	numInputDeviceAdapters=inputDeviceAdapterNames.size();
	inputDeviceAdapters=new InputDeviceAdapter*[numInputDeviceAdapters];
	
	/* Initialize input device adapters: */
	int numIgnoredAdapters=0;
	int mouseAdapterIndex=-1;
	int multitouchAdapterIndex=-1;
	for(int i=0;i<numInputDeviceAdapters;++i)
		{
		/* Go to input device adapter's section: */
		Misc::ConfigurationFileSection inputDeviceAdapterSection=configFileSection.getSection(inputDeviceAdapterNames[i].c_str());
		
		/* Determine input device adapter's type: */
		std::string inputDeviceAdapterType=inputDeviceAdapterSection.retrieveString("./inputDeviceAdapterType");
		bool typeFound=true;
		try
			{
			if(inputDeviceAdapterType=="Mouse")
				{
				/* Check if there is already a mouse input device adapter: */
				if(mouseAdapterIndex>=0)
					{
					/* Ignore this input device adapter: */
					inputDeviceAdapters[i]=0;
					++numIgnoredAdapters;
					Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Ignoring mouse input device adapter %s because there is already a mouse input device adapter",inputDeviceAdapterNames[i].c_str());
					}
				else
					{
					/* Create mouse input device adapter: */
					inputDeviceAdapters[i]=new InputDeviceAdapterMouse(this,inputDeviceAdapterSection);
					mouseAdapterIndex=i;
					}
				}
			else if(inputDeviceAdapterType=="Multitouch")
				{
				/* Check if there is already a multitouch input device adapter: */
				if(multitouchAdapterIndex>=0)
					{
					/* Ignore this input device adapter: */
					inputDeviceAdapters[i]=0;
					++numIgnoredAdapters;
					Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Ignoring multitouch input device adapter %s because there is already a multitouch input device adapter",inputDeviceAdapterNames[i].c_str());
					}
				else
					{
					/* Create multitouch input device adapter: */
					inputDeviceAdapters[i]=new InputDeviceAdapterMultitouch(this,inputDeviceAdapterSection);
					multitouchAdapterIndex=i;
					}
				}
			else if(inputDeviceAdapterType=="DeviceDaemon")
				{
				/* Create device daemon input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterDeviceDaemon(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="Trackd")
				{
				/* Create trackd input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterTrackd(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="VisBox")
				{
				/* Create VisBox input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterVisBox(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="HID")
				{
				/* Create HID input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterHID(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="PenPad")
				{
				/* Create pen pad input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterPenPad(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="OVRD")
				{
				/* Create OVRD input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterOVRD(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="Playback")
				{
				/* Create playback input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterPlayback(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="Dummy")
				{
				/* Create dummy input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterDummy(this,inputDeviceAdapterSection);
				}
			else
				typeFound=false;
			}
		catch(const std::runtime_error& err)
			{
			/* Print an error message: */
			Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Ignoring input device adapter %s due to exception %s",inputDeviceAdapterNames[i].c_str(),err.what());
			
			/* Ignore the input device adapter: */
			inputDeviceAdapters[i]=0;
			++numIgnoredAdapters;
			}
		
		if(!typeFound)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown input device adapter type \"%s\"",inputDeviceAdapterType.c_str());
		}
	
	if(numIgnoredAdapters!=0)
		{
		/* Remove any ignored input device adapters from the array: */
		InputDeviceAdapter** newInputDeviceAdapters=new InputDeviceAdapter*[numInputDeviceAdapters-numIgnoredAdapters];
		int newNumInputDeviceAdapters=0;
		for(int i=0;i<numInputDeviceAdapters;++i)
			if(inputDeviceAdapters[i]!=0)
				{
				newInputDeviceAdapters[newNumInputDeviceAdapters]=inputDeviceAdapters[i];
				++newNumInputDeviceAdapters;
				}
		delete[] inputDeviceAdapters;
		numInputDeviceAdapters=newNumInputDeviceAdapters;
		inputDeviceAdapters=newInputDeviceAdapters;
		}
	
	/* If there is a mouse input device adapter, put it last in the list because it might implicitly depend on other input devices: */
	if(mouseAdapterIndex>=0&&mouseAdapterIndex<numInputDeviceAdapters-1)
		std::swap(inputDeviceAdapters[mouseAdapterIndex],inputDeviceAdapters[numInputDeviceAdapters-1]);
	
	/* Check if there are any valid input device adapters: */
	if(numInputDeviceAdapters==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No valid input device adapters found; I refuse to work under conditions like these!");
	}

void InputDeviceManager::addAdapter(InputDeviceAdapter* newAdapter)
	{
	/* Make room in the input device adapter array: */
	InputDeviceAdapter** newInputDeviceAdapters=new InputDeviceAdapter*[numInputDeviceAdapters+1];
	for(int i=0;i<numInputDeviceAdapters;++i)
		newInputDeviceAdapters[i]=inputDeviceAdapters[i];
	++numInputDeviceAdapters;
	delete[] inputDeviceAdapters;
	inputDeviceAdapters=newInputDeviceAdapters;
	
	/* Add the new adapter: */
	inputDeviceAdapters[numInputDeviceAdapters-1]=newAdapter;
	}

InputDeviceAdapter* InputDeviceManager::findInputDeviceAdapter(const InputDevice* device) const
	{
	/* Search all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		{
		/* Search through all input devices: */
		for(int j=0;j<inputDeviceAdapters[i]->getNumInputDevices();++j)
			if(inputDeviceAdapters[i]->getInputDevice(j)==device)
				return inputDeviceAdapters[i];
		}
	
	return 0;
	}

InputDevice* InputDeviceManager::createInputDevice(const char* deviceName,int trackType,int numButtons,int numValuators,bool physicalDevice)
	{
	/* Get the length of the given device name's prefix: */
	int deviceNamePrefixLength=getPrefixLength(deviceName);
		
	/* Check if a device of the same name prefix already exists: */
	bool exists=false;
	int maxAliasIndex=0;
	for(InputDevices::iterator devIt=inputDevices.begin();devIt!=inputDevices.end();++devIt)
		{
		/* Compare the two prefixes: */
		if(getPrefixLength(devIt->getDeviceName())==deviceNamePrefixLength&&strncmp(deviceName,devIt->getDeviceName(),deviceNamePrefixLength)==0)
			{
			exists=true;
			if(devIt->getDeviceName()[deviceNamePrefixLength]==':')
				{
				int aliasIndex=atoi(devIt->getDeviceName()+deviceNamePrefixLength);
				if(maxAliasIndex<aliasIndex)
					maxAliasIndex=aliasIndex;
				}
			}
		}
	
	/* Create a new (uninitialized) input device: */
	InputDevices::iterator newDevice=inputDevices.insert(inputDevices.end(),InputDevice());
	InputDevice* newDevicePtr=&(*newDevice); // This looks iffy, but I don't know of a better way
	
	/* Initialize the new input device: */
	if(exists)
		{
		/* Create a new alias name for the new device: */
		char newDeviceNameBuffer[256];
		strncpy(newDeviceNameBuffer,deviceName,deviceNamePrefixLength);
		snprintf(newDeviceNameBuffer+deviceNamePrefixLength,sizeof(newDeviceNameBuffer)-deviceNamePrefixLength,":%d",maxAliasIndex+1);
		newDevicePtr->set(newDeviceNameBuffer,trackType,numButtons,numValuators);
		}
	else
		newDevicePtr->set(deviceName,trackType,numButtons,numValuators);
	
	#if DEBUGGING
	std::cout<<"IDM: Creating "<<(physicalDevice?"physical":"virtual")<<" input device "<<newDevicePtr<<" ("<<newDevicePtr->getDeviceName()<<") with "<<numButtons<<" buttons and "<<numValuators<<" valuators"<<std::endl;
	#endif
	
	/* Add the new input device to the input graph: */
	inputGraphManager->addInputDevice(newDevicePtr);
	
	/* If it's a physical device, grab it permanently: */
	if(physicalDevice)
		inputGraphManager->grabInputDevice(newDevicePtr,0); // Passing in null as grabber grabs for the physical layer
	
	/* Call the input device creation callbacks: */
	InputDeviceCreationCallbackData cbData(this,newDevicePtr);
	inputDeviceCreationCallbacks.call(&cbData);
	
	/* Return a pointer to the new input device: */
	return newDevicePtr;
	}

InputDevice* InputDeviceManager::getInputDevice(int deviceIndex)
	{
	InputDevices::iterator idIt;
	for(idIt=inputDevices.begin();idIt!=inputDevices.end()&&deviceIndex>0;++idIt,--deviceIndex)
		;
	if(idIt!=inputDevices.end())
		return &(*idIt);
	else
		return 0;
	}

InputDevice* InputDeviceManager::findInputDevice(const char* deviceName)
	{
	/* Search the given name in the list of devices: */
	InputDevice* result=0;
	for(InputDevices::iterator devIt=inputDevices.begin();devIt!=inputDevices.end();++devIt)
		if(strcmp(deviceName,devIt->getDeviceName())==0)
			{
			result=&(*devIt);
			break;
			}
	return result;
	}

void InputDeviceManager::destroyInputDevice(InputDevice* inputDevice)
	{
	#if DEBUGGING
	std::cout<<"IDM: Destruction process for input device "<<inputDevice<<" ("<<inputDevice->getDeviceName()<<")"<<std::endl;
	#endif
	
	/* Call the input device destruction callbacks: */
	#if DEBUGGING
	std::cout<<"IDM: Calling destruction callbacks for input device "<<inputDevice<<std::endl;
	#endif
	InputDeviceDestructionCallbackData cbData(this,inputDevice);
	inputDeviceDestructionCallbacks.call(&cbData);
	
	/* Remove the device from the input graph: */
	#if DEBUGGING
	std::cout<<"IDM: Removing input device "<<inputDevice<<" from input graph"<<std::endl;
	#endif
	inputGraphManager->removeInputDevice(inputDevice);
	
	/* Find the input device in the list: */
	for(InputDevices::iterator idIt=inputDevices.begin();idIt!=inputDevices.end();++idIt)
		if(&(*idIt)==inputDevice)
			{
			/* Delete it: */
			inputDevices.erase(idIt);
			break;
			}
	
	#if DEBUGGING
	std::cout<<"IDM: Finished destruction process for input device "<<inputDevice<<std::endl;
	#endif
	}

std::string InputDeviceManager::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Find the input device adapter owning the given device: */
	const InputDeviceAdapter* adapter=0;
	for(int i=0;adapter==0&&i<numInputDeviceAdapters;++i)
		{
		/* Search through all input devices: */
		for(int j=0;j<inputDeviceAdapters[i]->getNumInputDevices();++j)
			if(inputDeviceAdapters[i]->getInputDevice(j)==feature.getDevice())
				{
				adapter=inputDeviceAdapters[i];
				break;
				}
		}
	
	if(adapter!=0)
		{
		/* Return the feature name defined by the input device adapter: */
		return adapter->getFeatureName(feature);
		}
	else
		{
		/* Return a default feature name: */
		return InputDeviceAdapter::getDefaultFeatureName(feature);
		}
	}

int InputDeviceManager::getFeatureIndex(InputDevice* device, const char* featureName) const
	{
	/* Find the input device adapter owning the given device: */
	const InputDeviceAdapter* adapter=0;
	for(int i=0;adapter==0&&i<numInputDeviceAdapters;++i)
		{
		/* Search through all input devices: */
		for(int j=0;j<inputDeviceAdapters[i]->getNumInputDevices();++j)
			if(inputDeviceAdapters[i]->getInputDevice(j)==device)
				{
				adapter=inputDeviceAdapters[i];
				break;
				}
		}
	
	if(adapter!=0)
		{
		/* Return the feature index defined by the input device adapter: */
		return adapter->getFeatureIndex(device,featureName);
		}
	else
		{
		/* Parse a default feature name: */
		return InputDeviceAdapter::getDefaultFeatureIndex(device,featureName);
		}
	}

void InputDeviceManager::addHapticFeature(InputDevice* device,InputDeviceAdapter* adapter,unsigned int hapticFeatureIndex)
	{
	/* Add the given haptic feature to the map: */
	HapticFeature hf;
	hf.adapter=adapter;
	hf.hapticFeatureIndex=hapticFeatureIndex;
	hapticFeatureMap.setEntry(HapticFeatureMap::Entry(device,hf));
	}

void InputDeviceManager::addHandleTransform(InputDevice* device,const ONTransform& handleTransform)
	{
	/* Add the handle transformation to the map: */
	handleTransformMap.setEntry(HandleTransformMap::Entry(device,handleTransform));
	}

void InputDeviceManager::disablePrediction(void)
	{
	predictDeviceStates=false;
	}

void InputDeviceManager::prepareMainLoop(void)
	{
	/* Notify all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		inputDeviceAdapters[i]->prepareMainLoop();
	}

void InputDeviceManager::setPredictionTime(const TimePoint& newPredictionTime)
	{
	/* Enable device state prediction and store the given prediction time point: */
	predictDeviceStates=true;
	predictionTime=newPredictionTime;
	}

void InputDeviceManager::setPredictionTimeNow(void)
	{
	/* Set the prediction time to the current time: */
	predictionTime.set();
	}

void InputDeviceManager::updateInputDevices(void)
	{
	/* Grab new data from all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		inputDeviceAdapters[i]->updateInputDevices();
	
	/* Call the update callbacks: */
	InputDeviceUpdateCallbackData cbData(this);
	inputDeviceUpdateCallbacks.call(&cbData);
	}

void InputDeviceManager::glRenderAction(GLContextData& contextData) const
	{
	/* Render all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		inputDeviceAdapters[i]->glRenderAction(contextData);
	}

void InputDeviceManager::hapticTick(InputDevice* device,unsigned int duration,unsigned int frequency,unsigned int amplitude)
	{
	/* Find the input device adapter and haptic feature index for the given device: */
	HapticFeatureMap::Iterator hfmIt=hapticFeatureMap.findEntry(device);
	if(!hfmIt.isFinished())
		{
		/* Forward the request to the input device adapter: */
		HapticFeature& hf=hfmIt->getDest();
		hf.adapter->hapticTick(hf.hapticFeatureIndex,duration,frequency,amplitude);
		}
	}

}

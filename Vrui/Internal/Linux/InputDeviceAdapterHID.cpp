/***********************************************************************
InputDeviceAdapterHID - Linux-specific version of HID input device
adapter.
Copyright (c) 2009-2026 Oliver Kreylos

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

#include <Vrui/Internal/Linux/InputDeviceAdapterHID.h>

#include <Misc/SelfDestructPointer.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Misc/PrintIntegerString.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/MessageLogger.h>
#include <Threads/RunLoop.h>
#include <RawHID/EventDeviceMatcher.h>
#include <Math/MathValueCoders.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/UIManager.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/Internal/HIDPositionerCopy.h>

namespace Vrui {

/**********************************************
Methods of class InputDeviceAdapterHID::Device:
**********************************************/

void InputDeviceAdapterHID::Device::keyFeatureEventCallback(RawHID::EventDevice::KeyFeatureEventCallbackData* cbData)
	{
	/* Check if the changed key feature is represented by a Vrui input device button: */
	int buttonIndex=keyFeatureMap[cbData->featureIndex];
	if(buttonIndex>=0)
		{
		/* Set the state of the Vrui input device button: */
		device->setButtonState(buttonIndex,cbData->newValue);
		}
	}

void InputDeviceAdapterHID::Device::absAxisFeatureEventCallback(RawHID::EventDevice::AbsAxisFeatureEventCallbackData* cbData)
	{
	/* Check if the changed absolue axis feature is represented by a Vrui input device valuator: */
	int valuatorIndex=absAxisFeatureMap[cbData->featureIndex];
	if(valuatorIndex>=0)
		{
		/* Set the value of the Vrui input device valuator: */
		device->setValuator(valuatorIndex,absAxisValueMappers[valuatorIndex].map(double(cbData->newValue)));
		}
	}

void InputDeviceAdapterHID::Device::relAxisFeatureEventCallback(RawHID::EventDevice::RelAxisFeatureEventCallbackData* cbData)
	{
	/* Check if the changed absolue axis feature is represented by a Vrui input device valuator: */
	int valuatorIndex=relAxisFeatureMap[cbData->featureIndex];
	if(valuatorIndex>=0)
		{
		/* Accumulate the new relative axis value into the relative axis value array: */
		relAxisValues[valuatorIndex]+=cbData->value;
		}
	}

void InputDeviceAdapterHID::Device::errorCallback(RawHID::EventDevice::ErrorCallbackData* cbData)
	{
	/* Log an error message: */
	Misc::formattedUserError("InputDeviceAdapterHID: Disabling input device %s due to exception %s",device->getDeviceName(),cbData->exception.what());
	
	/* Disable the HID's associated Vrui input device: */
	getInputGraphManager()->disable(device);
	}

InputDeviceAdapterHID::Device::Device(RawHID::EventDeviceMatcher& deviceMatcher,InputDeviceAdapterHID& sAdapter)
	:RawHID::EventDevice(deviceMatcher),
	 adapter(sAdapter),grabbed(false),device(0),
	 positioner(0),positionerReady(false),
	 numMappedKeys(0),keyFeatureMap(0),
	 numMappedAbsAxes(0),absAxisFeatureMap(0),absAxisValueMappers(0),
	 numMappedRelAxes(0),relAxisFeatureMap(0),relAxisValues(0),relAxisValueMappers(0)
	{
	/* Attempt to grab the HID: */
	grabbed=grabDevice();
	}

InputDeviceAdapterHID::Device::~Device(void)
	{
	/* Release the HID if it was grabbed: */
	if(grabbed)
		releaseDevice();
	
	/* Release allocated resources: */
	delete positioner;
	delete[] keyFeatureMap;
	delete[] absAxisFeatureMap;
	delete[] absAxisValueMappers;
	delete[] relAxisFeatureMap;
	delete[] relAxisValues;
	delete[] relAxisValueMappers;
	}

void InputDeviceAdapterHID::Device::prepareMainLoop(void)
	{
	/* Prepare a potential positioner: */
	if(positioner!=0)
		{
		positioner->prepareMainLoop();
		positionerReady=true;
		}
	}

void InputDeviceAdapterHID::Device::update(void)
	{
	/* Update the device's relative axes and reset the accumulated axis values: */
	for(int i=0;i<numMappedRelAxes;++i)
		{
		device->setValuator(numMappedAbsAxes+i,relAxisValueMappers[i].map(double(relAxisValues[i])));
		relAxisValues[i]=0;
		}
	
	/* Update the device's tracking state: */
	if(positionerReady)
		positioner->updateDevice(device);
	}

/**************************************
Methods of class InputDeviceAdapterHID:
**************************************/

void InputDeviceAdapterHID::initializeInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Retrieve the name of this device: */
	std::string name=configFileSection.retrieveString("./name",configFileSection.getName());
	
	/*************************************************************
	Find and open the HID to be associated with this input device:
	*************************************************************/
	
	/* Get the index of the device among matching devices: */
	RawHID::SelectEventDeviceMatcher deviceMatcher;
	
	/* Add an optional vendor/product ID to the device matcher: */
	if(configFileSection.hasTag("./deviceVendorProductId"))
		{
		/* Read HID's vendor / product IDs: */
		std::string deviceVendorProductId=configFileSection.retrieveString("./deviceVendorProductId");
		
		/* Split ID string into vendor ID / product ID: */
		char* colonPtr;
		unsigned int vendorId=strtoul(deviceVendorProductId.c_str(),&colonPtr,16);
		char* endPtr;
		unsigned int productId=strtoul(colonPtr+1,&endPtr,16);
		if(*colonPtr!=':'||*endPtr!='\0')
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Malformed vendorId:productId string \"%s\" for device %s",deviceVendorProductId.c_str(),name.c_str());
		
		deviceMatcher.setVendorId(vendorId);
		deviceMatcher.setProductId(productId);
		}
	
	/* Add an optional version number to the device matcher: */
	if(configFileSection.hasTag("./deviceVersion"))
		deviceMatcher.setVersion(configFileSection.retrieveValue<unsigned int>("./deviceVersion"));
	
	/* Add an optional device name to the device matcher: */
	if(configFileSection.hasTag("./deviceName"))
		deviceMatcher.setDeviceName(configFileSection.retrieveString("./deviceName"));
	
	/* Add an optional device serial number to the device matcher: */
	if(configFileSection.hasTag("./deviceSerialNumber"))
		deviceMatcher.setSerialNumber(configFileSection.retrieveString("./deviceSerialNumber"));
	
	/* Add an optional match index to the device matcher: */
	if(configFileSection.hasTag("./deviceIndex"))
		deviceMatcher.setIndex(configFileSection.retrieveValue<unsigned int>("./deviceIndex"));
	
	/* Create a new device object: */
	Misc::SelfDestructPointer<Device> newDevice=new Device(deviceMatcher,*this);
	
	/************************************
	Set up tracking for the input device:
	************************************/
	
	/* Create an array of flags to ignore a subset of the HID's key, absolute axis, and relative axis features, in that order: */
	unsigned int numFeatures=newDevice->getNumKeyFeatures()+newDevice->getNumAbsAxisFeatures()+newDevice->getNumRelAxisFeatures();
	Misc::SelfDestructArray<bool> ignoredFeatures(numFeatures);
	for(unsigned int i=0;i<numFeatures;++i)
		ignoredFeatures[i]=false;
	
	/* Get pointers to the ignore array parts: */
	bool* ignoredKeys=ignoredFeatures.getArray();
	bool* ignoredAbsAxes=ignoredKeys+newDevice->getNumKeyFeatures();
	bool* ignoredRelAxes=ignoredAbsAxes+newDevice->getNumAbsAxisFeatures();
	
	/* Create a positioner for the associated Vrui input device: */
	int trackType=InputDevice::TRACK_NONE;
	if(configFileSection.hasTag("./positioner"))
		{
		/* Create a HID positioner from the configuration file section of the given name: */
		newDevice->positioner=HIDPositioner::create(*newDevice,configFileSection.getSection(configFileSection.retrieveString("./positioner").c_str()),ignoredFeatures.getArray());
		
		/* Override the HID positioner's tracking type from the configuration file: */
		trackType=updateTrackType(newDevice->positioner->getTrackType(),configFileSection);
		
		/* Determine whether the new input device should be projected by the UI manager: */
		bool projectDevice=((newDevice->positioner->getTrackType()^trackType)&InputDevice::TRACK_ORIENT)!=0x0; // Project if the source device is a 6-DOF device, and the HID device is a ray device
		configFileSection.updateValue("./projectDevice",projectDevice);
		newDevice->positioner->setProject(projectDevice);
		}
	
	/* Read lists of key and absolute and relative axis features to ignore: */
	typedef std::vector<unsigned int> IndexList;
	
	IndexList ignoreKeyFeatures;
	configFileSection.updateValue("./ignoreKeyFeatures",ignoreKeyFeatures);
	for(IndexList::iterator iIt=ignoreKeyFeatures.begin();iIt!=ignoreKeyFeatures.end();++iIt)
		if(*iIt<newDevice->getNumKeyFeatures())
			ignoredKeys[*iIt]=true;
	
	IndexList ignoreAbsAxisFeatures;
	configFileSection.updateValue("./ignoreAbsAxisFeatures",ignoreAbsAxisFeatures);
	for(IndexList::iterator iIt=ignoreAbsAxisFeatures.begin();iIt!=ignoreAbsAxisFeatures.end();++iIt)
		if(*iIt<newDevice->getNumAbsAxisFeatures())
			ignoredAbsAxes[*iIt]=true;
	
	IndexList ignoreRelAxisFeatures;
	configFileSection.updateValue("./ignoreRelAxisFeatures",ignoreRelAxisFeatures);
	for(IndexList::iterator iIt=ignoreRelAxisFeatures.begin();iIt!=ignoreRelAxisFeatures.end();++iIt)
		if(*iIt<newDevice->getNumRelAxisFeatures())
			ignoredRelAxes[*iIt]=true;
	
	/********************************************************
	Represent the HID's key features as input device buttons:
	********************************************************/
	
	/* Count the number of unignored key features: */
	newDevice->numMappedKeys=0;
	for(unsigned int i=0;i<newDevice->getNumKeyFeatures();++i)
		if(!ignoredKeys[i])
			++newDevice->numMappedKeys;
	
	/* Create the key feature map: */
	newDevice->keyFeatureMap=new int[newDevice->getNumKeyFeatures()];
	int keyIndex=0;
	for(unsigned int i=0;i<newDevice->getNumKeyFeatures();++i)
		if(!ignoredKeys[i])
			{
			/* Assign the next Vrui device button index to the key feature: */
			newDevice->keyFeatureMap[i]=keyIndex++;
			}
		else
			{
			/* Mark the key feature as ignored: */
			newDevice->keyFeatureMap[i]=-1;
			}
	
	/********************************************************************
	Represent the HID's absolute axis features as input device valuators:
	********************************************************************/
	
	/* Count the number of unignored absolute axes: */
	newDevice->numMappedAbsAxes=0;
	for(unsigned int i=0;i<newDevice->getNumAbsAxisFeatures();++i)
		if(!ignoredAbsAxes[i])
			++newDevice->numMappedAbsAxes;
	
	/* Create the absolute axis feature map and axis value mappers: */
	newDevice->absAxisFeatureMap=new int[newDevice->getNumAbsAxisFeatures()];
	newDevice->absAxisValueMappers=new Device::AxisValueMapper[newDevice->numMappedAbsAxes];
	int absAxisIndex=0;
	for(unsigned int i=0;i<newDevice->getNumAbsAxisFeatures();++i)
		if(!ignoredAbsAxes[i])
			{
			/* Assign the next Vrui device valuator index to the absolute axis feature: */
			newDevice->absAxisFeatureMap[i]=absAxisIndex;
			
			/* Retrieve the absolute axis feature's default axis mapping: */
			const RawHID::EventDevice::AbsAxisConfig& absAxisConfig=newDevice->getAbsAxisFeatureConfig(i);
			
			/* Create an axis value mapper in normalized axis space: */
			Device::AxisValueMapper avm;
			double s=double(absAxisConfig.max)-double(absAxisConfig.min);
			double o=double(absAxisConfig.min);
			avm.min=(double(absAxisConfig.min)-o)/s;
			double mid=Math::mid(double(absAxisConfig.min),double(absAxisConfig.max));
			double flat=Math::div2(double(absAxisConfig.flat));
			avm.deadMin=(mid-flat-o)/s;
			avm.deadMax=(mid+flat-o)/s;
			avm.max=(double(absAxisConfig.max)-o)/s;
			
			/* Override the axis value mapper from the configuration file section: */
			configFileSection.updateValue((std::string("./valuatorMapping")+Misc::printString(absAxisIndex)).c_str(),avm);
			
			/* Store the axis value mapper in raw axis space: */
			avm.min=avm.min*s+o;
			avm.deadMin=avm.deadMin*s+o;
			avm.deadMax=avm.deadMax*s+o;
			avm.max=avm.max*s+o;
			newDevice->absAxisValueMappers[absAxisIndex]=avm;
			
			/* Go to the next valuator index: */
			++absAxisIndex;
			}
		else
			{
			/* Mark the absolute axis feature as ignored: */
			newDevice->absAxisFeatureMap[i]=-1;
			}
	
	/********************************************************************
	Represent the HID's relative axis features as input device valuators:
	********************************************************************/
	
	/* Count the number of unignored relative axes: */
	newDevice->numMappedRelAxes=0;
	for(unsigned int i=0;i<newDevice->getNumRelAxisFeatures();++i)
		if(!ignoredRelAxes[i])
			++newDevice->numMappedRelAxes;
	
	/* Create the relative axis feature map, value array, and axis value mappers: */
	newDevice->relAxisFeatureMap=new int[newDevice->getNumRelAxisFeatures()];
	newDevice->relAxisValues=new int[newDevice->numMappedRelAxes];
	newDevice->relAxisValueMappers=new Device::AxisValueMapper[newDevice->numMappedRelAxes];
	int relAxisIndex=0;
	for(unsigned int i=0;i<newDevice->getNumRelAxisFeatures();++i)
		if(!ignoredRelAxes[i])
			{
			/* Assign the next Vrui device valuator index to the relative axis feature: */
			newDevice->relAxisFeatureMap[i]=newDevice->numMappedAbsAxes+relAxisIndex;
			
			/* Initialize the relative axis' value accumulator: */
			newDevice->relAxisValues[relAxisIndex]=0;
			
			/* Create a default axis value mapper: */
			Device::AxisValueMapper avm(-1.0,0.0,0.0,1.0);
			
			/* Override the axis value mapper from the configuration file section: */
			configFileSection.updateValue((std::string("./valuatorMapping")+Misc::printString(newDevice->numMappedAbsAxes+relAxisIndex)).c_str(),avm);
			
			/* Store the axis value mapper: */
			newDevice->relAxisValueMappers[relAxisIndex]=avm;
			
			/* Go to the next valuator index: */
			++relAxisIndex;
			}
		else
			{
			/* Mark the relative axis feature as ignored: */
			newDevice->relAxisFeatureMap[i]=-1;
			}
	
	/**************************************************
	Create the Vrui input device representing this HID:
	**************************************************/
	
	/* Create the Vrui input device representing this HID as a physical input device: */
	inputDevices[deviceIndex]=newDevice->device=createInputDevice(name.c_str(),trackType,newDevice->numMappedKeys,newDevice->numMappedAbsAxes+newDevice->numMappedRelAxes,configFileSection,newDevice->buttonNames,newDevice->valuatorNames);
	
	#if 0
	
	/* Initialize the Vrui input device's tracking state: */
	if(newDevice->positioner!=0)
		newDevice->positioner->updateDevice(newDevice->device);
	
	#endif
	
	/*****************************
	Finalize the new input device:
	*****************************/
	
	/* Register callbacks with the HID: */
	newDevice->getKeyFeatureEventCallbacks().add(newDevice.getTarget(),&Device::keyFeatureEventCallback);
	newDevice->getAbsAxisFeatureEventCallbacks().add(newDevice.getTarget(),&Device::absAxisFeatureEventCallback);
	newDevice->getRelAxisFeatureEventCallbacks().add(newDevice.getTarget(),&Device::relAxisFeatureEventCallback);
	newDevice->getErrorCallbacks().add(newDevice.getTarget(),&Device::errorCallback);
	
	/* Store the new device structure: */
	devices.push_back(newDevice.releaseTarget());
	}

InputDeviceAdapterHID::InputDeviceAdapterHID(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager)
	{
	/* Initialize input device adapter: */
	InputDeviceAdapter::initializeAdapter(configFileSection);
	
	/* Register all HIDs with Vrui's main run loop: */
	for(std::vector<Device*>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		(*dIt)->watch(getRunLoop());
	}

InputDeviceAdapterHID::~InputDeviceAdapterHID(void)
	{
	/* Delete all devices: */
	for(std::vector<Device*>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		delete *dIt;
	}

std::string InputDeviceAdapterHID::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Find the HID structure for the given input device: */
	std::vector<Device*>::const_iterator dIt;
	for(dIt=devices.begin();dIt!=devices.end()&&(*dIt)->device!=feature.getDevice();++dIt)
		;
	if(dIt==devices.end())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",feature.getDevice()->getDeviceName());
	
	/* Check whether the feature is a button or a valuator: */
	std::string result;
	if(feature.isButton())
		{
		/* Return the button feature's name: */
		result=(*dIt)->buttonNames[feature.getIndex()];
		}
	if(feature.isValuator())
		{
		/* Return the valuator feature's name: */
		result=(*dIt)->valuatorNames[feature.getIndex()];
		}
	
	return result;
	}

int InputDeviceAdapterHID::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Find the HID structure for the given input device: */
	std::vector<Device*>::const_iterator dIt;
	for(dIt=devices.begin();dIt!=devices.end()&&(*dIt)->device!=device;++dIt)
		;
	if(dIt==devices.end())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",device->getDeviceName());
	
	/* Check if the feature names a button or a valuator: */
	int numButtons=(*dIt)->numMappedKeys;
	for(int buttonIndex=0;buttonIndex<numButtons;++buttonIndex)
		if((*dIt)->buttonNames[buttonIndex]==featureName)
			return device->getButtonFeatureIndex(buttonIndex);
	int numValuators=(*dIt)->numMappedAbsAxes+(*dIt)->numMappedRelAxes;
	for(int valuatorIndex=0;valuatorIndex<numValuators;++valuatorIndex)
		if((*dIt)->valuatorNames[valuatorIndex]==featureName)
			return device->getValuatorFeatureIndex(valuatorIndex);
	
	return -1;
	}

void InputDeviceAdapterHID::prepareMainLoop(void)
	{
	/* Prepare all represented devices: */
	for(std::vector<Device*>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		(*dIt)->prepareMainLoop();
	}

void InputDeviceAdapterHID::updateInputDevices(void)
	{
	/* Call the update methods of all represented devices: */
	Threads::Mutex::Lock deviceStateLock(deviceStateMutex);
	for(std::vector<Device*>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		(*dIt)->update();
	}

}

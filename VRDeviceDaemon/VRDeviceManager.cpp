/***********************************************************************
VRDeviceManager - Class to gather position, button and valuator data
from one or several VR devices and associate them with logical input
devices.
Copyright (c) 2002-2023 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <VRDeviceDaemon/VRDeviceManager.h>

#include <stdio.h>
#include <dlfcn.h>
#include <vector>
#include <Misc/PrintInteger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Realtime/Time.h>
#include <Realtime/SharedMemory.h>
#include <Threads/Config.h>
#include <Threads/EventDispatcher.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>

#include <VRDeviceDaemon/VRFactory.h>
#include <VRDeviceDaemon/VRDevice.h>
#include <VRDeviceDaemon/VRCalibrator.h>
#include <VRDeviceDaemon/Config.h>

/********************************
Methods of class VRDeviceManager:
********************************/

void VRDeviceManager::postStateUpdate(void)
	{
	/* Write the given data into the buffer half not currently used by readers: */
	unsigned int* counter=stateMemory->getValue<unsigned int>(0);
	#if THREADS_CONFIG_HAVE_BUILTIN_ATOMICS
	unsigned int nextHalf=(~__atomic_load_n(counter,__ATOMIC_ACQUIRE))&0x1U;
	#endif
	size_t headerSize=sizeof(ptrdiff_t);
	size_t stateSize=state.getStateSize();
	state.write(stateMemory->getValue<char>(0)+headerSize+nextHalf*stateSize);
	
	/* Increment the counter to invalidate previous data and flip the readable buffer half: */
	#if THREADS_CONFIG_HAVE_BUILTIN_ATOMICS
	__atomic_add_fetch(counter,1,__ATOMIC_RELEASE);
	#endif
	}

VRDeviceManager::VRDeviceManager(Threads::EventDispatcher& sDispatcher,Misc::ConfigurationFile& configFile)
	:dispatcher(sDispatcher),
	 deviceFactories(configFile.retrieveString("./deviceDirectory",VRDEVICEDAEMON_CONFIG_VRDEVICESDIR),this),
	 calibratorFactories(configFile.retrieveString("./calibratorDirectory",VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR)),
	 numDevices(0),
	 devices(0),trackerIndexBases(0),buttonIndexBases(0),valuatorIndexBases(0),
	 stateMemory(0),
	 fullTrackerReportMask(0x0),trackerReportMask(0x0),streamer(0)
	{
	/* Allocate device and base index arrays: */
	typedef std::vector<std::string> StringList;
	StringList deviceNames=configFile.retrieveValue<StringList>("./deviceNames");
	numDevices=deviceNames.size();
	devices=new VRDevice*[numDevices];
	trackerIndexBases=new int[numDevices];
	buttonIndexBases=new int[numDevices];
	valuatorIndexBases=new int[numDevices];
	
	/* Initialize VR devices: */
	for(currentDeviceIndex=0;currentDeviceIndex<numDevices;++currentDeviceIndex)
		{
		/* Save the device's base indices: */
		trackerIndexBases[currentDeviceIndex]=int(trackerNames.size());
		buttonIndexBases[currentDeviceIndex]=int(buttonNames.size());
		valuatorIndexBases[currentDeviceIndex]=int(valuatorNames.size());
		
		/* Go to device's section: */
		configFile.setCurrentSection(deviceNames[currentDeviceIndex].c_str());
		
		/* Retrieve device type: */
		std::string deviceType=configFile.retrieveString("./deviceType");
		
		/* Initialize device: */
		#ifdef VERBOSE
		printf("VRDeviceManager: Loading device %s of type %s\n",deviceNames[currentDeviceIndex].c_str(),deviceType.c_str());
		fflush(stdout);
		#endif
		DeviceFactoryManager::Factory* deviceFactory=deviceFactories.getFactory(deviceType);
		devices[currentDeviceIndex]=deviceFactory->createObject(configFile);
		
		if(configFile.hasTag("./trackerNames"))
			{
			StringList deviceTrackerNames=configFile.retrieveValue<StringList>("./trackerNames");
			int trackerIndex=trackerIndexBases[currentDeviceIndex];
			int numTrackers=trackerNames.size();
			for(StringList::iterator dtnIt=deviceTrackerNames.begin();dtnIt!=deviceTrackerNames.end()&&trackerIndex<numTrackers;++dtnIt,++trackerIndex)
				trackerNames[trackerIndex]=*dtnIt;
			}
		
		/* Override device's button names: */
		if(configFile.hasTag("./buttonNames"))
			{
			StringList deviceButtonNames=configFile.retrieveValue<StringList>("./buttonNames");
			int buttonIndex=buttonIndexBases[currentDeviceIndex];
			int numButtons=buttonNames.size();
			for(StringList::iterator dtnIt=deviceButtonNames.begin();dtnIt!=deviceButtonNames.end()&&buttonIndex<numButtons;++dtnIt,++buttonIndex)
				buttonNames[buttonIndex]=*dtnIt;
			}
		
		/* Override device's valuator names: */
		if(configFile.hasTag("./valuatorNames"))
			{
			StringList deviceValuatorNames=configFile.retrieveValue<StringList>("./valuatorNames");
			int valuatorIndex=valuatorIndexBases[currentDeviceIndex];
			int numValuators=valuatorNames.size();
			for(StringList::iterator dtnIt=deviceValuatorNames.begin();dtnIt!=deviceValuatorNames.end()&&valuatorIndex<numValuators;++dtnIt,++valuatorIndex)
				valuatorNames[valuatorIndex]=*dtnIt;
			}
		
		/* Return to parent section: */
		configFile.setCurrentSection("..");
		}
	#ifdef VERBOSE
	printf("VRDeviceManager: Managing %d trackers, %d buttons, %d valuators\n",int(trackerNames.size()),int(buttonNames.size()),int(valuatorNames.size()));
	fflush(stdout);
	#endif
	
	/* Set server state's layout: */
	state.setLayout(trackerNames.size(),buttonNames.size(),valuatorNames.size());
	
	/* Read names of all virtual devices: */
	StringList virtualDeviceNames=configFile.retrieveValue<StringList>("./virtualDeviceNames",StringList());
	
	/* Initialize virtual devices: */
	for(StringList::iterator vdnIt=virtualDeviceNames.begin();vdnIt!=virtualDeviceNames.end();++vdnIt)
		{
		/* Create a new virtual device descriptor by reading the virtual device's configuration file section: */
		Vrui::VRDeviceDescriptor* vdd=new Vrui::VRDeviceDescriptor;
		vdd->name=*vdnIt;
		vdd->load(configFile.getSection(vdnIt->c_str()));
		
		/* Store the virtual device descriptor: */
		virtualDevices.push_back(vdd);
		
		/* Create a battery state for the new virtual device: */
		batteryStates.push_back(Vrui::BatteryState());
		}
	#ifdef VERBOSE
	printf("VRDeviceManager: Managing %d virtual devices\n",int(virtualDevices.size()));
	fflush(stdout);
	#endif
	
	/* Initialize all loaded devices: */
	#ifdef VERBOSE
	printf("VRDeviceManager: Initializing %d device driver modules\n",numDevices);
	fflush(stdout);
	#endif
	for(int i=0;i<numDevices;++i)
		devices[i]->initialize();
	}

VRDeviceManager::~VRDeviceManager(void)
	{
	/* Delete device objects: */
	for(int i=0;i<numDevices;++i)
		VRDevice::destroy(devices[i]);
	delete[] devices;
	
	/* Delete base index arrays: */
	delete[] trackerIndexBases;
	delete[] buttonIndexBases;
	delete[] valuatorIndexBases;
	
	/* Release a potential device state shared memory segment (this is safe to do here because if there is one, the device state object won't release its pointers): */
	delete stateMemory;
	
	/* Delete virtual devices: */
	for(std::vector<Vrui::VRDeviceDescriptor*>::iterator vdIt=virtualDevices.begin();vdIt!=virtualDevices.end();++vdIt)
		delete *vdIt;
	
	/* Delete HMD configurations: */
	for(std::vector<Vrui::HMDConfiguration*>::iterator hcIt=hmdConfigurations.begin();hcIt!=hmdConfigurations.end();++hcIt)
		delete *hcIt;
	}

int VRDeviceManager::addTracker(const char* name)
	{
	/* Get the next tracker index: */
	int result=trackerNames.size();
	
	/* Push back a new tracker name: */
	if(name==0)
		{
		std::string tname="Tracker";
		char index[10];
		tname.append(Misc::print(result,index+sizeof(index)-1));
		trackerNames.push_back(tname);
		}
	else
		trackerNames.push_back(name);
	
	/* Update the tracker report mask: */
	fullTrackerReportMask=(0x1U<<(result+1))-1U;
	
	return result;
	}

int VRDeviceManager::addButton(const char* name)
	{
	/* Get the next button index: */
	int result=buttonNames.size();
	
	/* Push back a new button name: */
	if(name==0)
		{
		std::string bname="Button";
		char index[10];
		bname.append(Misc::print(result,index+sizeof(index)-1));
		buttonNames.push_back(bname);
		}
	else
		buttonNames.push_back(name);
	
	return result;
	}

int VRDeviceManager::addValuator(const char* name)
	{
	/* Get the next valuator index: */
	int result=valuatorNames.size();
	
	/* Push back a new valuator name: */
	if(name==0)
		{
		std::string vname="Valuator";
		char index[10];
		vname.append(Misc::print(result,index+sizeof(index)-1));
		valuatorNames.push_back(vname);
		}
	else
		valuatorNames.push_back(name);
	
	return result;
	}

VRCalibrator* VRDeviceManager::createCalibrator(const std::string& calibratorType,Misc::ConfigurationFile& configFile)
	{
	CalibratorFactoryManager::Factory* calibratorFactory=calibratorFactories.getFactory(calibratorType);
	return calibratorFactory->createObject(configFile);
	}

unsigned int VRDeviceManager::addVirtualDevice(Vrui::VRDeviceDescriptor* newVirtualDevice)
	{
	/* Store the virtual device: */
	unsigned int result=virtualDevices.size();
	virtualDevices.push_back(newVirtualDevice);
	
	/* New devices are connected by default to keep legacy intact: */
	deviceConnecteds.push_back(true);
	
	/* Create a battery state for the new virtual device: */
	batteryStates.push_back(Vrui::BatteryState());
	
	return result;
	}

Vrui::HMDConfiguration* VRDeviceManager::addHmdConfiguration(void)
	{
	/* Store a new HMD configuration: */
	Vrui::HMDConfiguration* newConfiguration=new Vrui::HMDConfiguration;
	hmdConfigurations.push_back(newConfiguration);
	
	return newConfiguration;
	}

int VRDeviceManager::addPowerFeature(VRDevice* device,int deviceFeatureIndex)
	{
	/* Create a new feature: */
	int result=int(powerFeatures.size());
	Feature newPowerFeature;
	newPowerFeature.device=device;
	newPowerFeature.deviceFeatureIndex=deviceFeatureIndex;
	powerFeatures.push_back(newPowerFeature);
	
	return result;
	}

int VRDeviceManager::addHapticFeature(VRDevice* device,int deviceFeatureIndex)
	{
	/* Create a new feature: */
	int result=int(hapticFeatures.size());
	Feature newHapticFeature;
	newHapticFeature.device=device;
	newHapticFeature.deviceFeatureIndex=deviceFeatureIndex;
	hapticFeatures.push_back(newHapticFeature);
	
	return result;
	}

void VRDeviceManager::setVirtualDeviceConnected(int deviceIndex,bool newConnected)
	{
	deviceConnecteds[deviceIndex]=newConnected;
	}

void VRDeviceManager::disableTracker(int trackerIndex)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	
	/* Update the device state: */
	state.setTrackerValid(trackerIndex,false);
	if(stateMemory!=0)
		postStateUpdate();
	
	/* Check if update notifications are requested: */
	if(streamer!=0)
		{
		/* Notify streamer of single tracker update: */
		streamer->trackerUpdated(trackerIndex);
		
		/* Update tracker report mask: */
		trackerReportMask|=1<<trackerIndex;
		if(trackerReportMask==fullTrackerReportMask)
			{
			/* Notify streamer that device state has completed update: */
			streamer->updateCompleted();
			
			/* Reset the tracker report mask: */
			trackerReportMask=0x0;
			}
		}
	}

void VRDeviceManager::setTrackerState(int trackerIndex,const Vrui::VRDeviceState::TrackerState& newTrackerState,Vrui::VRDeviceState::TimeStamp newTimeStamp)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	
	/* Update the device state: */
	state.setTrackerState(trackerIndex,newTrackerState);
	state.setTrackerTimeStamp(trackerIndex,newTimeStamp);
	state.setTrackerValid(trackerIndex,true);
	if(stateMemory!=0)
		postStateUpdate();
	
	/* Check if update notifications are requested: */
	if(streamer!=0)
		{
		/* Notify streamer of single tracker update: */
		streamer->trackerUpdated(trackerIndex);
		
		/* Update tracker report mask: */
		trackerReportMask|=1<<trackerIndex;
		if(trackerReportMask==fullTrackerReportMask)
			{
			/* Notify streamer that device state has completed update: */
			streamer->updateCompleted();
			
			/* Reset the tracker report mask: */
			trackerReportMask=0x0;
			}
		}
	}

void VRDeviceManager::setButtonState(int buttonIndex,Vrui::VRDeviceState::ButtonState newButtonState)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	
	/* Update the button state: */
	state.setButtonState(buttonIndex,newButtonState);
	if(stateMemory!=0)
		postStateUpdate();
	
	/* Check if update notifications are requested: */
	if(streamer!=0)
		{
		/* Notify streamer of single button update: */
		streamer->buttonUpdated(buttonIndex);
		}
	}

void VRDeviceManager::setValuatorState(int valuatorIndex,Vrui::VRDeviceState::ValuatorState newValuatorState)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	
	/* Update the valuator state: */
	state.setValuatorState(valuatorIndex,newValuatorState);
	if(stateMemory!=0)
		postStateUpdate();
	
	/* Check if update notifications are requested: */
	if(streamer!=0)
		{
		/* Notify streamer of single valuator update: */
		streamer->valuatorUpdated(valuatorIndex);
		}
	}

void VRDeviceManager::updateState(void)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	
	/* Check if update notifications are requested and an update is necessary: */
	if(streamer!=0&&(trackerReportMask!=0x0||fullTrackerReportMask==0x0))
		{
		/* Notify streamer that device state has completed update: */
		streamer->updateCompleted();
		
		/* Reset the tracker report mask: */
		trackerReportMask=0x0;
		}
	}

void VRDeviceManager::updateBatteryState(unsigned int virtualDeviceIndex,const Vrui::BatteryState& newBatteryState)
	{
	Threads::Mutex::Lock batteryStateLock(batteryStateMutex);
	
	/* Remember that this device has a battery: */
	virtualDevices[virtualDeviceIndex]->hasBattery=true;
	
	/* Check if the battery state actually changed: */
	Vrui::BatteryState& bs=batteryStates[virtualDeviceIndex];
	bool changed=bs.charging!=newBatteryState.charging||bs.batteryLevel!=newBatteryState.batteryLevel;
	if(changed)
		{
		/* Update the battery state and call the state update callback: */
		bs=newBatteryState;
		if(streamer!=0)
			streamer->batteryStateUpdated(virtualDeviceIndex);
		}
	}

void VRDeviceManager::updateHmdConfiguration(const Vrui::HMDConfiguration* hmdConfiguration)
	{
	/* Check if update notifications are requested: */
	if(streamer!=0)
		streamer->hmdConfigurationUpdated(hmdConfiguration);
	}

int VRDeviceManager::addBaseStation(const std::string& serialNumber)
	{
	/* Add a new base station: */
	Threads::Mutex::Lock baseStationLock(baseStationMutex);
	int result(baseStations.size());
	baseStations.push_back(Vrui::VRBaseStation(serialNumber));
	
	return result;
	}

int VRDeviceManager::useSharedMemory(const char* sharedMemoryName)
	{
	/* Calculate the memory size required to hold a double buffer of device states: */
	size_t headerSize=sizeof(ptrdiff_t);
	size_t stateSize=state.getStateSize();
	
	/* Create a shared memory segment of sufficient size: */
	stateMemory=new Realtime::SharedMemory(sharedMemoryName,headerSize+2*stateSize);
	
	/* Initialize the device state double buffer with the current device state: */
	*stateMemory->getValue<unsigned int>(0)=0;
	state.write(stateMemory->getValue<char>(0)+headerSize+0*stateSize);
	
	/* Return the shared memory segment's file descriptor: */
	return stateMemory->getFd();
	}

void VRDeviceManager::powerOff(unsigned int powerFeatureIndex)
	{
	/* Check if the feature exists: */
	if(powerFeatureIndex<powerFeatures.size())
		{
		/* Forward request to managing device: */
		Feature& f=powerFeatures[powerFeatureIndex];
		f.device->powerOff(f.deviceFeatureIndex);
		}
	}

void VRDeviceManager::hapticTick(unsigned int hapticFeatureIndex,unsigned int duration,unsigned int frequency,unsigned int amplitude)
	{
	/* Check if the feature exists: */
	if(hapticFeatureIndex<hapticFeatures.size())
		{
		/* Forward request to managing device: */
		Feature& f=hapticFeatures[hapticFeatureIndex];
		f.device->hapticTick(f.deviceFeatureIndex,duration,frequency,amplitude);
		}
	}

void VRDeviceManager::setStreamer(VRStreamer* newStreamer)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	Threads::Mutex::Lock batteryStateLock(batteryStateMutex);
	Threads::Mutex::Lock hmdConfigurationLock(hmdConfigurationMutex);
	
	/* Set the streamer object: */
	streamer=newStreamer;
	}

void VRDeviceManager::start(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceManager: Starting devices\n");
	fflush(stdout);
	#endif
	for(int i=0;i<numDevices;++i)
		devices[i]->start();
	}

void VRDeviceManager::stop(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceManager: Stopping devices\n");
	fflush(stdout);
	#endif
	for(int i=0;i<numDevices;++i)
		devices[i]->stop();
	}

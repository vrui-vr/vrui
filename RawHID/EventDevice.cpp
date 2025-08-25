/***********************************************************************
EventDevice - Class representing an input device using the Linux event
subsystem.
Copyright (c) 2023-2025 Oliver Kreylos

This file is part of the Raw HID Support Library (RawHID).

The Raw HID Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Raw HID Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Raw HID Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <RawHID/EventDevice.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifndef __USE_GNU
#define __USE_GNU
#include <dirent.h>
#undef __USE_GNU
#else
#include <dirent.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <Misc/StdError.h>
#include <RawHID/Config.h>
#include <RawHID/EventDeviceMatcher.h>

namespace RawHID {

namespace {

/****************
Helper functions:
****************/

int isEventFile(const struct dirent* directoryEntry)
	{
	return strncmp(directoryEntry->d_name,"event",5)==0;
	}

}

/*************************************
Methods of class EventDevice::Feature:
*************************************/

EventDevice::Feature::Feature(void)
	:device(0),index(~0U)
	{
	}

EventDevice::Feature::Feature(EventDevice* sDevice,unsigned int sIndex)
	:device(sDevice),index(sIndex)
	{
	}

/****************************************
Methods of class EventDevice::KeyFeature:
****************************************/

EventDevice::KeyFeature::KeyFeature(void)
	:value(false)
	{
	}

EventDevice::KeyFeature::KeyFeature(EventDevice* sDevice,unsigned int sIndex)
	:Feature(sDevice,sIndex),
	 value(device->getKeyFeatureValue(index))
	{
	}

void EventDevice::KeyFeature::update(void)
	{
	value=device->getKeyFeatureValue(index);
	}

/********************************************
Methods of class EventDevice::AbsAxisFeature:
********************************************/

EventDevice::AbsAxisFeature::AbsAxisFeature(void)
	:value(0)
	{
	}

EventDevice::AbsAxisFeature::AbsAxisFeature(EventDevice* sDevice,unsigned int sIndex)
	:Feature(sDevice,sIndex),
	 value(device->getAbsAxisFeatureValue(index))
	{
	}

void EventDevice::AbsAxisFeature::update(void)
	{
	value=device->getAbsAxisFeatureValue(index);
	}

/****************************
Methods of class EventDevice:
****************************/

int EventDevice::findDevice(EventDeviceMatcher& deviceMatcher)
	{
	/* Create list of all available /dev/input/eventX event device files, in numerical order: */
	struct dirent** eventFiles=0;
	int numEventFiles=scandir(RAWHID_EVENTDEVICEFILEDIR,&eventFiles,isEventFile,versionsort);
	
	/* Check all event files against the given device specification: */
	int matchedFd=-1;
	for(int eventFileIndex=0;eventFileIndex<numEventFiles;++eventFileIndex)
		{
		/* Try opening the event device file: */
		char eventFileName[288];
		snprintf(eventFileName,sizeof(eventFileName),"%s/%s",RAWHID_EVENTDEVICEFILEDIR,eventFiles[eventFileIndex]->d_name);
		int eventFileFd=open(eventFileName,O_RDWR);
		if(eventFileFd>=0)
			{
			/* Get the device information: */
			input_id eventFileDeviceInfo;
			if(ioctl(eventFileFd,EVIOCGID,&eventFileDeviceInfo)>=0)
				{
				/* Get the device name: */
				char eventFileDeviceName[256];
				if(ioctl(eventFileFd,EVIOCGNAME(sizeof(eventFileDeviceName)),eventFileDeviceName)>=0)
					{
					/* Get the serial number: */
					char eventFileSerialNumber[256];
					if(ioctl(eventFileFd,EVIOCGUNIQ(sizeof(eventFileSerialNumber)),eventFileSerialNumber)>=0)
						{
						/* Call the device matcher: */
						if(deviceMatcher.match(eventFileDeviceInfo.bustype,eventFileDeviceInfo.vendor,eventFileDeviceInfo.product,eventFileDeviceInfo.version,eventFileDeviceName,eventFileSerialNumber))
							{
							/* Use this device: */
							matchedFd=eventFileFd;
							break;
							}
						}
					}
				}
			
			/* Close the event device file and keep looking: */
			close(eventFileFd);
			}
		}
	
	/* Destroy list of event device files: */
	for(int i=0;i<numEventFiles;++i)
		free(eventFiles[i]);
	free(eventFiles);
	
	return matchedFd;
	}

void EventDevice::initFeatureMaps(void)
	{
	/* Query all feature types on the device: */
	unsigned char featureTypeBits[EV_MAX/8+1];
	memset(featureTypeBits,0,EV_MAX/8+1);
	if(ioctl(fd,EVIOCGBIT(0,sizeof(featureTypeBits)),featureTypeBits)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to query device feature types");
	
	/* Check if the device has key features: */
	if(featureTypeBits[EV_KEY/8]&(1<<(EV_KEY%8)))
		{
		/* Query key features: */
		unsigned char keyBits[KEY_MAX/8+1];
		memset(keyBits,0,KEY_MAX/8+1);
		if(ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(keyBits)),keyBits)>=0)
			{
			/* Create the key feature map: */
			keyFeatureMap=new unsigned int[KEY_MAX+1];
			for(int i=0;i<=KEY_MAX;++i)
				keyFeatureMap[i]=(keyBits[i/8]&(1<<(i%8)))!=0x0U?numKeyFeatures++:(unsigned int)(-1);
			
			/* Create the key feature code table: */
			keyFeatureCodes=new unsigned int[numKeyFeatures];
			unsigned int index=0;
			for(int i=0;i<=KEY_MAX;++i)
				if((keyBits[i/8]&(1<<(i%8)))!=0x0U)
					keyFeatureCodes[index++]=(unsigned int)(i);
			
			/* Create the key feature values table: */
			keyFeatureValues=new bool[numKeyFeatures];
			for(unsigned int i=0;i<numKeyFeatures;++i)
				keyFeatureValues[i]=false;
			unsigned char keyValueBits[KEY_MAX/8+1];
			memset(keyValueBits,0,KEY_MAX/8+1);
			if(ioctl(fd,EVIOCGKEY(sizeof(keyValueBits)),keyValueBits)>=0)
				for(int i=0;i<=KEY_MAX;++i)
					if(keyFeatureMap[i]!=(unsigned int)(-1))
						keyFeatureValues[keyFeatureMap[i]]=(keyValueBits[i/8]&(1<<(i%8)))!=0x0U;
			}
		}
	
	/* Check if the device has absolute axis features: */
	if(featureTypeBits[EV_ABS/8]&(1<<(EV_ABS%8)))
		{
		/* Query absolute axis features: */
		unsigned char absAxisBits[ABS_MAX/8+1];
		memset(absAxisBits,0,ABS_MAX/8+1);
		if(ioctl(fd,EVIOCGBIT(EV_ABS,sizeof(absAxisBits)),absAxisBits)>=0)
			{
			/* Create the absolute axis feature map: */
			absAxisFeatureMap=new unsigned int[ABS_MAX+1];
			for(int i=0;i<=ABS_MAX;++i)
				absAxisFeatureMap[i]=(absAxisBits[i/8]&(1<<(i%8)))!=0x0U?numAbsAxisFeatures++:(unsigned int)(-1);
			
			/* Create the absolute axis feature configuration and values tables: */
			absAxisFeatureConfigs=new AbsAxisConfig[numAbsAxisFeatures];
			absAxisFeatureValues=new int[numAbsAxisFeatures];
			unsigned int index=0;
			for(int i=0;i<=ABS_MAX;++i)
				if((absAxisBits[i/8]&(1<<(i%8)))!=0x0U)
					{
					AbsAxisConfig& aafc=absAxisFeatureConfigs[index];
					aafc.code=(unsigned int)(i);
					
					/* Query the current value and configuration of this axis: */
					input_absinfo absAxisConf;
					if(ioctl(fd,EVIOCGABS(i),&absAxisConf)>=0)
						{
						aafc.min=absAxisConf.minimum;
						aafc.max=absAxisConf.maximum;
						aafc.fuzz=absAxisConf.fuzz;
						aafc.flat=absAxisConf.flat;
						aafc.resolution=absAxisConf.resolution;
						absAxisFeatureValues[index]=absAxisConf.value;
						}
					else
						{
						aafc.max=aafc.min=0;
						aafc.flat=aafc.fuzz=0;
						aafc.resolution=0;
						absAxisFeatureValues[index]=0;
						}
					
					++index;
					}
			}
		}
	
	/* Check if the device has relative axis features: */
	if(featureTypeBits[EV_REL/8]&(1<<(EV_REL%8)))
		{
		/* Query relative axis features: */
		unsigned char relAxisBits[REL_MAX/8+1];
		memset(relAxisBits,0,REL_MAX/8+1);
		if(ioctl(fd,EVIOCGBIT(EV_REL,sizeof(relAxisBits)),relAxisBits)>=0)
			{
			/* Create the relative axis feature map: */
			relAxisFeatureMap=new unsigned int[REL_MAX+1];
			for(int i=0;i<=REL_MAX;++i)
				relAxisFeatureMap[i]=(relAxisBits[i/8]&(1<<(i%8)))!=0x0U?numRelAxisFeatures++:(unsigned int)(-1);
			
			/* Create the relative axis feature code table: */
			relAxisFeatureCodes=new unsigned int[numRelAxisFeatures];
			unsigned int index=0;
			for(int i=0;i<=REL_MAX;++i)
				if((relAxisBits[i/8]&(1<<(i%8)))!=0x0U)
					relAxisFeatureCodes[index++]=(unsigned int)(i);
			}
		}
	
	/* Check if the device has synchronization features: */
	if(featureTypeBits[EV_SYN/8]&(1<<(EV_SYN%8)))
		{
		/* Query synchornization features: */
		unsigned char synBits[SYN_MAX/8+1];
		memset(synBits,0,SYN_MAX/8+1);
		if(ioctl(fd,EVIOCGBIT(EV_SYN,sizeof(synBits)),synBits)>=0)
			{
			/* Check if the device supports the SYN_REPORT event: */
			synReport=(synBits[SYN_REPORT/8]&(1<<(SYN_REPORT%8)))!=0x0U;
			}
		}
	}

void EventDevice::ioEventCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Process pending events: */
	static_cast<EventDevice*>(event.getUserData())->processEvents();
	}

std::vector<std::string> EventDevice::getEventDeviceFileNames(void)
	{
	std::vector<std::string> result;
	
	/* Create list of all available /dev/input/eventX event device files, in numerical order: */
	struct dirent** eventFiles=0;
	int numEventFiles=scandir(RAWHID_EVENTDEVICEFILEDIR,&eventFiles,isEventFile,versionsort);
	result.reserve(numEventFiles);
	for(int eventFileIndex=0;eventFileIndex<numEventFiles;++eventFileIndex)
		{
		/* Create the fully-qualified event device file name: */
		char eventFileName[288];
		snprintf(eventFileName,sizeof(eventFileName),"%s/%s",RAWHID_EVENTDEVICEFILEDIR,eventFiles[eventFileIndex]->d_name);
		
		/* Add the fully-qualified name to the result list: */
		result.push_back(eventFileName);
		}
	
	/* Destroy list of event device files: */
	for(int i=0;i<numEventFiles;++i)
		free(eventFiles[i]);
	free(eventFiles);
	
	return result;
	}

EventDevice::EventDevice(const char* deviceFileName)
	:fd(-1),
	 numKeyFeatures(0),keyFeatureMap(0),keyFeatureCodes(0),keyFeatureValues(0),
	 numAbsAxisFeatures(0),absAxisFeatureMap(0),absAxisFeatureConfigs(0),absAxisFeatureValues(0),
	 numRelAxisFeatures(0),relAxisFeatureMap(0),relAxisFeatureCodes(0),synReport(false),
	 eventDispatcher(0),listenerKey(0)
	{
	/* Try opening the device file of the given name: */
	fd=open(deviceFileName,O_RDWR);
	if(fd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot open event device file %s",deviceFileName);
	
	initFeatureMaps();
	}

EventDevice::EventDevice(EventDeviceMatcher& deviceMatcher)
	:fd(findDevice(deviceMatcher)),
	 numKeyFeatures(0),keyFeatureMap(0),keyFeatureCodes(0),keyFeatureValues(0),
	 numAbsAxisFeatures(0),absAxisFeatureMap(0),absAxisFeatureConfigs(0),absAxisFeatureValues(0),
	 numRelAxisFeatures(0),relAxisFeatureMap(0),relAxisFeatureCodes(0),synReport(false),
	 eventDispatcher(0),listenerKey(0)
	{
	if(fd<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No event device matching %s found",deviceMatcher.getMatchSpec().c_str());
	
	initFeatureMaps();
	}

EventDevice::EventDevice(EventDevice&& source)
	:fd(source.fd),
	 numKeyFeatures(source.numKeyFeatures),keyFeatureMap(source.keyFeatureMap),keyFeatureCodes(source.keyFeatureCodes),keyFeatureValues(source.keyFeatureValues),
	 numAbsAxisFeatures(source.numAbsAxisFeatures),absAxisFeatureMap(source.absAxisFeatureMap),absAxisFeatureConfigs(source.absAxisFeatureConfigs),absAxisFeatureValues(source.absAxisFeatureValues),
	 numRelAxisFeatures(source.numRelAxisFeatures),relAxisFeatureMap(source.relAxisFeatureMap),relAxisFeatureCodes(source.relAxisFeatureCodes),
	 synReport(source.synReport),
	 keyFeatureEventCallbacks(std::move(source.keyFeatureEventCallbacks)),
	 absAxisFeatureEventCallbacks(std::move(source.absAxisFeatureEventCallbacks)),
	 relAxisFeatureEventCallbacks(std::move(source.relAxisFeatureEventCallbacks)),
	 synReportEventCallbacks(std::move(source.synReportEventCallbacks)),
	 eventDispatcher(0),listenerKey(0)
	{
	/* Throw an exception if the source is currently registered with an event dispatcher: */
	if(source.eventDispatcher!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to move-copy an event device currently registered with an event dispatcher");
	
	/* Invalidate the source: */
	source.fd=-1;
	source.numKeyFeatures=0;
	source.keyFeatureMap=0;
	source.keyFeatureCodes=0;
	source.keyFeatureValues=0;
	source.numAbsAxisFeatures=0;
	source.absAxisFeatureMap=0;
	source.absAxisFeatureConfigs=0;
	source.absAxisFeatureValues=0;
	source.numRelAxisFeatures=0;
	source.relAxisFeatureMap=0;
	source.relAxisFeatureCodes=0;
	}

EventDevice::~EventDevice(void)
	{
	/* Unregister the event device from any event dispatchers: */
	if(eventDispatcher!=0)
		eventDispatcher->removeIOEventListener(listenerKey);
	
	/* Release allocated resources: */
	delete[] keyFeatureMap;
	delete[] keyFeatureCodes;
	delete[] keyFeatureValues;
	delete[] absAxisFeatureMap;
	delete[] absAxisFeatureConfigs;
	delete[] absAxisFeatureValues;
	delete[] relAxisFeatureMap;
	delete[] relAxisFeatureCodes;
	
	/* Close the event device file: */
	if(fd>=0)
		close(fd);
	}

unsigned short EventDevice::getBusType(void) const
	{
	/* Get the device information: */
	input_id eventFileDeviceInfo;
	if(ioctl(fd,EVIOCGID,&eventFileDeviceInfo)>=0)
		return eventFileDeviceInfo.bustype;
	else
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve device information");
	}

unsigned short EventDevice::getVendorId(void) const
	{
	/* Get the device information: */
	input_id eventFileDeviceInfo;
	if(ioctl(fd,EVIOCGID,&eventFileDeviceInfo)>=0)
		return eventFileDeviceInfo.vendor;
	else
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve device information");
	}

unsigned short EventDevice::getProductId(void) const
	{
	/* Get the device information: */
	input_id eventFileDeviceInfo;
	if(ioctl(fd,EVIOCGID,&eventFileDeviceInfo)>=0)
		return eventFileDeviceInfo.product;
	else
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve device information");
	}

unsigned short EventDevice::getVersion(void) const
	{
	/* Get the device information: */
	input_id eventFileDeviceInfo;
	if(ioctl(fd,EVIOCGID,&eventFileDeviceInfo)>=0)
		return eventFileDeviceInfo.version;
	else
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve device information");
	}

std::string EventDevice::getDeviceName(void) const
	{
	/* Get the device name: */
	char eventFileDeviceName[256];
	if(ioctl(fd,EVIOCGNAME(sizeof(eventFileDeviceName)),eventFileDeviceName)>=0)
		return std::string(eventFileDeviceName);
	else
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve device name");
	}

std::string EventDevice::getSerialNumber(void) const
	{
	/* Get the device serial number: */
	char eventFileSerialNumber[256];
	if(ioctl(fd,EVIOCGUNIQ(sizeof(eventFileSerialNumber)),eventFileSerialNumber)>=0)
		return std::string(eventFileSerialNumber);
	else
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to retrieve serial number");
	}

bool EventDevice::grabDevice(void)
	{
	/* Call the EVIOCRAB ioctl: */
	return ioctl(fd,EVIOCGRAB,(void*)(1))==0;
	}

bool EventDevice::releaseDevice(void)
	{
	/* Call the EVIOCRAB ioctl: */
	return ioctl(fd,EVIOCGRAB,(void*)(0))==0;
	}

int EventDevice::addFFEffect(unsigned int direction,float strength)
	{
	/* Set up an effect structure: */
	ff_effect effect;
	memset(&effect,0,sizeof(ff_effect));
	effect.type=FF_CONSTANT;
	effect.id=-1;
	effect.direction=direction;
	effect.trigger.button=0U;
	effect.trigger.interval=0U;
	effect.replay.length=10000U;
	effect.replay.delay=0U;
	
	if(strength<-1.0f)
		effect.u.constant.level=-32767;
	else if(strength>=1.0f)
		effect.u.constant.level=32767;
	else if(strength<0.0f)
		effect.u.constant.level=(int)(strength*32767.0f-0.5f);
	else
		effect.u.constant.level=(int)(strength*32767.0f+0.5f);
	
	effect.u.constant.envelope.attack_length=0U;
	effect.u.constant.envelope.attack_level=0x7fffU;
	effect.u.constant.envelope.fade_length=0U;
	effect.u.constant.envelope.fade_level=0x7fffU;
	
	if(ioctl(fd,EVIOCSFF,&effect)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to upload force feedback effect");
	
	/* Return the assigned effect ID: */
	return effect.id;
	}

void EventDevice::updateFFEffect(int effectId,unsigned int direction,float strength)
	{
	/* Set up an effect structure: */
	ff_effect effect;
	memset(&effect,0,sizeof(ff_effect));
	effect.type=FF_CONSTANT;
	effect.id=effectId;
	effect.direction=direction;
	effect.trigger.button=0U;
	effect.trigger.interval=0U;
	effect.replay.length=10000U;
	effect.replay.delay=0U;
	
	if(strength<-1.0f)
		effect.u.constant.level=-32767;
	else if(strength>=1.0f)
		effect.u.constant.level=32767;
	else if(strength<0.0f)
		effect.u.constant.level=(int)(strength*32767.0f-0.5f);
	else
		effect.u.constant.level=(int)(strength*32767.0f+0.5f);
	
	effect.u.constant.envelope.attack_length=0U;
	effect.u.constant.envelope.attack_level=0x7fffU;
	effect.u.constant.envelope.fade_length=0U;
	effect.u.constant.envelope.fade_level=0x7fffU;
	
	if(ioctl(fd,EVIOCSFF,&effect)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to update force feedback effect");
	}

void EventDevice::removeFFEffect(int effectId)
	{
	/* Use a dummy ff_effect structure to cast the given effect ID to the correct type: */
	ff_effect effect;
	memset(&effect,0,sizeof(ff_effect));
	effect.id=effectId;
	if(ioctl(fd,EVIOCRMFF,effect.id)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to remove force feedback effect");
	}

void EventDevice::playFFEffect(int effectId,int numRepetitions)
	{
	/* Set up an event structure: */
	input_event event;
	memset(&event,0,sizeof(input_event));
	event.type=EV_FF;
	event.code=effectId;
	event.value=numRepetitions;
	
	/* Write the event structure to the device: */
	if(write(fd,&event,sizeof(input_event))!=sizeof(input_event))
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to play force feedback effect");
	}

void EventDevice::stopFFEffect(int effectId)
	{
	/* Set up an event structure: */
	input_event event;
	memset(&event,0,sizeof(input_event));
	event.type=EV_FF;
	event.code=effectId;
	event.value=0;
	if(write(fd,&event,sizeof(input_event))!=sizeof(input_event))
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to stop force feedback effect");
	}

void EventDevice::setFFGain(float gain)
	{
	/* Set up an event structure: */
	input_event event;
	memset(&event,0,sizeof(input_event));
	event.type=EV_FF;
	event.code=FF_GAIN;
	
	/* Convert the gain to the raw integer range: */
	if(gain<0.0f)
		event.value=0U;
	else if(gain>=1.0f)
		event.value=65535U;
	else
		event.value=(unsigned int)(gain*65536.0f);
	
	/* Write the event structure to the device: */
	if(write(fd,&event,sizeof(input_event))!=sizeof(input_event))
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to set force feedback gain");
	}

void EventDevice::setFFAutocenter(float strength)
	{
	/* Set up an event structure: */
	input_event event;
	memset(&event,0,sizeof(input_event));
	event.type=EV_FF;
	event.code=FF_AUTOCENTER;
	
	/* Convert the strength to the raw integer range: */
	if(strength<0.0f)
		event.value=0U;
	else if(strength>=1.0f)
		event.value=65535U;
	else
		event.value=(unsigned int)(strength*65536.0f);
	
	/* Write the event structure to the device: */
	if(write(fd,&event,sizeof(input_event))!=sizeof(input_event))
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to set force feedback autocenter strength");
	}

void EventDevice::processEvents(void)
	{
	/* Read a bunch of events at once: */
	input_event events[128];
	ssize_t numEvents=read(fd,events,sizeof(events));
	if(numEvents>=0)
		{
		numEvents/=sizeof(input_event);
		for(ssize_t i=0;i<numEvents;++i)
			{
			switch(events[i].type)
				{
				case EV_SYN:
					{
					/* Call callbacks if this is a SYN_REPORT event: */
					if(events[i].code==SYN_REPORT)
						{
						CallbackData cbData(this);
						synReportEventCallbacks.call(&cbData);
						}
					
					break;
					}
				
				case EV_KEY:
					{
					unsigned int keyIndex=keyFeatureMap[events[i].code];
					if(keyIndex!=(unsigned int)(-1))
						{
						bool newValue(events[i].value!=0);
						
						/* Call the key feature event callbacks: */
						KeyFeatureEventCallbackData cbData(this,keyIndex,newValue);
						keyFeatureEventCallbacks.call(&cbData);
						
						/* Update the key feature values table: */
						keyFeatureValues[keyIndex]=newValue;
						}
					
					break;
					}
				
				case EV_ABS:
					{
					unsigned int absAxisIndex=absAxisFeatureMap[events[i].code];
					if(absAxisIndex!=(unsigned int)(-1))
						{
						int newValue(events[i].value);
						
						/* Call the absolute axis feature event callbacks: */
						AbsAxisFeatureEventCallbackData cbData(this,absAxisIndex,newValue);
						absAxisFeatureEventCallbacks.call(&cbData);
						
						/* Update the absolute axis feature values table: */
						absAxisFeatureValues[absAxisIndex]=newValue;
						}
					
					break;
					}
				
				case EV_REL:
					{
					unsigned int relAxisIndex=relAxisFeatureMap[events[i].code];
					if(relAxisIndex!=(unsigned int)(-1))
						{
						int value(events[i].value);
						
						/* Call the relative axis feature event callbacks: */
						RelAxisFeatureEventCallbackData cbData(this,relAxisIndex,value);
						relAxisFeatureEventCallbacks.call(&cbData);
						}
					
					break;
					}
				}
			}
		}
	else
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Unable to read events");
	}

void EventDevice::registerEventHandler(Threads::EventDispatcher& newEventDispatcher)
	{
	/* Check if the event device is already registered with an event dispatcher: */
	if(eventDispatcher!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Event device is already registered with an event dispatcher");
	
	/* Add an I/O listener to the given event dispatcher: */
	eventDispatcher=&newEventDispatcher;
	listenerKey=eventDispatcher->addIOEventListener(fd,Threads::EventDispatcher::Read,ioEventCallback,this);
	}

void EventDevice::unregisterEventHandler(void)
	{
	/* Check if the event device is actually registered with an event dispatcher: */
	if(eventDispatcher==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Event device is not registered with an event dispatcher");
	
	if(eventDispatcher!=0)
		eventDispatcher->removeIOEventListener(listenerKey);
	eventDispatcher=0;
	}

}

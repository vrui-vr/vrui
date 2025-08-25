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

#ifndef RAWHID_EVENTDEVICE_INCLUDED
#define RAWHID_EVENTDEVICE_INCLUDED

#include <string>
#include <vector>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Threads/EventDispatcher.h>

/* Forward declarations: */
namespace RawHID {
class EventDeviceMatcher;
}

namespace RawHID {

class EventDevice
	{
	/* Embedded classes: */
	public:
	class Feature // Base class representing a generic event device feature
		{
		/* Elements: */
		protected:
		EventDevice* device; // Pointer to the device containing the feature
		unsigned int index; // Index of the feature on the device
		
		/* Constructors and destructors: */
		public:
		Feature(void); // Dummy constructor
		Feature(EventDevice* sDevice,unsigned int sIndex); // Represents the feature of the given index on the given device
		
		/* Methods: */
		virtual void update(void) =0; // Updates the feature from the event device's current state
		};
	
	struct KeyFeature:public Feature // Class representing a key/button event device feature
		{
		/* Elements: */
		private:
		bool value; // Current key feature value mirrored from low-level device
		
		/* Constructors and destructors: */
		public:
		KeyFeature(void); // Dummy constructor
		KeyFeature(RawHID::EventDevice* sDevice,unsigned int sIndex); // Represents the key feature of the given index on the given device
		
		/* Methods from class Feature: */
		virtual void update(void);
		
		/* New methods: */
		bool getValue(void) const // Returns the current key feature value
			{
			return value;
			}
		};
	
	struct AbsAxisConfig // Structure describing the configuration of an absolute axis
		{
		/* Elements: */
		public:
		unsigned int code; // Axis code
		int min,max; // Minimum and maximum axis value
		int fuzz;
		int flat;
		int resolution;
		};
	
	class AbsAxisFeature:public Feature // Class representing an absolute axis event device feature
		{
		/* Elements: */
		private:
		int value; // Current absolute axis feature value mirrored from low-level device
		
		/* Constructors and destructors: */
		public:
		AbsAxisFeature(void); // Dummy constructor
		AbsAxisFeature(RawHID::EventDevice* sDevice,unsigned int sIndex); // Represents the absolute axis feature of the given index on the given device
		
		/* Methods from class Feature: */
		virtual void update(void);
		
		/* New methods: */
		int getValue(void) const // Returns the current absolute axis feature value
			{
			return value;
			}
		double getNormalizedValueOneSide(void) const // Returns the current absolute axis feature value normalized to the [0, 1] interval
			{
			/* Retrieve the feature's configuration: */
			const AbsAxisConfig& config=device->getAbsAxisFeatureConfig(index);
			
			/* Normalize the current absolute axis feature value: */
			return double(value-config.min)/double(config.max-config.min);
			}
		double getNormalizedValueTwoSide(void) const // Returns the current absolute axis feature value normalized to the [-1, 1] interval including a flat region
			{
			/* Retrieve the feature's configuration: */
			const AbsAxisConfig& config=device->getAbsAxisFeatureConfig(index);
			
			/* Normalize the current absolute axis feature value: */
			int flatMin=(config.min+config.max-config.flat)/2;
			int flatMax=(config.min+config.max+config.flat)/2;
			if(value<=config.min)
				return -1.0;
			else if(value<flatMin)
				return double(value-config.min)/double(flatMin-config.min)-1.0;
			else if(value<flatMax)
				return 0.0;
			else if(value<config.max)
				return double(value-flatMax)/double(config.max-flatMax);
			else
				return 1.0;
			}
		};
	
	class CallbackData:public Misc::CallbackData // Base class for callback data structures sent by event devices
		{
		/* Elements: */
		public:
		EventDevice* device; // Pointer to the device that caused the callback
		
		/* Constructors and destructors: */
		CallbackData(EventDevice* sDevice)
			:device(sDevice)
			{
			}
		};
	
	class KeyFeatureEventCallbackData:public CallbackData // Class for key events
		{
		/* Elements: */
		public:
		unsigned int featureIndex; // Index of the key feature for which the event happened
		bool newValue; // New value of the key feature; event device's tables still contain previous value
		
		/* Constructors and destructors: */
		KeyFeatureEventCallbackData(EventDevice* sDevice,unsigned int sFeatureIndex,bool sNewValue)
			:CallbackData(sDevice),
			 featureIndex(sFeatureIndex),newValue(sNewValue)
			{
			}
		};
	
	class AbsAxisFeatureEventCallbackData:public CallbackData // Class for absolute axis events
		{
		/* Elements: */
		public:
		unsigned int featureIndex; // Index of the absolute axis feature for which the event happened
		int newValue; // New value of the absolute axis feature; event device's tables still contain previous value
		
		/* Constructors and destructors: */
		AbsAxisFeatureEventCallbackData(EventDevice* sDevice,unsigned int sFeatureIndex,int sNewValue)
			:CallbackData(sDevice),
			 featureIndex(sFeatureIndex),newValue(sNewValue)
			{
			}
		};
	
	class RelAxisFeatureEventCallbackData:public CallbackData // Class for relative axis events
		{
		/* Elements: */
		public:
		unsigned int featureIndex; // Index of the relative axis feature for which the event happened
		int value; // Reported value of the relative axis feature
		
		/* Constructors and destructors: */
		RelAxisFeatureEventCallbackData(EventDevice* sDevice,unsigned int sFeatureIndex,int sValue)
			:CallbackData(sDevice),
			 featureIndex(sFeatureIndex),value(sValue)
			{
			}
		};
	
	/* Elements: */
	private:
	int fd; // The event device file's file descriptor
	unsigned int numKeyFeatures; // Number of key features on the input device
	unsigned int* keyFeatureMap; // Table mapping key feature codes to key feature indices
	unsigned int* keyFeatureCodes; // Table mapping key feature indices to key feature codes
	bool* keyFeatureValues; // Table of current key statuses
	unsigned int numAbsAxisFeatures; // Number of absolute axis features on the input device
	unsigned int* absAxisFeatureMap; // Table mapping absolute axis feature codes to absolute axis feature indices
	AbsAxisConfig* absAxisFeatureConfigs; // Table of absolute axis configurations
	int* absAxisFeatureValues; // Table of current absolute axis values
	unsigned int numRelAxisFeatures; // Number of relative axis features on the input device
	unsigned int* relAxisFeatureMap; // Table mapping relative axis feature codes to relative axis feature indices
	unsigned int* relAxisFeatureCodes; // Table mapping relative axis feature indices to relative axis feature codes
	bool synReport; // Flag if the device supports the SYN_REPORT synchronization feature to bundle feature updates
	Misc::CallbackList keyFeatureEventCallbacks;
	Misc::CallbackList absAxisFeatureEventCallbacks;
	Misc::CallbackList relAxisFeatureEventCallbacks;
	Misc::CallbackList synReportEventCallbacks;
	Threads::EventDispatcher* eventDispatcher; // Pointer to event dispatcher with which this event device is registered, or null
	Threads::EventDispatcher::ListenerKey listenerKey; // Listener key with which this event device is registered
	
	/* Private methods: */
	static int findDevice(EventDeviceMatcher& deviceMatcher); // Returns a file descriptor for the event device file matching the given device matcher
	void initFeatureMaps(void); // Initializes the device's feature maps
	static void ioEventCallback(Threads::EventDispatcher::IOEvent& event); // Callback when there is data pending on the device's file
	
	/* Constructors and destructors: */
	public:
	static std::vector<std::string> getEventDeviceFileNames(void); // Returns a list containing the device file names of all event devices 
	EventDevice(const char* deviceFileName); // Opens the event device associated with the given event device file name
	EventDevice(EventDeviceMatcher& deviceMatcher); // Opens the first device that matches the given device matcher
	private:
	EventDevice(const EventDevice& source); // Prohibit copy constructor
	EventDevice& operator=(const EventDevice& source); // Prohibit assignment operator
	public:
	EventDevice(EventDevice&& source); // Move copy constructor
	~EventDevice(void); // Closes the device
	
	/* Methods: */
	int getFd(void) const // Returns the event file descriptor
		{
		return fd;
		}
	unsigned short getBusType(void) const;
	unsigned short getVendorId(void) const;
	unsigned short getProductId(void) const;
	unsigned short getVersion(void) const;
	std::string getDeviceName(void) const;
	std::string getSerialNumber(void) const;
	bool grabDevice(void); // Attempts to "grab" the device such that events are only sent to the caller; returns true if successful
	bool releaseDevice(void); // Releases a previously established device "grab"; returns true if successful
	unsigned int getNumKeyFeatures(void) const
		{
		return numKeyFeatures;
		}
	unsigned int getKeyFeatureCode(unsigned int keyFeatureIndex) const
		{
		return keyFeatureCodes[keyFeatureIndex];
		}
	bool getKeyFeatureValue(unsigned int keyFeatureIndex) const
		{
		return keyFeatureValues[keyFeatureIndex];
		}
	unsigned int getNumAbsAxisFeatures(void) const
		{
		return numAbsAxisFeatures;
		}
	const AbsAxisConfig& getAbsAxisFeatureConfig(unsigned int absAxisFeatureIndex) const
		{
		return absAxisFeatureConfigs[absAxisFeatureIndex];
		}
	int getAbsAxisFeatureValue(unsigned int absAxisFeatureIndex) const
		{
		return absAxisFeatureValues[absAxisFeatureIndex];
		}
	unsigned int getNumRelAxisFeatures(void) const
		{
		return numRelAxisFeatures;
		}
	unsigned int getRelAxisFeatureCode(unsigned int relAxisFeatureIndex) const
		{
		return relAxisFeatureCodes[relAxisFeatureIndex];
		}
	bool hasSynReport(void) const
		{
		return synReport;
		}
	Misc::CallbackList& getKeyFeatureEventCallbacks(void)
		{
		return keyFeatureEventCallbacks;
		}
	Misc::CallbackList& getAbsAxisFeatureEventCallbacks(void)
		{
		return absAxisFeatureEventCallbacks;
		}
	Misc::CallbackList& getRelAxisFeatureEventCallbacks(void)
		{
		return relAxisFeatureEventCallbacks;
		}
	Misc::CallbackList& getSynReportEventCallbacks(void)
		{
		return synReportEventCallbacks;
		}
	int addFFEffect(unsigned int direction,float strength); // Uploads a new constant force feedback effect to the device; returns the effect's per-device ID
	void updateFFEffect(int effectId,unsigned int direction,float strength); // Updates the constant force feedback effect with the given ID
	void removeFFEffect(int effectId); // Removes the force feedback effect of the given ID from the device
	void playFFEffect(int effectId,int numRepetitions); // Plays the force feedback effect of the given ID the given number of times
	void stopFFEffect(int effectId); // Stops playing the force feedback effect of the given ID
	void setFFGain(float gain); // Sets the overall gain of force feedback events on the device to the range [0, 1]
	void setFFAutocenter(float strength); // Sets the strength of the device's autocenter feature to the range [0, 1]
	void processEvents(void); // Processes a number of pending device events; blocks until at least one event has been processed
	void registerEventHandler(Threads::EventDispatcher& newEventDispatcher); // Registers the event device with the given event dispatcher
	void unregisterEventHandler(void); // Unregisters the event device from an event dispatcher with which it is currently registered
	};

}

#endif

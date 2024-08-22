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

#ifndef VRDEVICEMANAGER_INCLUDED
#define VRDEVICEMANAGER_INCLUDED

#include <string>
#include <Realtime/Time.h>
#include <Threads/Mutex.h>
#include <Vrui/Types.h>
#include <Vrui/Internal/VRDeviceState.h>
#include <Vrui/Internal/BatteryState.h>
#include <Vrui/Internal/VRBaseStation.h>

#include <VRDeviceDaemon/VRFactoryManager.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFile;
}
namespace Realtime {
class SharedMemory;
}
namespace Threads {
class EventDispatcher;
}
namespace Vrui {
class VRDeviceDescriptor;
class HMDConfiguration;
}
class VRDevice;
class VRCalibrator;

class VRDeviceManager
	{
	/* Embedded classes: */
	public:
	class DeviceFactoryManager:public VRFactoryManager<VRDevice>
		{
		/* Elements: */
		private:
		VRDeviceManager* deviceManager; // Pointer to device manager "owning" this factory manager
		
		/* Constructors and destructors: */
		public:
		DeviceFactoryManager(const std::string& sDsoPath,VRDeviceManager* sDeviceManager)
			:VRFactoryManager<VRDevice>(sDsoPath),
			 deviceManager(sDeviceManager)
			{
			};
		
		/* Methods: */
		VRDeviceManager* getDeviceManager(void) const // Returns pointer to device manager
			{
			return deviceManager;
			};
		};
	
	typedef VRFactoryManager<VRCalibrator> CalibratorFactoryManager;
	
	class VRStreamer // Abstract base class for objects receiving device state updates
		{
		/* Elements: */
		protected:
		VRDeviceManager* deviceManager; // Pointer to the device manager
		Threads::Mutex& stateMutex; // Reference to the device manager's device state mutex
		const Vrui::VRDeviceState& state; // Reference to the device manager's device state object
		Threads::Mutex& batteryStateMutex; // Reference to the device manager's battery state mutex
		const std::vector<Vrui::BatteryState>& batteryStates; // Reference to the device manager's battery state vector
		Threads::Mutex& hmdConfigurationMutex; // Reference to the device manager's HMD configuration mutex
		const std::vector<Vrui::HMDConfiguration*>& hmdConfigurations; // Reference to the device manager's HMD configuration vector
		Threads::Mutex& baseStationMutex; // Reference to the device manager's tracking base station mutex
		const std::vector<Vrui::VRBaseStation>& baseStations; // Reference to the device manager's list of tracking base stations
		
		/* Constructors and destructors: */
		public:
		VRStreamer(VRDeviceManager* sDeviceManager) // Creates a VR streamer listening to the given device manager
			:deviceManager(sDeviceManager),
			 stateMutex(deviceManager->stateMutex),state(deviceManager->state),
			 batteryStateMutex(deviceManager->batteryStateMutex),batteryStates(deviceManager->batteryStates),
			 hmdConfigurationMutex(deviceManager->hmdConfigurationMutex),hmdConfigurations(deviceManager->hmdConfigurations),
			 baseStationMutex(deviceManager->baseStationMutex),baseStations(deviceManager->baseStations)
			{
			}
		virtual ~VRStreamer(void)
			{
			}
		
		/* Methods: */
		
		/* Individual state component update notification methods; device state mutex will be locked during calls: */
		virtual void trackerUpdated(int trackerIndex) =0; // Notifies the VR streamer that a single tracker has been updated
		virtual void buttonUpdated(int buttonIndex) =0; // Notifies the VR streamer that a single button has been updated
		virtual void valuatorUpdated(int valuatorIndex) =0; // Notifies the VR streamer that a single valuator has been updated
		virtual void updateCompleted(void) =0; // Notifies the VR streamer that the device state has been updated completely
		
		/* Additional update notification methods: */
		virtual void batteryStateUpdated(unsigned int deviceIndex) =0; // Notifies the VR streamer that a battery state has been updated; battery state mutex will be locked during call
		virtual void hmdConfigurationUpdated(const Vrui::HMDConfiguration* hmdConfiguration) =0; // Notifies the VR streamer that an HMD configuration has been updated
		};
	
	friend class VRStreamer;
	
	struct Feature // Structure describing a client-controlled feature managed by a device driver module
		{
		/* Elements: */
		public:
		VRDevice* device; // Pointer to device driver module owning the feature
		int deviceFeatureIndex; // Index of the feature on the owning device driver module
		};
	
	/* Elements: */
	private:
	Threads::EventDispatcher& dispatcher; // Reference to the server's event dispatcher
	DeviceFactoryManager deviceFactories; // Factory manager to load VR device classes
	CalibratorFactoryManager calibratorFactories; // Factory manager to load VR calibrator classes
	int numDevices; // Number of managed devices
	VRDevice** devices; // Array of pointers to VR devices
	int* trackerIndexBases; // Array of base tracker indices for each VR device
	int* buttonIndexBases; // Array of base button indices for each VR device
	int* valuatorIndexBases; // Array of base valuator indices for each VR device
	int currentDeviceIndex; // Index of currently constructed device during initialization
	std::vector<std::string> trackerNames; // List of tracker names
	std::vector<std::string> buttonNames; // List of button names
	std::vector<std::string> valuatorNames; // List of valuator names
	Threads::Mutex stateMutex; // Mutex serializing access to all state elements
	Vrui::VRDeviceState state; // Current state of all managed devices
	Realtime::SharedMemory* stateMemory; // Pointer to an optional shared memory segment from which clients can directly read device states
	std::vector<Vrui::VRDeviceDescriptor*> virtualDevices; // List of virtual devices combining selected trackers, buttons, and valuators
	Threads::Mutex batteryStateMutex; // Mutex serializing access to the list of virtual device battery states
	std::vector<Vrui::BatteryState> batteryStates; // List of current battery states for all virtual devices
	Threads::Mutex hmdConfigurationMutex; // Mutex serializing access to the list of HMD configurations
	std::vector<Vrui::HMDConfiguration*> hmdConfigurations; // List of HMD configurations
	std::vector<Feature> powerFeatures; // List of device parts that can be powered off on request
	std::vector<Feature> hapticFeatures; // List of haptic feedback devices
	Threads::Mutex baseStationMutex; // Mutex serializing access to the list of tracking base stations
	std::vector<Vrui::VRBaseStation> baseStations; // List of tracking base stations
	unsigned int fullTrackerReportMask; // Bitmask containing 1-bits for all used logical tracker indices
	unsigned int trackerReportMask; // Bitmask of logical tracker indices that have reported state
	VRStreamer* streamer; // Pointer to VR streamer receiving state update notifications
	
	/* Private methods: */
	void postStateUpdate(void); // Posts an updated device state to the shared memory block
	
	/* Constructors and destructors: */
	public:
	VRDeviceManager(Threads::EventDispatcher& sDispatcher,Misc::ConfigurationFile& configFile); // Creates device manager by reading current section of configuration file
	~VRDeviceManager(void);
	
	/* Methods to communicate with device driver modules during initialization: */
	Threads::EventDispatcher& getDispatcher(void) // Returns the shared event dispatcher
		{
		return dispatcher;
		}
	int getTrackerIndexBase(void) const // Returns the tracker index base for the currently constructed device
		{
		return trackerIndexBases[currentDeviceIndex];
		}
	int getButtonIndexBase(void) const // Returns the button index base for the currently constructed device
		{
		return buttonIndexBases[currentDeviceIndex];
		}
	int getValuatorIndexBase(void) const // Returns the valuator index base for the currently constructed device
		{
		return valuatorIndexBases[currentDeviceIndex];
		}
	int addTracker(const char* name =0); // Adds a new tracker to the manager's namespace; returns tracker index
	int addButton(const char* name =0); // Adds a new button to the manager's namespace; returns button index
	int addValuator(const char* name =0); // Adds a new valuator to the manager's namespace; returns valuator index
	VRCalibrator* createCalibrator(const std::string& calibratorType,Misc::ConfigurationFile& configFile); // Loads calibrator of given type from current section in configuration file
	unsigned int addVirtualDevice(Vrui::VRDeviceDescriptor* newVirtualDevice); // Adds a virtual device; is adopted by device manager; returns new virtual device index
	Vrui::HMDConfiguration* addHmdConfiguration(void); // Adds a new HMD configuration
	int addPowerFeature(VRDevice* device,int deviceFeatureIndex); // Adds a new power feature; returns feature index
	int addHapticFeature(VRDevice* device,int deviceFeatureIndex); // Adds a new power feature; returns feature index
	
	/* Methods to communicate with device driver modules during operation: */
	static Vrui::VRDeviceState::TimeStamp getTimeStamp(void) // Returns a time stamp for the current time
		{
		/* Sample Vrui's time source: */
		Vrui::TimePoint now;
		
		/* Convert time point to a periodic time stamp with microsecond resolution: */
		return Vrui::VRDeviceState::TimeStamp(now.tv_sec*1000000L+(now.tv_nsec+500L)/1000L);
		}
	static Vrui::VRDeviceState::TimeStamp getTimeStamp(double offset) // Returns a time stamp offset from the current time by the given amount in seconds (positive is in the future)
		{
		/* Sample Vrui's time source: */
		Vrui::TimePoint now;
		
		/* Convert the offset to nanoseconds: */
		long nsecOffset=long(Math::floor(offset*1.0e9+0.5));
		
		/* Convert offset time point to a periodic time stamp with microsecond resolution: */
		return Vrui::VRDeviceState::TimeStamp(now.tv_sec*1000000L+(now.tv_nsec+nsecOffset+500L)/1000L);
		}
	void disableTracker(int trackerIndex); // Sets the given tracker's tracking state to invalid
	void setTrackerState(int trackerIndex,const Vrui::VRDeviceState::TrackerState& newTrackerState,Vrui::VRDeviceState::TimeStamp newTimeStamp); // Updates state of single tracker
	void setButtonState(int buttonIndex,Vrui::VRDeviceState::ButtonState newButtonState); // Updates state of single button
	void setValuatorState(int valuatorIndex,Vrui::VRDeviceState::ValuatorState newValuatorState); // Updates state of single valuator
	void updateState(void); // Tells device manager that the current state should be considered "complete"
	void updateBatteryState(unsigned int virtualDeviceIndex,const Vrui::BatteryState& newBatteryState); // Updates the battery state of the given virtual device with the given new state
	Threads::Mutex& getHmdConfigurationMutex(void) // Returns the mutex serializing access to the HMD configurations
		{
		return hmdConfigurationMutex;
		}
	void updateHmdConfiguration(const Vrui::HMDConfiguration* hmdConfiguration); // Tells device manager that the given HMD configuration was updated; must be called with HMD configurations locked
	Threads::Mutex& getBaseStationMutex(void) // Returns the mutex serializing access to the list of base stations
		{
		return baseStationMutex;
		}
	int addBaseStation(const std::string& serialNumber); // Adds a new tracking base station of the given serial number; returns base station index
	Vrui::VRBaseStation& getBaseStation(int index) // Returns the tracking base station of the given index
		{
		return baseStations[index];
		}
	
	/* Methods to communicate with device server: */
	int useSharedMemory(const char* sharedMemoryName); // Instructs the device manager to move its device state into a shared memory segment; returns a file handle for the shared memory segment
	int getNumVirtualDevices(void) const // Returns the number of managed virtual input devices
		{
		return int(virtualDevices.size());
		}
	const Vrui::VRDeviceDescriptor& getVirtualDevice(int deviceIndex) const // Returns the virtual input device of the given index
		{
		return *(virtualDevices[deviceIndex]);
		}
	unsigned int getNumPowerFeatures(void) const // Returns the number of power features
		{
		return powerFeatures.size();
		}
	void powerOff(unsigned int powerFeatureIndex); // Requests to power off the given power feature
	unsigned int getNumHapticFeatures(void) const // Returns the number of haptic features
		{
		return hapticFeatures.size();
		}
	void hapticTick(unsigned int hapticFeatureIndex,unsigned int duration,unsigned int frequency,unsigned int amplitude); // Requests a haptic tick of the given duration in milliseconds, frequency in Hertz, and relative amplitude in [0, 256) on the given power feature
	void setStreamer(VRStreamer* newStreamer); // Installs a new object receiving device state update notifications
	void start(void); // Starts device processing
	void stop(void); // Stops device processing
	};

#endif

/***********************************************************************
InputDeviceAdapterDeviceDaemon - Class to convert from Vrui's own
distributed device driver architecture to Vrui's internal device
representation.
Copyright (c) 2004-2023 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_INPUTDEVICEADAPTERDEVICEDAEMON_INCLUDED
#define VRUI_INTERNAL_INPUTDEVICEADAPTERDEVICEDAEMON_INCLUDED

#include <string>
#include <vector>
#include <Realtime/Time.h>
#include <Threads/Thread.h>
#include <Threads/EventDispatcherThread.h>
#include <Vrui/Types.h>
#include <Vrui/Internal/VRDeviceClient.h>
#include <Vrui/Internal/InputDeviceAdapterIndexMap.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class HMDConfiguration;
}

namespace Vrui {

class InputDeviceAdapterDeviceDaemon:public InputDeviceAdapterIndexMap
	{
	/* Elements: */
	private:
	
	/* Temporary stuff until Vrui's main loop gets fixed: */
	Threads::EventDispatcherThread dispatcher;
	
	VRDeviceClient deviceClient; // Device client delivering "raw" device state
	bool predictMotion; // Flag to enable motion prediction based on age of received tracking data and estimated frame presentation time
	TimeVector motionPredictionDelta; // Motion prediction time interval to apply to tracked devices
	std::vector<std::string> buttonNames; // Array of button names for all defined input devices
	std::vector<std::string> valuatorNames; // Array of valuator names for all defined input devices
	bool* validFlags; // Flag whether each tracked input device currently has valid tracking data
	int* batteryStateIndexMap; // Map from virtual device / battery state indices to input device indices
	unsigned int* batteryStates; // Battery charge level of each input device
	
	/* Private methods: */
	static void packetNotificationCallback(VRDeviceClient* client);
	void errorCallback(const VRDeviceClient::ProtocolError& error);
	void batteryStateUpdatedCallback(unsigned int deviceIndex);
	
	/* Protected methods from InputDeviceAdapter: */
	protected:
	virtual void createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection);
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterDeviceDaemon(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection); // Creates adapter by connecting to server and initializing Vrui input devices
	virtual ~InputDeviceAdapterDeviceDaemon(void);
	
	/* Methods from InputDeviceAdapter: */
	virtual std::string getFeatureName(const InputDeviceFeature& feature) const;
	virtual int getFeatureIndex(InputDevice* device,const char* featureName) const;
	virtual void updateInputDevices(void);
	virtual TrackerState peekTrackerState(int deviceIndex);
	virtual void hapticTick(unsigned int hapticFeatureIndex,unsigned int duration,unsigned int frequency,unsigned int amplitude);
	
	/* New methods: */
	VRDeviceClient& getDeviceClient(void) // Returns a reference to the VR device client
		{
		return deviceClient;
		}
	int findTrackerIndex(const InputDevice* device) const; // Returns the index of the tracker associated with the given input device, or -1
	const HMDConfiguration* findHmdConfiguration(const InputDevice* device) const; // Returns a pointer to an HMD configuration associated with the given input device, or 0
	};

}

#endif

/***********************************************************************
InputDeviceAdapterHID - Linux-specific version of HID input device
adapter.
Copyright (c) 2009-2025 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_LINUX_INPUTDEVICEADAPTERHID_INCLUDED
#define VRUI_INTERNAL_LINUX_INPUTDEVICEADAPTERHID_INCLUDED

#include <string>
#include <vector>
#include <Threads/Mutex.h>
#include <Threads/EventDispatcher.h>
#include <RawHID/EventDevice.h>
#include <Math/BrokenLine.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Vrui {
class InputDevice;
class HIDPositioner;
}

namespace Vrui {

class InputDeviceAdapterHID:public InputDeviceAdapter
	{
	/* Embedded classes: */
	private:
	class Device:public RawHID::EventDevice // Class describing a human interface device
		{
		/* Embedded classes: */
		public:
		typedef Math::BrokenLine<double> AxisValueMapper; // Type for axis value mappers
		
		/* Elements: */
		InputDeviceAdapterHID& adapter; // Pointer to the adapter owning this device for simpler event handling
		bool grabbed; // Flag if the HID was grabbed
		InputDevice* device; // Pointer to Vrui input device representing this HID
		
		/* Vrui input device positioning state: */
		HIDPositioner* positioner; // Object to assign a tracker state to the Vrui input device based on the HID's state
		bool positionerReady; // Flag if the device has a positioner, and it is ready to position
		
		/* State to deal with HID key features: */
		unsigned int numKeys; // Number of HID's key features that are represented as buttons on the Vrui input device
		unsigned int* keyFeatureIndices; // Array of HID key feature indices assigned to buttons on the Vrui input device
		std::vector<std::string> buttonNames; // Array of Vrui input device button names
		
		/* State to deal with HID absolute and relative axis features: */
		unsigned int numAbsAxes; // Number of HID's absolute axis features that are represented as valuators on the Vrui input device
		unsigned int* absAxisFeatureIndices; // Array of HID absolute axis feature indices assigned to valuators on the Vrui input device
		AxisValueMapper* absAxisValueMappers; // Array of value mappers for the HID's absolute axes
		unsigned int numRelAxes; // Number of HID's relative axis features that are represented as valuators on the Vrui input device
		unsigned int* relAxisFeatureMap; // Array mapping HID relative axis feature indices to valuators on the Vrui input device
		int* relAxisValues; // Array of current relative axis values
		AxisValueMapper* relAxisValueMappers; // Array of value mappers for the HID's relative axes
		std::vector<std::string> valuatorNames; // Array of Vrui input device valuator names
		
		/* Private methods: */
		void keyFeatureEventCallback(RawHID::EventDevice::KeyFeatureEventCallbackData* cbData); // Callback for HID key feature events
		void absAxisFeatureEventCallback(RawHID::EventDevice::AbsAxisFeatureEventCallbackData* cbData); // Callback for HID absolute axis feature events
		void relAxisFeatureEventCallback(RawHID::EventDevice::RelAxisFeatureEventCallbackData* cbData); // Callback for HID relative axis feature events
		void synReportEventCallback(RawHID::EventDevice::CallbackData* cbData); // Callback for synchronization report events
		
		/* Constructors and destructors: */
		Device(RawHID::EventDeviceMatcher& deviceMatcher,InputDeviceAdapterHID& sAdapter); // Creates a device matching the given device matcher for the given input device adapter
		~Device(void);
		
		/* Methods: */
		void prepareMainLoop(void); // Called right before Vrui starts its main loop
		void update(void); // Updates the Vrui input device associated with the HID
		};
	
	/* Elements: */
	private:
	std::vector<Device*> devices; // List of human interface devices
	Threads::Mutex deviceStateMutex; // Mutex protecting the device state array
	
	/* Protected methods from class InputDeviceAdapter: */
	protected:
	virtual void initializeInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection);
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterHID(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection); // Creates adapter connected to a set of human interface devices
	virtual ~InputDeviceAdapterHID(void);
	
	/* Methods from class InputDeviceAdapter: */
	virtual std::string getFeatureName(const InputDeviceFeature& feature) const;
	virtual int getFeatureIndex(InputDevice* device,const char* featureName) const;
	virtual void prepareMainLoop(void);
	virtual void updateInputDevices(void);
	};

}

#endif

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
#include <Math/BrokenLine.h>
#include <Geometry/Point.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Vrui {
class InputDevice;
}

namespace Vrui {

class InputDeviceAdapterHID:public InputDeviceAdapter
	{
	/* Embedded classes: */
	private:
	struct Device // Structure describing a human interface device
		{
		/* Embedded classes: */
		public:
		typedef Math::BrokenLine<double> AxisValueMapper; // Type for axis value mappers
		
		/* Elements: */
		InputDeviceAdapterHID* adapter; // Pointer to the adapter owning this device for simpler event handling
		int deviceFd; // HID's device file handle
		Threads::EventDispatcher::ListenerKey listenerKey; // Listener key for the HID on the shared event dispatcher
		int firstButtonIndex; // Index of HID's first button in device state array
		int numButtons; // Number of HID's buttons
		std::vector<int> keyMap; // Vector mapping key features to device button indices
		std::vector<int> buttonExclusionSets; // Indices of mutual exclusions sets to which each button belongs; -1 if not member of a set
		std::vector<int> exclusionSetPresseds; // Array of indices of which buttons activated each exclusion set; -1 if the exclusion set is inactive
		bool axisPosition; // Flag if the input device's position is defined by a subset of its absolute axes
		Point positionOrigin; // Origin of device position mapping
		std::vector<int> positionAxisMap; // Vector mapping absolute axis features to device position vectors
		std::vector<Vector> positionAxes; // Array of vectors spanning the absolute axis-based device position space
		std::vector<Scalar> positionValues; // Current values of position absolute axes
		int firstValuatorIndex; // Index of HID's first axis in device state array
		int numValuators; // Number of HID's axes translated to device valuators
		std::vector<int> absAxisMap; // Vector mapping absolute axis features to device valuator indices
		std::vector<int> relAxisMap; // Vector mapping relative axis features to device valuator indices
		std::vector<AxisValueMapper> axisValueMappers; // Vector of axis value mappers converting from raw HID axis values to [-1, 1]
		InputDevice* trackingDevice; // Pointer to Vrui input device from which this device gets its tracking data
		bool projectDevice; // Flag whether to project the Vrui input device through the UI manager
		InputDevice* device; // Pointer to Vrui input device associated with the HID
		std::vector<std::string> buttonNames; // Array of button feature names
		std::vector<std::string> valuatorNames; // Array of valuator feature names
		};
	
	/* Elements: */
	private:
	std::vector<Device> devices; // List of human interface devices
	Threads::Mutex deviceStateMutex; // Mutex protecting the device state array
	bool* buttonStates; // Button state array
	double* valuatorStates; // Valuator state array
	
	/* Protected methods from InputDeviceAdapter: */
	protected:
	virtual void createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection);
	
	/* New private methods: */
	private:
	static void ioEventCallback(Threads::EventDispatcher::IOEvent& event); // Callback called when there are events on a HID's device file
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterHID(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection); // Creates adapter connected to a set of human interface devices
	virtual ~InputDeviceAdapterHID(void);
	
	/* Methods from InputDeviceAdapter: */
	virtual std::string getFeatureName(const InputDeviceFeature& feature) const;
	virtual int getFeatureIndex(InputDevice* device,const char* featureName) const;
	virtual void updateInputDevices(void);
	};

}

#endif

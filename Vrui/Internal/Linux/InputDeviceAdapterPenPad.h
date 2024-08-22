/***********************************************************************
InputDeviceAdapterPenPad - Input device adapter for pen pads or pen
displays represented by one or more component HIDs.
Copyright (c) 2023-2024 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_LINUX_INPUTDEVICEADAPTERPENPAD_INCLUDED
#define VRUI_INTERNAL_LINUX_INPUTDEVICEADAPTERPENPAD_INCLUDED

#include <string>
#include <vector>
#include <Threads/Mutex.h>
#include <Threads/EventDispatcherThread.h>
#include <RawHID/EventDevice.h>
#include <Geometry/Point.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Vrui {
class InputDevice;
class VRScreen;
}

namespace Vrui {

class InputDeviceAdapterPenPad:public InputDeviceAdapter
	{
	/* Embedded classes: */
	private:
	typedef Geometry::Point<Scalar,2> PPoint; // Type for points in the pen pad plane
	
	/* Elements: */
	private:
	Threads::EventDispatcherThread eventDispatcher; // Dispatcher for events on the component HIDs
	std::vector<RawHID::EventDevice> devices; // List of component HIDs
	Threads::Mutex featureMutex; // Mutex serializing access to the device features
	RawHID::EventDevice::AbsAxisFeature posAxes[2]; // Absolute axis features defining the pen's position
	RawHID::EventDevice::KeyFeature hoverButton; // Button representing the pen's "hovering" state
	RawHID::EventDevice::KeyFeature touchButton; // Button representing the pen's "touching" state
	unsigned int numButtons; // Number of other buttons on the pen pad
	RawHID::EventDevice::KeyFeature* buttons; // Array of other buttons on the pen pad
	bool* buttonStates; // Array mirroring pen pad button states
	int degrees[2]; // Degrees of the calibration B-spline patch
	int numPoints[2]; // Numbers of control points of the calibration B-spline patch
	PPoint* cps; // Array of control points of the calibration B-spline patch
	PPoint* xs; // Evaluation array for x-direction B-splines
	PPoint* ys; // Evaluation array for y-direction B-spline
	std::vector<std::string> featureNames; // Names of pen device features
	VRScreen* penScreen; // Pointer to the Vrui screen representing the pen pad
	int pressedButtonIndex; // Index of the pen device button that's currently pressed, or -1 if no button is pressed
	
	/* Private methods: */
	void synReportCallback(RawHID::EventDevice::CallbackData* cbData); // Callback called when any of the component devices finish an update packet
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterPenPad(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection); // Creates adapter connected to a set of human interface devices
	virtual ~InputDeviceAdapterPenPad(void);
	
	/* Methods from class InputDeviceAdapter: */
	virtual std::string getFeatureName(const InputDeviceFeature& feature) const;
	virtual int getFeatureIndex(InputDevice* device,const char* featureName) const;
	virtual void prepareMainLoop(void);
	virtual void updateInputDevices(void);
	};

}

#endif

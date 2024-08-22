/***********************************************************************
LatencyTester - Class representing the USB latency tester Oculus shipped
with the first-generation Oculus Rift development kit when they were
still considering themselves an "open source" enterprise.
Copyright (c) 2013-2023 Oliver Kreylos

This file is part of the Vrui VR Compositing Server (VRCompositor).

The Vrui VR Compositing Server is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Compositing Server is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Compositing Server; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef LATENCYTESTER_INCLUDED
#define LATENCYTESTER_INCLUDED

#include <Misc/SizedTypes.h>
#include <Threads/EventDispatcher.h>
#include <RawHID/BusType.h>
#include <RawHID/Device.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}

class LatencyTester:public RawHID::Device
	{
	/* Embedded classes: */
	public:
	struct Color // Class for RGB colors detected by the latency tester's sensor
		{
		/* Elements: */
		public:
		Misc::UInt8 r,g,b;
		
		/* Constructors and destructors: */
		Color(void)
			{
			}
		Color(Misc::UInt8 sR,Misc::UInt8 sG,Misc::UInt8 sB)
			:r(sR),g(sG),b(sB)
			{
			}
		};
	
	typedef Misc::FunctionCall<unsigned int> SampleCallback; // Type of functions called when a new color sample arrives from the latency tester
	typedef Misc::FunctionCall<unsigned int> ButtonEventCallback; // Type of functions called when the latency tester's button is pressed; parameter is tester's internal ms timestamp
	
	/* Elements: */
	private:
	Threads::EventDispatcher& dispatcher; // Reference to event dispatcher
	Threads::EventDispatcher::ListenerKey ioListenerKey; // Key for I/O event listener registered with event dispatcher
	Misc::UInt16 nextTestId; // ID to associate test requests and their results
	SampleCallback* sampleCallback; // Callback called when a color sample exceeds the callback reporting threshold
	Color sampleCallbackThreshold; // Minimal color value to invoke the sampling callback
	ButtonEventCallback* buttonEventCallback; // Callback called when the latency tester's button is pressed
	
	/* Private methods: */
	void ioCallback(Threads::EventDispatcher::IOEvent& event); // Callback called when a report can be read from the raw HID device
	
	/* Constructors and destructors: */
	public:
	LatencyTester(int busTypeMask,unsigned int index,Threads::EventDispatcher& sDispatcher); // Connects to the Oculus latency tester of the given index on any of the included HID buses, using the given event dispatcher to communicate
	~LatencyTester(void);
	
	/* Methods: */
	void setLatencyConfiguration(bool sendSamples,const Color& threshold);
	void setLatencyCalibration(const Color& calibration);
	void startLatencyTest(const Color& target);
	void setLatencyDisplay(Misc::UInt8 mode,Misc::UInt32 value);
	void setSampleCallback(SampleCallback* newSampleCallback,const Color& sampleCallbackThreshold); // Sets the sample callback and callback threshold; takes ownership of function call object
	void setButtonEventCallback(ButtonEventCallback* newButtonEventCallback); // Sets the button event callback; takes ownership of function call object
	};

#endif

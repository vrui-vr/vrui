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

#include "LatencyTester.h"

#include <Misc/FunctionCalls.h>
#include <Misc/MessageLogger.h>
#include <RawHID/BusType.h>

#ifdef STANDALONE
#include <unistd.h>
#include <termios.h>
#endif

/******************************
Methods of class LatencyTester:
******************************/

namespace {

/****************
Helper functions:
****************/

inline Misc::UInt8 decodeUInt8(Misc::UInt8*& bufPtr)
	{
	return *(bufPtr++);
	}

inline Misc::UInt16 decodeUInt16(Misc::UInt8*& bufPtr)
	{
	Misc::UInt16 result=Misc::UInt16(bufPtr[0])|(Misc::UInt16(bufPtr[1])<<8);
	bufPtr+=2;
	return result;
	}

inline LatencyTester::Color decodeColor(Misc::UInt8*& bufPtr)
	{
	LatencyTester::Color result;
	result.r=*(bufPtr++);
	result.g=*(bufPtr++);
	result.b=*(bufPtr++);
	return result;
	}

}

void LatencyTester::ioCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Read the next raw HID report: */
	Misc::UInt8 buffer[64]; // 64 is largest message size
	size_t messageSize=readReport(buffer,sizeof(buffer));
	
	/* Process the report: */
	Misc::UInt8* bufPtr=buffer+1;
	switch(buffer[0])
		{
		case 0x01U: // Sample report
			if(messageSize==64)
				{
				/* Read the number of samples in this report and the sample timestamp: */
				unsigned int numSamples=decodeUInt8(bufPtr);
				unsigned int timeStamp=decodeUInt16(bufPtr);
				
				/* Check whether there is a registered sample callback: */
				if(sampleCallback!=0)
					{
					/* Check if any of the reported samples exceed the reporting threshold: */
					for(unsigned int i=0;i<numSamples;++i)
						{
						Color sample=decodeColor(bufPtr);
						if(sample.r>=sampleCallbackThreshold.r&&sample.g>=sampleCallbackThreshold.g&&sample.b>=sampleCallbackThreshold.b)
							{
							(*sampleCallback)(timeStamp+i);
							break;
							}
						}
					}
				}
			else
				Misc::consoleWarning("Received malformed sample report");
			break;
		
		case 0x02U: // Color_detected report
			if(messageSize==13)
				{
				/* Read the command ID, timestamp, and elapsed time: */
				unsigned int commandId=decodeUInt16(bufPtr);
				unsigned int timeStamp=decodeUInt16(bufPtr);
				unsigned int elapsed=decodeUInt16(bufPtr);
				
				/* Read the trigger and target colors: */
				Color trigger=decodeColor(bufPtr);
				Color target=decodeColor(bufPtr);
				
				/* Check if there is a sample callback: */
				if(sampleCallback!=0)
					{
					/* Call the sample callback: */
					(*sampleCallback)(timeStamp);
					}
				else
					{
					/* Write a message with the test result: */
					Misc::formattedConsoleNote("Latency test %u finished at %u after %u with trigger color (%u, %u, %u)",commandId,timeStamp,elapsed,(unsigned int)trigger.r,(unsigned int)trigger.g,(unsigned int)trigger.b);
					}
				}
			else
				Misc::consoleWarning("Received malformed color_detected report");
			break;
		
		case 0x03U: // Test_started report
			if(messageSize==8)
				{
				/* Read the command ID and timestamp: */
				unsigned int commandId=decodeUInt16(bufPtr);
				unsigned int timeStamp=decodeUInt16(bufPtr);
				
				/* Read the target color: */
				Color target=decodeColor(bufPtr);
				
				if(sampleCallback==0)
					Misc::formattedConsoleNote("Latency test %u started at %u with target color (%u, %u, %u)",commandId,timeStamp,(unsigned int)target.r,(unsigned int)target.g,(unsigned int)target.b);
				}
			else
				Misc::consoleWarning("Received malformed test_started report");
			break;
		
		case 0x04U: // Button report
			if(messageSize==5)
				{
				/* Read the command ID and timestamp: */
				unsigned int commandId=decodeUInt16(bufPtr);
				unsigned int timeStamp=decodeUInt16(bufPtr);
				
				/* Call the button event callback: */
				if(buttonEventCallback!=0)
					(*buttonEventCallback)(timeStamp);
				}
			else
				Misc::consoleWarning("Received malformed button report");
			break;
		
		default:
			Misc::consoleWarning("Received unknown device report");
		}
	}

LatencyTester::LatencyTester(int busTypeMask,unsigned int index,Threads::EventDispatcher& sDispatcher)
	:RawHID::Device(busTypeMask,0x2833U,0x0101U,index),
	 dispatcher(sDispatcher),ioListenerKey(0),
	 nextTestId(1U),
	 sampleCallback(0),buttonEventCallback(0)
	{
	/* Register an I/O callback for the raw HID device with the event dispatcher: */
	ioListenerKey=dispatcher.addIOEventListener(getFd(),Threads::EventDispatcher::Read,Threads::EventDispatcher::wrapMethod<LatencyTester,&LatencyTester::ioCallback>,this);
	}

LatencyTester::~LatencyTester(void)
	{
	/* Remove the I/O callback from the event dispatcher: */
	dispatcher.removeIOEventListener(ioListenerKey);
	
	/* Delete callbacks: */
	delete sampleCallback;
	delete buttonEventCallback;
	}

void LatencyTester::setLatencyConfiguration(bool sendSamples,const LatencyTester::Color& threshold)
	{
	/* Assemble the feature report: */
	Misc::UInt8 packet[5];
	packet[0]=0x05U;
	packet[1]=sendSamples?0x01U:0x00U;
	packet[2]=threshold.r;
	packet[3]=threshold.g;
	packet[4]=threshold.b;
	
	/* Send the feature report: */
	writeFeatureReport(packet,sizeof(packet));
	}

void LatencyTester::setLatencyCalibration(const LatencyTester::Color& calibration)
	{
	/* Assemble the feature report: */
	Misc::UInt8 packet[4];
	packet[0]=0x07U;
	packet[1]=calibration.r;
	packet[2]=calibration.g;
	packet[3]=calibration.b;
	
	/* Send the feature report: */
	writeFeatureReport(packet,sizeof(packet));
	}

void LatencyTester::startLatencyTest(const LatencyTester::Color& target)
	{
	/* Assemble the feature report: */
	Misc::UInt8 packet[6];
	packet[0]=0x08U;
	packet[1]=Misc::UInt8(nextTestId&0xffU);
	packet[2]=Misc::UInt8((nextTestId>>8)&0xffU);
	packet[3]=target.r;
	packet[4]=target.g;
	packet[5]=target.b;
	
	/* Send the feature report: */
	writeFeatureReport(packet,sizeof(packet));
	
	/* Increment the test ID: */
	++nextTestId;
	}

void LatencyTester::setLatencyDisplay(Misc::UInt8 mode,Misc::UInt32 value)
	{
	/* Assemble the feature report: */
	Misc::UInt8 packet[6];
	packet[0]=0x09U;
	packet[1]=mode;
	for(int i=0;i<4;++i)
		{
		packet[2+i]=Misc::UInt8(value&0xffU);
		value>>=8;
		}
	
	/* Send the feature report: */
	writeFeatureReport(packet,sizeof(packet));
	}

void LatencyTester::setSampleCallback(LatencyTester::SampleCallback* newSampleCallback,const LatencyTester::Color& newSampleCallbackThreshold)
	{
	/* Replace an existing sample callback: */
	delete sampleCallback;
	sampleCallback=newSampleCallback;
	
	/* Set the callback threshold: */
	sampleCallbackThreshold=newSampleCallbackThreshold;
	}

void LatencyTester::setButtonEventCallback(LatencyTester::ButtonEventCallback* newButtonEventCallback)
	{
	/* Replace an existing button event callback: */
	delete buttonEventCallback;
	buttonEventCallback=newButtonEventCallback;
	}

#ifdef STANDALONE

Threads::EventDispatcher dispatcher;

void stdinCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Read from stdin: */
	char buffer[2048];
	read(STDIN_FILENO,buffer,sizeof(buffer));
	
	/* Shut down event handling: */
	dispatcher.stop();
	}

int main(void)
	{
	/* Disable line buffering on stdin: */
	struct termios originalTerm;
	tcgetattr(STDIN_FILENO,&originalTerm);
	struct termios term=originalTerm;
	term.c_lflag&=~ICANON;
	tcsetattr(STDIN_FILENO,TCSANOW,&term);
	
	/* Listen for input on stdin: */
	Threads::EventDispatcher::ListenerKey stdinListener=dispatcher.addIOEventListener(STDIN_FILENO,Threads::EventDispatcher::Read,stdinCallback,0);
	
	/* Connect to the first Oculus latency tester on the USB bus: */
	LatencyTester latencyTester(RawHID::BUSTYPE_USB,0,dispatcher);
	
	/* Start sampling: */
	latencyTester.setLatencyConfiguration(true,LatencyTester::Color(128,128,128));
	dispatcher.dispatchEvents();
	
	/* Remove the stdin listener: */
	dispatcher.removeIOEventListener(stdinListener);
	
	/* Restore original terminal state: */
	tcsetattr(STDIN_FILENO,TCSANOW,&originalTerm);
	
	return 0;
	}

#endif

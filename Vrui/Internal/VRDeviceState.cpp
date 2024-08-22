/***********************************************************************
VRDeviceState - Class to represent the current state of a single or
multiple VR devices.
Copyright (c) 2002-2023 Oliver Kreylos

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

#include <Vrui/Internal/VRDeviceState.h>

#include <IO/File.h>

namespace Vrui {

/******************************
Methods of class VRDeviceState:
******************************/

namespace {

/****************
Helper functions:
****************/

size_t padSize(size_t size) // Pads a given size for conservative memory alignment
	{
	return ((size+sizeof(size_t)-1)/sizeof(size_t))*sizeof(size_t);
	}

}

void VRDeviceState::createState(void)
	{
	/* Calculate the size of the memory block to hold all state component arrays: */
	size_t trackerSize=padSize(numTrackers*sizeof(TrackerState));
	size_t trackerTimeStampSize=padSize(numTrackers*sizeof(TimeStamp));
	size_t trackerValidSize=padSize(numTrackers*sizeof(ValidFlag));
	size_t buttonSize=padSize(numButtons*sizeof(ButtonState));
	size_t valuatorSize=padSize(numValuators*sizeof(ValuatorState));
	stateSize=trackerSize+trackerTimeStampSize+trackerValidSize+buttonSize+valuatorSize;
	
	/* Allocate the memory block: */
	char* memPtr=static_cast<char*>(malloc(stateSize));
	
	/* Initialize the device state component arrays: */
	trackerStates=reinterpret_cast<TrackerState*>(memPtr);
	for(int i=0;i<numTrackers;++i)
		{
		new (trackerStates+i) TrackerState();
		trackerStates[i].positionOrientation=TrackerState::PositionOrientation::identity;
		trackerStates[i].linearVelocity=TrackerState::LinearVelocity::zero;
		trackerStates[i].angularVelocity=TrackerState::AngularVelocity::zero;
		}
	memPtr+=trackerSize;
	
	trackerTimeStamps=reinterpret_cast<TimeStamp*>(memPtr);
	for(int i=0;i<numTrackers;++i)
		{
		new (trackerTimeStamps+i) TimeStamp();
		trackerTimeStamps[i]=0;
		}
	memPtr+=padSize(numTrackers*sizeof(TimeStamp));
	
	trackerValids=reinterpret_cast<ValidFlag*>(memPtr);
	for(int i=0;i<numTrackers;++i)
		{
		new (trackerValids+i) ValidFlag();
		trackerValids[i]=false;
		}
	memPtr+=padSize(numTrackers*sizeof(ValidFlag));
	
	buttonStates=reinterpret_cast<ButtonState*>(memPtr);
	for(int i=0;i<numButtons;++i)
		{
		new (buttonStates+i) ButtonState();
		buttonStates[i]=false;
		}
	memPtr+=padSize(numButtons*sizeof(ButtonState));
	
	valuatorStates=reinterpret_cast<ValuatorState*>(memPtr);
	for(int i=0;i<numValuators;++i)
		{
		new (valuatorStates+i) ValuatorState();
		valuatorStates[i]=ValuatorState(0);
		}
	}

void VRDeviceState::destroyState(void)
	{
	/* Destroy all state component arrays: */
	for(int i=0;i<numTrackers;++i)
		trackerStates[i].~TrackerState();
	for(int i=0;i<numTrackers;++i)
		trackerTimeStamps[i].~TimeStamp();
	for(int i=0;i<numTrackers;++i)
		trackerValids[i].~ValidFlag();
	for(int i=0;i<numButtons;++i)
		buttonStates[i].~ButtonState();
	for(int i=0;i<numValuators;++i)
		valuatorStates[i].~ValuatorState();
	
	/* Release the allocated memory block: */
	free(trackerStates);
	}

VRDeviceState::VRDeviceState(void)
	:numTrackers(0),numButtons(0),numValuators(0),stateSize(0),
	 trackerStates(0),trackerTimeStamps(0),trackerValids(0),
	 buttonStates(0),valuatorStates(0)
	{
	}

VRDeviceState::VRDeviceState(int sNumTrackers,int sNumButtons,int sNumValuators)
	:numTrackers(sNumTrackers),numButtons(sNumButtons),numValuators(sNumValuators),stateSize(0),
	 trackerStates(0),trackerTimeStamps(0),trackerValids(0),
	 buttonStates(0),valuatorStates(0)
	{
	/* Create the device state: */
	createState();
	}

VRDeviceState::~VRDeviceState(void)
	{
	/* Destroy the device state: */
	if(stateSize!=0)
		destroyState();
	}

void VRDeviceState::setLayout(int newNumTrackers,int newNumButtons,int newNumValuators)
	{
	/* Destroy the current device state: */
	if(stateSize!=0)
		destroyState();
	
	/* Update the number of device state components: */
	numTrackers=newNumTrackers;
	numButtons=newNumButtons;
	numValuators=newNumValuators;
	
	/* Create the new device state: */
	createState();
	}

void VRDeviceState::writeLayout(IO::File& sink) const
	{
	/* Write the number of device state components: */
	sink.write<int>(numTrackers);
	sink.write<int>(numButtons);
	sink.write<int>(numValuators);
	}

void VRDeviceState::readLayout(IO::File& source)
	{
	/* Read the number of device state components: */
	int newNumTrackers=source.read<int>();
	int newNumButtons=source.read<int>();
	int newNumValuators=source.read<int>();
	
	/* Set the new layout: */
	setLayout(newNumTrackers,newNumButtons,newNumValuators);
	}

void VRDeviceState::write(IO::File& sink,bool writeTimeStamps,bool writeValids) const
	{
	Misc::FixedArrayMarshaller<TrackerState>::write(trackerStates,numTrackers,sink);
	if(writeTimeStamps)
		sink.write(trackerTimeStamps,numTrackers);
	if(writeValids)
		Misc::FixedArrayMarshaller<Misc::UInt8>::write(trackerValids,numTrackers,sink);
	Misc::FixedArrayMarshaller<Misc::UInt8>::write(buttonStates,numButtons,sink);
	Misc::FixedArrayMarshaller<ValuatorState>::write(valuatorStates,numValuators,sink);
	}

void VRDeviceState::read(IO::File& source,bool readTimeStamps,bool readValids) const
	{
	Misc::FixedArrayMarshaller<TrackerState>::read(trackerStates,numTrackers,source);
	if(readTimeStamps)
		source.read(trackerTimeStamps,numTrackers);
	if(readValids)
		Misc::FixedArrayMarshaller<Misc::UInt8>::read(trackerValids,numTrackers,source);
	Misc::FixedArrayMarshaller<Misc::UInt8>::read(buttonStates,numButtons,source);
	Misc::FixedArrayMarshaller<ValuatorState>::read(valuatorStates,numValuators,source);
	}

}

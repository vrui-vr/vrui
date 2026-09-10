/***********************************************************************
InputDevice - Class to represent input devices (6-DOF tracker with
associated buttons and valuators) in virtual reality environments.
Copyright (c) 2000-2026 Oliver Kreylos

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

#include <string.h>
#include <Misc/StdError.h>

#include <Vrui/InputDevice.h>

namespace Vrui {

/*************************************************
Declaration of struct InputDevice::ChangeListItem:
*************************************************/

struct InputDevice::ChangeListItem
	{
	/* Embedded classes: */
	enum ChangeTypes // Enumerated type for types of state changes
		{
		DeviceRayChanged,TrackingChanged,ButtonChanged,ValuatorChanged,
		Invalid // Type to invalidate events that have already been handled
		};
	
	/* Elements: */
	public:
	int changeType; // Type of this change, i.e., the affected state component
	union
		{
		struct
			{
			int index; // Index of the changed button
			bool newState; // The new button state for a button change
			} button;
		struct
			{
			int index; // Index of the changed valuator
			double newValue; // The new valuator value for a valuator change
			} valuator;
		};
	
	/* Constructors and destructors: */
	ChangeListItem(int sChangeType) // Constructor for device ray or tracking changes
		:changeType(sChangeType)
		{
		}
	ChangeListItem(int sButtonIndex,bool sNewButtonState) // Constructor for button state changes
		:changeType(ButtonChanged)
		{
		button.index=sButtonIndex;
		button.newState=sNewButtonState;
		}
	ChangeListItem(int sValuatorIndex,double sNewValuatorValue) // Constructor for valuator state changes
		:changeType(ValuatorChanged)
		{
		valuator.index=sValuatorIndex;
		valuator.newValue=sNewValuatorValue;
		}
	};

/****************************
Methods of class InputDevice:
****************************/

InputDevice::InputDevice(void)
	:deviceName(new char[1]),trackType(TRACK_NONE),
	 numButtons(0),numValuators(0),
	 buttonCallbacks(0),valuatorCallbacks(0),
	 deviceRayDirection(0,1,0),deviceRayStart(0),
	 transformation(TrackerState::identity),linearVelocity(Vector::zero),angularVelocity(Vector::zero),
	 buttonStates(0),valuatorValues(0),
	 callbacksEnabled(true)
	{
	deviceName[0]='\0';
	}

InputDevice::InputDevice(const char* sDeviceName,int sTrackType,int sNumButtons,int sNumValuators)
	:deviceName(new char[strlen(sDeviceName)+1]),trackType(sTrackType),
	 numButtons(sNumButtons),numValuators(sNumValuators),
	 buttonCallbacks(numButtons>0?new Misc::CallbackList[numButtons]:0),
	 valuatorCallbacks(numValuators>0?new Misc::CallbackList[numValuators]:0),
	 deviceRayDirection(0,1,0),deviceRayStart(0),
	 transformation(TrackerState::identity),linearVelocity(Vector::zero),angularVelocity(Vector::zero),
	 buttonStates(numButtons>0?new bool[numButtons]:0),
	 valuatorValues(numValuators>0?new double[numValuators]:0),
	 callbacksEnabled(true)
	{
	/* Copy device name: */
	strcpy(deviceName,sDeviceName);
	
	/* Initialize button and valuator states: */
	for(int i=0;i<numButtons;++i)
		buttonStates[i]=false;
	for(int i=0;i<numValuators;++i)
		valuatorValues[i]=0.0;
	}

InputDevice::InputDevice(const InputDevice& source)
	:deviceName(new char[1]),trackType(TRACK_NONE),
	 numButtons(0),numValuators(0),
	 buttonCallbacks(0),valuatorCallbacks(0),
	 deviceRayDirection(0,1,0),deviceRayStart(0),
	 transformation(TrackerState::identity),linearVelocity(Vector::zero),angularVelocity(Vector::zero),
	 buttonStates(0),valuatorValues(0),
	 callbacksEnabled(true)
	{
	deviceName[0]='\0';
	
	/*********************************************************************
	Since we don't actually copy the source data here, throw an exception
	if somebody attempts to copy an already initialized input device.
	That'll teach them.
	*********************************************************************/
	
	if(source.deviceName[0]!='\0'||source.numButtons!=0||source.numValuators!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot copy initialized input device");
	}

InputDevice::~InputDevice(void)
	{
	delete[] deviceName;
	
	/* Delete state arrays: */
	delete[] buttonCallbacks;
	delete[] valuatorCallbacks;
	delete[] buttonStates;
	delete[] valuatorValues;
	}

InputDevice& InputDevice::set(const char* sDeviceName,int sTrackType,int sNumButtons,int sNumValuators)
	{
	delete[] deviceName;
	
	/* Delete old state arrays: */
	delete[] buttonCallbacks;
	delete[] valuatorCallbacks;
	delete[] buttonStates;
	delete[] valuatorValues;
	
	/* Set new device layout: */
	deviceName=new char[strlen(sDeviceName)+1];
	strcpy(deviceName,sDeviceName);
	trackType=sTrackType;
	numButtons=sNumButtons;
	numValuators=sNumValuators;
	
	/* Allocate new state arrays: */
	buttonCallbacks=numButtons!=0?new Misc::CallbackList[numButtons]:0;
	valuatorCallbacks=numValuators>0?new Misc::CallbackList[numValuators]:0;
	buttonStates=numButtons!=0?new bool[numButtons]:0;
	valuatorValues=numValuators>0?new double[numValuators]:0;
	
	/* Clear all button and valuator states: */
	for(int i=0;i<numButtons;++i)
		buttonStates[i]=false;
	for(int i=0;i<numValuators;++i)
		valuatorValues[i]=0.0;
	
	return *this;
	}

void InputDevice::setTrackType(int newTrackType)
	{
	/* Set the tracking type: */
	trackType=newTrackType;
	}

void InputDevice::setDeviceRay(const Vector& newDeviceRayDirection,Scalar newDeviceRayStart)
	{
	/* Set ray direction and starting parameter: */
	deviceRayDirection=newDeviceRayDirection;
	deviceRayStart=newDeviceRayStart;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all device ray callbacks: */
		CallbackData cbData(this);
		deviceRayCallbacks.call(&cbData);
		}
	else
		{
		/* Keep track of a device ray change: */
		changes.push_back(ChangeListItem(ChangeListItem::DeviceRayChanged));
		}
	}

void InputDevice::setTransformation(const TrackerState& newTransformation)
	{
	/* Set transformation: */
	transformation=newTransformation;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		{
		/* Keep track of a tracking change: */
		changes.push_back(ChangeListItem(ChangeListItem::TrackingChanged));
		}
	}

void InputDevice::setLinearVelocity(const Vector& newLinearVelocity)
	{
	/* Set the linear velocity: */
	linearVelocity=newLinearVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		{
		/* Keep track of a tracking change: */
		changes.push_back(ChangeListItem(ChangeListItem::TrackingChanged));
		}
	}

void InputDevice::setAngularVelocity(const Vector& newAngularVelocity)
	{
	/* Set the angular velocity: */
	angularVelocity=newAngularVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		{
		/* Keep track of a tracking change: */
		changes.push_back(ChangeListItem(ChangeListItem::TrackingChanged));
		}
	}

void InputDevice::setTrackingState(const TrackerState& newTransformation,const Vector& newLinearVelocity,const Vector& newAngularVelocity)
	{
	/* Update tracking state: */
	transformation=newTransformation;
	linearVelocity=newLinearVelocity;
	angularVelocity=newAngularVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		{
		/* Keep track of a tracking change: */
		changes.push_back(ChangeListItem(ChangeListItem::TrackingChanged));
		}
	}

void InputDevice::copyTrackingState(const InputDevice* source)
	{
	/* Copy device ray state: */
	deviceRayDirection=source->deviceRayDirection;
	deviceRayStart=source->deviceRayStart;
	
	/* Copy tracking state: */
	transformation=source->transformation;
	linearVelocity=source->linearVelocity;
	angularVelocity=source->angularVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all device ray and tracking callbacks: */
		CallbackData cbData(this);
		deviceRayCallbacks.call(&cbData);
		trackingCallbacks.call(&cbData);
		}
	else
		{
		/* Keep track of a tracking change: */
		changes.push_back(ChangeListItem(ChangeListItem::DeviceRayChanged));
		changes.push_back(ChangeListItem(ChangeListItem::TrackingChanged));
		}
	}

void InputDevice::clearButtonStates(void)
	{
	if(callbacksEnabled)
		{
		/* Call callbacks for and set the states of all currently pressed buttons: */
		for(int i=0;i<numButtons;++i)
			if(buttonStates[i])
				{
				/* Call the button's callbacks: */
				ButtonCallbackData cbData(this,i,false);
				buttonCallbacks[i].call(&cbData);
				
				/* Clear the button's state: */
				buttonStates[i]=false;
				}
		}
	else
		{
		/*******************************************************************
		Unfortunately, we have to queue a potential change for all buttons,
		because we don't know current states.
		Fortunately, this method is never called. :)
		*******************************************************************/
		
		for(int i=0;i<numButtons;++i)
			changes.push_back(ChangeListItem(i,false));
		}
	}

void InputDevice::setButtonState(int index,bool newButtonState)
	{
	if(callbacksEnabled)
		{
		/* Call callbacks for and set the state of the button if it actually changed: */
		if(buttonStates[index]!=newButtonState)
			{
			ButtonCallbackData cbData(this,index,newButtonState);
			buttonCallbacks[index].call(&cbData);
			buttonStates[index]=newButtonState;
			}
		}
	else
		{
		/* Keep track of a potential button state change: */
		changes.push_back(ChangeListItem(index,newButtonState));
		}
	}

void InputDevice::setSingleButtonPressed(int index)
	{
	if(callbacksEnabled)
		{
		/* Set the states of all buttons: */
		for(int i=0;i<numButtons;++i)
			{
			bool newState=i==index;
			if(buttonStates[i]!=newState)
				{
				/* Call the button's callbacks: */
				ButtonCallbackData cbData(this,i,newState);
				buttonCallbacks[i].call(&cbData);
				
				/* Set the button's state: */
				buttonStates[i]=newState;
				}
			}
		}
	else
		{
		/*******************************************************************
		Unfortunately, we have to queue a potential change for all buttons,
		because we don't know current states.
		Fortunately, this method is never called. :)
		*******************************************************************/
		
		for(int i=0;i<numButtons;++i)
			changes.push_back(ChangeListItem(i,i==index));
		}
	}

void InputDevice::setValuator(int index,double newValuatorValue)
	{
	if(callbacksEnabled)
		{
		/* Call callbacks for and set the value of the valuator if it actually changed: */
		if(valuatorValues[index]!=newValuatorValue)
			{
			ValuatorCallbackData cbData(this,index,newValuatorValue);
			valuatorCallbacks[index].call(&cbData);
			
			valuatorValues[index]=newValuatorValue;
			}
		}
	else
		{
		/* Keep track of a potential valuator value change: */
		changes.push_back(ChangeListItem(index,newValuatorValue));
		}
	}

void InputDevice::disableCallbacks(void)
	{
	/* Disable callbacks: */
	callbacksEnabled=false;
	}

void InputDevice::triggerFeatureCallback(int featureIndex)
	{
	/* Bail out if callbacks are currently enabled: */
	if(callbacksEnabled)
		return;
	
	/*********************************************************************
	Unfortunately, we have to go through the entire change list to find
	all changes for the requested feature.
	Fortunately, this method is never called. :)
	*********************************************************************/
	
	/* Check if the given feature is a button or a valuator: */
	if(featureIndex>=numButtons)
		{
		/* Check a valuator: */
		int valuatorIndex=featureIndex-numButtons;
		for(ChangeList::iterator clIt=changes.begin();clIt!=changes.end();++clIt)
			if(clIt->changeType==ChangeListItem::ValuatorChanged&&clIt->valuator.index==valuatorIndex&&valuatorValues[valuatorIndex]!=clIt->valuator.newValue)
				{
				ValuatorCallbackData cbData(this,valuatorIndex,clIt->valuator.newValue);
				valuatorCallbacks[valuatorIndex].call(&cbData);
				
				valuatorValues[valuatorIndex]=clIt->valuator.newValue;
				
				/* Mark the change as invalid so the callbacks won't be called again later: */
				clIt->changeType=ChangeListItem::Invalid;
				}
		}
	else
		{
		/* Check a button: */
		int buttonIndex=featureIndex;
		for(ChangeList::iterator clIt=changes.begin();clIt!=changes.end();++clIt)
			if(clIt->changeType==ChangeListItem::ButtonChanged&&clIt->button.index==buttonIndex&&buttonStates[buttonIndex]!=clIt->button.newState)
				{
				ButtonCallbackData cbData(this,buttonIndex,clIt->button.newState);
				buttonCallbacks[buttonIndex].call(&cbData);
				
				buttonStates[buttonIndex]=clIt->button.newState;
				
				/* Mark the change as invalid so the callbacks won't be called again later: */
				clIt->changeType=ChangeListItem::Invalid;
				}
		}
	}

void InputDevice::enableCallbacks(void)
	{
	/* We have to enable callbacks here so that any changes made to the device while we're processing changes don't get added to the list: */
	callbacksEnabled=true;
	
	/* Call the appropriate callbacks for every change in the change list: */
	bool deviceRayChangeCalled=false; // Flag to ensure that this type of callback gets called at most once
	bool trackingChangeCalled=false; // Flag to ensure that this type of callback gets called at most once
	for(ChangeList::iterator clIt=changes.begin();clIt!=changes.end();++clIt)
		{
		switch(clIt->changeType)
			{
			case ChangeListItem::DeviceRayChanged:
				if(!deviceRayChangeCalled)
					{
					CallbackData cbData(this);
					deviceRayCallbacks.call(&cbData);
					
					deviceRayChangeCalled=true;
					}
				
				break;
			
			case ChangeListItem::TrackingChanged:
				if(!trackingChangeCalled)
					{
					CallbackData cbData(this);
					trackingCallbacks.call(&cbData);
					
					trackingChangeCalled=true;
					}
				
				break;
			
			case ChangeListItem::ButtonChanged:
				if(buttonStates[clIt->button.index]!=clIt->button.newState)
					{
					ButtonCallbackData cbData(this,clIt->button.index,clIt->button.newState);
					buttonCallbacks[clIt->button.index].call(&cbData);
					
					buttonStates[clIt->button.index]=clIt->button.newState;
					}
				
				break;
			
			case ChangeListItem::ValuatorChanged:
				if(valuatorValues[clIt->valuator.index]!=clIt->valuator.newValue)
					{
					ValuatorCallbackData cbData(this,clIt->valuator.index,clIt->valuator.newValue);
					valuatorCallbacks[clIt->valuator.index].call(&cbData);
					
					valuatorValues[clIt->valuator.index]=clIt->valuator.newValue;
					}
				
				break;
			
			default:
				; // Nothing to do
			}
		}
	
	/* Clear the change list: */
	changes.clear();
	}

}

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

#include <Vrui/InputDevice.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <IO/File.h>

namespace Vrui {

/*************************************************
Declaration of struct InputDevice::ChangeListItem:
*************************************************/

struct InputDevice::ChangeListItem
	{
	/* Embedded classes: */
	enum ChangeTypes // Enumerated type for types of state changes
		{
		ButtonChanged,ValuatorChanged,
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
	 callbacksEnabled(true),changeMask(0x0U)
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
	 callbacksEnabled(true),changeMask(0x0U)
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
	 callbacksEnabled(true),changeMask(0x0U)
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
		/* Mark the device ray as updated: */
		changeMask|=0x1U;
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
		/* Mark the transformation as updated: */
		changeMask|=0x2U;
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
		/* Mark the linear velocity as updated: */
		changeMask|=0x4U;
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
		/* Mark the angular velocity as updated: */
		changeMask|=0x8U;
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
		/* Mark the transformation and linear and angular velocities as updated: */
		changeMask|=0xeU;
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
		/* Mark the entire tracking state as updated: */
		changeMask|=0xfU;
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
		
		/* Mark any feature as being updated and enqueue all new button states: */
		changeMask|=0x10U;
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
		changeMask|=0x10U;
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
		
		/* Mark any feature as being updated and enqueue all new button states: */
		changeMask|=0x10U;
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
		changeMask|=0x10U;
		changes.push_back(ChangeListItem(index,newValuatorValue));
		}
	}

void InputDevice::disableCallbacks(void)
	{
	/* Disable callbacks: */
	callbacksEnabled=false;
	}

void InputDevice::writeChanges(IO::File& file) const
	{
	/* Write the change mask to the file: */
	file.write(Misc::UInt8(changeMask));
	
	if((changeMask&0x1U)!=0x0U)
		{
		/* Write the new device ray and ray start to the file: */
		file.write(deviceRayDirection.getComponents(),3);
		file.write(deviceRayStart);
		}
	if((changeMask&0x2U)!=0x0U)
		{
		/* Write the new device transformation to the file: */
		file.write(transformation.getTranslation().getComponents(),3);
		file.write(transformation.getRotation().getQuaternion(),4);
		}
	if((changeMask&0x4U)!=0x0U)
		{
		/* Write the new linear velocity to the file: */
		file.write(linearVelocity.getComponents(),3);
		}
	if((changeMask&0x8U)!=0x0U)
		{
		/* Write the new angular velocity to the file: */
		file.write(angularVelocity.getComponents(),3);
		}
	if((changeMask&0x10U)!=0x0U)
		{
		/* Write the length of the feature change list to the file: */
		file.write(Misc::UInt16(changes.size()));
		
		/* Write the change list to the file: */
		for(ChangeList::const_iterator clIt=changes.begin();clIt!=changes.end();++clIt)
			{
			/* Write the change list item type to the file: */
			file.write(Misc::UInt8(clIt->changeType));
			
			switch(clIt->changeType)
				{
				case ChangeListItem::ButtonChanged:
					/* Write a button state change to the file: */
					file.write(Misc::UInt16(clIt->button.index));
					file.write(Misc::UInt8(clIt->button.newState?1:0));
					
					break;
				
				case ChangeListItem::ValuatorChanged:
					/* Write a valuator state change to the file: */
					file.write(Misc::UInt16(clIt->valuator.index));
					file.write(clIt->valuator.newValue);
					
					break;
				
				default:
					; // Nothing to do
				}
			}
		}
	}

void InputDevice::readChanges(IO::File& file)
	{
	/* Read the change mask from the file: */
	changeMask=file.read<Misc::UInt8>();
	
	if((changeMask&0x1U)!=0x0U)
		{
		/* Read the new device ray and ray start from the file: */
		file.read(deviceRayDirection.getComponents(),3);
		file.read(deviceRayStart);
		}
	if((changeMask&0x2U)!=0x0U)
		{
		/* Read the new device transformation from the file: */
		TrackerState::Vector t;
		file.read(t.getComponents(),3);
		TrackerState::Scalar q[4];
		file.read(q,4);
		transformation=TrackerState(t,TrackerState::Rotation(q));
		}
	if((changeMask&0x4U)!=0x0U)
		{
		/* Read the new linear velocity from the file: */
		file.read(linearVelocity.getComponents(),3);
		}
	if((changeMask&0x8U)!=0x0U)
		{
		/* Read the new angular velocity from the file: */
		file.read(angularVelocity.getComponents(),3);
		}
	if((changeMask&0x10U)!=0x0U)
		{
		/* Read the length of the feature change list from the file: */
		unsigned int numChanges=file.read<Misc::UInt16>();
		
		/* Read the change list from the file: */
		changes.reserve(numChanges);
		for(unsigned int i=0;i<numChanges;++i)
			{
			/* Read the change list item type from the file: */
			switch(file.read<Misc::UInt8>())
				{
				case ChangeListItem::ButtonChanged:
					{
					/* Read a button state change from the file: */
					int index=file.read<Misc::UInt16>();
					bool newState=file.read<Misc::UInt8>()!=0U;
					changes.push_back(ChangeListItem(index,newState));
					
					break;
					}
				
				case ChangeListItem::ValuatorChanged:
					{
					/* Read a valuator state change from the file: */
					int index=file.read<Misc::UInt16>();
					double newValue=file.read<double>();
					changes.push_back(ChangeListItem(index,newValue));
					
					break;
					}
				
				default:
					; // Nothing to do
				}
			}
		}
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
	
	/* Call device ray and tracking state callbacks first: */
	if((changeMask&0x1U)!=0x0U)
		{
		CallbackData cbData(this);
		deviceRayCallbacks.call(&cbData);
		}
	if((changeMask&0xeU)!=0x0U)
		{
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	if((changeMask&0x10U)!=0x0U)
		{
		/* Call the appropriate callbacks for every change in the change list: */
		for(ChangeList::iterator clIt=changes.begin();clIt!=changes.end();++clIt)
			{
			switch(clIt->changeType)
				{
				case ChangeListItem::ButtonChanged:
					/* Only call the callback if the button state actually changed: */
					if(buttonStates[clIt->button.index]!=clIt->button.newState)
						{
						ButtonCallbackData cbData(this,clIt->button.index,clIt->button.newState);
						buttonCallbacks[clIt->button.index].call(&cbData);
						
						buttonStates[clIt->button.index]=clIt->button.newState;
						}
					
					break;
				
				case ChangeListItem::ValuatorChanged:
					/* Only call the callback if the valuator state actually changed: */
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
		}
	
	/* Clear the change list: */
	changeMask=0x0U;
	changes.clear();
	}

}

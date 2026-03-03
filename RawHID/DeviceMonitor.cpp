/***********************************************************************
DeviceMonitor - Class monitoring dynamic appearances/disappearances of
raw HID devices.
Copyright (c) 2016-2026 Oliver Kreylos

This file is part of the Raw HID Support Library (RawHID).

The Raw HID Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Raw HID Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Raw HID Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <RawHID/DeviceMonitor.h>

#include <stdlib.h>
#include <string.h>
#include <Threads/FunctionCalls.h>
#include <Threads/Thread.h>
#include <RawHID/Internal/UdevContext.h>
#include <RawHID/Internal/UdevDevice.h>
#include <RawHID/Internal/UdevMonitor.h>
#include <RawHID/Internal/UdevEnumerator.h>
#include <RawHID/Internal/UdevListIterator.h>

// DEBUGGING
#include <iostream>

namespace RawHID {

/******************************
Methods of class DeviceMonitor:
******************************/

void DeviceMonitor::handleNextDeviceEvent(void)
	{
	/* Recive the next device event from the low-level udev monitor and check its validity: */
	UdevDevice device=monitor->receiveDeviceEvent();
	if(device.isValid())
		{
		/* Check if the event is an add or remove event: */
		const char* action=device.getAction();
		if(action!=0&&strcmp(action,"add")==0)
			{
			/* Get the device's parent in the HID subsystem: */
			UdevDevice hid=device.getParent("hid");
			if(hid.isValid())
				{
				/* Prepare an event callback structure: */
				AddEvent ev;
				ev.deviceMonitor=this;
				ev.deviceNode=device.getDevnode();
				
				/* Extract the HID device's bus type, vendor and product IDs, and serial number: */
				char* hidId=const_cast<char*>(hid.getPropertyValue("HID_ID")); // Necessary as strtoul doesn't take a const char* as endptr, but it's fine
				ev.busType=int(strtoul(hidId,&hidId,16));
				if(*hidId!='\0')
					ev.vendorId=(unsigned short)(strtoul(hidId+1,&hidId,16));
				else
					ev.vendorId=0U;
				if(*hidId!='\0')
					ev.productId=(unsigned short)(strtoul(hidId+1,&hidId,16));
				else
					ev.productId=0U;
				ev.serialNumber=hid.getPropertyValue("HID_UNIQ");
				
				{
				/* Lock the list of add listeners: */
				Threads::Mutex::Lock addListenersLock(addListenersMutex);
				
				/* Match the device's parameters against any registered listeners: */
				for(std::vector<AddListener>::iterator lIt=addListeners.begin();lIt!=addListeners.end();++lIt)
					if(((lIt->matchMask&BusTypeMask)==0x0||lIt->busType==ev.busType)&&
					   ((lIt->matchMask&VendorIdMask)==0x0||lIt->vendorId==ev.vendorId)&&
					   ((lIt->matchMask&ProductIdMask)==0x0||lIt->productId==ev.productId)&&
					   ((lIt->matchMask&SerialNumberMask)==0x0||lIt->serialNumber==ev.serialNumber))
						{
						/* Call the callback: */
						ev.listenerKey=lIt->listenerKey;
						(*lIt->callback)(ev);
						}
				}
				}
			}
		else if(action!=0&&strcmp(action,"remove")==0)
			{
			/* Prepare an event callback structure: */
			RemoveEvent ev;
			ev.deviceMonitor=this;
			ev.deviceNode=device.getDevnode();
				
			{
			/* Lock the list of remove listeners: */
			Threads::Mutex::Lock removeListenersLock(removeListenersMutex);
			
			/* Match the device's node file name against any registered listeners: */
			std::vector<RemoveListener>::iterator end=removeListeners.end();
			for(std::vector<RemoveListener>::iterator lIt=removeListeners.begin();lIt!=end;++lIt)
				if(lIt->deviceNode==ev.deviceNode)
					{
					/* Call the callback: */
					ev.listenerKey=lIt->listenerKey;
					(*lIt->callback)(ev);
					
					/* Remove the listener from the list, as its device node just disappeared: */
					--end;
					*lIt=std::move(*end);
					--lIt;
					}
			
			/* Now actually remove all removed listeners from the list: */
			removeListeners.erase(end,removeListeners.end());
			}
			}
		}
	}

void* DeviceMonitor::eventDispatcherThreadMethod(void)
	{
	/* Enable thread cancellation: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	// Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	while(true)
		{
		/* Wait for and handle the next device event: */
		handleNextDeviceEvent();
		}
	
	return 0;
	}

void DeviceMonitor::ioEventHandler(Threads::RunLoop::IOWatcher::Event& event)
	{
	/* Handle the next device event: */
	handleNextDeviceEvent();
	}

DeviceMonitor::DeviceMonitor(void)
	:context(0),monitor(0),enumerator(0),
	 nextAddKey(0),nextRemoveKey(0),
	 eventDispatcherThread(0)
	{
	/* Create a new udev context: */
	UdevContext context;
	
	/* Create a new udev monitor in the new context: */
	monitor=new UdevMonitor(context);
	
	/* Create a new udev enumerator for the same udev context: */
	enumerator=new UdevEnumerator(context);
	
	/* Configure the monitor and enumerator to wait for events and enumerate devices on the raw HID subsystem: */
	monitor->addSubsystemFilter("hidraw",0);
	enumerator->addMatchSubsystem("hidraw");
	
	/* Start listening for udev events on the monitor: */
	monitor->listen();
	}

DeviceMonitor::~DeviceMonitor(void)
	{
	/* Shut down the event dispatcher thread if it is running: */
	if(eventDispatcherThread!=0)
		{
		eventDispatcherThread->cancel();
		eventDispatcherThread->join();
		}
	
	/* Destroy the udev context, monitor, and enumerator: */
	delete enumerator;
	delete monitor;
	delete context;
	}

void DeviceMonitor::startHandlingEvents(void)
	{
	/* Start the event dispatcher thread if it is not already running: */
	if(eventDispatcherThread==0)
		eventDispatcherThread=new Threads::Thread(this,&DeviceMonitor::eventDispatcherThreadMethod);
	}

void DeviceMonitor::stopHandlingEvents(void)
	{
	/* Shut down the event dispatcher thread if it is running: */
	if(eventDispatcherThread!=0)
		{
		eventDispatcherThread->cancel();
		eventDispatcherThread->join();
		}
	eventDispatcherThread=0;
	}

void DeviceMonitor::watch(Threads::RunLoop& runLoop)
	{
	/* Stop handling device events in the background: */
	stopHandlingEvents();
	
	/* Start watching the low-level udev monitor are register our I/O event handler: */
	monitor->watch(runLoop,*Threads::createFunctionCall(this,&DeviceMonitor::ioEventHandler));
	}

void DeviceMonitor::unwatch(void)
	{
	/* Stop watching the low-level udev monitor: */
	monitor->unwatch();
	}

DeviceMonitor::ListenerKey DeviceMonitor::registerAddEventListener(int newMatchMask,int newBusType,unsigned short newVendorId,unsigned short newProductId,const char* newSerialNumber,DeviceMonitor::AddEventCallback& newCallback)
	{
	Threads::Mutex::Lock addListenersLock(addListenersMutex);
	
	/* Create a listener structure: */
	AddListener al;
	al.listenerKey=nextAddKey;
	++nextAddKey;
	al.matchMask=newMatchMask;
	al.busType=newBusType;
	al.vendorId=newVendorId;
	al.productId=newProductId;
	if(newSerialNumber!=0)
		al.serialNumber=newSerialNumber;
	else
		al.matchMask&=~SerialNumberMask;
	al.callback=&newCallback;
	
	/* Get the list of all currently connected raw HID devices: */
	enumerator->scanDevices();
	
	/* Iterate over all devices and check them against the new match rule: */
	UdevContext context=enumerator->getContext();
	for(UdevListIterator dIt=enumerator->getDevices();dIt.isValid();++dIt)
		{
		/* Get the udev device for the current list entry: */
		UdevDevice rawhid=context.getDeviceFromSyspath(dIt.getName());
		
		/* Get the raw HID device's HID parent: */
		UdevDevice hid=rawhid.getParent("hid");
		if(hid.isValid())
			{
			/* Prepare an event callback structure: */
			AddEvent ev;
			ev.deviceMonitor=this;
			ev.deviceNode=rawhid.getDevnode();
			
			/* Extract the HID device's bus type, vendor and product IDs, and serial number: */
			char* hidId=const_cast<char*>(hid.getPropertyValue("HID_ID")); // Necessary as strtoul doesn't take a const char* as endptr, but it's fine
			ev.busType=int(strtoul(hidId,&hidId,16));
			if(*hidId!='\0')
				ev.vendorId=(unsigned short)(strtoul(hidId+1,&hidId,16));
			else
				ev.vendorId=0U;
			if(*hidId!='\0')
				ev.productId=(unsigned short)(strtoul(hidId+1,&hidId,16));
			else
				ev.productId=0U;
			ev.serialNumber=hid.getPropertyValue("HID_UNIQ");
			
			/* Match the device's parameters against the new listener: */
			if(((al.matchMask&BusTypeMask)==0x0||al.busType==ev.busType)&&
			   ((al.matchMask&VendorIdMask)==0x0||al.vendorId==ev.vendorId)&&
			   ((al.matchMask&ProductIdMask)==0x0||al.productId==ev.productId)&&
			   ((al.matchMask&SerialNumberMask)==0x0||al.serialNumber==ev.serialNumber))
				{
				/* Call the callback: */
				ev.listenerKey=al.listenerKey;
				(*al.callback)(ev);
				}
			}
		}
	
	/* Add the listener structure to the list: */
	addListeners.push_back(al);
	
	/* Return the assigned key: */
	return al.listenerKey;
	}

void DeviceMonitor::unregisterAddEventListener(DeviceMonitor::ListenerKey listenerKey)
	{
	Threads::Mutex::Lock addListenersLock(addListenersMutex);
	
	/* Find the listener structure in the list: */
	std::vector<AddListener>::iterator lIt;
	for(lIt=addListeners.begin();lIt!=addListeners.end()&&lIt->listenerKey!=listenerKey;++lIt)
		;
	if(lIt!=addListeners.end())
		{
		/* Remove the found list entry: */
		*lIt=std::move(addListeners.back());
		addListeners.pop_back();
		}
	}

DeviceMonitor::ListenerKey DeviceMonitor::registerRemoveEventListener(const char* newDeviceNode,DeviceMonitor::RemoveEventCallback& newCallback)
	{
	Threads::Mutex::Lock removeListenersLock(removeListenersMutex);
	
	/* Create a listener structure: */
	RemoveListener rl;
	rl.listenerKey=nextRemoveKey;
	++nextRemoveKey;
	rl.deviceNode=newDeviceNode;
	rl.callback=&newCallback;
	
	/* Add the listener structure to the list: */
	removeListeners.push_back(rl);
	
	/* Return the assigned key: */
	return rl.listenerKey;
	}

void DeviceMonitor::unregisterRemoveEventListener(DeviceMonitor::ListenerKey listenerKey)
	{
	Threads::Mutex::Lock removeListenersLock(removeListenersMutex);
	
	/* Find the listener structure in the list: */
	std::vector<RemoveListener>::iterator lIt;
	for(lIt=removeListeners.begin();lIt!=removeListeners.end()&&lIt->listenerKey!=listenerKey;++lIt)
		;
	if(lIt!=removeListeners.end())
		{
		/* Remove the found list entry: */
		*lIt=std::move(removeListeners.back());
		removeListeners.pop_back();
		}
	}

}

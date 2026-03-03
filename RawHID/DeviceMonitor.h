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

#ifndef RAWHID_DEVICEMONITOR_INCLUDED
#define RAWHID_DEVICEMONITOR_INCLUDED

#include <string>
#include <vector>
#include <Misc/Autopointer.h>
#include <Threads/Mutex.h>
#include <Threads/RunLoop.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
class Thread;
}
namespace RawHID {
class UdevContext;
class UdevMonitor;
class UdevEnumerator;
}

namespace RawHID {

class DeviceMonitor
	{
	/* Embedded classes: */
	public:
	typedef unsigned int ListenerKey; // Type for keys identifying registered event listeners
	
	enum AddEventMatchMask // Mask to select matching features for device addition events
		{
		NoneMask=0x0,BusTypeMask=0x1,VendorIdMask=0x2,ProductIdMask=0x4,SerialNumberMask=0x8,
		VendorProductIdMask=0x6, // Mask to match by vendor/product ID pair
		UniqueMask=0xe, // Mask to match a uniquely-identified device on any bus
		AllMask=0xf
		};
	
	struct Event // Base class for raw HID device events
		{
		/* Elements: */
		public:
		DeviceMonitor* deviceMonitor; // Pointer to the device monitor that caused this event
		ListenerKey listenerKey; // Key identifying the event listener who received this callback
		};
	
	struct AddEvent:public Event // Structure notifying a client that a raw HID device was added to the system
		{
		/* Elements: */
		public:
		const char* deviceNode; // Name of device's I/O node in the file system
		int busType; // Type of bus to which the new device is connected
		unsigned short vendorId,productId; // Device's vendor/product ID
		const char* serialNumber; // Device's unique serial number
		};
	
	struct RemoveEvent:public Event // Structure notifying a client that a raw HID device was removed from the system
		{
		/* Elements: */
		public:
		const char* deviceNode; // Name of device's I/O node in the file system
		};
	
	typedef Threads::FunctionCall<const AddEvent&> AddEventCallback; // Type for callbacks called when a device is added to the system
	typedef Threads::FunctionCall<const RemoveEvent&> RemoveEventCallback; // Type for callbacks called when a device is removed from the system
	
	private:
	struct AddListener // Structure defining a registered listener for add events
		{
		/* Embedded classes: */
		public:
		
		/* Elements: */
		ListenerKey listenerKey; // Key identifying the registered listener
		int matchMask; // Bit mask of device features in which the listener is interested
		int busType; // Bus type in which the listener is interested
		unsigned short vendorId,productId; // Vendor and product ID of devices in which the listener is interested
		std::string serialNumber; // Serial number of a specific device in which the listener is interested
		Misc::Autopointer<AddEventCallback> callback; // Callback to be called when a matching event occurs
		};
	
	struct RemoveListener // Structure defining a registered listener for remove events
		{
		/* Elements: */
		public:
		ListenerKey listenerKey; // Key identifying the registered listener
		std::string deviceNode; // Name of the device node in whose disappearance the listener is interested
		Misc::Autopointer<RemoveEventCallback> callback; // Callback to be called when a matching event occurs
		};
	
	/* Elements: */
	private:
	UdevContext* context; // Pointer to a private udev context
	UdevMonitor* monitor; // Pointer to a low-level udev monitor dispatching device events
	UdevEnumerator* enumerator; // Pointer to a low-level udev enumerator to immediately apply new add listeners to all currently connected devices
	Threads::Mutex addListenersMutex; // Mutex protecting the list of listeners registered for device addition events
	ListenerKey nextAddKey; // Key to be assigned to the next listener who registers for device addition events
	std::vector<AddListener> addListeners; // List of listeners registered for device addition events
	Threads::Mutex removeListenersMutex; // Mutex protecting the list of listeners registered for device removal events
	ListenerKey nextRemoveKey; // Key to be assigned to the next listener who registers for device removal events
	std::vector<RemoveListener> removeListeners; // List of listeners registered for device removal events
	Threads::Thread* eventDispatcherThread; // Thread to dispatch device events from the udev monitor to listeners
	
	/* Private methods: */
	void handleNextDeviceEvent(void); // Receives and handles the next device event from the low-level udev monitor
	void* eventDispatcherThreadMethod(void); // Thread method for the event dispatcher thread
	void ioEventHandler(Threads::RunLoop::IOWatcher::Event& event); // Event handler for I/O events on the low-level udev monitor's event file descriptor
	
	/* Constructors and destructors: */
	public:
	DeviceMonitor(void); // Creates a device monitor for raw HID events
	private:
	DeviceMonitor(const DeviceMonitor& source); // Prohibit copy constructor
	DeviceMonitor& operator=(const DeviceMonitor& source); // Prohibit assignment operator
	public:
	~DeviceMonitor(void); // Destroys the device monitor
	
	/* Methods: */
	void startHandlingEvents(void); // Starts handling device monitor events from a background thread
	void stopHandlingEvents(void); // Stops handling events from a background thread
	void watch(Threads::RunLoop& runLoop); // Watches the device monitor from the given run loop
	void unwatch(void); // Stops watching the device monitor from a run loop
	ListenerKey registerAddEventListener(int newMatchMask,int newBusType,unsigned short newVendorId,unsigned short newProductId,const char* newSerialNumber,AddEventCallback& newCallback); // Registers a listener's interest in device addition events
	void unregisterAddEventListener(ListenerKey listenerKey); // Unregisters a listener's interest in device addition events
	ListenerKey registerRemoveEventListener(const char* newDeviceNode,RemoveEventCallback& newCallback); // Registers a listener's interest in device removal events
	void unregisterRemoveEventListener(ListenerKey listenerKey); // Unregisters a listener's interest in device removal events; removal event listeners will automatically be de-registered when the removal event occurs
	};

}

#endif

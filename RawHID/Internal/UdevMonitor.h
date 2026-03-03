/***********************************************************************
UdevMonitor - Class to represent a udev event monitor, to receive
notification of device plug-in/removal.
Copyright (c) 2014-2026 Oliver Kreylos

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

#ifndef RAWHID_INTERNAL_UDEVMONITOR_INCLUDED
#define RAWHID_INTERNAL_UDEVMONITOR_INCLUDED

#include <Misc/Autopointer.h>
#include <Misc/FdSet.h>
#include <Threads/RunLoop.h>

/* Forward declarations: */
struct udev_monitor;
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}
namespace RawHID {
class UdevContext;
class UdevDevice;
}

namespace RawHID {

class UdevMonitor
	{
	/* Embedded classes: */
	public:
	typedef Threads::FunctionCall<UdevDevice&> DeviceEventHandler; // Type for handlers for Udev device events
	
	/* Elements: */
	private:
	udev_monitor* monitor; // Pointer to the low-level udev monitor
	int fd; // The monitor's event file descriptor
	bool listening; // Flag if the monitor is already listening to events
	Threads::RunLoop::IOWatcherOwner ioWatcher; // I/O watcher for the monitor's event file descriptor
	Misc::Autopointer<DeviceEventHandler> deviceEventHandler; // Event handler for Udev device events
	
	/* Private methods: */
	void ioEventHandler(Threads::RunLoop::IOWatcher::Event& event); // Handler for I/O events on the monitor's event file descriptor
	
	/* Constructors and destructors: */
	public:
	UdevMonitor(void); // Creates a udev monitor from a new udev context and the "udev" netlink
	UdevMonitor(UdevContext& context); // Creates monitor from given udev context and the "udev" netlink
	private:
	UdevMonitor(const UdevMonitor& source); // Prohibit copy constructor
	UdevMonitor& operator=(const UdevMonitor& source); // Prohibit assignment operator
	public:
	~UdevMonitor(void); // Destroys the udev monitor
	
	/* Methods: */
	UdevContext getContext(void); // Returns the udev context to which this monitor belongs
	void addSubsystemFilter(const char* subsystem,const char* deviceType); // Adds notifications for the given subsystem and device type
	void addTagFilter(const char* tag); // Adds notifications for the given tag
	void removeFilters(void); // Remove all filters from the monitor
	void listen(void); // Starts listening to events on the selected subsystem(s) or tag(s)
	void watch(Threads::RunLoop& runLoop,DeviceEventHandler& newDeviceEventHandler); // Watches the monitor from the given run loop and calls the given event handler when an event occurs
	void watch(Threads::RunLoop& runLoop,Threads::RunLoop::IOWatcher::EventHandler& newIoEventHandler); // Ditto, but calls the given raw I/O event handler if a device event can be read via receiveDeviceEvent()
	void unwatch(void); // Stops watching the monitor from a run loop
	void addEvent(Misc::FdSet& fdSet) const // Adds the monitor's event socket to the given wait set
		{
		fdSet.add(fd);
		}
	void removeEvent(Misc::FdSet& fdSet) const // Removes the monitor's event socket from the given wait set
		{
		fdSet.remove(fd);
		}
	bool isTriggered(const Misc::FdSet& fdSet) const // Returns true if the monitor's event socket has an event pending in the given wait set
		{
		return fdSet.isSet(fd);
		}
	UdevDevice receiveDeviceEvent(void); // Suspends calling thread until a device event occurs and returns a device structure representing the event with action string set; returns invalid device if there are no pending events, or an error occurred
	};

}

#endif

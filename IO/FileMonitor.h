/***********************************************************************
FileMonitor - Class to monitor a set of files and/or directories and
send callbacks on any changes to any of the monitored files or
directories.
Copyright (c) 2012-2026 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IO_FILEMONITOR_INCLUDED
#define IO_FILEMONITOR_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <Misc/HashTable.h>
#include <Threads/Mutex.h>
#include <Threads/Thread.h>
#include <Threads/RunLoop.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}

namespace IO {

class FileMonitor
	{
	/* Embedded classes: */
	public:
	typedef int Cookie; // Data type for cookies identifying monitored files or directories; values are unique inside the same FileMonitor object
	typedef unsigned int EventMask; // Type for masks of events that can be watched and be returned
	
	enum EventType // Enumerated type for events; can be or-ed together for event masks
		{
		/* Events for the monitored file/directory itself, or for a file/directory contained in the monitored directory: */
		Accessed=0x1U, // File/directory was accessed
		Modified=0x2U, // File/directory was modified
		AttributesChanged=0x4U, // File/directory's attributes changed
		Opened=0x8U, // File/directory was opened
		ClosedWrite=0x10U, // File/directory was closed after being opened for writing
		ClosedNoWrite=0x20U, // File/directory was closed after not being opened for writing
		Closed=0x30U, // File/directory was closed, after being opened for writing or not
		
		/* Events for the monitored directory itself, or for a directory contained in the monitored directory: */
		Created=0x40U, // File/directory was created inside directory
		MovedFrom=0x80U, // File/directory was moved out of directory
		MovedTo=0x100U, // File/directory was moved into directory
		Moved=0x180U, // File/directory was moved out of or into directory
		Deleted=0x200U, // File/directory was deleted inside directory
		
		/* Events for the monitored file/directory itself: */
		SelfMoved=0x400U, // Monitored file/directory itself was moved
		SelfDeleted=0x800U, // Monitored file/directory itself was deleted
		AllEvents=0xfffU, // Mask for all events that can be monitored
		
		/* Events that are reported even if they were not requested: */
		Unmounted=0x1000U // The file system containing the monitored file/directory was unmounted
		};
	
	enum EventModifiers // Enumerated types for modifier flags that can be or-ed onto event masks
		{
		DontFollowLinks=0x2000U, // If monitored path is a symbolic link, monitor link itself and not file/directory to which it points
		IgnoreUnlinkedFiles=0x4000U // Don't report events for already-unlinked files/directories inside a monitored directory
		};
	
	struct Event // Structure describing an event passed to event callbacks
		{
		/* Elements: */
		public:
		FileMonitor& fileMonitor; // Reference to the file monitor that called the callback
		Cookie cookie; // Watch cookie identifying the monitored file/directory
		EventMask eventMask; // Mask of event types that occurred
		bool directory; // Subject of this event is a directory
		unsigned int moveCookie; // Cookie to relate the MovedFrom and MovedTo events caused by a rename or move
		const char* name; // For watched directories: name of file/directory inside watched directory to which event applies
		
		/* Constructors and destructors; */
		Event(FileMonitor& sFileMonitor,Cookie sCookie,uint32_t sEventMask,bool sDirectory,unsigned int sMoveCookie,const char* sName); // Elementwise constructor with raw inotify event mask
		};
	
	typedef Threads::FunctionCall<Event&> EventCallback; // Type for event callback functions
	
	private:
	struct WatchedPath; // Structure identifying a watched path
	typedef Misc::HashTable<Cookie,WatchedPath> WatchedPathMap; // Type for hash tables mapping from watch cookies to watched path structures
	
	/* Elements: */
	private:
	int fd; // Low-level file descriptor connecting to the OS' file monitoring service
	Threads::Mutex watchedPathsMutex; // Mutex serializing access to the watched path hash table
	WatchedPathMap watchedPaths; // Hash table from monitor cookies to watched path structures
	#ifndef __linux__
	Cookie nextCookie; // Dummy cookie to be returned when adding paths, to keep up the appearance of working
	#endif
	size_t eventBufferSize; // Size of the current event buffer
	char* eventBuffer; // Current event buffer
	Threads::Thread* eventHandlingThread; // Background thread receiving monitoring events from the OS and dispatching callbacks to clients
	Threads::RunLoop::IOWatcherOwner ioWatcher; // I/O watcher for the file monitor's file descriptor registered with a run loop
	
	/* Private methods: */
	void* eventHandlingThreadMethod(void); // Method for the background event handling thread
	void processEventsCallback(Threads::RunLoop::IOWatcher::Event& event); // Event handler called from a run loop
	
	/* Constructors and destructors: */
	public:
	FileMonitor(void); // Creates an empty file monitor
	private:
	FileMonitor(const FileMonitor& source); // Prohibit copy constructor
	FileMonitor& operator=(const FileMonitor& source); // Prohibit assignment operator
	public:
	~FileMonitor(void); // Releases all resources allocated by this file monitor
	
	/* Methods: */
	void startEventHandling(void); // Starts background event handling; resets the file monitor to block on empty queue
	void stopEventHandling(void); // Stops background event handling
	void watch(Threads::RunLoop& runLoop); // Watches the file monitor from the given run loop
	void unwatch(void); // Stops watching the file monitor
	void startPolling(void); // Sets the file monitor to allow polling; ignored if background event handling is active
	void stopPolling(void); // Resets the file monitor to block on empty event queue
	bool processEvents(void); // Processes all pending events that have occurred; blocks until at least one event occurred if not in polling mode; returns true if any event callbacks were called
	Cookie addPath(const char* pathName,EventMask eventMask,EventCallback& eventCallback); // Adds a file or directory to the monitor's watch list with the given event mask; given callback is called from background thread when event occurs; returns cookie unique for the given path inside this FileMonitor object
	std::string getWatchedPath(Cookie pathCookie); // Returns the path watched by the given cookie; returns an empty string if the watch for the path has been removed
	void removePath(Cookie pathCookie); // Removes a file or directory from the monitor's watch list based on a cookie previously returned by addPath
	};

}

#endif

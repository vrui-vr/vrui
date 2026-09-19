/***********************************************************************
IOWatcher - Class to watch for input/output events on operating system
file descriptors in the context of a run loop.
Copyright (c) 2016-2026 Oliver Kreylos

This file is part of the Portable Threading Library (Threads).

The Portable Threading Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Portable Threading Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Threading Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef THREADS_IOWATCHER_INCLUDED
#define THREADS_IOWATCHER_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/EventTypes.h>

namespace Threads {

class IOWatcherEvent; // Forward declaration of descriptor class passed to I/O event handlers
typedef Threads::FunctionCall<IOWatcherEvent&> IOWatcherEventHandler; // Type for I/O event handlers

class IOWatcher:public EventSource
	{
	friend class RunLoop;
	friend class IOWatcherEvent;
	
	/* Embedded classes: */
	public:
	enum EventType // Enumerated type for I/O event types
		{
		Read=0x01, // It is possible to read from the file descriptor; note: on some file types (sockets), a read call may still block unless the file is O_NONBLOCK
		Exception=0x02, // Some exception, such as arrival of priority out-of-band data, has occurred on the file descriptor
		Write=0x04, // It is possible to write to the file descriptor
		ReadWrite=0x05, // It is both possible to read from and write to the file descriptor; see note for Read
		Error=0x08, // Some error occurred on the file descriptor, such as watching the write end of a pipe when the read end has been closed
		HangUp=0x10, // The peer on the other end of a pipe or stream socket has closed its end of the channel
		Invalid=0x20, // The file descriptor is invalid because the file was closed
		ProblemMask=0x38 // Bit mask for "problem" events that get sent to signal handlers even if they weren't interested in them
		};
	
	/* Elements: */
	private:
	int fd; // File descriptor to watch for I/O events
	unsigned int eventMask; // Bit mask of I/O events in which the I/O watcher is interested
	Misc::Autopointer<IOWatcherEventHandler> eventHandler; // The event handler
	unsigned int activeIndex; // Index of an enabled I/O watcher's entry in the run loop's active I/O watcher list
	
	/* Protected methods from class Ownable: */
	virtual void disowned(void);
	
	/* New private methods: */
	void enableInternal(void); // Enables this I/O watcher in its run loop
	void enablePM(void); // Enables this I/O watcher in response to a received pipe message
	void disableInternal(bool block); // Disables this I/O watcher in its run loop; if flag is true, blocks until it has actually been disabled
	void disablePM(void); // Disables this I/O watcher in response to a received pipe message
	void setEventMaskPM(unsigned int newEventMask); // Sets this I/O watcher's event mask in response to a received pipe message
	void setEventHandlerPM(IOWatcherEventHandler* newEventHandler); // Sets this I/O watcher's event handler in response to a received pipe message
	
	/* Constructors and destructors: */
	public:
	IOWatcher(RunLoop& sRunLoop,int sFd,unsigned int sEventMask,bool sEnabled,IOWatcherEventHandler& sEventHandler); // Elementwise constructor
	
	/* Methods: */
	static int eventMaskToPollEvents(unsigned int eventMask); // Helper function to convert an event mask to an event set for the poll() system call
	static unsigned int pollEventsToEventMask(int events); // Helper function to convert an event set from the poll() system call to an event mask
	int getFd(void) const // Returns the file descriptor this I/O watcher is watching
		{
		return fd;
		}
	unsigned int getEventMask(void) const // Returns the bit mask of I/O events in which the I/O watcher is interested
		{
		return eventMask;
		}
	void enable(void) // Enables the I/O watcher
		{
		/* Delegate to the internal methods: */
		enableInternal();
		}
	void disable(void) // Disables the I/O watcher
		{
		/* Delegate to the internal methods: */
		disableInternal(false);
		}
	void setEnabled(bool newEnabled) // Sets the I/O watcher's enabled state
		{
		/* Delegate to the internal methods: */
		if(newEnabled)
			enableInternal();
		else
			disableInternal(false);
		}
	void setEventMask(unsigned int newEventMask); // Sets the bit mask of I/O events in which the I/O watcher is interested
	void setEventHandler(IOWatcherEventHandler& newEventHandler); // Sets the I/O watcher's event handler
	};

typedef OwningPointer<IOWatcher> IOWatcherOwner; // Type for ownership-establishing pointers to I/O watchers
typedef Misc::Autopointer<IOWatcher> IOWatcherPtr; // Type for non-ownership-establishing pointers to I/O watchers

class IOWatcherEvent:public Event<IOWatcher> // Class for event descriptors passed to I/O event handlers
	{
	friend class RunLoop;
	
	/* Elements: */
	private:
	unsigned int eventMask; // The bit mask of I/O events that actually happened
	
	/* Constructors and destructors: */
	IOWatcherEvent(const EventTime& sDispatchTime) // Creates an event for the given dispatch time; rest of the elements will be filled in later
		:Event<IOWatcher>(0,sDispatchTime)
		{
		}
	
	/* Methods: */
	public:
	int getFd(void) const // Returns the file descripter on which the event happened
		{
		return source->fd;
		}
	unsigned int getEventMask(void) const // Returns the bit mask of events that actually occurred
		{
		return eventMask;
		}
	bool canRead(void) const // Returns true if the file descriptor can be read from; a subsequent read call can still block on some file types unless O_NONBLOCK is set
		{
		return (eventMask&IOWatcher::Read)!=0x0U;
		}
	bool canWrite(void) const // Returns true if the file descriptor can be written to
		{
		return (eventMask&IOWatcher::Write)!=0x0U;
		}
	bool hasException(void) const // Returns true if the file descriptor has an exception
		{
		return (eventMask&IOWatcher::Exception)!=0x0U;
		}
	bool hadProblem(void) const // Returns true if there was some kind of problem with the file descriptor
		{
		return (eventMask&IOWatcher::ProblemMask)!=0x0U;
		}
	bool hadError(void) const // Returns true if there was an error, such as watching the write end of a pipe when the read end has been closed
		{
		return (eventMask&IOWatcher::Error)!=0x0U;
		}
	bool hadHangUp(void) const // Returns true if the peer on the other end of a pipe or stream socket has closed its end of the channel
		{
		return (eventMask&IOWatcher::HangUp)!=0x0U;
		}
	bool isInvalid(void) const // Returns true if the watched file descriptor became invalid because the file was closed
		{
		return (eventMask&IOWatcher::Invalid)!=0x0U;
		}
	};

}

#endif

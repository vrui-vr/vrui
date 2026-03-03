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

#include <IO/FileMonitor.h>

#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#include <Misc/Autopointer.h>
#include <Misc/StdError.h>
#include <Threads/FunctionCalls.h>

namespace IO {

/************************************
Methods of struct FileMonitor::Event:
************************************/

FileMonitor::Event::Event(FileMonitor& sFileMonitor,FileMonitor::Cookie sCookie,uint32_t sEventMask,bool sDirectory,unsigned int sMoveCookie,const char* sName)
	:fileMonitor(sFileMonitor),cookie(sCookie),eventMask(0x0U),directory(sDirectory),moveCookie(sMoveCookie),name(sName)
	{
	/* Convert the raw inotify event mask: */
	if(sEventMask&IN_ACCESS)
		eventMask|=FileMonitor::Accessed;
	if(sEventMask&IN_MODIFY)
		eventMask|=FileMonitor::Modified;
	if(sEventMask&IN_ATTRIB)
		eventMask|=FileMonitor::AttributesChanged;
	if(sEventMask&IN_OPEN)
		eventMask|=FileMonitor::Opened;
	if(sEventMask&IN_CLOSE_WRITE)
		eventMask|=FileMonitor::ClosedWrite;
	if(sEventMask&IN_CLOSE_NOWRITE)
		eventMask|=FileMonitor::ClosedNoWrite;
	if(sEventMask&IN_CREATE)
		eventMask|=FileMonitor::Created;
	if(sEventMask&IN_MOVED_FROM)
		eventMask|=FileMonitor::MovedFrom;
	if(sEventMask&IN_MOVED_TO)
		eventMask|=FileMonitor::MovedTo;
	if(sEventMask&IN_DELETE)
		eventMask|=FileMonitor::Deleted;
	if(sEventMask&IN_MOVE_SELF)
		eventMask|=FileMonitor::SelfMoved;
	if(sEventMask&IN_DELETE_SELF)
		eventMask|=FileMonitor::SelfDeleted;
	if(sEventMask&IN_UNMOUNT)
		eventMask|=FileMonitor::Unmounted;
	}

/*********************************************
Declaration of class FileMonitor::WatchedPath:
*********************************************/

struct FileMonitor::WatchedPath
	{
	/* Elements: */
	public:
	std::string path; // The watched path, as given to the addPath method
	Misc::Autopointer<EventCallback> eventCallback; // The callback to be called when an event occurs for the watched path
	
	/* Constructors and destructors: */
	WatchedPath(const char* sPath,EventCallback& sEventCallback) // Elementwise constructor
		:path(sPath),eventCallback(&sEventCallback)
		{
		}
	};

/****************************
Methods of class FileMonitor:
****************************/

void* FileMonitor::eventHandlingThreadMethod(void)
	{
	/* Enable thread cancellation: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	
	/* Process events until cancelled: */
	while(true)
		processEvents();
	
	return 0;
	}

void FileMonitor::processEventsCallback(Threads::RunLoop::IOWatcher::Event& event)
	{
	#ifdef __linux__
	
	/* Try reading into the event buffer and check for errors: */
	ssize_t readResult=read(fd,eventBuffer,eventBufferSize);
	if(readResult<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot read from event file");
	
	/* Handle all returned events: */
	char* ebPtr=eventBuffer;
	while(readResult>0)
		{
		/* Get a pointer to the next event structure in the buffer: */
		inotify_event* eventStruct=reinterpret_cast<inotify_event*>(ebPtr);
		
		/* Check if the event needs to be reported: */
		if(eventStruct->wd>=0&&(eventStruct->mask&IN_IGNORED)==0x0)
			{
			/* Create an event reporting structure for the event: */
			Event event(*this,eventStruct->wd,eventStruct->mask,(eventStruct->mask&IN_ISDIR)!=0x0,eventStruct->cookie,eventStruct->len!=0?ebPtr+sizeof(inotify_event):0);
			
			/* Take a temporary reference to the event callback registered for this watched path: */
			Misc::Autopointer<EventCallback> eventCallback;
			{
			/* Lock the watched path map: */
			Threads::Mutex::Lock watchedPathsLock(watchedPathsMutex);
			
			/* Retrieve the watched path's event callback if it's still in the map: */
			WatchedPathMap::Iterator wpIt=watchedPaths.findEntry(eventStruct->wd);
			if(!wpIt.isFinished())
				eventCallback=wpIt->getDest().eventCallback;
			}
			
			/* Call the event callback: */
			if(eventCallback!=0)
				(*eventCallback)(event);
			}
		
		/* Go to the next event in the buffer: */
		size_t eventSize=sizeof(inotify_event)+eventStruct->len;
		readResult-=eventSize;
		ebPtr+=eventSize;
		}
	
	#endif
	}

FileMonitor::FileMonitor(void)
	:fd(-1),
	 watchedPaths(17),
	 eventBufferSize(sizeof(struct inotify_event)+NAME_MAX+1),eventBuffer(new char[eventBufferSize]),
	 eventHandlingThread(0)
	{
	#ifdef __linux__
	
	/* Create the inotify instance: */
	fd=inotify_init();
	if(fd<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create inotify instance");
	
	#else
	
	/* Initialize the dummy cookie: */
	nextCookie=0;
	
	#endif
	}

FileMonitor::~FileMonitor(void)
	{
	/* Shut down the background event handling thread if one is running: */
	if(eventHandlingThread!=0)
		{
		eventHandlingThread->cancel();
		eventHandlingThread->join();
		delete eventHandlingThread;
		}
	
	/* Stop watching the file monitor from a run loop: */
	ioWatcher=0;
	
	/* Delete the event buffer: */
	delete[] eventBuffer;
	
	#ifdef __linux__
	
	/* Close the inotify instance: */
	close(fd);
	
	#endif
	}

void FileMonitor::startEventHandling(void)
	{
	/* Start event handling if it's not already running: */
	if(eventHandlingThread==0)
		{
		/* Stop watching the file monitor from a run loop: */
		ioWatcher=0;
		
		/* Disable polling: */
		stopPolling();
		
		/* Start the background event handling thread: */
		eventHandlingThread=new Threads::Thread(this,&FileMonitor::eventHandlingThreadMethod);
		}
	}

void FileMonitor::stopEventHandling(void)
	{
	/* Stop event handling if it is running: */
	if(eventHandlingThread!=0)
		{
		/* Stop the event handling thread: */
		eventHandlingThread->cancel();
		eventHandlingThread->join();
		delete eventHandlingThread;
		eventHandlingThread=0;
		}
	}

void FileMonitor::watch(Threads::RunLoop& runLoop)
	{
	/* Start watching if it's not already being watched: */
	if(ioWatcher==0)
		{
		/* Stop event handling: */
		stopEventHandling();
		
		/* Disable polling: */
		stopPolling();
		
		/* Create an I/O watcher for the inotify instance's file descriptor: */
		ioWatcher=runLoop.createIOWatcher(fd,Threads::RunLoop::IOWatcher::Read,true,*Threads::createFunctionCall(this,&FileMonitor::processEventsCallback));
		}
	}

void FileMonitor::unwatch(void)
	{
	/* Stop watching the inotify instance's file descriptor: */
	ioWatcher=0;
	}

void FileMonitor::startPolling(void)
	{
	/* Ignore if event handling is running or the file monitor is being watched from a run loop: */
	if(eventHandlingThread==0&&ioWatcher==0)
		{
		#ifdef __linux__
		
		/* Set the inotify instance's file descriptor to non-blocking: */
		int fileFlags=fcntl(fd,F_GETFL);
		if(fileFlags<0)
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot read inotify instance's flags");
		fileFlags|=O_NONBLOCK;
		if(fcntl(fd,F_SETFL,fileFlags)<0)
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot set inotify instance to non-blocking");
		
		#endif
		}
	}

void FileMonitor::stopPolling(void)
	{
	/* Ignore if event handling is running or the file monitor is being watched from a run loop: */
	if(eventHandlingThread==0&&ioWatcher==0)
		{
		#ifdef __linux__
		
		/* Set the inotify instance's file descriptor to blocking: */
		int fileFlags=fcntl(fd,F_GETFL);
		if(fileFlags<0)
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot read inotify instance's flags");
		fileFlags&=~O_NONBLOCK;
		if(fcntl(fd,F_SETFL,fileFlags)<0)
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot set inotify instance to blocking");
		
		#endif
		}
	}

bool FileMonitor::processEvents(void)
	{
	#ifdef __linux__
	
	/* Try reading into the event buffer and check for errors: */
	ssize_t readResult=read(fd,eventBuffer,eventBufferSize);
	if(readResult<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot read from event file");
	
	/* Handle all returned events: */
	bool dispatchedEvent=false;
	char* ebPtr=eventBuffer;
	while(readResult>0)
		{
		/* Get a pointer to the next event structure in the buffer: */
		inotify_event* eventStruct=reinterpret_cast<inotify_event*>(ebPtr);
		
		/* Check if the event needs to be reported: */
		if(eventStruct->wd>=0&&(eventStruct->mask&IN_IGNORED)==0x0)
			{
			/* Create an event reporting structure for the event: */
			Event event(*this,eventStruct->wd,eventStruct->mask,(eventStruct->mask&IN_ISDIR)!=0x0,eventStruct->cookie,eventStruct->len!=0?ebPtr+sizeof(inotify_event):0);
			
			/* Take a temporary reference to the event callback registered for this watched path: */
			Misc::Autopointer<EventCallback> eventCallback;
			{
			/* Lock the watched path map: */
			Threads::Mutex::Lock watchedPathsLock(watchedPathsMutex);
			
			/* Retrieve the watched path's event callback if it's still in the map: */
			WatchedPathMap::Iterator wpIt=watchedPaths.findEntry(eventStruct->wd);
			if(!wpIt.isFinished())
				eventCallback=wpIt->getDest().eventCallback;
			}
			
			/* Call the event callback: */
			if(eventCallback!=0)
				(*eventCallback)(event);
			
			/* Remember that an event was dispatched: */
			dispatchedEvent=true;
			}
		
		/* Go to the next event in the buffer: */
		size_t eventSize=sizeof(inotify_event)+eventStruct->len;
		readResult-=eventSize;
		ebPtr+=eventSize;
		}
	
	return dispatchedEvent;
	
	#else
	
	/* Pretend nothing happened: */
	return false;
	
	#endif
	}

FileMonitor::Cookie FileMonitor::addPath(const char* pathName,FileMonitor::EventMask eventMask,FileMonitor::EventCallback& eventCallback)
	{
	#ifdef __linux__
	
	/* Convert the given event mask to inotify's internal types and flags: */
	uint32_t em=0x0U;
	if(eventMask&Accessed)
		em|=IN_ACCESS;
	if(eventMask&Modified)
		em|=IN_MODIFY;
	if(eventMask&AttributesChanged)
		em|=IN_ATTRIB;
	if(eventMask&Opened)
		em|=IN_OPEN;
	if(eventMask&ClosedWrite)
		em|=IN_CLOSE_WRITE;
	if(eventMask&ClosedNoWrite)
		em|=IN_CLOSE_NOWRITE;
	if(eventMask&Created)
		em|=IN_CREATE;
	if(eventMask&MovedFrom)
		em|=IN_MOVED_FROM;
	if(eventMask&MovedTo)
		em|=IN_MOVED_TO;
	if(eventMask&Deleted)
		em|=IN_DELETE;
	if(eventMask&SelfMoved)
		em|=IN_MOVE_SELF;
	if(eventMask&SelfDeleted)
		em|=IN_DELETE_SELF;
	
	if(eventMask&DontFollowLinks)
		em|=IN_DONT_FOLLOW;
	#ifdef IN_EXCL_UNLINK // Not supported on older Linux kernels
	if(eventMask&IgnoreUnlinkedFiles)
		em|=IN_EXCL_UNLINK;
	#endif
	
	/* Add the given path to inotify's watch map: */
	Cookie cookie=inotify_add_watch(fd,pathName,eventMask);
	if(cookie<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot monitor path %s",pathName);
	
	/* Enter the new watch into the watched path map: */
	{
	Threads::Mutex::Lock watchedPathsLock(watchedPathsMutex);
	WatchedPathMap::Iterator wpIt=watchedPaths.findEntry(cookie);
	if(!wpIt.isFinished())
		{
		/*******************************************************************
		Someone else already registered a watch for the given path. This
		could be handled using a multi-value hash table and stored masks;
		for now, simply replace the old callback with the new one:
		*******************************************************************/
		
		wpIt->getDest().eventCallback=&eventCallback;
		}
	else
		{
		/* Add the new event callback to the list: */
		watchedPaths.setEntry(WatchedPathMap::Entry(cookie,WatchedPath(pathName,eventCallback)));
		}
	}
	
	/* Return the OS-supplied cookie to the caller: */
	return cookie;
	
	#else
	
	/* Lock the watched path map: */
	Threads::Mutex::Lock watchedPathsLock(watchedPathsMutex);
	
	/* Store the callback with a new dummy cookie in the watched path map: */
	Cookie cookie=nextCookie;
	++nextCookie;
	watchedPaths.setEntry(WatchedPathMap::Entry(cookie,WatchedPath(pathName,eventCallback));
	
	/* Return the dummy cookie: */
	return cookie;
	
	#endif
	}

std::string FileMonitor::getWatchedPath(Cookie pathCookie)
	{
	std::string result;
	
	{
	/* Lock the watched path map: */
	Threads::Mutex::Lock watchedPathsLock(watchedPathsMutex);
	
	/* Find the watched path associated with the given cookie: */
	WatchedPathMap::Iterator wpIt=watchedPaths.findEntry(pathCookie);
	if(!wpIt.isFinished())
		{
		/* Return the watched path: */
		result=wpIt->getDest().path;
		}
	}
	
	return result;
	}

void FileMonitor::removePath(FileMonitor::Cookie pathCookie)
	{
	/* Lock the watched path map: */
	Threads::Mutex::Lock watchedPathsLock(watchedPathsMutex);
	
	/* Find the watched path associated with the given cookie: */
	WatchedPathMap::Iterator wpIt=watchedPaths.findEntry(pathCookie);
	if(!wpIt.isFinished())
		{
		/* Remove the event callback from the event callback list: */
		watchedPaths.removeEntry(wpIt);
		
		#ifdef __linux__
		
		/* Remove the watch from inotify's watch list: */
		inotify_rm_watch(fd,pathCookie);
		
		#endif
		}
	}

}

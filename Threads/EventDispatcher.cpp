/***********************************************************************
EventDispatcher - Class to dispatch events from a central listener to
any number of interested clients.
Copyright (c) 2016-2024 Oliver Kreylos

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

#include <Threads/EventDispatcher.h>

#include <math.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>

namespace Threads {

namespace {

/****************
Helper functions:
****************/

EventDispatcher* stopDispatcher=0; // Event dispatcher to be stopped when a SIGINT or SIGTERM occur

void stopSignalHandler(int signum)
	{
	/* Stop the dispatcher: */
	if(stopDispatcher!=0&&(signum==SIGINT||signum==SIGTERM))
		stopDispatcher->stop();
	}

}

/*****************************************
Embedded classes of class EventDispatcher:
*****************************************/

struct EventDispatcher::IOEventListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key identifying this event
	int fd; // File descriptor belonging to the event
	int typeMask; // Mask of event types (read, write, exception) in which the listener is interested
	IOEventCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	
	/* Constructors and destructors: */
	IOEventListener(ListenerKey sKey,int sFd,int sTypeMask,IOEventCallback sCallback,void* sCallbackUserData)
		:key(sKey),fd(sFd),typeMask(sTypeMask),callback(sCallback),callbackUserData(sCallbackUserData)
		{
		}
	};

struct EventDispatcher::TimerEventListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key identifying this event
	Time time; // Next time point at which to trigger the event
	Time interval; // Time interval at which the event re-occurs; all timer events are considered recurring; one-shot events must call remove() on their event object to prevent reoccurrence
	TimerEventCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	bool suspended; // Flag if the timer event listener is currently suspended
	size_t numMissedEvents; // Number of event recurrences that were missed since the last callback
	
	/* Constructors and destructors: */
	TimerEventListener(ListenerKey sKey,const Time& sTime,const Time& sInterval,TimerEventCallback sCallback,void* sCallbackUserData)
		:key(sKey),time(sTime),interval(sInterval),callback(sCallback),callbackUserData(sCallbackUserData),
		 suspended(false),
		 numMissedEvents(0)
		{
		}
	};

struct EventDispatcher::ProcessListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key identifying this event
	ProcessCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	
	/* Constructors and destructors: */
	ProcessListener(ListenerKey sKey,ProcessCallback sCallback,void* sCallbackUserData)
		:key(sKey),callback(sCallback),callbackUserData(sCallbackUserData)
		{
		}
	};

struct EventDispatcher::SignalListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key for identifying this event
	SignalCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	
	/* Constructors and destructors: */
	SignalListener(ListenerKey sKey,SignalCallback sCallback,void* sCallbackUserData)
		:key(sKey),callback(sCallback),callbackUserData(sCallbackUserData)
		{
		}
	};

class EventDispatcher::IOEventImpl:public EventDispatcher::IOEvent
	{
	friend class EventDispatcher;
	
	/* Elements: */
	private:
	EventDispatcher& dispatcher; // Reference to the event dispatcher object
	std::vector<IOEventListener>::iterator lIt; // Iterator to traverse the listener list
	std::vector<IOEventListener>::iterator nextLIt; // Iterator to next element in list
	
	/* Constructors and destructors: */
	public:
	IOEventImpl(const Time& sDispatchTime,EventDispatcher& sDispatcher)
		:IOEvent(sDispatchTime),
		 dispatcher(sDispatcher),
		 lIt(dispatcher.ioEventListeners.begin())
		{
		}
	
	/* Methods from class Event: */
	virtual void removeListener(void)
		{
		/* Update the dispatcher's file descriptor sets: */
		dispatcher.updateFdSets(lIt->fd,lIt->typeMask,0x0);
		
		/* Remove the current listener from the list: */
		*lIt=dispatcher.ioEventListeners.back();
		dispatcher.ioEventListeners.pop_back();
		
		/* Continue with the listener that replaced the current one: */
		nextLIt=lIt;
		}
	
	/* Methods from class IOEvent: */
	virtual void setCallback(IOEventCallback newCallback,void* newUserData)
		{
		/* Update the current listener: */
		lIt->callback=newCallback;
		lIt->callbackUserData=newUserData;
		}
	virtual void setEventTypeMask(int newEventTypeMask)
		{
		/* Update the dispatcher's file descriptor sets: */
		dispatcher.updateFdSets(lIt->fd,lIt->typeMask,newEventTypeMask);
		
		/* Update the current listener: */
		lIt->typeMask=newEventTypeMask;
		}
	};

class EventDispatcher::TimerEventImpl:public EventDispatcher::TimerEvent
	{
	friend class EventDispatcher;
	
	/* Elements: */
	private:
	TimerEventListener* listener; // The current listener
	bool suspend; // Flag to suspend or remove the current listener after the callback returns
	bool remove; // Flag to remove the current listener after the callback returns (best we can do)
	
	/* Constructors and destructors: */
	public:
	TimerEventImpl(const Time& sDispatchTime)
		:TimerEvent(sDispatchTime)
		{
		}
	
	/* Methods from class Event: */
	virtual void removeListener(void)
		{
		/* Set the flags, main method will have to handle it: */
		suspend=true;
		remove=true;
		}
	
	/* Methods from class TimerEvent: */
	virtual size_t getNumMissedEvents(void) const
		{
		return listener->numMissedEvents;
		}
	virtual void setCallback(TimerEventCallback newCallback,void* newUserData)
		{
		/* Update the current listener: */
		listener->callback=newCallback;
		listener->callbackUserData=newUserData;
		}
	virtual void setEventTime(const Time& newEventTime)
		{
		/* Set the current listener's next event time: */
		listener->time=newEventTime;
		}
	virtual void setEventInterval(const Time& newEventInterval)
		{
		/* Shift the current listener's next event time by the difference in event interval: */
		listener->time-=listener->interval;
		listener->time+=newEventInterval;
		
		/* Set the current listener's event interval: */
		listener->interval=newEventInterval;
		}
	virtual void suspendListener(void)
		{
		/* Set the flag, main method will have to handle it: */
		suspend=true;
		}
	};

class EventDispatcher::ProcessEventImpl:public EventDispatcher::ProcessEvent
	{
	friend class EventDispatcher;
	
	/* Elements: */
	private:
	EventDispatcher& dispatcher; // Reference to the event dispatcher object
	std::vector<ProcessListener>::iterator lIt; // Iterator to traverse the listener list
	std::vector<ProcessListener>::iterator nextLIt; // Iterator to next element in list
	
	/* Constructors and destructors: */
	public:
	ProcessEventImpl(const Time& sDispatchTime,EventDispatcher& sDispatcher)
		:ProcessEvent(sDispatchTime),
		 dispatcher(sDispatcher),
		 lIt(dispatcher.processListeners.begin())
		{
		}
	
	/* Methods from class Event: */
	virtual void removeListener(void)
		{
		/* Remove the current listener from the list: */
		*lIt=dispatcher.processListeners.back();
		dispatcher.processListeners.pop_back();
		
		/* Continue with the listener that replaced the current one: */
		nextLIt=lIt;
		}
	
	/* Methods from class ProcessEvent: */
	virtual void setCallback(ProcessCallback newCallback,void* newUserData)
		{
		/* Update the current listener: */
		lIt->callback=newCallback;
		lIt->callbackUserData=newUserData;
		}
	};

class EventDispatcher::SignalEventImpl:public EventDispatcher::SignalEvent
	{
	friend class EventDispatcher;
	
	/* Elements: */
	private:
	EventDispatcher& dispatcher; // Reference to the event dispatcher object
	SignalListenerMap::Iterator lIt; // Iterator to the current listener
	
	/* Constructors and destructors: */
	public:
	SignalEventImpl(const Time& sDispatchTime,EventDispatcher& sDispatcher)
		:SignalEvent(sDispatchTime),
		 dispatcher(sDispatcher)
		{
		}
	
	/* Methods from class Event: */
	virtual void removeListener(void)
		{
		/* Remove the current listener from the signal listener map: */
		dispatcher.signalListeners.removeEntry(lIt);
		}
	
	/* Methods from class SignalEvent: */
	virtual void setCallback(SignalCallback newCallback,void* newUserData)
		{
		/* Update the current listener: */
		lIt->getDest().callback=newCallback;
		lIt->getDest().callbackUserData=newUserData;
		}
	};

class EventDispatcher::TimerEventListenerComp
	{
	/* Methods: */
	public:
	static bool lessEqual(const TimerEventListener* v1,const TimerEventListener* v2)
		{
		return v1->time<=v2->time;
		}
	};

struct EventDispatcher::PipeMessage // Type for messages sent on an EventDispatcher's self-pipe
	{
	/* Embedded classes: */
	public:
	enum MessageType // Enumerated type for types of messages
		{
		INTERRUPT=0,
		STOP,
		ADD_IO_LISTENER,
		SET_IO_LISTENER_TYPEMASK,
		REMOVE_IO_LISTENER,
		ADD_TIMER_LISTENER,
		SUSPEND_TIMER_LISTENER,
		RESUME_TIMER_LISTENER,
		REMOVE_TIMER_LISTENER,
		ADD_PROCESS_LISTENER,
		REMOVE_PROCESS_LISTENER,
		ADD_SIGNAL_LISTENER,
		REMOVE_SIGNAL_LISTENER,
		SIGNAL,
		NUM_MESSAGETYPES
		};
	
	/* Elements: */
	int messageType; // Type of message
	union
		{
		struct
			{
			ListenerKey key;
			int fd;
			int typeMask;
			IOEventCallback callback;
			void* callbackUserData;
			} addIOListener; // Message to add a new input/output event listener to the event dispatcher
		struct
			{
			ListenerKey key;
			int newTypeMask;
			} setIOListenerEventTypeMask; // Message to change the event type mask of an input/output event listener
		ListenerKey removeIOListener; // Message to remove an input/output event listener from the event dispatcher
		struct
			{
			ListenerKey key;
			struct timeval time;
			struct timeval interval;
			TimerEventCallback callback;
			void* callbackUserData;
			bool startSuspended;
			} addTimerListener; // Message to add a new timer event listener to the event dispatcher
		ListenerKey suspendTimerListener; // Message to suspend a timer event listener
		struct
			{
			ListenerKey key;
			struct timeval time;
			} resumeTimerListener;
		ListenerKey removeTimerListener; // Message to remove a timer event listener from the event dispatcher
		struct
			{
			ListenerKey key;
			ProcessCallback callback;
			void* callbackUserData;
			} addProcessListener; // Message to add a new process listener to the event dispatcher
		ListenerKey removeProcessListener; // Message to remove a process listener from the event dispatcher
		struct
			{
			ListenerKey key;
			SignalCallback callback;
			void* callbackUserData;
			} addSignalListener; // Message to add a new signal listener to the event dispatcher
		ListenerKey removeSignalListener; // Message to remove a signal listener from the event dispatcher
		struct
			{
			ListenerKey key;
			void* signalData;
			} signal; // Message to raise a signal
		};
	};

/**************************************
Methods of class EventDispatcher::Time:
**************************************/

EventDispatcher::Time::Time(double sSeconds)
	{
	/* Take integer and fractional parts of given time, ensuring that tv_usec>=0: */
	tv_sec=long(floor(sSeconds));
	tv_usec=long(floor((sSeconds-double(tv_sec))*1.0e6+0.5));
	
	/* Check for rounding: */
	if(tv_usec>=1000000)
		{
		++tv_sec;
		tv_usec=0;
		}
	}

EventDispatcher::Time EventDispatcher::Time::now(void)
	{
	Time result;
	gettimeofday(&result,0);
	return result;
	}

/********************************
Methods of class EventDispatcher:
********************************/

EventDispatcher::ListenerKey EventDispatcher::getNextKey(void)
	{
	/* Lock the self-pipe (mutex does double duty for key increment): */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Increment the next key: */
	do
		{
		++nextKey;
		}
	while(nextKey==0);
	
	/* Return the new key: */
	return nextKey;
	}

size_t EventDispatcher::readPipeMessages(void)
	{
	/* Check if there was a partial message during the previous call: */
	char* messageReadPtr=reinterpret_cast<char*>(messages);
	size_t readSize=numMessages*sizeof(PipeMessage);
	size_t partialSize=messageReadSize%sizeof(PipeMessage);
	if(partialSize!=0)
		{
		// DEBUGGING
		Misc::logNote(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Partial read from event pipe").c_str());
		
		/* Move the partial data to the front of the buffer: */
		memcpy(messageReadPtr,messageReadPtr+messageReadSize-partialSize,partialSize);
		messageReadPtr+=partialSize;
		readSize-=partialSize;
		}
	
	/* Read up to the given number of messages: */
	ssize_t readResult=read(pipeFds[0],messageReadPtr,readSize);
	
	/* Check for errors: */
	if(readResult<=0)
		{
		if(readResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))
			Misc::logWarning(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"No data to read on event pipe").c_str());
		else
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot read commands from event pipe");
		}
	
	/* Calculate the number of complete messages read: */
	messageReadSize=partialSize+size_t(readResult);
	return messageReadSize/sizeof(PipeMessage);
	}

void EventDispatcher::writePipeMessage(const EventDispatcher::PipeMessage& pm,const char* methodName)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Make sure to write the complete message: */
	const unsigned char* writePtr=reinterpret_cast<const unsigned char*>(&pm);
	size_t writeSize=sizeof(PipeMessage);
	while(writeSize>0U)
		{
		ssize_t writeResult=write(pipeFds[1],writePtr,writeSize);
		if(writeResult>0)
			{
			writePtr+=writeResult;
			writeSize-=size_t(writeResult);
			if(writeSize>0U)
				Misc::logWarning(Misc::makeStdErrMsg(methodName,"Incomplete write to event pipe").c_str());
			}
		else if(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)
			{
			/* Print a warning and wait for a tiny little while: */
			Misc::logWarning(Misc::makeLibcErrMsg(methodName,errno,"Cannot write command to event pipe").c_str());
			usleep(100000); // At this point we're pretty much fucked anyway, so why not wait it out?
			}
		else
			throw Misc::makeLibcErr(methodName,errno,"Cannot write command to event pipe");
		}
	}

void EventDispatcher::updateFdSets(int fd,int oldEventMask,int newEventMask)
	{
	/* Check if the read set needs to be updated: */
	if((oldEventMask^newEventMask)&Read)
		{
		/* Add or remove the file descriptor to/from the read set: */
		if(newEventMask&Read)
			{
			FD_SET(fd,&readFds);
			++numReadFds;
			}
		else
			{
			FD_CLR(fd,&readFds);
			--numReadFds;
			}
		}
	
	/* Check if the write set needs to be updated: */
	if((oldEventMask^newEventMask)&Write)
		{
		/* Add or remove the file descriptor to/from the write set: */
		if(newEventMask&Write)
			{
			FD_SET(fd,&writeFds);
			++numWriteFds;
			}
		else
			{
			FD_CLR(fd,&writeFds);
			--numWriteFds;
			}
		}
	
	/* Check if the exception set needs to be updated: */
	if((oldEventMask^newEventMask)&Exception)
		{
		/* Add or remove the file descriptor to/from the exception set: */
		if(newEventMask&Exception)
			{
			FD_SET(fd,&exceptionFds);
			++numExceptionFds;
			}
		else
			{
			FD_CLR(fd,&exceptionFds);
			--numExceptionFds;
			}
		}
	
	/* Check if the maximum file descriptor needs to be updated: */
	if(newEventMask!=0x0)
		{
		if(maxFd<fd)
			maxFd=fd;
		}
	else
		{
		if(maxFd==fd)
			{
			/* Find the new largest file descriptor: */
			maxFd=pipeFds[0];
			for(std::vector<IOEventListener>::iterator it=ioEventListeners.begin();it!=ioEventListeners.end();++it)
				if(it->typeMask!=0&&maxFd<it->fd)
					maxFd=it->fd;
			}
		}
	}

EventDispatcher::EventDispatcher(void)
	:numMessages(4096/sizeof(PipeMessage)),messages(new PipeMessage[numMessages]),messageReadSize(0),
	 nextKey(0),
	 timerEventListeners(17),signalListeners(17),
	 numReadFds(0),numWriteFds(0),numExceptionFds(0),
	 hadBadFd(false)
	{
	/* Create the self-pipe: */
	pipeFds[1]=pipeFds[0]=-1;
	if(pipe2(pipeFds,O_NONBLOCK)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create event pipe");
	
	/* Initialize the three file descriptor sets: */
	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptionFds);
	
	/* Add the read end of the self-pipe to the read descriptor set: */
	FD_SET(pipeFds[0],&readFds);
	numReadFds=1;
	maxFd=pipeFds[0];
	}

EventDispatcher::~EventDispatcher(void)
	{
	/* Close the self-pipe: */
	close(pipeFds[0]);
	close(pipeFds[1]);
	delete[] messages;
	}

bool EventDispatcher::dispatchNextEvent(bool wait)
	{
	/* Get the dispatch time point: */
	Time dispatchTime=Time::now();
	
	/* Handle elapsed timer events and find the time interval to the next unelapsed event: */
	Time interval;
	TimerEventImpl timerEvent(dispatchTime);
	while(!timerEventListenerHeap.isEmpty())
		{
		/* Get the timer event listener associated with the next timer event: */
		TimerEventListener* tel=timerEventListenerHeap.getSmallest();
		timerEvent.listener=tel;
		timerEvent.key=tel->key;
		timerEvent.userData=tel->callbackUserData;
		
		/* Calculate the interval to the next timer event: */
		interval=tel->time;
		interval-=dispatchTime;
		
		/* Bail out if the event is still in the future: */
		if(interval.tv_sec>=0)
			break;
		
		/* Increment the event listener's event time: */
		tel->time+=tel->interval;
		
		/* Call the event callback: */
		timerEvent.suspend=false;
		timerEvent.remove=false;
		tel->callback(timerEvent);
		
		/* Check if the event listener wants to be suspended or removed: */
		if(timerEvent.suspend)
			{
			/* Mark the event listener as suspended: */
			tel->suspended=true;
			
			/* Remove the event listener from the heap: */
			timerEventListenerHeap.removeSmallest();
			
			/* Check if the event listener wants to be removed for good: */
			if(timerEvent.remove)
				{
				/* Remove the event listener from the map: */
				timerEventListeners.removeEntry(tel->key);
				}
			}
		else
			{
			/* Count the number of missed events: */
			tel->numMissedEvents=0;
			while(!(dispatchTime<=tel->time))
				{
				++tel->numMissedEvents;
				tel->time+=tel->interval;
				}
			
			/* Re-schedule the event at the next time: */
			timerEventListenerHeap.reinsertSmallest();
			}
		}
	
	/* Create lists of watched file descriptors: */
	fd_set rds,wds,eds;
	int numRfds,numWfds,numEfds,numFds;
	if(hadBadFd)
		{
		/* Listen only on the self-pipe to recover from EBADF errors: */
		FD_ZERO(&rds);
		FD_SET(pipeFds[0],&rds);
		numRfds=1;
		numWfds=0;
		numEfds=0;
		numFds=pipeFds[0]+1;
		
		hadBadFd=false;
		}
	else
		{
		/* Copy all used file descriptor sets: */
		if(numReadFds>0)
			rds=readFds;
		if(numWriteFds>0)
			wds=writeFds;
		if(numExceptionFds>0)
			eds=exceptionFds;
		numRfds=numReadFds;
		numWfds=numWriteFds;
		numEfds=numExceptionFds;
		numFds=maxFd+1;
		}
	
	int numSetFds=0;
	if(!wait)
		{
		/* Collect pending events on any watched file descriptor: */
		interval.tv_usec=interval.tv_sec=0;
		numSetFds=select(numFds,numRfds>0?&rds:0,numWfds>0?&wds:0,numEfds>0?&eds:0,&interval);
		}
	else if(timerEventListenerHeap.isEmpty())
		{
		/* Wait forever for the next event on any watched file descriptor: */
		numSetFds=select(numFds,numRfds>0?&rds:0,numWfds>0?&wds:0,numEfds>0?&eds:0,0);
		}
	else
		{
		/* Wait for the next event on any watched file descriptor or until the next timer event elapses: */
		numSetFds=select(numFds,numRfds>0?&rds:0,numWfds>0?&wds:0,numEfds>0?&eds:0,&interval);
		}
	
	/* Update the dispatch time point: */
	dispatchTime=Time::now();
	
	/* Handle all received events: */
	if(numSetFds>0)
		{
		/* Check for a message on the self-pipe: */
		if(FD_ISSET(pipeFds[0],&rds))
			{
			/* Read and handle pipe messages: */
			size_t numMessages=readPipeMessages();
			PipeMessage* pmPtr=messages;
			for(size_t i=0;i<numMessages;++i,++pmPtr)
				{
				switch(pmPtr->messageType)
					{
					case PipeMessage::INTERRUPT: // Interrupt wait
						
						/* Do nothing */
						
						break;
					
					case PipeMessage::STOP: // Stop dispatching events
						return false;
						break;
					
					case PipeMessage::ADD_IO_LISTENER: // Add input/output event listener
						
						/* Add the new input/output event listener to the list: */
						ioEventListeners.push_back(IOEventListener(pmPtr->addIOListener.key,pmPtr->addIOListener.fd,pmPtr->addIOListener.typeMask,pmPtr->addIOListener.callback,pmPtr->addIOListener.callbackUserData));
						
						/* Update the file descriptor sets: */
						updateFdSets(pmPtr->addIOListener.fd,0x0,pmPtr->addIOListener.typeMask);
						
						break;
					
					case PipeMessage::SET_IO_LISTENER_TYPEMASK: // Change the event type mask of an input/output event listener
						
						/* Find the input/output event listener with the given key: */
						for(std::vector<IOEventListener>::iterator elIt=ioEventListeners.begin();elIt!=ioEventListeners.end();++elIt)
							if(elIt->key==pmPtr->setIOListenerEventTypeMask.key)
								{
								/* Update the input/output event listener: */
								int typeMask=elIt->typeMask;
								elIt->typeMask=pmPtr->setIOListenerEventTypeMask.newTypeMask;
								
								/* Update the file descriptor sets: */
								updateFdSets(elIt->fd,typeMask,elIt->typeMask);
								
								/* Stop looking: */
								break;
								}
						
						break;
					
					case PipeMessage::REMOVE_IO_LISTENER: // Remove input/output event listener
						
						/* Find the input/output event listener with the given key: */
						for(std::vector<IOEventListener>::iterator elIt=ioEventListeners.begin();elIt!=ioEventListeners.end();++elIt)
							if(elIt->key==pmPtr->removeIOListener)
								{
								/* Remove the input/output event listener from the list: */
								int fd=elIt->fd;
								int typeMask=elIt->typeMask;
								*elIt=ioEventListeners.back();
								ioEventListeners.pop_back();
								
								/* Update the file descriptor sets: */
								updateFdSets(fd,typeMask,0x0);
								
								/* Stop looking: */
								break;
								}
						
						break;
					
					case PipeMessage::ADD_TIMER_LISTENER: // Add timer event listener
						{
						/* Create a new timer event listener in the map: */
						TimerEventListenerMap::Iterator telIt=timerEventListeners.setAndFindEntry(TimerEventListenerMap::Entry(pmPtr->addTimerListener.key,TimerEventListener(pmPtr->addTimerListener.key,pmPtr->addTimerListener.time,pmPtr->addTimerListener.interval,pmPtr->addTimerListener.callback,pmPtr->addTimerListener.callbackUserData)));
						telIt->getDest().suspended=pmPtr->addTimerListener.startSuspended;
						
						/* Add the just-created timer event listener to the heap if it is not suspended: */
						if(!telIt->getDest().suspended)
							timerEventListenerHeap.insert(&telIt->getDest());
						
						break;
						}
					
					case PipeMessage::SUSPEND_TIMER_LISTENER: // Suspend timer event listener
						{
						/* Find the timer event listener with the given key: */
						TimerEventListener* tel=&timerEventListeners.getEntry(pmPtr->suspendTimerListener).getDest();
						
						/* Remove the timer event listener from the heap if it is not already suspended: */
						if(!tel->suspended)
							{
							for(TimerEventListenerHeap::Iterator elIt=timerEventListenerHeap.begin();elIt!=timerEventListenerHeap.end();++elIt)
								if(*elIt==tel)
									{
									/* Remove the timer event listener from the heap: */
									timerEventListenerHeap.remove(elIt);
									
									/* Stop looking: */
									break;
									}
							}
						
						/* Mark the timer event listener as suspended: */
						tel->suspended=true;
						
						break;
						}
					
					case PipeMessage::RESUME_TIMER_LISTENER: // Resume timer event listener
						{
						/* Find the timer event listener with the given key: */
						TimerEventListener* tel=&timerEventListeners.getEntry(pmPtr->resumeTimerListener.key).getDest();
						
						/* Update and add the timer event listener to the heap if it is currently suspended: */
						if(tel->suspended)
							{
							tel->time=pmPtr->resumeTimerListener.time;
							timerEventListenerHeap.insert(tel);
							}
						
						/* Mark the timer event listener as active: */
						tel->suspended=false;
						
						break;
						}
					
					case PipeMessage::REMOVE_TIMER_LISTENER: // Remove timer event listener
						{
						/* Find the timer event listener with the given key: */
						TimerEventListenerMap::Iterator telIt=timerEventListeners.findEntry(pmPtr->removeTimerListener);
						if(telIt.isFinished())
							throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Timer event listener key not found in hash table");
						TimerEventListener* tel=&telIt->getDest();
						
						/* Remove the timer event listener from the heap if it is not suspended: */
						if(!tel->suspended)
							{
							for(TimerEventListenerHeap::Iterator elIt=timerEventListenerHeap.begin();elIt!=timerEventListenerHeap.end();++elIt)
								if(*elIt==tel)
									{
									/* Remove the timer event listener from the heap: */
									timerEventListenerHeap.remove(elIt);
									
									/* Stop looking: */
									break;
									}
							}
						
						/* Remove the timer event listener from the map: */
						timerEventListeners.removeEntry(telIt);
						
						break;
						}
					
					case PipeMessage::ADD_PROCESS_LISTENER:
						
						/* Add the new process listener to the list: */
						processListeners.push_back(ProcessListener(pmPtr->addProcessListener.key,pmPtr->addProcessListener.callback,pmPtr->addProcessListener.callbackUserData));
						
						break;
					
					case PipeMessage::REMOVE_PROCESS_LISTENER:
						
						/* Find the process listener with the given key: */
						for(std::vector<ProcessListener>::iterator plIt=processListeners.begin();plIt!=processListeners.end();++plIt)
							if(plIt->key==pmPtr->removeProcessListener)
								{
								/* Remove the process listener from the list: */
								*plIt=processListeners.back();
								processListeners.pop_back();
								
								/* Stop looking: */
								break;
								}
						
						break;
					
					case PipeMessage::ADD_SIGNAL_LISTENER:
						
						/* Add the new signal listener to the map: */
						signalListeners.setEntry(SignalListenerMap::Entry(pmPtr->addSignalListener.key,SignalListener(pmPtr->addSignalListener.key,pmPtr->addSignalListener.callback,pmPtr->addSignalListener.callbackUserData)));
						
						break;
					
					case PipeMessage::REMOVE_SIGNAL_LISTENER:
						
						/* Remove the signal listener with the given key from the map: */
						signalListeners.removeEntry(pmPtr->removeSignalListener);
						
						break;
					
					case PipeMessage::SIGNAL:
						{
						/* Find the signal listener with the given key in the map: */
						SignalListenerMap::Iterator slIt=signalListeners.findEntry(pmPtr->signal.key);
						if(slIt.isFinished())
							throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Signal event listener key not found");
						SignalListener& sl=slIt->getDest();
						
						/* Call the signal callback: */
						SignalEventImpl signalEvent(dispatchTime,*this);
						signalEvent.key=sl.key;
						signalEvent.userData=sl.callbackUserData;
						signalEvent.signalData=pmPtr->signal.signalData;
						signalEvent.lIt=slIt;
						sl.callback(signalEvent);
						
						break;
						}
						
					default:
						/* Do nothing: */
						
						// DEBUGGING
						Misc::logWarning(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Unknown pipe message %d",pmPtr->messageType).c_str() );
					}
				}
			
			--numSetFds;
			}
		
		/* Handle all input/output events: */
		IOEventImpl ioEvent(dispatchTime,*this);
		for(ioEvent.nextLIt=ioEvent.lIt;numSetFds>0&&ioEvent.lIt!=ioEventListeners.end();ioEvent.lIt=ioEvent.nextLIt)
			{
			/* Prepare to go to the next listener: */
			++ioEvent.nextLIt;
			
			/* Determine all event types on the listener's file descriptor: */
			int eventTypeMask=0x0;
			if(numRfds>0&&FD_ISSET(ioEvent.lIt->fd,&rds))
				{
				/* Signal a read event: */
				eventTypeMask|=Read;
				--numSetFds;
				}
			if(numWfds>0&&FD_ISSET(ioEvent.lIt->fd,&wds))
				{
				/* Signal a write event: */
				eventTypeMask|=Write;
				--numSetFds;
				}
			if(numEfds>0&&FD_ISSET(ioEvent.lIt->fd,&eds))
				{
				/* Signal an exception event: */
				eventTypeMask|=Exception;
				--numSetFds;
				}
			
			/* Limit to events in which the listener is interested: */
			ioEvent.eventTypeMask=eventTypeMask&ioEvent.lIt->typeMask;
			
			/* Check for spurious events, i.e., events in which the listener was not actually interested: */
			if(ioEvent.eventTypeMask!=eventTypeMask)
				Misc::logWarning(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Spurious event").c_str());
			
			/* Call the listener's event callback if any relevant events occurred: */
			if(ioEvent.eventTypeMask!=0x0)
				{
				ioEvent.key=ioEvent.lIt->key;
				ioEvent.userData=ioEvent.lIt->callbackUserData;
				ioEvent.lIt->callback(ioEvent);
				}
			}
		}
	else if(numSetFds<0&&errno!=EINTR)
		{
		if(errno==EBADF)
			{
			// DEBUGGING
			Misc::logWarning(Misc::makeStdErrMsg(__PRETTY_FUNCTION__,"Bad file descriptor in select").c_str());
			
			/* Set error flag; only wait on self-pipe on next iteration to hopefully receive a "remove listener" message for the bad descriptor: */
			hadBadFd=true;
			}
		else
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"select() failed");
		}
	
	/* Call all process listeners: */
	ProcessEventImpl processEvent(dispatchTime,*this);
	for(processEvent.nextLIt=processEvent.lIt;processEvent.lIt!=processListeners.end();processEvent.lIt=processEvent.nextLIt)
		{
		/* Prepare to go to the next listener: */
		++processEvent.nextLIt;
		
		/* Call the listener's callback: */
		processEvent.key=processEvent.lIt->key;
		processEvent.userData=processEvent.lIt->callbackUserData;
		processEvent.lIt->callback(processEvent);
		}
	
	return true;
	}

void EventDispatcher::dispatchEvents(void)
	{
	/* Dispatch events until the stop() method is called: */
	while(dispatchNextEvent(true))
		;
	}

void EventDispatcher::interrupt(void)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::INTERRUPT;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

void EventDispatcher::stop(void)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::STOP;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

void EventDispatcher::stopOnSignals(void)
	{
	/* Check if there is already a signal-stopped event dispatcher: */
	if(stopDispatcher!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Already registered another dispatcher");
	
	/* Register this dispatcher: */
	stopDispatcher=this;
	
	/* Intercept SIGINT and SIGTERM: */
	struct sigaction sigIntAction;
	memset(&sigIntAction,0,sizeof(struct sigaction));
	sigIntAction.sa_handler=stopSignalHandler;
	if(sigaction(SIGINT,&sigIntAction,0)<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot intercept SIGINT");
	struct sigaction sigTermAction;
	memset(&sigTermAction,0,sizeof(struct sigaction));
	sigTermAction.sa_handler=stopSignalHandler;
	if(sigaction(SIGTERM,&sigTermAction,0)<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot intercept SIGTERM");
	}

EventDispatcher::ListenerKey EventDispatcher::addIOEventListener(int eventFd,int eventTypeMask,EventDispatcher::IOEventCallback eventCallback,void* eventCallbackUserData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_IO_LISTENER;
	pm.addIOListener.key=getNextKey();
	pm.addIOListener.fd=eventFd;
	pm.addIOListener.typeMask=eventTypeMask;
	pm.addIOListener.callback=eventCallback;
	pm.addIOListener.callbackUserData=eventCallbackUserData;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	
	return pm.addIOListener.key;
	}

void EventDispatcher::setIOEventListenerEventTypeMask(EventDispatcher::ListenerKey listenerKey,int newEventTypeMask)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::SET_IO_LISTENER_TYPEMASK;
	pm.setIOListenerEventTypeMask.key=listenerKey;
	pm.setIOListenerEventTypeMask.newTypeMask=newEventTypeMask;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

void EventDispatcher::removeIOEventListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_IO_LISTENER;
	pm.removeIOListener=listenerKey;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

EventDispatcher::ListenerKey EventDispatcher::addTimerEventListener(const EventDispatcher::Time& eventTime,const EventDispatcher::Time& eventInterval,EventDispatcher::TimerEventCallback eventCallback,void* eventCallbackUserData,bool startSuspended)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_TIMER_LISTENER;
	pm.addTimerListener.key=getNextKey();
	pm.addTimerListener.time=eventTime;
	pm.addTimerListener.interval=eventInterval;
	pm.addTimerListener.callback=eventCallback;
	pm.addTimerListener.callbackUserData=eventCallbackUserData;
	pm.addTimerListener.startSuspended=startSuspended;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	
	return pm.addTimerListener.key;
	}

void EventDispatcher::suspendTimerEventListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::SUSPEND_TIMER_LISTENER;
	pm.suspendTimerListener=listenerKey;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

void EventDispatcher::resumeTimerEventListener(EventDispatcher::ListenerKey listenerKey,const EventDispatcher::Time& eventTime)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::RESUME_TIMER_LISTENER;
	pm.resumeTimerListener.key=listenerKey;
	pm.resumeTimerListener.time=eventTime;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

void EventDispatcher::removeTimerEventListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_TIMER_LISTENER;
	pm.removeTimerListener=listenerKey;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

EventDispatcher::ListenerKey EventDispatcher::addProcessListener(EventDispatcher::ProcessCallback eventCallback,void* eventCallbackUserData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_PROCESS_LISTENER;
	pm.addProcessListener.key=getNextKey();
	pm.addProcessListener.callback=eventCallback;
	pm.addProcessListener.callbackUserData=eventCallbackUserData;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	
	return pm.addProcessListener.key;
	}

void EventDispatcher::removeProcessListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_PROCESS_LISTENER;
	pm.removeProcessListener=listenerKey;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

EventDispatcher::ListenerKey EventDispatcher::addSignalListener(EventDispatcher::SignalCallback eventCallback,void* eventCallbackUserData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_SIGNAL_LISTENER;
	pm.addSignalListener.key=getNextKey();
	pm.addSignalListener.callback=eventCallback;
	pm.addSignalListener.callbackUserData=eventCallbackUserData;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	
	return pm.addSignalListener.key;
	}

void EventDispatcher::removeSignalListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_SIGNAL_LISTENER;
	pm.removeSignalListener=listenerKey;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

void EventDispatcher::signal(EventDispatcher::ListenerKey listenerKey,void* signalData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::SIGNAL;
	pm.signal.key=listenerKey;
	pm.signal.signalData=signalData;
	writePipeMessage(pm,__PRETTY_FUNCTION__);
	}

}

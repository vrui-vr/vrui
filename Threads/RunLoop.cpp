/***********************************************************************
RunLoop - Class for event handlers and message dispatchers constituting
a thread's run loop.
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

#include <Threads/RunLoop.h>

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <poll.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Threads/RefCounted.h>
#include <Threads/Mutex.h>
#include <Threads/Cond.h>
#include <Threads/FunctionCalls.h>
#include <Threads/IOWatcher.h>
#include <Threads/Timer.h>
#include <Threads/SignalHandler.h>
#include <Threads/UserSignal.h>
#include <Threads/ProcessFunction.h>
#include <Threads/RunLoopInternal.h>

namespace Threads {

/********************************
Static elements of class RunLoop:
********************************/

const size_t RunLoop::messageBufferSize=PIPE_BUF/sizeof(RunLoop::PipeMessage); // Reserve for the number of messages that fits into the guaranteed atomic write size

/************************
Methods of class RunLoop:
************************/

bool RunLoop::writePipeMessage(const RunLoop::PipeMessage& pm,const char* methodName,Ownable* messageSender,RefCounted* messageObject)
	{
	bool result=true;
	
	/* If there is a message sender and/or object, take references to them: */
	if(messageSender!=0)
		messageSender->ref();
	if(messageObject!=0)
		messageObject->ref();
	
	/* Make a blocking atomic write to the self-pipe: */
	ssize_t writeResult=write(pipeFds[1],&pm,sizeof(PipeMessage));
	if(writeResult<0)
		{
		/* Check if the self-pipe was closed because the run loop is shutting down: */
		if(errno==EBADF)
			{
			/* Drop the references to a message sender and/or object again and signal failure: */
			if(messageSender!=0)
				messageSender->unref();
			if(messageObject!=0)
				messageObject->unref();
			result=false;
			}
		else
			throw Misc::makeLibcErr(methodName,errno,"Cannot write to event pipe");
		}
	else if(writeResult!=sizeof(PipeMessage))
		throw Misc::makeStdErr(methodName,"Partial write to event pipe");
	
	return result;
	}

void RunLoop::insertActiveTimer(Timer* newTimer,const EventTime& newTimeout)
	{
	/* Insert a new (dummy) entry at the end of the heap array: */
	activeTimers.push_back(ActiveTimer(newTimer,newTimeout));
	
	/* Find the correct heap slot for the new timer by iteratively fixing the heap invariance from the bottom up: */
	unsigned int heapSlot=activeTimers.size()-1;
	ActiveTimer* slots=activeTimers.data();
	unsigned int parentSlot;
	while(heapSlot>0&&slots[parentSlot=(heapSlot-1)>>1].timeout>newTimeout)
		{
		/* Move the parent into the proposed slot and continue checking from the parent: */
		slots[heapSlot]=slots[parentSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=parentSlot;
		}
	
	/* Place the new timer into the final heap slot: */
	slots[heapSlot].timer=newTimer;
	slots[heapSlot].timeout=newTimeout;
	slots[heapSlot].timer->activeIndex=heapSlot;
	}

void RunLoop::updateActiveTimer(Timer* timer)
	{
	/* Fix any broken heap invariances from changes to an active timer's time-out: */
	unsigned int numSlots=activeTimers.size();
	ActiveTimer* slots=activeTimers.data();
	unsigned int heapSlot=timer->activeIndex;
	
	/* Fix the heap invariance from the current heap slot up: */
	bool mustFix=true;
	unsigned int parentSlot;
	while(heapSlot>0&&slots[parentSlot=(heapSlot-1)>>1].timeout>timer->timeout)
		{
		/* Move the parent into the proposed slot and continue checking from the parent: */
		slots[heapSlot]=slots[parentSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=parentSlot;
		
		mustFix=false; // The heap will be fixed once we're done with this loop
		}
	
	/* Fix the heap invariance from the current heap slot down; this is not necessary if we already fixed the heap: */
	while(mustFix)
		{
		/* Find the earliest time-out between the last time-out and the up to two children of its proposed heap slot: */
		unsigned int minSlot=heapSlot;
		const EventTime* minTimeout=&timer->timeout;
		unsigned int child1Slot=(heapSlot<<1)+1;
		if(child1Slot<numSlots&&*minTimeout>slots[child1Slot].timeout)
			{
			minTimeout=&slots[child1Slot].timeout;
			minSlot=child1Slot;
			}
		unsigned int child2Slot=(heapSlot<<1)+2;
		if(child2Slot<numSlots&&*minTimeout>slots[child2Slot].timeout)
			{
			minTimeout=&slots[child2Slot].timeout;
			minSlot=child2Slot;
			}
		
		/* Bail out if the new time-out is the earliest: */
		if(minSlot==heapSlot)
			break;
		
		/* Move the earliest child into the parent's slot and continue checking from the earliest child: */
		slots[heapSlot]=slots[minSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=minSlot;
		}
	
	/* Place the timer's active timer heap entry into the final heap slot: */
	slots[heapSlot].timer=timer;
	slots[heapSlot].timeout=timer->timeout;
	slots[heapSlot].timer->activeIndex=heapSlot;
	}

void RunLoop::replaceFirstActiveTimer(Timer* newTimer,const EventTime& newTimeout)
	{
	/* Find the correct heap slot for the new timer by iteratively fixing the heap invariance from the top down: */
	unsigned int numSlots=activeTimers.size();
	ActiveTimer* slots=activeTimers.data();
	unsigned int heapSlot=0;
	while(true)
		{
		/* Find the earliest time-out between the new time-out and the up to two children of its proposed heap slot: */
		unsigned int minSlot=heapSlot;
		const EventTime* minTimeout=&newTimeout;
		unsigned int child1Slot=(heapSlot<<1)+1;
		if(child1Slot<numSlots&&*minTimeout>slots[child1Slot].timeout)
			{
			minTimeout=&slots[child1Slot].timeout;
			minSlot=child1Slot;
			}
		unsigned int child2Slot=(heapSlot<<1)+2;
		if(child2Slot<numSlots&&*minTimeout>slots[child2Slot].timeout)
			{
			minTimeout=&slots[child2Slot].timeout;
			minSlot=child2Slot;
			}
		
		/* Bail out if the new time-out is the earliest: */
		if(minSlot==heapSlot)
			break;
		
		/* Move the earliest child into the parent's slot and continue checking from the earliest child: */
		slots[heapSlot]=slots[minSlot];
		slots[heapSlot].timer->activeIndex=heapSlot;
		heapSlot=minSlot;
		}
	
	/* Place the new timer into the final heap slot: */
	slots[heapSlot].timer=newTimer;
	slots[heapSlot].timeout=newTimeout;
	newTimer->activeIndex=heapSlot;
	}

void RunLoop::handlePipeMessages(RunLoop::PipeMessage* end)
	{
	/* Handle all read messages: */
	do
		{
		/* Handle the message based on its type: */
		switch(messagePtr->messageType)
			{
			case PipeMessage::WakeUp:
				/* Do nothing... */
				
				break;
			
			case PipeMessage::Stop:
				/* Set the shutdown flag: */
				shutdownRequested=true;
				
				break;
			
			case PipeMessage::Signal:
				/* Forward the signal to whichever OS signal handler registered for it and set the shutdown flag if the signal is set to "stop": */
				if(SignalHandler::signalPM(this,messagePtr->signal.signum))
					shutdownRequested=true;
				
				break;
			
			case PipeMessage::SignalUserSignal:
				/* Forward the message to the referenced user signal handler and drop the message's references to it and the optional signal data: */
				messagePtr->signalUserSignal.userSignal->signalPM(messagePtr->signalUserSignal.signalData);
				messagePtr->signalUserSignal.userSignal->unref();
				if(messagePtr->signalUserSignal.signalData!=0)
					messagePtr->signalUserSignal.signalData->unref();
				
				break;
			
			case PipeMessage::SetIOWatcherEventMask:
				/* Forward the message to the referenced I/O watcher and drop the message's reference to it: */
				messagePtr->setIOWatcherEventMask.ioWatcher->setEventMaskPM(messagePtr->setIOWatcherEventMask.newEventMask);
				messagePtr->setIOWatcherEventMask.ioWatcher->unref();
				
				break;
			
			case PipeMessage::EnableIOWatcher:
				/* Forward the message to the referenced I/O watcher and drop the message's reference to it: */
				messagePtr->enableIOWatcher.ioWatcher->enablePM();
				messagePtr->enableIOWatcher.ioWatcher->unref();
				
				break;
			
			case PipeMessage::DisableIOWatcher:
				/* Forward the message to the referenced I/O watcher: */
				messagePtr->disableIOWatcher.ioWatcher->disablePM();
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(messagePtr->disableIOWatcher.cond!=0)
					messagePtr->disableIOWatcher.cond->signal();
				
				/* Drop the message's reference to the I/O watcher: */
				messagePtr->disableIOWatcher.ioWatcher->unref();
				
				break;
			
			case PipeMessage::SetIOWatcherEventHandler:
				/* Forward the message to the referenced I/O watcher and drop the message's references to it and the new event handler: */
				messagePtr->setIOWatcherEventHandler.ioWatcher->setEventHandlerPM(messagePtr->setIOWatcherEventHandler.eventHandler);
				messagePtr->setIOWatcherEventHandler.ioWatcher->unref();
				messagePtr->setIOWatcherEventHandler.eventHandler->unref();
				
				break;
			
			case PipeMessage::SetTimerTimeout:
			case PipeMessage::SetTimerTimeoutReenable:
				/* Forward the message to the referenced timer and drop the message's reference to it: */
				messagePtr->setTimerTimeout.timer->setTimeoutPM(EventTime(messagePtr->setTimerTimeout.timeout),messagePtr->messageType==PipeMessage::SetTimerTimeoutReenable);
				messagePtr->setTimerTimeout.timer->unref();
				
				break;
			
			case PipeMessage::SetTimerInterval:
				/* Forward the message to the referenced timer and drop the message's reference to it: */
				messagePtr->setTimerInterval.timer->setIntervalPM(EventInterval(messagePtr->setTimerInterval.interval));
				messagePtr->setTimerInterval.timer->unref();
				
				break;
			
			case PipeMessage::EnableTimer:
				/* Forward the message to the referenced timer and drop the message's reference to it: */
				messagePtr->enableTimer.timer->enablePM();
				messagePtr->enableTimer.timer->unref();
				
				break;
			
			case PipeMessage::DisableTimer:
				/* Forward the message to the referenced timer: */
				messagePtr->disableTimer.timer->disablePM();
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(messagePtr->disableTimer.cond!=0)
					messagePtr->disableTimer.cond->signal();
				
				/* Drop the message's reference to the timer: */
				messagePtr->disableTimer.timer->unref();
				
				break;
			
			case PipeMessage::SetTimerEventHandler:
				/* Forward the message to the referenced I/O watcher and drop the message's references to it and the new event handler: */
				messagePtr->setTimerEventHandler.timer->setEventHandlerPM(messagePtr->setTimerEventHandler.eventHandler);
				messagePtr->setTimerEventHandler.timer->unref();
				messagePtr->setTimerEventHandler.eventHandler->unref();
				
				break;
			
			case PipeMessage::EnableSignalHandler:
				/* Forward the message to the referenced OS signal handler and drop the message's reference to it: */
				messagePtr->enableSignalHandler.signalHandler->enablePM();
				messagePtr->enableSignalHandler.signalHandler->unref();
				
				break;
			
			case PipeMessage::DisableSignalHandler:
				/* Forward the message to the referenced OS signal handler: */
				messagePtr->disableSignalHandler.signalHandler->disablePM(messagePtr->disableSignalHandler.cond!=0);
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(messagePtr->disableSignalHandler.cond!=0)
					messagePtr->disableSignalHandler.cond->signal();
				
				/* Drop the message's reference to the OS signal handler: */
				messagePtr->disableTimer.timer->unref();
				
				break;
			
			case PipeMessage::SetSignalHandlerEventHandler:
				/* Forward the message to the referenced OS signal handler and drop the message's references to it and the new event handler: */
				messagePtr->setSignalHandlerEventHandler.signalHandler->setEventHandlerPM(messagePtr->setSignalHandlerEventHandler.eventHandler);
				messagePtr->setSignalHandlerEventHandler.signalHandler->unref();
				messagePtr->setSignalHandlerEventHandler.eventHandler->unref();
				
				break;
			
			case PipeMessage::EnableUserSignal:
				/* Forward the message to the referenced user signal handler and drop the message's references to it: */
				messagePtr->enableUserSignal.userSignal->enablePM();
				messagePtr->enableUserSignal.userSignal->unref();
				
				break;
			
			case PipeMessage::DisableUserSignal:
				/* Forward the message to the referenced user signal handler: */
				messagePtr->disableUserSignal.userSignal->disablePM();
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(messagePtr->disableUserSignal.cond!=0)
					messagePtr->disableUserSignal.cond->signal();
				
				/* Drop the message's reference to the user signal handler: */
				messagePtr->disableUserSignal.userSignal->unref();
				
				break;
			
			case PipeMessage::SetUserSignalEventHandler:
				/* Forward the message to the referenced user signal handler and drop the message's references to it and the new event handler: */
				messagePtr->setUserSignalEventHandler.userSignal->setEventHandlerPM(messagePtr->setUserSignalEventHandler.eventHandler);
				messagePtr->setUserSignalEventHandler.userSignal->unref();
				messagePtr->setUserSignalEventHandler.eventHandler->unref();
				
				break;
			
			case PipeMessage::SetProcessFunctionSpinning:
				/* Forward the message to the referenced process function and drop the message's reference to it: */
				messagePtr->setProcessFunctionSpinning.processFunction->setSpinningPM(messagePtr->setProcessFunctionSpinning.spinning);
				messagePtr->setProcessFunctionSpinning.processFunction->unref();
				
				break;
			
			case PipeMessage::EnableProcessFunction:
				/* Forward the message to the referenced process function and drop the message's reference to it: */
				messagePtr->enableProcessFunction.processFunction->enablePM();
				messagePtr->enableProcessFunction.processFunction->unref();
				
				break;
			
			case PipeMessage::DisableProcessFunction:
				/* Forward the message to the referenced process function: */
				messagePtr->disableProcessFunction.processFunction->disablePM();
				
				/* If the caller provided a condition variable for synchronization, signal it: */
				if(messagePtr->disableProcessFunction.cond!=0)
					messagePtr->disableProcessFunction.cond->signal();
				
				/* Drop the message's reference to the process function: */
				messagePtr->disableProcessFunction.processFunction->unref();
				
				break;
			
			case PipeMessage::SetProcessFunctionFunction:
				/* Forward the message to the referenced process function and drop the message's references to it and the new function: */
				messagePtr->setProcessFunctionFunction.processFunction->setFunctionPM(messagePtr->setProcessFunctionFunction.function);
				messagePtr->setProcessFunctionFunction.processFunction->unref();
				messagePtr->setProcessFunctionFunction.function->unref();
				
				break;
			}
		
		/* Go to the next message: */
		++messagePtr;
		}
	while(messagePtr!=end);
	}

RunLoop::RunLoop(void)
	:threadId(Threads::Thread::getSelfId()),
	 pipeClosed(false),
	 messageBuffer(new PipeMessage[messageBufferSize]),messageEnd(messageBuffer),messagePtr(messageBuffer),
	 numActiveIOWatchers(0),
	 numSpinningProcessFunctions(0),
	 shutdownRequested(false),
	 handlingIOWatchers(false),
	 handlingProcessFunctions(false)
	{
	/* Create the self-pipe: */
	pipeFds[1]=pipeFds[0]=-1;
	if(pipe(pipeFds)<0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create event pipe");
	
	/* Create the permanent poll request for the self-pipe's read end: */
	struct pollfd pollFd;
	pollFd.fd=pipeFds[0];
	pollFd.events=POLLIN;
	pollFds.push_back(pollFd);
	}

RunLoop::~RunLoop(void)
	{
	/* Drain and close the self-pipe: */
	shutdown();
	
	/* Drop all references held by the active I/O watcher list: */
	for(unsigned int i=0;i<numActiveIOWatchers;++i)
		activeIOWatchers[i].ioWatcher->unref();
	
	/* Drop all references held by the active timer heap: */
	for(size_t i=0;i<activeTimers.size();++i)
		activeTimers[i].timer->unref();
	
	/* Unregister this run loop from all OS signals: */
	SignalHandler::unregisterRunLoop(this);
	
	/* Drop all references held by the active process function list: */
	for(size_t i=0;i<activeProcessFunctions.size();++i)
		activeProcessFunctions[i].processFunction->unref();
	
	/* Release the message buffer: */
	delete[] messageBuffer;
	}

void RunLoop::stopOnSignal(int signum)
	{
	/* Forward the request to the SignalHandler class: */
	SignalHandler::registerRunLoop(this,signum);
	}

void RunLoop::wakeUp(void)
	{
	/* Check if this call was made from outside the run loop's thread: */
	if(!Threads::Thread::isSelfEqual(threadId))
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::WakeUp);
		writePipeMessage(pm,__PRETTY_FUNCTION__);
		}
	}

void RunLoop::stop(void)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(threadId))
		{
		/* Set the shutdown flag: */
		shutdownRequested=true;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		PipeMessage pm(PipeMessage::Stop);
		writePipeMessage(pm,__PRETTY_FUNCTION__);
		}
	}

void RunLoop::attachToThread(void)
	{
	/* Override the run loop's thread ID: */
	threadId=Threads::Thread::getSelfId();
	}

void RunLoop::restart(void)
	{
	/* Check if the self-pipe needs to be re-opened: */
	if(pipeClosed)
		{
		/* Create the self-pipe: */
		pipeFds[1]=pipeFds[0]=-1;
		if(pipe(pipeFds)<0)
			throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot create event pipe");
		
		/* Re-enable the self-pipe's poll request: */
		pollFds[0].fd=pipeFds[0];
		
		/* Mark the self-pipe as open: */
		pipeClosed=false;
		}
	
	/* Reset the shutdown flag: */
	shutdownRequested=false;
	}

bool RunLoop::waitForEvents(EventTime* wakeUp)
	{
	/* Bail out and signal shutdown if a shutdown has been requested: */
	if(shutdownRequested)
		return false;
	
	/* Keep polling until there's an actual event to report: */
	bool keepPolling=true;
	do
		{
		#ifdef __linux__ // On Linux, we have ppoll()
		
		/* Calculate a time-out for the ppoll() call: */
		EventInterval pollTimeout(0,0); // In case we don't want to block, only poll
		EventInterval* pt=0; // Assume that we'll block forever
		if(numSpinningProcessFunctions>0)
			{
			/* Don't block for I/O events; only poll: */
			pt=&pollTimeout;
			}
		else if(wakeUp!=0||!activeTimers.empty())
			{
			/* Sample the current time: */
			lastDispatchTime.set();
			
			/* Calculate the interval from now to the next timer to elapse or the wake-up time, clamping to zero if that time-out already elapsed: */
			if(activeTimers[0].timeout<*wakeUp)
				wakeUp=&activeTimers[0].timeout;
			if(*wakeUp>lastDispatchTime)
				pollTimeout=*wakeUp-lastDispatchTime;
			pt=&pollTimeout;
			}
		
		/* Block until an I/O event occurs or the time-out expires: */
		int pollResult=ppoll(pollFds.data(),numActiveIOWatchers+1,pt,0); // Account for the extra watcher for the self-pipe's read end
		
		#else
		
		/* Calculate a time-out for the poll() call: */
		int pollTimeout=-1; // Assume that we'll block forever
		if(numSpinningProcessFunctions>0)
			{
			/* Don't block for I/O events; only poll: */
			pollTimeout=0;
			}
		else if(wakeUp!=0||!activeTimers.empty())
			{
			/* Sample the current time: */
			lastDispatchTime.set();
			
			/* Calculate the interval from now to the next timer to elapse or the wake-up time, clamping to zero if the next timer already elapsed: */
			pollTimeout=0;
			if(activeTimers[0].timeout<*wakeUp)
				wakeUp=&activeTimers[0].timeout;
			if(*wakeUp>lastDispatchTime)
				{
				EventInterval timeout=*wakeUp-lastDispatchTime;
				pollTimeout=int(timeout.tv_sec*1000L+(timeout.tv_nsec+999999L)/1000000L); // poll() takes timeouts in ms, which is a tad unfortunate
				}
			}
		
		/* Block until an I/O event occurs or the time-out expires: */
		int pollResult=poll(pollFds.data(),numActiveIOWatchers+1,pollTimeout); // Account for the extra watcher for the self-pipe's read end
		
		#endif
		
		/* Check the result of blocking: */
		if(pollResult>0)
			{
			/* Read all messages available on the self-pipe and handle internal messages only: */
			if((pollFds[0].revents&POLLIN)!=0x0)
				{
				/* Read a batch of messages from the self-pipe: */
				ssize_t readResult=read(pipeFds[0],messageBuffer,messageBufferSize*sizeof(PipeMessage));
				if(readResult<0)
					throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot read from event pipe");
				
				/* Set up the message handling buffer: */
				size_t numMessages=size_t(readResult)/sizeof(PipeMessage);
				if(numMessages*sizeof(PipeMessage)!=size_t(readResult))
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Partial read on event pipe");
				messageEnd=messageBuffer+numMessages;
				messagePtr=messageBuffer;
				
				/* Find the first pipe message that is an actual event, i.e., requires returning from this method: */
				PipeMessage* imPtr;
				for(imPtr=messageBuffer;imPtr!=messageEnd&&imPtr->messageType>PipeMessage::PipeMessage::SignalUserSignal;++imPtr)
					;
				
				/* Handle all initial non-event messages read from the self-pipe: */
				if(imPtr!=messageBuffer)
					handlePipeMessages(imPtr);
				
				/* Discount the self-pipe's readiness if no actual events were encountered: */
				if(imPtr==messageEnd)
					--pollResult;
				}
			
			/* Stop polling if any watched file descriptors have events: */
			keepPolling=pollResult==0;
			}
		else if(pollResult==0)
			{
			/* A timer timed out, stop polling: */
			keepPolling=false;
			}
		else
			{
			/* Handle errors from poll/ppoll: */
			if(errno!=EINTR&&errno!=EAGAIN)
				throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot poll for I/O events");
			}
		}
	while(keepPolling);
	
	/* Sample the current time: */
	lastDispatchTime.set();
	
	return true;
	}

void RunLoop::dispatchPendingEvents(void)
	{
	/* Handle any potential messages on the self-pipe: */
	if(messagePtr!=messageEnd)
		handlePipeMessages(messageEnd);

	/* Handle all elapsed active timers, i.e., timers whose time-out is strictly before the current time: */
	while(!activeTimers.empty()&&activeTimers[0].timeout<lastDispatchTime)
		{
		/* Create an event descriptor structure: */
		Timer* timer=activeTimers[0].timer;
		TimerEvent event(timer,lastDispatchTime,activeTimers[0].timeout);
		
		/* Check if this is a repeating time-out: */
		bool dropRef=false;
		if(timer->interval.tv_sec!=0||timer->interval.tv_nsec!=0)
			{
			/* Advance the timer's time-out and re-schedule the timer: */
			timer->timeout+=timer->interval;
			replaceFirstActiveTimer(timer,timer->timeout);
			}
		else
			{
			/* Disable the timer: */
			timer->enabled=false;
			
			/* Replace the timer's entry in the active timers heap by re-inserting the current last element in the heap's array: */
			ActiveTimer last=activeTimers[activeTimers.size()-1];
			activeTimers.pop_back();
			replaceFirstActiveTimer(last.timer,last.timeout);
			
			/* Drop the active timer heap's reference to the timer after the callback returns: */
			dropRef=true;
			}
		
		/* Call the timer's event handler: */
		(*timer->eventHandler)(event);
		
		if(dropRef)
			timer->unref();
		}
	
	/* Handle all active I/O watchers: */
	handlingIOWatchers=true;
	IOWatcherEvent event(lastDispatchTime);
	for(handledIOWatcherIndex=0;handledIOWatcherIndex<numActiveIOWatchers;++handledIOWatcherIndex)
		if(pollFds[handledIOWatcherIndex+1].revents!=0x0)
			{
			/* Set the I/O watcher who is to receive this event: */
			event.source=activeIOWatchers[handledIOWatcherIndex].ioWatcher;
			
			/* Set the bit mask of events that actually occurred and mask out events in which the I/O watcher is not interested: */
			event.eventMask=IOWatcher::pollEventsToEventMask(pollFds[handledIOWatcherIndex+1].revents);
			event.eventMask&=event.source->eventMask|IOWatcher::ProblemMask; // Can't mask out "problem" indicators
			
			/* Call the I/O watcher's event handler: */
			(*event.source->eventHandler)(event);
			}
	handlingIOWatchers=false;
	
	/* Handle all active process functions: */
	handlingProcessFunctions=true;
	for(handledProcessFunctionIndex=0;handledProcessFunctionIndex<activeProcessFunctions.size();++handledProcessFunctionIndex)
		{
		/* Call the process function's event handler: */
		ProcessFunction* pf=activeProcessFunctions[handledProcessFunctionIndex].processFunction;
		(*pf->function)(*pf);
		}
	handlingProcessFunctions=false;
	}

void RunLoop::run(void)
	{
	/* Restart the run loop in case it was shut down: */
	restart();
	
	/* Dispatch events until stopped: */
	while(waitForEvents())
		dispatchPendingEvents();
	}

void RunLoop::shutdown(void)
	{
	/* Check that the self-pipe is not already closed: */
	if(!pipeClosed)
		{
		/* Close the write end of the self-pipe: */
		close(pipeFds[1]);
		
		/* Completely drain the self-pipe: */
		while(true)
			{
			/* Read a batch of messages from the self-pipe: */
			ssize_t readResult=read(pipeFds[0],messageBuffer,messageBufferSize*sizeof(PipeMessage));
			
			/* Bail out on error or end-of-file: */
			if(readResult<=0)
				break;
			
			/* Set up the message handling buffer and bail out if there's a partial message; can't do anything about it: */
			size_t numMessages=size_t(readResult)/sizeof(PipeMessage);
			if(numMessages*sizeof(PipeMessage)!=size_t(readResult))
				break;
			messageEnd=messageBuffer+numMessages;
			messagePtr=messageBuffer;
			
			/* Handle all messages on the self-pipe: */
			handlePipeMessages(messageEnd);
			}
		
		/* Close the read end of the self-pipe: */
		close(pipeFds[0]);
		
		/* Disable the self-pipe's poll request: */
		pollFds[0].fd=-1; // This is the specified way to disable a poll request
		
		/* Mark the pipe as closed: */
		pipeClosed=true;
		}
	}

}

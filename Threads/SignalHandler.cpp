/***********************************************************************
SignalHandler - Class to handle operating system signals in the context
of a run loop.
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

#include <Threads/SignalHandler.h>

#include <string.h>
#include <signal.h>
#include <errno.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <Threads/RunLoopInternal.h>

namespace Threads {

/************************************************************
Declaration of struct SignalHandler::RegisteredSignalHandler:
************************************************************/

struct SignalHandler::RegisteredSignalHandler
	{
	/* Elements: */
	public:
	RunLoop* runLoop; // The run loop that registered for this OS signal or null for unhandled signals
	SignalHandler* signalHandler; // Pointer to the OS signal handler registered for this signal, or 0 if the registered run loop should stop on receiving the signal; holds a reference to the OS signal handler object
	
	/* Constructors and destructors: */
	RegisteredSignalHandler(void) // Default constructor represents unhandled signal
		:runLoop(0),signalHandler(0)
		{
		}
	};

/**************************************
Static elements of class SignalHandler:
**************************************/

Threads::Mutex SignalHandler::registeredSignalHandlersMutex;
SignalHandler::RegisteredSignalHandler SignalHandler::registeredSignalHandlers[SignalHandler::maxSignal+1];

/******************************
Methods of class SignalHandler:
******************************/

void SignalHandler::disowned(void)
	{
	/* Disable this OS signal handler synchronously, thus dropping all references to it and not sending further events: */
	disableInternal(true);
	}

void SignalHandler::enableInternal(void)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this OS signal handler is not already enabled and still has an owner: */
		if(!enabled&&isOwned())
			{
			/* Enable this OS signal handler: */
			enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::EnableSignalHandler);
		pm.enableSignalHandler.signalHandler=this;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void SignalHandler::enablePM(void)
	{
	/* Check that this OS signal handler is not already enabled and still has an owner: */
	if(!enabled&&isOwned())
		{
		/* Enable this OS signal handler: */
		enabled=true;
		}
	}

void SignalHandler::disableInternal(bool block)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Disable this OS signal handler: */
		enabled=false;
		
		/* Check if we need to block, which in this context means that we must un-register from the low-level OS signal: */
		if(block)
			{
			/* Lock the OS signal handler table: */
			Threads::Mutex::Lock registeredSignalHandlersLock(registeredSignalHandlersMutex);
			
			/* Unregister this OS signal handler and its run loop: */
			registeredSignalHandlers[signum].runLoop=0;
			registeredSignalHandlers[signum].signalHandler=0;
			unref(); // Drop the reference to this OS signal handler from the OS signal handler table
			
			/* Return the signal to default disposition: */
			struct sigaction sigAction;
			memset(&sigAction,0,sizeof(struct sigaction));
			sigAction.sa_handler=SIG_DFL;
			if(sigaction(signum,&sigAction,0)<0)
				{
				/* Print an error message; nothing else we can do: */
				Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Cannot restore OS signal %d",signum);
				}
			}
		}
	else
		{
		/* Create a pipe message: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::DisableSignalHandler);
		pm.disableSignalHandler.signalHandler=this;
		
		/* Check if we need to block until the pipe message has been processed: */
		if(block)
			{
			/*******************************************************************
			Make a synchronous request by creating and locking a temporary
			condition variable, then writing to the self-pipe, then waiting on
			the condition variable:
			*******************************************************************/
			
			/* Create a temporary condition variable and put it into the pipe message: */
			RunLoop::TempCond tempCond;
			pm.disableSignalHandler.cond=&tempCond;
			
			/* Write to the self-pipe, but only wait on the condition variable if the run loop is still running: */
			if(runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this))
				tempCond.wait();
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableSignalHandler.cond=0;
			runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
			}
		}
	}

void SignalHandler::disablePM(bool unregister)
	{
	/* Disable this OS signal handler: */
	enabled=true;
	
	/* Check if the caller wants us to unregister the OS signal handler: */
	if(unregister)
		{
		/* Lock the OS signal handler table: */
		Threads::Mutex::Lock registeredSignalHandlersLock(registeredSignalHandlersMutex);
		
		/* Unregister this OS signal handler and its run loop: */
		registeredSignalHandlers[signum].runLoop=0;
		registeredSignalHandlers[signum].signalHandler=0;
		unref(); // Drop the reference to this OS signal handler from the OS signal handler table
		
		/* Return the OS signal to default disposition: */
		struct sigaction sigAction;
		memset(&sigAction,0,sizeof(struct sigaction));
		sigAction.sa_handler=SIG_DFL;
		if(sigaction(signum,&sigAction,0)<0)
			{
			/* Print an error message; nothing else we can do: */
			Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Cannot restore OS signal %d",signum);
			}
		}
	}

void SignalHandler::setEventHandlerPM(SignalHandlerEventHandler* newEventHandler)
	{
	/* Set this OS signal handler's event handler: */
	eventHandler=newEventHandler;
	}

void SignalHandler::registerRunLoop(RunLoop* runLoop,int signum)
	{
	/* Check if the signal number is valid: */
	if(signum<0||signum>maxSignal)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid OS signal number %d",signum);
	
	/* Lock the OS signal handler table: */
	{
	Threads::Mutex::Lock registeredSignalHandlersLock(registeredSignalHandlersMutex);
	
	/* Check if there is already a run loop registered for the given OS signal: */
	if(registeredSignalHandlers[signum].runLoop!=0)
		{
		if(registeredSignalHandlers[signum].runLoop!=runLoop)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled by another run loop",signum);
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled",signum);
		}
	
	/* Register the given run loop for the given OS signal: */
	registeredSignalHandlers[signum].runLoop=runLoop;
	registeredSignalHandlers[signum].signalHandler=0;
	
	/* Capture the given signal: */
	struct sigaction sigAction;
	memset(&sigAction,0,sizeof(struct sigaction));
	sigAction.sa_handler=signalHandlerFunction;
	if(sigaction(signum,&sigAction,0)<0)
		{
		/* Unregister the given run loop again and throw an exception: */
		registeredSignalHandlers[signum].runLoop=0;
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot intercept OS signal %d",signum);
		}
	}
	}

void SignalHandler::unregisterRunLoop(RunLoop* runLoop)
	{
	/* Lock the OS signal handler table: */
	Threads::Mutex::Lock registeredSignalHandlersLock(registeredSignalHandlersMutex);
	
	/* Unregister the given run loop from all OS signals for which it is registered: */
	for(int signum=0;signum<=maxSignal;++signum)
		if(registeredSignalHandlers[signum].runLoop==runLoop)
			{
			/* If there is an OS signal handler registered, drop the OS signal table's reference to it: */
			if(registeredSignalHandlers[signum].signalHandler!=0)
				registeredSignalHandlers[signum].signalHandler->unref();
			
			/* Return the OS signal to default disposition: */
			struct sigaction sigAction;
			memset(&sigAction,0,sizeof(struct sigaction));
			sigAction.sa_handler=SIG_DFL;
			if(sigaction(signum,&sigAction,0)<0)
				{
				/* Print an error message; nothing else we can do: */
				Misc::sourcedConsoleError(__PRETTY_FUNCTION__,"Cannot restore OS signal %d",signum);
				}
			}
	}

void SignalHandler::signalHandlerFunction(int signum)
	{
	/* Send a message to the run loop that registered for this signal: */
	if(registeredSignalHandlers[signum].runLoop!=0) // Just double-checking to minimize impacts from the race conditions inherent in OS signals
		{
		/* Create a pipe message: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::Signal);
		pm.signal.signum=signum;
		
		/* Make a blocking atomic write to the self-pipe, but don't bother checking for errors; nothing we can do: */
		int savedErrno=errno;
		write(registeredSignalHandlers[signum].runLoop->pipeFds[1],&pm,sizeof(RunLoop::PipeMessage));
		errno=savedErrno;
		}
	}
bool SignalHandler::signalPM(RunLoop* runLoop,int signum)
	{
	bool result=false;
	
	/* Retrieve the OS signal handler from the OS signal handler table: */
	bool isForRunLoop=false;
	SignalHandler* signalHandler=0;
	{
	Threads::Mutex::Lock registeredSignalHandlersLock(registeredSignalHandlersMutex);
	isForRunLoop=registeredSignalHandlers[signum].runLoop==runLoop;
	signalHandler=registeredSignalHandlers[signum].signalHandler;
	}
	
	/* Double-check that this signal is really for the run loop that called us: */
	if(isForRunLoop)
		{
		/* Check if there is a registered OS signal handler: */
		if(signalHandler!=0)
			{
			/* Call the OS signal handler if it is enabled: */
			if(signalHandler->enabled)
				{
				SignalHandlerEvent event(signalHandler,runLoop->lastDispatchTime,signum);
				(*signalHandler->eventHandler)(event);
				}
			}
		else
			{
			/* Tell the calling run loop to shut down: */
			result=true;
			}
		}
	
	return result;
	}

SignalHandler::SignalHandler(RunLoop& sRunLoop,int sSignum,bool sEnabled,SignalHandlerEventHandler& sEventHandler)
	:EventSource(sRunLoop),
	 signum(sSignum),eventHandler(&sEventHandler)
	{
	/* Check if the signal number is valid: */
	if(signum<0||signum>maxSignal)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid OS signal number %d",signum);
	
	/* Lock the OS signal handler table: */
	{
	Threads::Mutex::Lock registeredSignalHandlersLock(registeredSignalHandlersMutex);
	
	/* Check if there is already a run loop registered for the given OS signal: */
	if(registeredSignalHandlers[signum].runLoop!=0)
		{
		if(registeredSignalHandlers[signum].runLoop!=&runLoop)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled by another run loop",signum);
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"OS signal %d already handled",signum);
		}
	
	/* Register this OS signal handler and its run loop for the given OS signal: */
	registeredSignalHandlers[signum].runLoop=&runLoop;
	registeredSignalHandlers[signum].signalHandler=this;
	ref(); // Take a reference to this OS signal handler for the OS signal handler table
	
	/* Capture the given signal: */
	struct sigaction sigAction;
	memset(&sigAction,0,sizeof(struct sigaction));
	sigAction.sa_handler=signalHandlerFunction;
	if(sigaction(signum,&sigAction,0)<0)
		{
		/* Unregister this OS signal handler and its run loop again and throw an exception: */
		registeredSignalHandlers[signum].runLoop=0;
		registeredSignalHandlers[signum].signalHandler=0;
		unref(); // Drop the OS signal handler table's reference to this OS signal handler
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,errno,"Cannot intercept OS signal %d",signum);
		}
	}
	
	/* If the enabled flag is set, enable this signal handler immediately: */
	if(sEnabled)
		enabled=true;
	}

void SignalHandler::setEventHandler(SignalHandlerEventHandler& newEventHandler)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Replace this signal handler's event handler: */
		eventHandler=&newEventHandler;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetSignalHandlerEventHandler);
		pm.setSignalHandlerEventHandler.signalHandler=this;
		pm.setSignalHandlerEventHandler.eventHandler=&newEventHandler;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this,&newEventHandler);
		}
	}

}

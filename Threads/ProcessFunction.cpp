/***********************************************************************
ProcessFunction - Class for functions that are called synchronously
every time a run loop processes any events.
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

#include <Threads/ProcessFunction.h>

#include <Threads/FunctionCalls.h>
#include <Threads/RunLoop.h>
#include <Threads/RunLoopInternal.h>

namespace Threads {

/********************************
Methods of class ProcessFunction:
********************************/

void ProcessFunction::disowned(void)
	{
	/* Disable this process function synchronously, thus dropping all references to it and not sending further events: */
	disableInternal(true);
	}

void ProcessFunction::enableInternal(void)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this process function is not already enabled and still has an owner: */
		if(!enabled&&isOwned())
			{
			/* Append an entry for the process function to the end of the active process functions list: */
			runLoop.activeProcessFunctions.push_back(RunLoop::ActiveProcessFunction(this));
			activeIndex=runLoop.activeProcessFunctions.size()-1;
			ref(); // Take another reference to this process function object for the active process functions list
			
			/* Increase the number of spinning process functions if this process function wants to spin: */
			if(spinning)
				++runLoop.numSpinningProcessFunctions;
			
			/* Mark this process function as enabled: */
			enabled=true;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::EnableProcessFunction);
		pm.enableProcessFunction.processFunction=this;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void ProcessFunction::enablePM(void)
	{
	/* Check that this process function is not already enabled and still has an owner: */
	if(!enabled&&isOwned())
		{
		/* Append an entry for this process function to the end of the active process functions list: */
		runLoop.activeProcessFunctions.push_back(RunLoop::ActiveProcessFunction(this));
		activeIndex=runLoop.activeProcessFunctions.size()-1;
		ref(); // Take a reference to this process function for the active process functions list
		
		/* Increase the number of spinning process functions if this process function wants to spin: */
		if(spinning)
			++runLoop.numSpinningProcessFunctions;
		
		/* Mark the process function as enabled: */
		enabled=true;
		}
	}

void ProcessFunction::disableInternal(bool block)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that this process function is not already disabled: */
		if(enabled)
			{
			/*****************************************************************
			Shuffle the active process functions in the list such that the one
			that actually has to be removed is the last entry. This guarantees
			that disabling a process function is an O(1) operation.
			We have to take extra care if the one to be removed is before the
			one that is currently handled by event dispatching, to avoid
			missing events for the process function that is currently the last
			entry.
			*****************************************************************/
			
			/* Check if the run loop is currently handling process functions and this one is in the list not after the currently-handled one: */
			unsigned int apfi=activeIndex; // List index of this process function
			unsigned int hpfi=runLoop.handledProcessFunctionIndex; // List index of the currently handled process function
			if(runLoop.handlingProcessFunctions&&apfi<=hpfi)
				{
				/* Move the currently-handled process function to the place of this one: */
				runLoop.activeProcessFunctions[apfi]=runLoop.activeProcessFunctions[hpfi];
				runLoop.activeProcessFunctions[apfi].processFunction->activeIndex=apfi;
				
				/* Move the last process function in the lists to the place of the currently-handled one: */
				runLoop.activeProcessFunctions[hpfi]=runLoop.activeProcessFunctions[runLoop.activeProcessFunctions.size()-1];
				runLoop.activeProcessFunctions[hpfi].processFunction->activeIndex=hpfi;
				
				/* Reduce the currently-handled index to handle the process function that used to be the last in the lists next: */
				--runLoop.handledProcessFunctionIndex;
				}
			else
				{
				/* Move the last process function in the list to the place of this one: */
				runLoop.activeProcessFunctions[apfi]=runLoop.activeProcessFunctions[runLoop.activeProcessFunctions.size()-1];
				runLoop.activeProcessFunctions[apfi].processFunction->activeIndex=apfi;
				}
			
			/* Remove the now unused last entry from the active process function list: */
			runLoop.activeProcessFunctions.pop_back();
			
			/* Decrease the number of spinning process functions if this process function wants to spin: */
			if(spinning)
				--runLoop.numSpinningProcessFunctions;
			
			/* Mark this process function as disabled: */
			enabled=false;
			
			/* Drop the active process function list's reference to this process function object: */
			unref();
			}
		}
	else
		{
		/* Create a pipe message: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::DisableProcessFunction);
		pm.disableProcessFunction.processFunction=this;
		
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
			pm.disableProcessFunction.cond=&tempCond;
			
			/* Write to the self-pipe, but only wait on the condition variable if the run loop is still running: */
			if(runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this))
				tempCond.wait();
			}
		else
			{
			/* Make an asynchronous request by writing to the self-pipe: */
			pm.disableProcessFunction.cond=0;
			runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
			}
		}
	}

void ProcessFunction::disablePM(void)
	{
	/* Check that this process function is not already disabled: */
	if(enabled)
		{
		/* Replace this process function's entry in the active process function list with the last entry in the list, then drop the last entry: */
		unsigned int apfi=activeIndex;
		runLoop.activeProcessFunctions[apfi]=runLoop.activeProcessFunctions[runLoop.activeProcessFunctions.size()-1];
		runLoop.activeProcessFunctions[apfi].processFunction->activeIndex=apfi;
		runLoop.activeProcessFunctions.pop_back();
		
		/* Decrease the number of spinning process functions if this process function wants to spin: */
		if(spinning)
			--runLoop.numSpinningProcessFunctions;
		
		/* Mark this process function as disabled: */
		enabled=false;
		
		/* Drop the active process function list's reference to this process function: */
		unref();
		}
	}

void ProcessFunction::setSpinningPM(bool newSpinning)
	{
	/* Check that this process function is not already spinning: */
	if(spinning!=newSpinning)
		{
		/* Check if this process function is currently enabled: */
		if(enabled)
			{
			/* Update the number of spinning process functions: */
			if(newSpinning)
				++runLoop.numSpinningProcessFunctions;
			else
				--runLoop.numSpinningProcessFunctions;
			}
		
		/* Set this process function's spinning flag: */
		spinning=newSpinning;
		}
	}

void ProcessFunction::setFunctionPM(ProcessFunctionFunction* newFunction)
	{
	/* Replace this process function's function: */
	function=newFunction;
	}

ProcessFunction::ProcessFunction(RunLoop& sRunLoop,bool sSpinning,bool sEnabled,ProcessFunctionFunction& sFunction)
	:EventSource(sRunLoop),
	 spinning(sSpinning),function(&sFunction)
	{
	/* If the enabled flag is set, enable this process function immediately: */
	if(sEnabled)
		enableInternal();
	}

void ProcessFunction::setSpinning(bool newSpinning)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Check that the spinning state actually changed: */
		if(spinning!=newSpinning)
			{
			/* If this process function is currently enabled, update the number of spinning process functions: */
			if(enabled)
				{
				if(newSpinning)
					++runLoop.numSpinningProcessFunctions;
				else
					--runLoop.numSpinningProcessFunctions;
				}
			
			/* Update this process function's spinning state: */
			spinning=newSpinning;
			}
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetProcessFunctionSpinning);
		pm.setProcessFunctionSpinning.processFunction=this;
		pm.setProcessFunctionSpinning.spinning=newSpinning;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this);
		}
	}

void ProcessFunction::setFunction(ProcessFunctionFunction& newFunction)
	{
	/* Check if this call was made from inside the run loop's thread: */
	if(Threads::Thread::isSelfEqual(runLoop.threadId))
		{
		/* Replace this process function's function: */
		function=&newFunction;
		}
	else
		{
		/* Make an asynchronous request by writing to the self-pipe: */
		RunLoop::PipeMessage pm(RunLoop::PipeMessage::SetProcessFunctionFunction);
		pm.setProcessFunctionFunction.processFunction=this;
		pm.setProcessFunctionFunction.function=&newFunction;
		runLoop.writePipeMessage(pm,__PRETTY_FUNCTION__,this,&newFunction);
		}
	}

}

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

#ifndef THREADS_PROCESSFUNCTION_INCLUDED
#define THREADS_PROCESSFUNCTION_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/EventTypes.h>

namespace Threads {

class ProcessFunction; // Forward declaration
typedef Threads::FunctionCall<ProcessFunction&> ProcessFunctionFunction; // Type for process function functions

class ProcessFunction:public EventSource
	{
	friend class RunLoop;
	
	/* Elements: */
	private:
	bool spinning; // Flag if this process function should be called continuously, preventing blocking in the run loop
	Misc::Autopointer<ProcessFunctionFunction> function; // The function called by the process function
	unsigned int activeIndex; // Index of an enabled process function's entry in the run loop's active process function list
	
	/* Protected methods from class Ownable: */
	virtual void disowned(void);
	
	/* New private methods: */
	void enableInternal(void); // Enables this process function in its run loop
	void enablePM(void);  // Enables this process function in response to a received pipe message
	void disableInternal(bool block); // Disables this process function in its run loop; if flag is true, blocks until it has actually been disabled
	void disablePM(void); // Disables this process function in response to a received pipe message
	void setSpinningPM(bool newSpinning); // Sets this process function's spinning request in response to a received pipe message
	void setFunctionPM(ProcessFunctionFunction* newFunction); // Sets this process function's function in response to a received pipe message
	
	/* Constructors and destructors: */
	public:
	ProcessFunction(RunLoop& sRunLoop,bool sSpinning,bool sEnabled,ProcessFunctionFunction& sFunction); // Elementwise constructor
	
	/* Methods: */
	public:
	bool isSpinning(void) const // Returns true if the process function wants to be called continuously, preventing blocking in the run loop
		{
		return spinning;
		}
	void enable(void) // Enables the process function
		{
		/* Delegate to the internal method: */
		enableInternal();
		}
	void disable(void) // Disables the process function
		{
		/* Delegate to the internal method: */
		disableInternal(false);
		}
	void setEnabled(bool newEnabled) // Sets the process function's enabled state
		{
		/* Delegate to the internal methods: */
		if(newEnabled)
			enableInternal();
		else
			disableInternal(false);
		}
	void setSpinning(bool newSpinning); // Sets the process function's spinning request
	void setFunction(ProcessFunctionFunction& newFunction); // Sets the process function's function
	};

typedef OwningPointer<ProcessFunction> ProcessFunctionOwner; // Type for ownership-establishing pointers to process functions
typedef Misc::Autopointer<ProcessFunction> ProcessFunctionPtr; // Type for non-ownership-establishing pointers to process functions

}

#endif

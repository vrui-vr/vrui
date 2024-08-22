/***********************************************************************
Pipe - Base class derived from IO::File for files representing pipes
supporting waiting and automatic endianness negotiation.
Copyright (c) 2011-2024 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Comm/Pipe.h>

#include <signal.h>
#include <Misc/StdError.h>

namespace Comm {

/*********************
Methods of class Pipe:
*********************/

void Pipe::negotiateEndianness(void)
	{
	/* Disable endianness swapping on write (receiver makes it right): */
	writeMustSwapEndianness=false;
	
	/* Write an endianness indicator: */
	write<unsigned int>(0x12345678U);
	flush();
	
	/* Temporarily disable endianness swapping on read: */
	readMustSwapEndianness=false;
	
	/* Read the other side's endianness indicator: */
	unsigned int otherEndiannessIndicator=read<unsigned int>();
	if(otherEndiannessIndicator==0x78563412U)
		readMustSwapEndianness=true;
	else if(otherEndiannessIndicator!=0x12345678U)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to negotiate endianness");
	}

void Pipe::shutdown(bool read,bool write)
	{
	/* Don't do anything */
	}

void ignorePipeSignals(void)
	{
	struct sigaction sigPipeAction;
	sigPipeAction.sa_handler=SIG_IGN;
	sigemptyset(&sigPipeAction.sa_mask);
	sigPipeAction.sa_flags=0x0;
	sigaction(SIGPIPE,&sigPipeAction,0);
	}

}

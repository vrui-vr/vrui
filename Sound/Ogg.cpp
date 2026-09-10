/***********************************************************************
Ogg - Wrapper classes for structures in the Ogg multimedia container
format library.
Copyright (c) 2010-2026 Oliver Kreylos

This file is part of the Basic Sound Library (Sound).

The Basic Sound Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Sound Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Sound Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Sound/Ogg.h>

#include <Misc/StdError.h>

namespace Sound {

namespace Ogg {

/***********************
Methods of class Stream:
***********************/

Stream::Stream(int serialNumber)
	{
	/* Initialize the ogg_stream_state: */
	if(ogg_stream_init(this,serialNumber)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error in ogg_stream_init");
	}

Stream::~Stream(void)
	{
	/* Clear the ogg_stream_state; do not call ogg_stream_destroy because the delete will release the memory: */
	ogg_stream_clear(this);
	}

void Stream::reset(void)
	{
	/* Reset the stream: */
	if(ogg_stream_reset(this)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error in ogg_stream_reset");
	}

void Stream::reset(int newSerialNumber)
	{
	/* Reset the stream and change the serial number: */
	if(ogg_stream_reset_serialno(this,newSerialNumber)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error in ogg_stream_reset_serialno");
	}

/*********************
Methods of class Sync:
*********************/

Sync::Sync(void)
	{
	/* Initialize the ogg_sync_state: */
	if(ogg_sync_init(this)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error in ogg_sync_init");
	}

Sync::~Sync(void)
	{
	/* Clear the ogg_sync_state; do not call ogg_sync_destroy because the delete will release the memory: */
	ogg_sync_clear(this);
	}

void Sync::reset(void)
	{
	if(ogg_sync_reset(this)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error in ogg_sync_reset");
	}

void* Sync::getBuffer(long bufferSize)
	{
	void* result=ogg_sync_buffer(this,bufferSize);
	if(result==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error in ogg_sync_buffer");
	return result;
	}

void Sync::setDataSize(long dataSize)
	{
	if(ogg_sync_wrote(this,dataSize)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error in ogg_sync_wrote");
	}

}

}

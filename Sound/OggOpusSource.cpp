/***********************************************************************
OggOpusSource - Class to read Opus-encoded audio data from an Ogg file.
Copyright (c) 2026 Oliver Kreylos

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

#include <Sound/OggOpusSource.h>

#include <stddef.h>
#include <string.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <IO/MemoryReader.h>
#include <Sound/Ogg.h>

namespace Sound {

/******************************
Methods of class OggOpusSource:
******************************/

Sound::Ogg::Page OggOpusSource::readPage(void)
	{
	Sound::Ogg::Page result;
	
	/* Ask the sync object for an Ogg page; feed it more data if it doesn't have one yet: */
	while(!sync->pageOut(result))
		{
		/* Read from the source directly into the sync object's internal buffer, ideally bypassing the source's buffer: */
		sync->setDataSize(source->readUpTo(sync->getBuffer(sourceBufferSize),sourceBufferSize));
		}
	
	return result;
	}

namespace {

/****************
Helper functions:
****************/

bool readString(IO::File& source,std::string& string) // Reads a string from an Opus comment header
	{
	/* Read the string's length: */
	bool result=source.getUnreadDataSize()>=sizeof(Misc::UInt32);
	size_t stringLength=0;
	if(result)
		stringLength=source.read<Misc::UInt32>();
	
	/* Read the string's characters: */
	void* stringPtr;
	result=result&&source.readInBuffer(stringPtr,stringLength)==stringLength;
	
	/* Construct the result string: */
	if(result)
		string=std::string(static_cast<char*>(stringPtr),static_cast<char*>(stringPtr)+stringLength);
	
	return result;
	}

}

OggOpusSource::OggOpusSource(IO::File& sSource,OggOpusSource::OpusInfo& opusInfo)
	:source(&sSource),sourceBufferSize(source->getReadBufferSize()),
	 sync(new Sound::Ogg::Sync),
	 stream(0)
	{
	/* Mark the sync object for self-destruction: */
	Misc::SelfDestructPointer<Sound::Ogg::Sync> syncDestroyer(sync);
	
	/* Keep track if the Ogg bitstream is a valid Opus stream: */
	bool streamOk=true;
	
	/* Retrieve the first Ogg page from the source and check that it's the beginning of a stream: */
	Sound::Ogg::Page page=readPage();
	streamOk=page.isBos();
	
	/* Create an Ogg stream with the first page's serial number: */
	if(streamOk)
		stream=new Sound::Ogg::Stream(page.getSerialNumber());
	
	/* Mark the stream for self-destruction: */
	Misc::SelfDestructPointer<Sound::Ogg::Stream> streamDestroyer(stream);
	
	/*********************************************************************
	Read, process, and check the Opus ID header:
	*********************************************************************/
	
	{
	/* Retrieve the first Ogg packet, which must contain the Opus ID header: */
	Sound::Ogg::Packet packet(0,0,0,0,0,0);
	streamOk=streamOk&&stream->pageIn(page);
	streamOk=streamOk&&stream->packetOut(packet);
	
	/* Check and process the Opus ID header: */
	IO::MemoryReader opusIdHeader(packet.packet,packet.bytes);
	opusIdHeader.setEndianness(Misc::LittleEndian);
	
	/* Check the Opus ID header ID: */
	if(streamOk)
		{
		void* packetId;
		streamOk=opusIdHeader.readInBuffer(packetId,8)==8&&memcmp(packetId,"OpusHead",8)==0;
		}
	
	/* Check the Opus ID header version: */
	streamOk=streamOk&&opusIdHeader.getUnreadDataSize()>=sizeof(Misc::UInt8)&&opusIdHeader.read<Misc::UInt8>()==1U;
	
	/* Retrieve and check the source number of channels: */
	streamOk=streamOk&&opusIdHeader.getUnreadDataSize()>=sizeof(Misc::UInt8);
	if(streamOk)
		{
		opusInfo.numChannels=opusIdHeader.read<Misc::UInt8>();
		streamOk=opusInfo.numChannels>=1&&opusInfo.numChannels<=2;
		}
	
	/* Retrieve the pre-skip amount: */
	streamOk=streamOk&&opusIdHeader.getUnreadDataSize()>=sizeof(Misc::UInt16);
	if(streamOk)
		opusInfo.preSkip=opusIdHeader.read<Misc::UInt16>();
	
	/* Retrieve the source sample frequency: */
	streamOk=streamOk&&opusIdHeader.getUnreadDataSize()>=sizeof(Misc::UInt32);
	if(streamOk)
		opusInfo.sampleFrequency=opusIdHeader.read<Misc::UInt32>();
	
	/* Retrieve the intended output gain: */
	streamOk=streamOk&&opusIdHeader.getUnreadDataSize()>=sizeof(Misc::SInt16);
	if(streamOk)
		opusInfo.outputGainDb=double(opusIdHeader.read<Misc::SInt16>())/(2.0*256.0);
	
	/* Check the channel map family: */
	streamOk=streamOk&&opusIdHeader.getUnreadDataSize()>=sizeof(Misc::UInt8)&&opusIdHeader.read<Misc::UInt8>()==0U;
	
	/* Check that the Opus ID header is complete and the only packet in the first page: */
	streamOk=streamOk&&opusIdHeader.eof()&&!stream->packetOut(packet);
	}
	
	/*********************************************************************
	Read, process, and check the Opus comment header:
	*********************************************************************/
	
	{
	/* Retrieve the next Ogg packet, which must contain the Opus comment header: */
	Sound::Ogg::Packet packet(0,0,0,0,0,0);
	while(streamOk&&!stream->packetOut(packet))
		{
		/* Feed another Ogg page from the source into the Ogg stream: */
		streamOk=stream->pageIn(readPage());
		}
	
	/* Check and process the Opus comment header: */
	IO::MemoryReader opusCommentHeader(packet.packet,packet.bytes);
	opusCommentHeader.setEndianness(Misc::LittleEndian);
	
	/* Check the Opus comment header ID: */
	if(streamOk)
		{
		void* packetId;
		streamOk=opusCommentHeader.readInBuffer(packetId,8)==8&&memcmp(packetId,"OpusTags",8)==0;
		}
	
	/* Read the vendor string: */
	streamOk=streamOk&&readString(opusCommentHeader,opusInfo.opusVendor);
	
	/* Read the number of comments: */
	streamOk=streamOk&&opusCommentHeader.getUnreadDataSize()>=sizeof(Misc::UInt32);
	size_t numComments=0;
	if(streamOk)
		numComments=opusCommentHeader.read<Misc::UInt32>();
	for(size_t i=0;i<numComments;++i)
		{
		/* Read a comment: */
		std::string comment;
		streamOk=streamOk&&readString(opusCommentHeader,comment);
		if(streamOk)
			opusInfo.comments.push_back(std::move(comment));
		}
	
	/* Check that the Opus comment header is complete and the only packet in the last page: */
	streamOk=streamOk&&opusCommentHeader.eof()&&!stream->packetOut(packet);
	}
	
	/* Throw an exception if the Opus stream was invalid: */
	if(!streamOk)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source does not contain a valid Opus audio stream");
	
	/* Release the sync object and stream from destruction: */
	syncDestroyer.releaseTarget();
	streamDestroyer.releaseTarget();
	}

OggOpusSource::~OggOpusSource(void)
	{
	/* Destroy the Ogg stream and sync object: */
	delete stream;
	delete sync;
	}

Ogg::Packet OggOpusSource::packetOut(void)
	{
	Ogg::Packet result;
	
	/* Retrieve the next packet from the Ogg stream: */
	while(!stream->packetOut(result))
		{
		/* Feed another Ogg page from the source into the Ogg stream: */
		stream->pageIn(readPage());
		}
	
	return result;
	}

}

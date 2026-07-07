/***********************************************************************
OggOpusSink - Class to write Opus-encoded audio data to an Ogg file.
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

#include <Sound/OggOpusSink.h>

#include <math.h>
#include <Misc/Utility.h>
#include <Misc/SelfDestructPointer.h>
#include <IO/FixedMemoryFile.h>
#include <Sound/Ogg.h>
#include <opus/opus.h>
#include <Sound/OpusEncoder.h>

namespace Sound {

/*****************************************
Methods of class OggOpusSink::Initializer:
*****************************************/

OggOpusSink::Initializer::Initializer(const OpusEncoder& encoder)
	:serialNumber(0),preSkipMs(0),outputGainDb(0)
	{
	/* Initialize pre-skip to the encoder's lookahead size expressed in ms: */
	preSkipMs=(unsigned int)(encoder.getLookaheadSamples())*1000U/(unsigned int)(encoder.getSampleFrequency());
	}

void OggOpusSink::Initializer::setSerialNumber(int newSerialNumber)
	{
	serialNumber=newSerialNumber;
	}

void OggOpusSink::Initializer::setPreSkipMs(unsigned int newPreSkipMs)
	{
	preSkipMs=newPreSkipMs;
	}

void OggOpusSink::Initializer::setOutputGainDb(double newOutputGainDb)
	{
	outputGainDb=newOutputGainDb;
	}

void OggOpusSink::Initializer::setOutputGainLinear(double newOutputGain)
	{
	/* Check for valid gain range: */
	if(newOutputGain>0.0)
		outputGainDb=log10(newOutputGain)*10.0;
	else
		outputGainDb=-1000.0; // Set a very small output gain
	}

void OggOpusSink::Initializer::addComment(const std::string& newComment)
	{
	comments.push_back(newComment);
	}

void OggOpusSink::Initializer::addComment(std::string&& newComment)
	{
	comments.push_back(std::move(newComment));
	}

/****************************
Methods of class OggOpusSink:
****************************/

OggOpusSink::OggOpusSink(OpusEncoder& sEncoder,IO::File& sSink,const OggOpusSink::Initializer& initializer)
	:encoder(sEncoder),sink(&sSink),
	 oggStream(0),
	 granulePos(0),packetNo(0)
	{
	/* Create the Ogg stream and wrap it for self-destruction in case something goes wrong: */
	Misc::SelfDestructPointer<Ogg::Stream> os(new Ogg::Stream(initializer.serialNumber));
	
	/*********************************************************************
	Write the OGG/Opus file header to the OGG/Opus file:
	*********************************************************************/
	
	{
	/* Create an Opus ID header: */
	IO::FixedMemoryFile opusIdHeader(20);
	opusIdHeader.setEndianness(Misc::LittleEndian);
	opusIdHeader.writeRaw("OpusHead",8);
	opusIdHeader.write(Misc::UInt8(1));
	opusIdHeader.write(Misc::UInt8(encoder.getNumChannels()));
	
	/* Write the number of pre-skip frames at nominal 48 kHz sample frequency: */
	unsigned int sampleFrequency=encoder.getSampleFrequency();
	unsigned int preSkip=Misc::min((initializer.preSkipMs*sampleFrequency+500U)/1000U,65535U);
	opusIdHeader.write(Misc::UInt16(preSkip));
	
	/* Write the source sample frequency: */
	opusIdHeader.write(Misc::UInt32(sampleFrequency));
	
	/* Write the output gain as an 8:8 signed integer: */
	opusIdHeader.write(Misc::SInt16(Misc::clamp(floor(initializer.outputGainDb*2.0*256.0+0.5),-32768.0,32767.0)));
	
	/* Write the channel mapping family: */
	opusIdHeader.write(Misc::UInt8(0));
	
	/* Write the Opus ID header to the sink as a complete Ogg page: */
	Ogg::Packet opusIdHeaderPacket(opusIdHeader.getMemory(),opusIdHeader.getWriteSize(),1,0,granulePos,packetNo++);
	os->packetIn(opusIdHeaderPacket);
	Ogg::Page opusIdHeaderPage;
	if(os->flush(opusIdHeaderPage))
		opusIdHeaderPage.write(*sink);
	}
	
	{
	/* Calculate the total size of the Opus comment header: */
	const char* vendorString=opus_get_version_string();
	size_t vendorStringLength=strlen(vendorString);
	size_t commentHeaderSize=8+sizeof(Misc::UInt32)+vendorStringLength+sizeof(Misc::UInt32);
	for(std::vector<std::string>::const_iterator cIt=initializer.comments.begin();cIt!=initializer.comments.end();++cIt)
		commentHeaderSize+=sizeof(Misc::UInt32)+cIt->length();
	
	/* Create an Opus comment header: */
	IO::FixedMemoryFile opusCommentHeader(commentHeaderSize);
	opusCommentHeader.writeRaw("OpusTags",8);
	opusCommentHeader.write(Misc::UInt32(vendorStringLength));
	opusCommentHeader.writeRaw(vendorString,vendorStringLength);
	
	/* Write all comments: */
	opusCommentHeader.write(Misc::UInt32(initializer.comments.size()));
	for(std::vector<std::string>::const_iterator cIt=initializer.comments.begin();cIt!=initializer.comments.end();++cIt)
		{
		size_t length=cIt->length();
		opusCommentHeader.write(Misc::UInt32(length));
		opusCommentHeader.writeRaw(cIt->data(),length);
		}
	
	/* Write the Opus comment header to the sink as one or more complete Ogg pages: */
	Ogg::Packet opusCommentHeaderPacket(opusCommentHeader.getMemory(),opusCommentHeader.getWriteSize(),0,0,granulePos,packetNo++);
	os->packetIn(opusCommentHeaderPacket);
	Ogg::Page opusCommentHeaderPage;
	while(os->flush(opusCommentHeaderPage))
		opusCommentHeaderPage.write(*sink);
	}
	
	/* Remember the Ogg stream: */
	oggStream=os.releaseTarget();
	}

OggOpusSink::~OggOpusSink(void)
	{
	/* Destroy the Ogg stream: */
	delete oggStream;
	}

void OggOpusSink::writePacket(size_t numFrames,size_t packetSize)
	{
	/* Wrap the encoder's current packet in an Ogg packet and append it to the Ogg stream: */
	Ogg::Packet oggPacket(encoder.getPacket(),packetSize,0,0,granulePos+=numFrames,packetNo++);
	oggStream->packetIn(oggPacket);
	
	/* Write any filled pages in the Ogg stream to the sink: */
	Ogg::Page oggPage;
	while(oggStream->pageOut(oggPage))
		oggPage.write(*sink);
	}

void OggOpusSink::encodeChunk(const Misc::SInt16* chunkData,size_t numChunkFrames)
	{
	/* Encode the PCM data chunk: */
	size_t packetSize=encoder.encodeChunk(chunkData,numChunkFrames);
	
	/* Wrap the just-encoded packet in an Ogg packet and append it to the Ogg stream: */
	Ogg::Packet oggPacket(encoder.getPacket(),packetSize,0,0,granulePos+=numChunkFrames,packetNo++);
	oggStream->packetIn(oggPacket);
	
	/* Write any filled pages in the Ogg stream to the sink: */
	Ogg::Page oggPage;
	while(oggStream->pageOut(oggPage))
		oggPage.write(*sink);
	}

void OggOpusSink::flush(void)
	{
	/* Write any remaining pages in the Ogg stream to the sink: */
	Ogg::Page oggPage;
	while(oggStream->flush(oggPage))
		oggPage.write(*sink);
	
	/* Flush the sink: */
	sink->flush();
	}

}

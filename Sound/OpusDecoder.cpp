/***********************************************************************
OpusDecoder - Class encapsulating an audio decoder using the Opus codec.
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

#include <Sound/OpusDecoder.h>

#include <math.h>
#include <Misc/Utility.h>
#include <Misc/StdError.h>
#include <Sound/Ogg.h>
#include <opus/opus.h>

namespace Sound {

/****************************
Methods of class OpusDecoder:
****************************/

OpusDecoder::OpusDecoder(int sSampleFrequency,int sNumChannels)
	:sampleFrequency(sSampleFrequency),numChannels(sNumChannels),
	 decoder(0)
	{
	/* Create the Opus decoder and check for errors: */
	int opusError;
	decoder=opus_decoder_create(sampleFrequency,numChannels,&opusError);
	if(opusError!=OPUS_OK)
		{
		decoder=0;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_decoder_create",opusError,opus_strerror(opusError));
		}
	}

OpusDecoder::~OpusDecoder(void)
	{
	/* Destroy the Opus decoder: */
	if(decoder!=0)
		opus_decoder_destroy(decoder);
	}

size_t OpusDecoder::getMaxNumChunkFrames(void) const
	{
	/* The maximum specified Opus packet duration is 120 ms; calculate the maximum number of frames from that: */
	return (size_t(sampleFrequency)*120U+999U)/1000U;
	}

int OpusDecoder::getBandwidth(void) const
	{
	opus_int32 bandwidth;
	opus_decoder_ctl(decoder,OPUS_GET_BANDWIDTH(&bandwidth));
	switch(bandwidth)
		{
		case OPUS_BANDWIDTH_NARROWBAND:
			return 4000;
		
		case OPUS_BANDWIDTH_MEDIUMBAND:
			return 6000;
		
		case OPUS_BANDWIDTH_WIDEBAND:
			return 8000;
		
		case OPUS_BANDWIDTH_SUPERWIDEBAND:
			return 12000;
		
		case OPUS_BANDWIDTH_FULLBAND:
			return 20000;
		
		default:
			return -1;
		}
	}

void OpusDecoder::reset(void)
	{
	opus_decoder_ctl(decoder,OPUS_RESET_STATE);
	}

double OpusDecoder::getGain(void) const
	{
	opus_int32 rawGain;
	opus_decoder_ctl(decoder,OPUS_GET_GAIN(&rawGain));
	return double(rawGain)/(2.0*256.0);
	}

void OpusDecoder::setGain(double newGainDb)
	{
	opus_int32 rawGain(Misc::clamp(floor(newGainDb*(2.0*256.0)+0.5),-32768.0,32767.0));
	opus_decoder_ctl(decoder,OPUS_SET_GAIN(rawGain));
	}

int OpusDecoder::getBandwidth(const void* packetData) const
	{
	int bandwidth=opus_packet_get_bandwidth(static_cast<const unsigned char*>(packetData));
	switch(bandwidth)
		{
		case OPUS_BANDWIDTH_NARROWBAND:
			return 4000;
		
		case OPUS_BANDWIDTH_MEDIUMBAND:
			return 6000;
		
		case OPUS_BANDWIDTH_WIDEBAND:
			return 8000;
		
		case OPUS_BANDWIDTH_SUPERWIDEBAND:
			return 12000;
		
		case OPUS_BANDWIDTH_FULLBAND:
			return 20000;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_bandwidth",bandwidth,opus_strerror(bandwidth));
		}
	}

int OpusDecoder::getBandwidth(const Ogg::Packet& packet) const
	{
	int bandwidth=opus_packet_get_bandwidth(packet.packet);
	switch(bandwidth)
		{
		case OPUS_BANDWIDTH_NARROWBAND:
			return 4000;
		
		case OPUS_BANDWIDTH_MEDIUMBAND:
			return 6000;
		
		case OPUS_BANDWIDTH_WIDEBAND:
			return 8000;
		
		case OPUS_BANDWIDTH_SUPERWIDEBAND:
			return 12000;
		
		case OPUS_BANDWIDTH_FULLBAND:
			return 20000;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_bandwidth",bandwidth,opus_strerror(bandwidth));
		}
	}

int OpusDecoder::getNumChannels(const void* packetData) const
	{
	int numChannels=opus_packet_get_nb_channels(static_cast<const unsigned char*>(packetData));
	if(numChannels<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_nb_channels",numChannels,opus_strerror(numChannels));
	
	return numChannels;
	}

int OpusDecoder::getNumChannels(const Ogg::Packet& packet) const
	{
	int numChannels=opus_packet_get_nb_channels(packet.packet);
	if(numChannels<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_nb_channels",numChannels,opus_strerror(numChannels));
	
	return numChannels;
	}

size_t OpusDecoder::getNumChunks(const void* packetData,size_t packetSize) const
	{
	int numFrames=opus_packet_get_nb_frames(static_cast<const unsigned char*>(packetData),packetSize);
	if(numFrames<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_nb_frames",numFrames,opus_strerror(numFrames));
	
	return numFrames;
	}

size_t OpusDecoder::getNumChunks(const Ogg::Packet& packet) const
	{
	int numFrames=opus_packet_get_nb_frames(packet.packet,packet.bytes);
	if(numFrames<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_nb_frames",numFrames,opus_strerror(numFrames));
	
	return numFrames;
	}

int OpusDecoder::getNumFramesPerChunk(const void* packetData) const
	{
	int numFrames=opus_packet_get_samples_per_frame(static_cast<const unsigned char*>(packetData),sampleFrequency);
	if(numFrames<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_samples_per_frame",numFrames,opus_strerror(numFrames));
	
	return numFrames;
	}

int OpusDecoder::getNumFramesPerChunk(const Ogg::Packet& packet) const
	{
	int numFrames=opus_packet_get_samples_per_frame(packet.packet,sampleFrequency);
	if(numFrames<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_samples_per_frame",numFrames,opus_strerror(numFrames));
	
	return numFrames;
	}

size_t OpusDecoder::getNumChunkFrames(const void* packetData,size_t packetSize) const
	{
	int numSamples=opus_packet_get_nb_samples(static_cast<const unsigned char*>(packetData),packetSize,sampleFrequency);
	if(numSamples<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_nb_samples",numSamples,opus_strerror(numSamples));
	
	return size_t(numSamples);
	}

size_t OpusDecoder::getNumChunkFrames(const Ogg::Packet& packet) const
	{
	int numSamples=opus_packet_get_nb_samples(packet.packet,packet.bytes,sampleFrequency);
	if(numSamples<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_packet_get_nb_samples",numSamples,opus_strerror(numSamples));
	
	return size_t(numSamples);
	}

size_t OpusDecoder::decodePacket(const void* packetData,size_t packetSize,Misc::SInt16* chunkData,size_t numChunkFrames,bool useFec)
	{
	int numDecodedFrames=opus_decode(decoder,static_cast<const unsigned char*>(packetData),packetSize,chunkData,numChunkFrames,useFec?1:0);
	if(numDecodedFrames<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_decode",numDecodedFrames,opus_strerror(numDecodedFrames));
	
	return size_t(numDecodedFrames);
	}

size_t OpusDecoder::decodePacket(const Ogg::Packet& packet,Misc::SInt16* chunkData,size_t numChunkFrames,bool useFec)
	{
	int numDecodedFrames=opus_decode(decoder,packet.packet,packet.bytes,chunkData,numChunkFrames,useFec?1:0);
	if(numDecodedFrames<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_decode",numDecodedFrames,opus_strerror(numDecodedFrames));
	
	return size_t(numDecodedFrames);
	}

size_t OpusDecoder::fillInPacket(Misc::SInt16* chunkData,size_t numChunkFrames,bool useFec)
	{
	int numDecodedFrames=opus_decode(decoder,0,0,chunkData,numChunkFrames,useFec?1:0);
	if(numDecodedFrames<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_decode",numDecodedFrames,opus_strerror(numDecodedFrames));
	
	return size_t(numDecodedFrames);
	}

}

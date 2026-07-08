/***********************************************************************
OpusEncoder - Class encapsulating an audio encoder using the Opus codec.
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

#include <Sound/OpusEncoder.h>

#include <Misc/StdError.h>
#include <opus/opus.h>

namespace Sound {

/************************************
Static elements of class OpusEncoder:
************************************/

const int OpusEncoder::applicationModes[3]=
	{
	OPUS_APPLICATION_VOIP,OPUS_APPLICATION_AUDIO,OPUS_APPLICATION_RESTRICTED_LOWDELAY
	};

/****************************
Methods of class OpusEncoder:
****************************/

OpusEncoder::OpusEncoder(int sampleFrequency,int sNumChannels,OpusEncoder::ApplicationMode applicationMode)
	:numChannels(sNumChannels),
	 encoder(0),
	 codeBufferSize(4000), // Recommended by Opus documentation
	 codeBuffer(0)
	{
	/* Create the Opus encoder and check for errors: */
	int opusError;
	encoder=opus_encoder_create(sampleFrequency,numChannels,applicationModes[applicationMode],&opusError);
	if(opusError!=OPUS_OK)
		{
		encoder=0;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_encoder_create",opusError,opus_strerror(opusError));
		}
	
	/* Allocate the code buffer: */
	codeBuffer=new unsigned char[codeBufferSize];
	}

OpusEncoder::~OpusEncoder(void)
	{
	/* Destroy the code buffer: */
	delete[] codeBuffer;
	
	/* Destroy the Opus encoder: */
	if(encoder!=0)
		opus_encoder_destroy(encoder);
	}

int OpusEncoder::getSampleFrequency(void) const
	{
	opus_int32 sampleFrequency;
	opus_encoder_ctl(encoder,OPUS_GET_SAMPLE_RATE(&sampleFrequency));
	return int(sampleFrequency);
	}

int OpusEncoder::getBandwidth(void) const
	{
	opus_int32 bandwidth;
	opus_encoder_ctl(encoder,OPUS_GET_BANDWIDTH(&bandwidth));
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

void OpusEncoder::reset(void)
	{
	opus_encoder_ctl(encoder,OPUS_RESET_STATE);
	}

OpusEncoder::ApplicationMode OpusEncoder::getApplicationMode(void) const
	{
	opus_int32 applicationMode;
	opus_encoder_ctl(encoder,OPUS_GET_APPLICATION(&applicationMode));
	switch(applicationMode)
		{
		case OPUS_APPLICATION_VOIP:
			return VoIP;
		
		case OPUS_APPLICATION_AUDIO:
			return Audio;
		
		case OPUS_APPLICATION_RESTRICTED_LOWDELAY:
			return LowLatency;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid application model %d",int(applicationMode));
		}
	}

void OpusEncoder::setApplicationMode(OpusEncoder::ApplicationMode newApplicationMode)
	{
	opus_encoder_ctl(encoder,OPUS_SET_APPLICATION(opus_int32(applicationModes[newApplicationMode])));
	}

int OpusEncoder::getBitrate(void) const
	{
	opus_int32 bitrate;
	opus_encoder_ctl(encoder,OPUS_GET_BITRATE(&bitrate));
	return int(bitrate);
	}

void OpusEncoder::setBitrate(unsigned int newBitrate)
	{
	opus_encoder_ctl(encoder,OPUS_SET_BITRATE(opus_int32(newBitrate)));
	}

int OpusEncoder::getComplexity(void) const
	{
	opus_int32 complexity;
	opus_encoder_ctl(encoder,OPUS_GET_COMPLEXITY(&complexity));
	return int(complexity);
	}

void OpusEncoder::setComplexity(int newComplexity)
	{
	opus_encoder_ctl(encoder,OPUS_SET_COMPLEXITY(opus_int32(newComplexity)));
	}

int OpusEncoder::getLookaheadSamples(void) const
	{
	opus_int32 lookahead;
	opus_encoder_ctl(encoder,OPUS_GET_LOOKAHEAD(&lookahead));
	return int(lookahead);
	}

size_t OpusEncoder::encodeChunk(const Misc::SInt16* chunkData,size_t numChunkFrames)
	{
	/* Encode the received chunk of raw PCM data and check for errors: */
	int encodedSize=opus_encode(encoder,chunkData,numChunkFrames,codeBuffer,codeBufferSize);
	if(encodedSize<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s) in opus_encode",encodedSize,opus_strerror(encodedSize));
	
	/* Return the encoded size: */
	return size_t(encodedSize);
	}

}

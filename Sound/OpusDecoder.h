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

#ifndef SOUND_OPUSDECODER_INCLUDED
#define SOUND_OPUSDECODER_INCLUDED

#include <stddef.h>
#include <Misc/SizedTypes.h>

/* Forward declarations: */
struct OpusDecoder;

namespace Sound {

class OpusDecoder
	{
	/* Embedded classes: */
	public:
	
	/* Elements: */
	private:
	int numChannels; // Number of channels in destination audio data (1 or 2)
	::OpusDecoder* decoder; // Pointer to the Opus decoder object
	
	/* Constructors and destructors: */
	public:
	OpusDecoder(int sampleFrequency,int sNumChannels); // Creates an Opus decoder for the given destination sound format
	~OpusDecoder(void);
	
	/* Methods: */
	int getSampleFrequency(void) const; // Returns the destination sound format's sample frequency in Hz
	int getNumChannels(void) const // Returns the destination sound format's number of channels (1 or 2)
		{
		return numChannels;
		}
	int getBandwidth(void) const; // Returns the decoder's current bandwidth in Hz, or a negative value if the bandwidth is not yet determined
	void reset(void); // Resets the decoder's state; should be called between using a decoder to decode unrelated sound clips
	
	/* Decoder control methods, can be called at any time: */
	double getGain(void) const; // Returns the decoder's output gain in dB
	void setGain(double newGainDb); // Sets the decoder's output gain in dB
	
	/* Decoding methods: */
	size_t getMaxNumChunkFrames(void) const; // Returns the maximum number of PCM frames that can be returned by a call to decodePacket with current decoder settings
	size_t decodePacket(const void* packet,size_t packetSize,Misc::SInt16* chunkData,size_t numChunkFrames,bool useFec); // Decodes an opus packet into the chunk of PCM data able to hold up to the given number of frames; attempts to use forward error correction if the given flag is true; returns the number of frames in the decoded PCM chunk
	size_t fillInPacket(Misc::SInt16* chunkData,size_t numChunkFrames,bool useFec); // Returns a chunk of PCM data to conceal the loss of an Opus packet; returns the number of frames in the filled-in PCM chunk
	};

}

#endif

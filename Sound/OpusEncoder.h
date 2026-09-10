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

#ifndef SOUND_OPUSENCODER_INCLUDED
#define SOUND_OPUSENCODER_INCLUDED

#include <stddef.h>
#include <Misc/SizedTypes.h>

/* Forward declarations: */
struct OpusEncoder;

namespace Sound {

class OpusEncoder
	{
	/* Embedded classes: */
	public:
	enum ApplicationMode // Enumerated types for Opus application modes
		{
		VoIP=0, // For encoding voice data for voice-over-IP applications
		Audio, // For generic audio data (voice and/or music)
		LowLatency // For minimum-latency encoding/decoding
		};
	
	/* Elements: */
	private:
	static const int applicationModes[3]; // Opus API codes for the three application modes
	int numChannels; // Number of channels in source audio data (1 or 2)
	::OpusEncoder* encoder; // Pointer to the Opus encoder object
	size_t codeBufferSize; // Size of the buffer holding encoded audio data in bytes
	unsigned char* codeBuffer; // Buffer holding encoded audio data
	
	/* Constructors and destructors: */
	public:
	OpusEncoder(int sampleFrequency,int sNumChannels,ApplicationMode applicationMode); // Creates an Opus encoder for the given source sound format and Opus application mode, with default bitrate and encoder complexity
	~OpusEncoder(void);
	
	/* Methods: */
	int getSampleFrequency(void) const; // Returns the source sound format's sample frequency in Hz
	int getNumChannels(void) const // Returns the source sound format's number of channels (1 or 2)
		{
		return numChannels;
		}
	int getBandwidth(void) const; // Returns the encoder's configured bandwidth in Hz, or a negative value if the encoder is configured for automatic bandwidth detection
	void reset(void); // Resets the encoder's state; should be called between using an encoder to encode unrelated sound clips
	
	/* Encoder control methods, can be called at any time: */
	ApplicationMode getApplicationMode(void) const; // Returns the Opus encoder's application mode
	void setApplicationMode(ApplicationMode newApplicationMode); // Sets the Opus encoder's application mode
	int getBitrate(void) const; // Returns the Opus encoder's target bitrate in bit/s
	void setBitrate(unsigned int newBitrate); // Sets the Opus encoder's target bitrate in bit/s
	int getComplexity(void) const; // Returns the Opus encoder's computational complexity from 0 (minimum complexity) to 10 (maximum complexity)
	void setComplexity(int newComplexity); // Sets the Opus encoder's computational complexity from 0 (minimum complexity) to 10 (maximum complexity)
	int getLookaheadSamples(void) const; // Returns the Opus encoder's number of look-ahead samples (i.e., encoder latency)
	
	/* Encoding methods: */
	size_t encodeChunk(const Misc::SInt16* chunkData,size_t numChunkFrames); // Encodes a chunk of audio data in signed 16 bit integer format; returns the size of the encoded audio chunk in bytes
	const unsigned char* getPacket(void) const // Returns a pointer to the code buffer containing the most recently encoded Opus packet
		{
		return codeBuffer;
		}
	};

}

#endif

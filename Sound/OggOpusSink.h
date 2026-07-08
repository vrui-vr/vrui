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

#ifndef SOUND_OGGOPUSSINK_INCLUDED
#define SOUND_OGGOPUSSINK_INCLUDED

#include <stddef.h>
#include <string>
#include <vector>
#include <Misc/SizedTypes.h>
#include <IO/File.h>

/* Forward declarations: */
namespace Sound {
namespace Ogg {
class Stream;
}
class OpusEncoder;
}

namespace Sound {

class OggOpusSink
	{
	/* Embedded classes: */
	public:
	class Initializer // Class containing additional data used when initializing an Ogg/Opus sink
		{
		friend class OggOpusSink;
		
		/* Elements: */
		private:
		int serialNumber; // Serial number assigned to the Ogg stream embedded in the Ogg/Opus sink
		unsigned int preSkip; // Number of frames of decoded audio data to skip at 48kHz sample frequency from the beginning of the encoded stream when playing back, to avoid bad-quality frames from the encoder's start-up phase
		double outputGainDb; // Output gain applied to decoded audio data in dB
		std::vector<std::string> comments; // List of comments in tag=value form
		
		/* Constructors and destructors: */
		public:
		Initializer(const OpusEncoder& encoder); // Creates an Ogg/Opus sink initializer with default settings from the given Opus encoder
		
		/* Methods: */
		void setSerialNumber(int newSerialNumber); // Sets the Ogg stream's serial number
		void setPreSkip(unsigned int newPreSkip); // Sets the pre-skip in units of frames at 48kHz
		void setPreSkipMs(double newPreSkipMs); // Sets the pre-skip in units of milliseconds
		void setOutputGainDb(double newOutputGainDb); // Sets the output gain in dB
		void setOutputGainLinear(double newOutputGain); // Sets the output gain as a linear factor
		void addComment(const std::string& newComment); // Adds a comment in tag=value form to the comment list
		void addComment(std::string&& newComment); // Ditto, using move
		};
	
	/* Elements: */
	private:
	OpusEncoder& encoder; // Reference to the Opus encoder
	IO::FilePtr sink; // Pointer to the sink
	Ogg::Stream* oggStream; // Pointer to the Ogg stream receiving encoded audio data
	Misc::SInt64 granulePos; // Granule position of the most recently added PCM sample
	Misc::SInt64 packetNo; // Serial number of the next OGG packet to be added to the OGG stream
	
	/* Constructors and destructors: */
	public:
	OggOpusSink(OpusEncoder& sEncoder,IO::File& sSink,const Initializer& initializer); // Creates an Ogg/Opus sink for the given Opus encoder and sink and stream initializer
	~OggOpusSink(void); // Destroys the Ogg/Opus sink
	
	/* Methods: */
	OpusEncoder& getEncoder(void) const // Returns the Opus encoder attached to the Ogg/Opus sink
		{
		return encoder;
		}
	void writePacket(size_t numFrames,size_t packetSize); // Writes the most recently encoded Opus packet of the given size in bytes containing the given number of input audio frames from the Opus encoder to the sink
	void encodeChunk(const Misc::SInt16* chunkData,size_t numChunkFrames); // Convenience method to encode a chunk of raw PCM data and write it to the Ogg/Opus sink
	void flush(void); // Flushed the Ogg stream and the sink
	};

}

#endif

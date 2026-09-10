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

#ifndef SOUND_OGGOPUSSOURCE_INCLUDED
#define SOUND_OGGOPUSSOURCE_INCLUDED

#include <stddef.h>
#include <string>
#include <vector>
#include <Misc/SizedTypes.h>
#include <IO/File.h>

/* Forward declarations: */
namespace Sound {
namespace Ogg {
class Sync;
class Stream;
class Page;
class Packet;
}
}

namespace Sound {

class OggOpusSource
	{
	/* Embedded classes: */
	public:
	struct OpusInfo // Structure containing Opus header information extracted from an Opus audio stream
		{
		/* Elements: */
		public:
		int sampleFrequency,numChannels; // The encoded audio data's format
		int preSkip; // Number of samples (at nominal 48 kHz sample frequency) that should be skipped from the beginning of the decoded audio stream
		double outputGainDb; // Output gain to be applied to audio data during decoding in dB
		std::string opusVendor; // String identifying the Opus codec vendor and version number
		std::vector<std::string> comments; // The list of Opus comments
		};
	
	/* Elements: */
	private:
	IO::FilePtr source; // Pointer to the source file for the Ogg bitstream
	size_t sourceBufferSize; // Size of the source's read buffer
	Sound::Ogg::Sync* sync; // An Ogg synchronization object to read Ogg pages from the Ogg bitstream
	Sound::Ogg::Stream* stream; // An Ogg stream containing the Opus-encoded audio data
	
	/* Private methods: */
	Sound::Ogg::Page readPage(void); // Reads and returns the next Ogg page from the Ogg bitstream source
	
	/* Constructors and destructors: */
	public:
	OggOpusSource(IO::File& sSource,OpusInfo& opusInfo); // Creates an Ogg/Opus source for the given Ogg bitstream source; fills in the OpusInfo structure with data retrieved from the Opus stream headers
	~OggOpusSource(void); // Destroys the Ogg/Opus source
	
	/* Methods: */
	bool eof(void) const // Returns true when the source has been read completely
		{
		return source->eof();
		}
	Ogg::Packet packetOut(void); // Returns the next Ogg packet (containing one Opus packet) from the Ogg stream
	};

}

#endif

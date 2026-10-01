/***********************************************************************
InputDeviceDataSaver - Class to save input device data to a file for
later playback. The class is derived from Vrui::InputDeviceAdapter to
be able to hook it into the input device manager and simplify processing
in Vrui's main loop.
Copyright (c) 2004-2026 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_INTERNAL_INPUTDEVICEDATASAVER_INCLUDED
#define VRUI_INTERNAL_INPUTDEVICEDATASAVER_INCLUDED

#include <IO/File.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/SoundContext.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Sound {
class WAVFile;
class OpusEncoder;
class OggOpusSink;
}
namespace Vrui {
class InputDevice;
class InputGraphManager;
}

namespace Vrui {

class InputDeviceDataSaver:public InputDeviceAdapter
	{
	/* Elements: */
	private:
	InputGraphManager* inputGraphManager; // Pointer to the input graph manager
	IO::FilePtr inputDeviceDataFile; // File to which input device data is saved
	int numSavedInputDevices; // Number of input devices whose states will be saved to the file
	InputDevice** savedInputDevices; // Array of pointers to input devices whose states will be dispatched to the slaves
	bool* inputDeviceEnableds; // Array of enabled flags for all dispatched input devices as seen on the slaves
	bool* newInputDeviceEnableds; // Array of new enabled flags not yet shared with the slaves
	IO::FilePtr soundFile; // File into which to record sound
	int soundFileFormat; // Indicator of the sound file's requested format, -1: no sound, 0: WAV, 1: Ogg/Opus
	Sound::WAVFile* wavFile; // Pointer to a helper object to save recorded sound in WAV format
	int opusBitrate; // Selected encoding bit rate for Opus encoder
	Sound::OpusEncoder* opusEncoder; // Pointer to an Opus encoder for recorded sound
	Sound::OggOpusSink* oggOpusSink; // Pointer to helper object to save recorded sound in Ogg/Opus format
	
	/* Private methods: */
	void inputDeviceStateChangedCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData); // Callback called when a saved input device changes enabled state
	void soundRecordingCallback(const SoundContext::RecordingCallbackData& cbData); // Callback called when a packet of audio data has been read from the recording source
	
	/* Constructors and destructors: */
	public:
	InputDeviceDataSaver(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection,unsigned int randomSeed); // Creates an object saving the states of all input devices managed by the given manager
	virtual ~InputDeviceDataSaver(void);
	
	/* Methods from class InputDeviceAdapter: */
	virtual void prepareMainLoop(void);
	virtual void updateInputDevices(void);
	};

}

#endif

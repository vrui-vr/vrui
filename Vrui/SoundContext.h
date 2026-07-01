/***********************************************************************
SoundContext - Class for OpenAL contexts that are used to map a listener
to an OpenAL sound device.
Copyright (c) 2008-2026 Oliver Kreylos

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

#ifndef VRUI_SOUNDCONTEXT_INCLUDED
#define VRUI_SOUNDCONTEXT_INCLUDED

#include <string>
#include <vector>
#include <Misc/Autopointer.h>
#include <Threads/Mutex.h>
#include <Threads/FunctionCalls.h>
#include <Sound/SoundDataFormat.h>
#include <AL/Config.h>
#if ALSUPPORT_CONFIG_HAVE_OPENAL
#ifdef __APPLE__
#include <OpenAL/alc.h>
#else
#include <AL/alc.h>
#endif
#endif

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}
namespace Sound {
namespace PulseAudio {
class Context;
class Source;
}
}
class ALContextData;
namespace SceneGraph {
class ALRenderState;
}
namespace Vrui {
class Listener;
class VruiState;
}

namespace Vrui {

class SoundContext
	{
	/* Embedded classes: */
	public:
	enum DistanceAttenuationModel // Enumerated type for distance attenuation models
		{
		CONSTANT,INVERSE,INVERSE_CLAMPED,LINEAR,LINEAR_CLAMPED,EXPONENTIAL,EXPONENTIAL_CLAMPED
		};
	
	struct RecordingCallbackData // Structure passed to a recording callback when new sound data has been recorded
		{
		/* Elements: */
		public:
		const SoundContext& soundContext; // Reference to the sound context that recorded the sound samples
		const void* frames; // Opaque pointer to an array of sound samples in the format defined by recordingFormat
		size_t numFrames; // Number of frames in the sound sample array
		
		/* Constructors and destructors: */
		RecordingCallbackData(const SoundContext& sSoundContext,const void* sFrames,size_t sNumFrames)
			:soundContext(sSoundContext),frames(sFrames),numFrames(sNumFrames)
			{
			}
		};
	
	typedef Threads::FunctionCall<const RecordingCallbackData&> RecordingCallback; // Type for functions called when new sound data has been recorded
	typedef Misc::Autopointer<RecordingCallback> RecordingCallbackPtr; // Type for smart pointers to recording callbacks
	typedef std::vector<RecordingCallbackPtr> RecordingCallbackList; // Type for lists of recording callback pointers
	
	/* Elements: */
	private:
	VruiState* vruiState; // Pointer to the Vrui state object this sound context belongs to
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	ALCdevice* alDevice; // Pointer to OpenAL sound device
	ALCcontext* alContext; // Pointer to OpenAL sound context
	#endif
	ALContextData* contextData; // An OpenAL context data structure for this sound context
	SceneGraph::ALRenderState* renderState; // Pointer to a scene graph traversal object for OpenAL sound rendering
	Listener* listener; // Pointer to listener listening to this sound context
	float speedOfSound; // Speed of sound in physical coordinate units/s
	float dopplerFactor; // Exaggeration factor for Doppler effect
	DistanceAttenuationModel distanceAttenuationModel; // Distance attenuation model
	float referenceDistance; // Reference distance for distance attenuation in physical coordinate units
	float rolloffFactor; // Roll-off factor for distance attenuation
	std::string recordingDeviceName; // Name of a recording device to be used with this sound context
	Sound::SoundDataFormat recordingFormat; // Format of sound data returned when registering a recording sink
	int recordingLatency; // Target recording latency in milliseconds
	Sound::PulseAudio::Context* pulseAudioContext; // A PulseAudio context to record sound
	Threads::Mutex recordingMutex; // Mutex protecting the PulseAudio source and the list of active sound recording callbacks
	Sound::PulseAudio::Source* pulseAudioSource; // A PulseAudio source to record sound
	RecordingCallbackList recordingCallbacks; // List of active sound recording callbacks
	
	/* Private methods: */
	static void pulseAudioRecordingCallback(Sound::PulseAudio::Source& source,size_t numFrames,const void* frames,void* userData); // Callback called when the PulseAudio source received new sound data
	
	/* Constructors and destructors: */
	public:
	SoundContext(const Misc::ConfigurationFileSection& configFileSection,VruiState* sVruiState); // Initializes sound context based on settings from given configuration file section
	~SoundContext(void);
	
	/* Methods: */
	const Listener* getListener(void) const // Returns the listener listening to this sound context
		{
		return listener;
		}
	float getReferenceDistance(void) const // Returns the reference distance
		{
		return referenceDistance;
		}
	float getRolloffFactor(void) const // Returns the roll-off factor
		{
		return rolloffFactor;
		}
	const std::string& getRecordingDeviceName(void) const // Returns the name of the recording device associated with this sound context
		{
		return recordingDeviceName;
		}
	bool canRecord(void) const; // Returns true if the sound context can record sound
	const Sound::SoundDataFormat& getRecordingFormat(void) const // Returns the sound data format streamed to recording sinks
		{
		return recordingFormat;
		}
	int getRecordingLatency(void) const // Returns the target recording latency in milliseconds
		{
		return recordingLatency;
		}
	void addRecordingCallback(RecordingCallback& newRecordingCallback); // Adds a recording callback
	void removeRecordingCallback(RecordingCallback& recordingCallback); // Removes the given recording callback
	ALContextData& getContextData(void) // Returns the sound context's context data
		{
		return *contextData;
		}
	SceneGraph::ALRenderState& getRenderState(void) // Returns the sound context's scene graph traversal state
		{
		return *renderState;
		}
	void makeCurrent(void); // Makes sound context current
	void draw(void); // Updates the sound context
	};

}

#endif

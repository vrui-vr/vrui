/***********************************************************************
PulseAudio - Simple PulseAudio wrapper to enumerate sources/sinks and
play/record audio.
Copyright (c) 2022 Oliver Kreylos

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

#include <string>
#include <vector>
#include <Threads/Mutex.h>
#include <Threads/MutexCond.h>
#include <Sound/SoundDataFormat.h>

/* Forward declarations: */
struct pa_threaded_mainloop;
struct pa_context;
struct pa_stream;

namespace Sound {

namespace PulseAudio {

class Context // Class representing a PulseAudio context and the mainloop running it
	{
	/* Embedded classes: */
	public:
	struct SourceInfo // Structure providing information about an audio source available to a PulseAudio context
		{
		/* Embedded classes: */
		public:
		struct Port // Structure providing information about an audio port available to the audio source
			{
			/* Elements: */
			public:
			std::string name;
			std::string description;
			};
		
		/* Elements: */
		std::string name;
		std::string description;
		Sound::SoundDataFormat format;
		bool monitor;
		std::vector<Port> ports;
		};
	
	struct SinkInfo // Structure providing information about an audio sink available to a PulseAudio context
		{
		/* Elements: */
		public:
		std::string name;
		std::string description;
		};
	
	private:
	enum State // Enumerated type for PulseAudio context states
		{
		Created=0,
		ContextConnecting,
		MainLoopRunning,
		ContextReady,
		QueryingSources,
		SourcesComplete,
		SourcesFailed,
		QueryingSinks,
		SinksComplete,
		SinksFailed,
		ContextDisconnecting,
		MainLoopTerminating
		};
	
	class SourceQuerier;
	class SinkQuerier;
	
	/* Elements: */
	Threads::MutexCond stateCond; // Condition variable to signal changes to the device's state
	State state; // State of the PulseAudio context state machine
	pa_threaded_mainloop* mainLoop; // A PulseAudio threaded mainloop running the context
	pa_context* context; // A PulseAudio context connected to a local PulseAudio server
	
	/* Private methods: */
	void changeState(State newState) // Changes the context's state
		{
		/* Lock the state to change it, then signal the change: */
		Threads::MutexCond::Lock stateLock(stateCond);
		state=newState;
		stateCond.signal();
		}
	void waitForState(State waitState) // Waits for the context to reach the given state
		{
		/* Lock the state and wait until it gets there: */
		Threads::MutexCond::Lock stateLock(stateCond);
		while(state<waitState)
			stateCond.wait(stateLock);
		}
	static void contextStateCallback(pa_context* context,void* userData); // Callback called when the PulseAudio context changes state
	
	/* Constructors and destructors: */
	public:
	Context(const char* applicationName); // Creates a PulseAudio context for the given application
	~Context(void); // Destroys the PulseAudio context
	
	/* Methods: */
	pa_context* getContext(void) // Returns the low-level PulseAudio context
		{
		return context;
		}
	std::vector<SourceInfo> getSources(void); // Returns the list of audio sources currently available to the context
	std::vector<SinkInfo> getSinks(void); // Returns the list of audio sources currently available to the context
	};

class Source // Class representing a PulseAudio audio source
	{
	/* Embedded classes: */
	public:
	typedef void (*RecordingCallback)(Source& source,size_t numFrames,const void* frames,void* userData); // Type for user callbacks receiving recorded sound data
	
	private:
	enum State // Enumerated type for PulseAudio stream states
		{
		Created=0,
		StreamConnecting,
		StreamConnected,
		StreamDisconnecting,
		StreamDisconnected
		};
	
	/* Elements: */
	private:
	Threads::MutexCond stateCond; // Condition variable to signal changes to the device's state
	State state; // State of the PulseAudio stream state machine
	Sound::SoundDataFormat format; // The requested audio sample format
	size_t bytesPerFrame; // Number of bytes in each audio frame of the requested recording format
	pa_stream* stream; // The PulseAudio stream connected to the source
	Threads::Mutex recordingCallbackMutex; // Mutex protecting the recording callback state
	RecordingCallback recordingCallback; // Callback called when audio data is available to read
	void* recordingCallbackUserData; // Opaque user data pointer passes to recording callback
	
	/* Private methods: */
	void changeState(State newState) // Changes the context's state
		{
		/* Lock the state to change it, then signal the change: */
		Threads::MutexCond::Lock stateLock(stateCond);
		state=newState;
		stateCond.signal();
		}
	void waitForState(State waitState) // Waits for the context to reach the given state
		{
		/* Lock the state and wait until it gets there: */
		Threads::MutexCond::Lock stateLock(stateCond);
		while(state<waitState)
			stateCond.wait(stateLock);
		}
	static void streamStateCallback(pa_stream* stream,void* userData); // Callback called when the PulseAudio capture stream changes state
	static void readCallback(pa_stream* stream,size_t nbytes,void* userData); // Callback called when audio data can be read from the PulseAudio capture stream
	
	/* Constructors and destructors: */
	public:
	Source(Context& context,const char* sourceName,const Sound::SoundDataFormat& sFormat,unsigned int latencyMs); // Opens the source of the given name on the given PulseAudio context and prepares to capture audio in the given sample format
	~Source(void);
	
	/* Methods: */
	const Sound::SoundDataFormat& getFormat(void) const // Returns the stream's selected audio sample format
		{
		return format;
		}
	void start(RecordingCallback newRecordingCallback,void* newRecordingCallbackUserData); // Starts sending audio data to the given callback
	void stop(void); // Stops sending audio data to the recording callback
	};

}

}

/***********************************************************************
VruiSoundTest - Simple Vrui application to test the current audio
configuration.
Copyright (c) 2022-2026 Oliver Kreylos

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <deque>
#include <Misc/SizedTypes.h>
#include <Misc/Utility.h>
#include <Misc/StdError.h>
#include <Threads/MutexCond.h>
#include <Threads/Thread.h>
#include <Threads/FunctionCalls.h>
#include <Sound/Config.h>
#include <Sound/SoundDataFormat.h>
#include <AL/Config.h>
#include <AL/ALTemplates.h>
#include <AL/ALContextData.h>
#include <AL/ALGeometryWrappers.h>
#include <AL/ALObject.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/SoundContext.h>

// DEBUGGING
#include <iostream>

class VruiSoundTest:public Vrui::Application,public ALObject
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Embedded classes: */
	private:
	
	enum State // Enumerated type for audio playback state
		{
		Created=0,
		PlaybackThreadRunning,
		PlaybackThreadSuspended, // Playback thread is running, but the source is stopped for lack of audio data
		PlaybackThreadTerminating,
		PlaybackThreadTerminated
		};
	
	struct SoundBuffer // Structure holding a sound buffer not yet added to the playback source's queue
		{
		/* Elements: */
		public:
		size_t numFrames;
		void* frameData;
		};
	
	struct DataItem:public ALObject::DataItem
		{
		/* Elements: */
		public:
		Sound::SoundDataFormat recordingFormat; // Format in which Vrui's sound context delivers audio data
		unsigned int latencyMs; // Sound looping latency in ms
		Threads::MutexCond sourceStateCond; // Condition variable/mutex serializing access to the OpenAL sound source's state and signaling wake-ups to the playback thread
		volatile State state; // Current remote client state
		std::deque<SoundBuffer> soundBuffers; // List of recorded sound buffers not yet added to the playback source's queue
		ALuint playbackSource; // OpenAL audio source to play back decoded audio
		Threads::Thread playbackThread; // Thread running audio playback
		
		/* Private methods: */
		void recordingCallback(const Vrui::SoundContext::RecordingCallbackData& cbData); // Callback called with a chunk of audio data from Vrui's audio recording device
		void* playbackThreadMethod(void); // Method running the audio playback thread for the given OpenAL context data item
		
		/* Constructors and destructors: */
		DataItem(const VruiSoundTest* application);
		virtual ~DataItem(void);
		};
	
	#endif
	
	/* Elements: */
	private:
	unsigned int latencyMs; // Sound looping latency in ms
	
	/* Constructors and destructors: */
	public:
	VruiSoundTest(int& argc,char**& argv);
	
	/* Methods from class Vrui::Application: */
	virtual void sound(ALContextData& contextData) const;
	
	/* Methods from class ALObject: */
	virtual void initContext(ALContextData& contextData) const;
	};

#if ALSUPPORT_CONFIG_HAVE_OPENAL

/****************************************
Methods of class VruiSoundTest::DataItem:
****************************************/

void VruiSoundTest::DataItem::recordingCallback(const Vrui::SoundContext::RecordingCallbackData& cbData)
	{
	/* Copy the provided sound data into a new buffer: */
	size_t chunkSize=cbData.numFrames*recordingFormat.samplesPerFrame*recordingFormat.bytesPerSample;
	SoundBuffer newBuffer;
	newBuffer.numFrames=cbData.numFrames;
	newBuffer.frameData=malloc(chunkSize);
	memcpy(newBuffer.frameData,cbData.frames,chunkSize);
	
	/* Submit the new buffer to the playback thread and notify it: */
	Threads::MutexCond::Lock sourceStateLock(sourceStateCond);
	soundBuffers.push_back(newBuffer);
	sourceStateCond.signal();
	}

void* VruiSoundTest::DataItem::playbackThreadMethod(void)
	{
	/* Keep playing back audio until interrupted: */
	bool firstBuffer=true;
	while(true)
		{
		/* Grab the next buffer from the buffer queue: */
		SoundBuffer nextBuffer;
		{
		Threads::MutexCond::Lock sourceStateLock(sourceStateCond);
		while(state<PlaybackThreadTerminating&&soundBuffers.empty())
			sourceStateCond.wait(sourceStateLock);
		
		/* Bail out if the application is shutting down: */
		if(state>=PlaybackThreadTerminating)
			break;
		
		nextBuffer=soundBuffers.front();
		soundBuffers.pop_front();
		}
		
		/* Check if the source is not currently playing: */
		ALint sourceState;
		alGetSourcei(playbackSource,AL_SOURCE_STATE,&sourceState);
		if(sourceState!=AL_PLAYING)
			{
			// DEBUGGING
			std::cout<<"Starting OpenAL playback source with silence"<<std::endl;
			
			/* Preload the playback source with half a latency of silence: */
			unsigned int numSilenceSamples=(recordingFormat.framesPerSecond*latencyMs+1000U)/2000U;
			ALuint silenceBuffer;
			alGenBuffers(1,&silenceBuffer);
			Misc::SInt16* silence=new Misc::SInt16[numSilenceSamples];
			for(unsigned int i=0;i<numSilenceSamples;++i)
				silence[i]=0;
			alBufferData(silenceBuffer,AL_FORMAT_MONO16,silence,numSilenceSamples*sizeof(Misc::SInt16),recordingFormat.framesPerSecond);
			delete[] silence;
			alSourceQueueBuffers(playbackSource,1,&silenceBuffer);
			
			/* Start playing the playback source: */
			alSourcePlay(playbackSource);
			
			firstBuffer=false;
			}
		
		/* Query playback source state: */
		ALint numQueuedBuffers,numProcessedBuffers;
		alGetSourcei(playbackSource,AL_BUFFERS_QUEUED,&numQueuedBuffers);
		alGetSourcei(playbackSource,AL_BUFFERS_PROCESSED,&numProcessedBuffers);
		
		/* Dequeue all processed buffers from the source: */
		ALuint buffers[32]; // Way sufficient
		if(numProcessedBuffers>32)
			numProcessedBuffers=32;
		alSourceUnqueueBuffers(playbackSource,numProcessedBuffers,buffers);
		
		/* Upload the new sound data into an OpenAL buffer: */
		ALuint* bufPtr=buffers;
		if(numProcessedBuffers==0)
			{
			/* Generate a new buffer: */
			alGenBuffers(1,bufPtr);
			++numProcessedBuffers;
			}
		alBufferData(*bufPtr,AL_FORMAT_MONO16,nextBuffer.frameData,nextBuffer.numFrames*sizeof(Misc::SInt16),recordingFormat.framesPerSecond);
		alSourceQueueBuffers(playbackSource,1,bufPtr);
		++bufPtr;
		--numProcessedBuffers;
		
		/* Delete the new sound data: */
		free(nextBuffer.frameData);
		
		/* Delete all reclaimed but unused buffers: */
		if(numProcessedBuffers>0)
			alDeleteBuffers(numProcessedBuffers,bufPtr);
		}
	
	state=PlaybackThreadTerminated;
	return 0;
	}

VruiSoundTest::DataItem::DataItem(const VruiSoundTest* application)
	:latencyMs(application->latencyMs)
	{
	/* Create and initialize the playback source: */
	alGenSources(1,&playbackSource);
	if(alGetError()!=AL_NO_ERROR)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create OpenAL playback source");
	alSourceGain(playbackSource,1.0f);
	
	/* Start the audio playback thread: */
	state=PlaybackThreadRunning;
	playbackThread.start(this,&VruiSoundTest::DataItem::playbackThreadMethod);
	}

VruiSoundTest::DataItem::~DataItem(void)
	{
	/* Shut down the audio playback thread: */
	{
	Threads::MutexCond::Lock sourceStateLock(sourceStateCond);
	if(state==PlaybackThreadRunning||state==PlaybackThreadSuspended)
		{
		/* Tell the audio playback thread to pack it in: */
		state=PlaybackThreadTerminating;
		sourceStateCond.signal(); // Signal just in case the playback thread was suspended
		}
	}
	playbackThread.join();
	
	/* Stop the playback source: */
	alSourceStop(playbackSource);
	
	/* Reclaim and delete the playback source's audio buffers: */
	ALint numProcessedBuffers;
	alGetSourcei(playbackSource,AL_BUFFERS_PROCESSED,&numProcessedBuffers);
	while(numProcessedBuffers>0)
		{
		ALuint buffers[32]; // Way sufficient
		ALint reclaimed=Misc::min(numProcessedBuffers,32);
		alSourceUnqueueBuffers(playbackSource,reclaimed,buffers);
		alDeleteBuffers(reclaimed,buffers);
		numProcessedBuffers-=reclaimed;
		}
	
	/* Delete the playback source: */
	alDeleteSources(1,&playbackSource);
	}

#endif

/******************************
Methods of class VruiSoundTest:
******************************/

VruiSoundTest::VruiSoundTest(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 latencyMs(500)
	{
	/* Parse the command line: */
	if(argc>=2)
		latencyMs=atoi(argv[1]);
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL

	/* Request OpenAL sound processing from Vrui: */
	Vrui::requestSound();
	
	#else
	
	/* Audio won't work: */
	Vrui::showErrorMessage("Vrui Sound Configuration Test","Sound recording and/or playback are disabled because OpenAL is not installed on system.");
	
	#endif
	}

void VruiSoundTest::sound(ALContextData& contextData) const
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set the source position transformed to physical coordinates: */
	alSourcePosition(dataItem->playbackSource,Vrui::getDisplayCenter());
	
	/* Set source velocity to zero to avoid Doppler shift: */
	alSourceVelocity(dataItem->playbackSource,Vrui::Vector::zero);
	
	/* Set the source's distance attenuation parameters (they don't change, but this is the best place to do it): */
	alSourceReferenceDistance(dataItem->playbackSource,contextData.getReferenceDistance());
	alSourceRolloffFactor(dataItem->playbackSource,contextData.getRolloffFactor());
	
	#endif
	}

void VruiSoundTest::initContext(ALContextData& contextData) const
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Create a new context data item and associate it with the context data: */
	DataItem* dataItem=new DataItem(this);
	contextData.addDataItem(this,dataItem);
	
	/* Start recording audio: */
	Vrui::SoundContext* recordingSoundContext=Vrui::getRecordingSoundContext();
	if(recordingSoundContext!=0)
		{
		dataItem->recordingFormat=recordingSoundContext->getRecordingFormat();
		
		/* Register a recording callback: */
		recordingSoundContext->addRecordingCallback(*Threads::createFunctionCall(dataItem,&VruiSoundTest::DataItem::recordingCallback));
		}
	else
		std::cerr<<"No recording-capable sound contexts found"<<std::endl;
	
	#endif
	}

VRUI_APPLICATION_RUN(VruiSoundTest)

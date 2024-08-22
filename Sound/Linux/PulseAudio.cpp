/***********************************************************************
PulseAudio - Simple PulseAudio wrapper to enumerate sources/sinks and
play/record audio.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <Sound/Linux/PulseAudio.h>

#define DEBUG_CONTEXT 0
#define DEBUG_STREAM 0

#include <iostream>
#include <Misc/StdError.h>
#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/stream.h>
#include <pulse/introspect.h>

namespace Sound {

namespace PulseAudio {

/***************************************
Methods of class Context::SourceQuerier:
***************************************/

class Context::SourceQuerier // Helper class to collect the list of sources
	{
	/* Elements: */
	private:
	Context& context; // Reference to the PulseAudio context
	std::vector<SourceInfo> sources; // List of sources to be returned
	
	/* Constructors and destructors: */
	public:
	SourceQuerier(Context& sContext)
		:context(sContext)
		{
		}
	
	/* Methods: */
	static void sourceInfoListCallback(pa_context* context,const pa_source_info* info,int eol,void* userData)
		{
		/* Access the querier object: */
		SourceQuerier* thisPtr=static_cast<SourceQuerier*>(userData);
		
		/* Check if the source is valid: */
		if(info!=0)
			{
			/* Add the given source's information to the result list: */
			thisPtr->sources.push_back(SourceInfo());
			SourceInfo& si=thisPtr->sources.back();
			si.name=info->name;
			si.description=info->description;
			
			/* Determine the source's current sample format: */
			switch(info->sample_spec.format)
				{
				case PA_SAMPLE_U8:
					si.format.signedSamples=false;
					si.format.sampleEndianness=Sound::SoundDataFormat::DontCare;
					si.format.bitsPerSample=8;
					si.format.bytesPerSample=1;
					break;
				
				case PA_SAMPLE_S16LE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::LittleEndian;
					si.format.bitsPerSample=16;
					si.format.bytesPerSample=2;
					break;
				
				case PA_SAMPLE_S16BE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::BigEndian;
					si.format.bitsPerSample=16;
					si.format.bytesPerSample=2;
					break;
				
				case PA_SAMPLE_S24LE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::LittleEndian;
					si.format.bitsPerSample=24;
					si.format.bytesPerSample=3;
					break;
				
				case PA_SAMPLE_S24BE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::BigEndian;
					si.format.bitsPerSample=24;
					si.format.bytesPerSample=3;
					break;
				
				case PA_SAMPLE_S24_32LE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::LittleEndian;
					si.format.bitsPerSample=24;
					si.format.bytesPerSample=4;
					break;
				
				case PA_SAMPLE_S24_32BE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::BigEndian;
					si.format.bitsPerSample=24;
					si.format.bytesPerSample=4;
					break;
				
				case PA_SAMPLE_S32LE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::LittleEndian;
					si.format.bitsPerSample=32;
					si.format.bytesPerSample=4;
					break;
				
				case PA_SAMPLE_S32BE:
					si.format.signedSamples=true;
					si.format.sampleEndianness=Sound::SoundDataFormat::BigEndian;
					si.format.bitsPerSample=32;
					si.format.bytesPerSample=4;
					break;
				
				default:
					si.format.signedSamples=false;
					si.format.sampleEndianness=Sound::SoundDataFormat::DontCare;
					si.format.bitsPerSample=0;
					si.format.bytesPerSample=0;
				}
			si.format.samplesPerFrame=info->sample_spec.channels;
			si.format.framesPerSecond=info->sample_spec.rate;
			
			/* Check if the source is a monitor: */
			si.monitor=info->monitor_of_sink!=PA_INVALID_INDEX;
			
			/* Query the source's ports: */
			for(uint32_t i=0;i<info->n_ports;++i)
				{
				pa_source_port_info* port=info->ports[i];
				
				/* Add the port's information to the source information structure: */
				si.ports.push_back(SourceInfo::Port());
				SourceInfo::Port& p=si.ports.back();
				p.name=port->name;
				p.description=port->description;
				}
			
			/* Remove the source again if it has no supported audio sample format: */
			if(si.format.bitsPerSample==0)
				thisPtr->sources.pop_back();
			}
		
		/* Check for the end of the list: */
		if(eol)
			{
			/* Notify the context that the query is complete: */
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context::SourceQuerier::sourceInfoListCallback: Changing state to SourcesComplete"<<std::endl;
			#endif
			thisPtr->context.changeState(SourcesComplete);
			}
		}
	const std::vector<SourceInfo>& getSources(void) const
		{
		return sources;
		}
	};

/************************
Methods of class Context:
************************/

void Context::contextStateCallback(pa_context* context,void* userData)
	{
	Context* thisPtr=static_cast<Context*>(userData);
	
	/* Check the context's new state: */
	switch(pa_context_get_state(context))
		{
		case PA_CONTEXT_UNCONNECTED:
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Context is unconnected"<<std::endl;
			#endif
			
			break;
		
		case PA_CONTEXT_CONNECTING:
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Context is connecting"<<std::endl;
			#endif
			
			break;
		
		case PA_CONTEXT_AUTHORIZING:
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Context is authorizing"<<std::endl;
			#endif
			
			break;
		
		case PA_CONTEXT_SETTING_NAME:
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Context is setting name"<<std::endl;
			#endif
			
			break;
		
		case PA_CONTEXT_READY:
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Context is ready"<<std::endl;
			#endif
			
			thisPtr->changeState(ContextReady);
			
			break;
		
		case PA_CONTEXT_TERMINATED:
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Context terminated"<<std::endl;
			#endif
			
			/* Release the context: */
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Unreferencing context"<<std::endl;
			#endif
			pa_context_unref(thisPtr->context);
			thisPtr->context=0;
			
			/* Terminate the main loop: */
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Terminating mainloop"<<std::endl;
			#endif
			pa_threaded_mainloop_get_api(thisPtr->mainLoop)->quit(pa_threaded_mainloop_get_api(thisPtr->mainLoop),42);
			thisPtr->changeState(MainLoopTerminating);
			break;
		
		case PA_CONTEXT_FAILED:
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Context failed"<<std::endl;
			#endif
			
			/* Terminate the main loop: */
			#if DEBUG_CONTEXT
			std::cout<<"PulseAudio::Context: Terminating mainloop"<<std::endl;
			#endif
			pa_threaded_mainloop_get_api(thisPtr->mainLoop)->quit(pa_threaded_mainloop_get_api(thisPtr->mainLoop),43);
			thisPtr->changeState(MainLoopTerminating);
			break;
		}
	}

Context::Context(const char* applicationName)
	:state(Created),
	 mainLoop(0),context(0)
	{
	/* Create a PulseAudio threaded mainloop: */
	#if DEBUG_CONTEXT
	std::cout<<"PulseAudio::Context::Context: Creating mainloop"<<std::endl;
	#endif
	mainLoop=pa_threaded_mainloop_new();
	if(mainLoop==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create mainloop");
	
	/* Create a PulseAudio context: */
	#if DEBUG_CONTEXT
	std::cout<<"PulseAudio::Context::Context: Creating context"<<std::endl;
	#endif
	context=pa_context_new(pa_threaded_mainloop_get_api(mainLoop),applicationName);
	if(context==0)
		{
		pa_threaded_mainloop_free(mainLoop);
		mainLoop=0;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create context");
		}
	
	/* Register a context state change callback: */
	pa_context_set_state_callback(context,&Context::contextStateCallback,this);
	
	/* Connect the context to the default PulseAudio server: */
	#if DEBUG_CONTEXT
	std::cout<<"PulseAudio::Context::Context: Connecting context"<<std::endl;
	#endif
	if(pa_context_connect(context,0,PA_CONTEXT_NOFLAGS,0)<0)
		{
		pa_context_unref(context);
		context=0;
		pa_threaded_mainloop_free(mainLoop);
		mainLoop=0;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot connect context to server");
		}
	state=ContextConnecting;
	
	/* Run the mainloop: */
	#if DEBUG_CONTEXT
	std::cout<<"PulseAudio::Context::Context: Starting mainloop"<<std::endl;
	#endif
	if(pa_threaded_mainloop_start(mainLoop)<0)
		{
		pa_context_disconnect(context);
		pa_threaded_mainloop_free(mainLoop);
		mainLoop=0;
		state=Created;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot start mainloop");
		}
	state=MainLoopRunning;
	
	#if DEBUG_CONTEXT
	std::cout<<"PulseAudio::Context::Context: mainloop running"<<std::endl;
	#endif
	
	/* Wait until the context is ready: */
	waitForState(ContextReady);
	if(state>ContextReady)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Failed to create context");
	}

Context::~Context(void)
	{
	if(state>=MainLoopRunning)
		{
		pa_threaded_mainloop_lock(mainLoop);
		
		/* Disconnect the context: */
		#if DEBUG_CONTEXT
		std::cout<<"PulseAudio::Context::~Context: Disconnecting context"<<std::endl;
		#endif
		state=ContextDisconnecting;
		pa_context_disconnect(context);
		
		pa_threaded_mainloop_unlock(mainLoop);
		
		/* Wait until the mainloop shuts down: */
		#if DEBUG_CONTEXT
		std::cout<<"PulseAudio::Context::~Context: Waiting for mainloop to shut down"<<std::endl;
		#endif
		waitForState(MainLoopTerminating);
		#if DEBUG_CONTEXT
		std::cout<<"PulseAudio::Context::~Context: Mainloop return value = "<<pa_threaded_mainloop_get_retval(mainLoop)<<std::endl;
		#endif
		
		/* Stop the mainloop: */
		#if DEBUG_CONTEXT
		std::cout<<"PulseAudio::Context::~Context: Stopping and releasing mainloop"<<std::endl;
		#endif
		pa_threaded_mainloop_stop(mainLoop);
		}
	
	/* Release the mainloop: */
	pa_threaded_mainloop_free(mainLoop);
	mainLoop=0;
	state=Created;
	}

std::vector<Context::SourceInfo> Context::getSources(void)
	{
	/* Query the list of sources: */
	#if DEBUG_CONTEXT
	std::cout<<"PulseAudio::Context::getSources: Querying list of sources"<<std::endl;
	#endif
	SourceQuerier sq(*this);
	changeState(QueryingSources);
	pa_operation_unref(pa_context_get_source_info_list(context,&SourceQuerier::sourceInfoListCallback,&sq));
	
	/* Wait until the operation is complete: */
	waitForState(SourcesComplete);
	if(state>SourcesComplete)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Failed to query sources");
	
	return sq.getSources();
	}

std::vector<Context::SinkInfo> Context::getSinks(void)
	{
	std::vector<Context::SinkInfo> result;
	
	return result;
	}

/***********************
Methods of class Source:
***********************/

void Source::streamStateCallback(pa_stream* stream,void* userData)
	{
	Source* thisPtr=static_cast<Source*>(userData);
	
	/* Check the stream's new state: */
	switch(pa_stream_get_state(stream))
		{
		case PA_STREAM_UNCONNECTED:
			#if DEBUG_STREAM
			std::cout<<"PulseAudio::Source::streamStateCallback: Stream is unconnected"<<std::endl;
			#endif
			break;
		
		case PA_STREAM_CREATING:
			#if DEBUG_STREAM
			std::cout<<"PulseAudio::Source::streamStateCallback: Stream is being created"<<std::endl;
			#endif
			break;
		
		case PA_STREAM_READY:
			#if DEBUG_STREAM
			std::cout<<"PulseAudio::Source::streamStateCallback: Stream is ready"<<std::endl;
			#endif
			thisPtr->changeState(StreamConnected);
			break;
		
		case PA_STREAM_TERMINATED:
			#if DEBUG_STREAM
			std::cout<<"PulseAudio::Source::streamStateCallback: Stream terminated"<<std::endl;
			#endif
			thisPtr->changeState(StreamDisconnected);
		
			/* Release the stream: */
			#if DEBUG_STREAM
			std::cout<<"PulseAudio::Source::streamStateCallback: Unreferencing stream"<<std::endl;
			#endif
			pa_stream_unref(thisPtr->stream);
			thisPtr->stream=0;
			
			break;
		
		case PA_STREAM_FAILED:
			#if DEBUG_STREAM
			std::cout<<"PulseAudio::Source::streamStateCallback: Stream failed"<<std::endl;
			#endif
			
			/* Release the stream: */
			#if DEBUG_STREAM
			std::cout<<"PulseAudio::Source::streamStateCallback: Unreferencing stream"<<std::endl;
			#endif
			pa_stream_unref(thisPtr->stream);
			thisPtr->stream=0;
		
			break;
		}
	}

void Source::readCallback(pa_stream* stream,size_t nbytes,void* userData)
	{
	Source* thisPtr=static_cast<Source*>(userData);
	
	/* Read all data available on the stream: */
	const void* readBuffer=0;
	size_t readBytes=0;
	pa_stream_peek(thisPtr->stream,&readBuffer,&readBytes);
	if(readBytes>0)
		{
		/* Check for a hole: */
		if(readBuffer!=0)
			{
			/* Forward to the recording callback: */
			Threads::Mutex::Lock recordingCallbackLock(thisPtr->recordingCallbackMutex);
			if(thisPtr->recordingCallback!=0)
				{
				size_t numFrames=readBytes/thisPtr->bytesPerFrame;
				if(numFrames*thisPtr->bytesPerFrame!=readBytes)
					std::cout<<"PulseAudio::Source::readCallback: Partial frame in read data"<<std::endl;
				(*thisPtr->recordingCallback)(*thisPtr,numFrames,readBuffer,thisPtr->recordingCallbackUserData);
				}
			}
		else
			std::cout<<"PulseAudio::Source::readCallback: Hole of size "<<readBytes<<std::endl;
		
		/* Mark the current capture fragment as complete: */
		pa_stream_drop(thisPtr->stream);
		}
	}

Source::Source(Context& context,const char* sourceName,const Sound::SoundDataFormat& sFormat,unsigned int latencyMs)
	:state(Created),
	 format(sFormat),bytesPerFrame(format.samplesPerFrame*format.bytesPerSample),
	 stream(0),
	 recordingCallback(0),recordingCallbackUserData(0)
	{
	/* Convert the given sample specification into a PulseAudio sample specification: */
	pa_sample_spec ss;
	if(format.signedSamples)
		{
		if(format.bitsPerSample==16&&format.bytesPerSample==2)
			ss.format=format.sampleEndianness==Sound::SoundDataFormat::BigEndian?PA_SAMPLE_S16BE:PA_SAMPLE_S16LE;
		else if(format.bitsPerSample==24&&format.bytesPerSample==3)
			ss.format=format.sampleEndianness==Sound::SoundDataFormat::BigEndian?PA_SAMPLE_S24BE:PA_SAMPLE_S24LE;
		else if(format.bitsPerSample==24&&format.bytesPerSample==4)
			ss.format=format.sampleEndianness==Sound::SoundDataFormat::BigEndian?PA_SAMPLE_S24_32BE:PA_SAMPLE_S24_32LE;
		else if(format.bitsPerSample==32&&format.bytesPerSample==4)
			ss.format=format.sampleEndianness==Sound::SoundDataFormat::BigEndian?PA_SAMPLE_S32BE:PA_SAMPLE_S32LE;
		}
	else if(format.bitsPerSample==8&&format.bytesPerSample==1)
		ss.format=PA_SAMPLE_U8;
	ss.channels=format.samplesPerFrame;
	ss.rate=format.framesPerSecond;
	
	/* Create the PulseAudio stream: */
	#if DEBUG_STREAM
	std::cout<<"PulseAudio::Source:Source: Creating capture stream"<<std::endl;
	#endif
	stream=pa_stream_new(context.getContext(),"Capture",&ss,0);
	if(stream==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create stream");
	state=StreamConnecting;
	
	/* Set the stream callbacks: */
	pa_stream_set_state_callback(stream,&Source::streamStateCallback,this);
	pa_stream_set_read_callback(stream,&Source::readCallback,this);
	
	/* Define capture buffer attributes to approximate 10ms latency: */
	pa_buffer_attr bufferAttrs;
	size_t periodBytes=(latencyMs*format.framesPerSecond*format.samplesPerFrame*format.bytesPerSample)/1000;
	bufferAttrs.fragsize=periodBytes;
	bufferAttrs.maxlength=periodBytes;
	bufferAttrs.prebuf=0;
	bufferAttrs.minreq=uint32_t(-1);
	bufferAttrs.tlength=uint32_t(-1);
	
	/* Connect the capture stream to the requested PulseAudio source: */
	#if DEBUG_STREAM
	std::cout<<"PulseAudio::Source::Source: Connecting capture stream to source"<<std::endl;
	#endif
	pa_stream_flags_t flags=PA_STREAM_ADJUST_LATENCY;
	#if 0
	flags=pa_stream_flags_t(flags|PA_STREAM_START_CORKED);
	#endif
	if(pa_stream_connect_record(stream,sourceName,&bufferAttrs,flags)<0)
		{
		state=Created;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot connect stream to audio source");
		}
	
	/* Wait until the stream is connected: */
	waitForState(StreamConnected);
	if(state>StreamConnected)
		{
		state=Created;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Failed to create stream");
		}
	}

Source::~Source(void)
	{
	#if DEBUG_STREAM
	std::cout<<"PulseAudio::Source::Source: Disconnecting stream"<<std::endl;
	#endif
	state=StreamDisconnecting;
	pa_stream_disconnect(stream);
	
	/* Wait until the stream is disconnected: */
	waitForState(StreamDisconnected);
	}

void Source::start(Source::RecordingCallback newRecordingCallback,void* newRecordingCallbackUserData)
	{
	/* Install the given recording callback: */
	Threads::Mutex::Lock recordingCallbackLock(recordingCallbackMutex);
	recordingCallback=newRecordingCallback;
	recordingCallbackUserData=newRecordingCallbackUserData;
	}

void Source::stop(void)
	{
	/* Uninstall the recording callback: */
	Threads::Mutex::Lock recordingCallbackLock(recordingCallbackMutex);
	recordingCallback=0;
	recordingCallbackUserData=0;
	}

}

}

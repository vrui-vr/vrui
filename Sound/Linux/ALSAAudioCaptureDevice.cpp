/***********************************************************************
ALSAAudioCaptureDevice - Wrapper class around audio capture devices as
represented by the ALSA sound library.
Copyright (c) 2010-2024 Oliver Kreylos

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

#include <Sound/Linux/ALSAAudioCaptureDevice.h>

#include <stdio.h>
#include <Misc/StdError.h>
#include <Misc/FunctionCalls.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/FrameBuffer.h>

namespace Sound {

/*************************************************
Methods of class ALSAAudioCaptureDevice::DeviceId:
*************************************************/

AudioCaptureDevice* ALSAAudioCaptureDevice::DeviceId::openDevice(void) const
	{
	/* Open the audio device by its ALSA PCM device name: */
	return new ALSAAudioCaptureDevice(pcmDeviceName.c_str());
	}

/***************************************
Methods of class ALSAAudioCaptureDevice:
***************************************/

void* ALSAAudioCaptureDevice::streamingThreadMethod(void)
	{
	/* Read frame buffers from the ALSA PCM device until interrupted: */
	while(streaming)
		{
		/* Read the next frame: */
		snd_pcm_sframes_t result=snd_pcm_readi(pcmDevice,frameBuffers[nextFrameBufferIndex],frameBufferSize);
		
		/* Check for correctness: */
		if(result>=0)
			{
			/* Hand the read frame to the streaming client: */
			FrameBuffer buffer;
			buffer.start=frameBuffers[nextFrameBufferIndex];
			buffer.size=result;
			(*streamingCallback)(&buffer);
			}
		else if(result==-EPIPE)
			{
			/* Restart the device: */
			snd_pcm_prepare(pcmDevice);
			snd_pcm_start(pcmDevice);
			}
		
		/* Go to the next frame buffer: */
		nextFrameBufferIndex=(nextFrameBufferIndex+1)%numFrameBuffers;
		}
	
	return 0;
	}

void ALSAAudioCaptureDevice::addDevices(AudioCaptureDevice::DeviceList& devices)
	{
	/* Create a device ID for the default device: */
	DeviceId* defaultDeviceId=new DeviceId("default");
	defaultDeviceId->pcmDeviceName="default";
	devices.push_back(defaultDeviceId);
	
	/* Enumerate all ALSA cards and devices: */
	int cardIndex=-1; // Start with first card
	while(true)
		{
		/* Get the index of the next card: */
		if(snd_card_next(&cardIndex)!=0||cardIndex<0)
			{
			/* There was an error, or there are no more cards: */
			break;
			}
		
		/* Open the card's control interface: */
		char cardName[20];
		snprintf(cardName,sizeof(cardName),"hw:%d",cardIndex);
		snd_ctl_t* cardHandle;
		if(snd_ctl_open(&cardHandle,cardName,0)!=0)
			break;
		
		/* Enumerate all PCM devices on this card: */
		int numCardDevices=0;
		int pcmIndex=-1;
		while(true)
			{
			/* Get the index of the next PCM device: */
			if(snd_ctl_pcm_next_device(cardHandle,&pcmIndex)!=0||pcmIndex<0)
				{
				/* There was an error, or there are no more PCM devices: */
				break;
				}
			
			/* Create an info structure for the PCM device: */
			snd_pcm_info_t* pcmInfo;
			snd_pcm_info_alloca(&pcmInfo);
			snd_pcm_info_set_device(pcmInfo,pcmIndex);
			snd_pcm_info_set_stream(pcmInfo,SND_PCM_STREAM_CAPTURE);
			
			/* Get the number of capture subdevices for the device: */
			if(snd_ctl_pcm_info(cardHandle,pcmInfo)!=0)
				break;
			int numSubDevices=snd_pcm_info_get_subdevices_count(pcmInfo);
			for(int subDeviceIndex=0;subDeviceIndex<numSubDevices;++subDeviceIndex)
				{
				/* Query information about the subdevice: */
				snd_pcm_info_set_subdevice(pcmInfo,subDeviceIndex);
				if(snd_ctl_pcm_info(cardHandle,pcmInfo)==0)
					{
					/* Query the card's name: */
					char* cardName;
					if(snd_card_get_name(cardIndex,&cardName)==0)
						{
						/* Create a device ID: */
						std::string deviceName=cardName;
						free(cardName);
						if(numCardDevices>0)
							{
							char suffix[16];
							snprintf(suffix,sizeof(suffix),":%d",numCardDevices);
							deviceName.append(suffix);
							}
						DeviceId* newDeviceId=new DeviceId(deviceName);
						
						/* Set the PCM device name: */
						char pcmDeviceName[20];
						if(numSubDevices>1)
							snprintf(pcmDeviceName,sizeof(pcmDeviceName),"plughw:%d,%d,%d",snd_pcm_info_get_card(pcmInfo),snd_pcm_info_get_device(pcmInfo),snd_pcm_info_get_subdevice(pcmInfo));
						else
							snprintf(pcmDeviceName,sizeof(pcmDeviceName),"plughw:%d,%d",snd_pcm_info_get_card(pcmInfo),snd_pcm_info_get_device(pcmInfo));
						newDeviceId->pcmDeviceName=pcmDeviceName;
						
						/* Store the device ID: */
						devices.push_back(newDeviceId);
						
						++numCardDevices;
						}
					}
				}
			}
		
		/* Close the card's control interface: */
		snd_ctl_close(cardHandle);
		}
	}

ALSAAudioCaptureDevice::ALSAAudioCaptureDevice(const char* pcmDeviceName)
	:pcmDevice(0),pcmHwParams(0),
	 sampleSize(0),frameBufferSize(0),
	 numFrameBuffers(0),frameBuffers(0),
	 nextFrameBufferIndex(0)
	{
	/* Open the PCM device: */
	int error=snd_pcm_open(&pcmDevice,pcmDeviceName,SND_PCM_STREAM_CAPTURE,0);
	if(error<0)
		{
		pcmDevice=0;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot open PCM device %s due to error %s",pcmDeviceName,snd_strerror(error));
		}
	
	/* Allocate and initialize a hardware parameter context: */
	if((error=snd_pcm_hw_params_malloc(&pcmHwParams))<0)
		{
		snd_pcm_close(pcmDevice);
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot allocate hardware parameter context due to error %s",snd_strerror(error));
		}
	if((error=snd_pcm_hw_params_any(pcmDevice,pcmHwParams))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		snd_pcm_close(pcmDevice);
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot initialize hardware parameter context due to error %s",snd_strerror(error));
		}
	
	/* Set the PCM device's access method: */
	if((error=snd_pcm_hw_params_set_access(pcmDevice,pcmHwParams,SND_PCM_ACCESS_RW_INTERLEAVED))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		snd_pcm_close(pcmDevice);
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set PCM device's access method due to error %s",snd_strerror(error));
		}
	}

ALSAAudioCaptureDevice::~ALSAAudioCaptureDevice(void)
	{
	/* Delete all still-allocated frame buffers: */
	for(unsigned int i=0;i<numFrameBuffers;++i)
		delete[] frameBuffers[i];
	delete[] frameBuffers;
	
	/* Destroy the hardware parameter context: */
	if(pcmHwParams!=0)
		snd_pcm_hw_params_free(pcmHwParams);
	
	/* Close the PCM device: */
	if(pcmDevice!=0)
		snd_pcm_close(pcmDevice);
	}

SoundDataFormat ALSAAudioCaptureDevice::getAudioFormat(void) const
	{
	SoundDataFormat result;
	
	return result;
	}

SoundDataFormat ALSAAudioCaptureDevice::setAudioFormat(const SoundDataFormat& format)
	{
	/* Check if the device's parameters have already been locked for streaming: */
	if(pcmHwParams==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device is streaming");
	
	/* Normalize the supplied audio format: */
	SoundDataFormat myFormat=format;
	myFormat.bitsPerSample=(format.bitsPerSample+7)&~0x07;
	if(myFormat.bitsPerSample<8)
		myFormat.bitsPerSample=8;
	if(myFormat.bitsPerSample>32)
		myFormat.bitsPerSample=32;
	myFormat.bytesPerSample=myFormat.bitsPerSample/8;
	if(myFormat.bytesPerSample==3)
		myFormat.bytesPerSample=4;
	
	int error;
	
	/* Set the PCM device's sample format: */
	snd_pcm_format_t pcmSampleFormat=myFormat.getPCMFormat();
	if((error=snd_pcm_hw_params_set_format(pcmDevice,pcmHwParams,pcmSampleFormat))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's sample format due to error %s",snd_strerror(error));
	
	/* Set the PCM device's number of channels: */
	unsigned int pcmChannels=(unsigned int)(myFormat.samplesPerFrame);
	if((error=snd_pcm_hw_params_set_channels(pcmDevice,pcmHwParams,pcmChannels))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's number of channels due to error %s",snd_strerror(error));
	
	#if 0
	/* Enable PCM device's hardware resampling for non-native sample rates: */
	if((error=snd_pcm_hw_params_set_rate_resample(pcmDevice,pcmHwParams,1))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot enable device's hardware resampler due to error %s",snd_strerror(error));
	#endif
	
	/* Set the PCM device's sample rate: */
	unsigned int pcmRate=(unsigned int)(myFormat.framesPerSecond);
	if((error=snd_pcm_hw_params_set_rate_near(pcmDevice,pcmHwParams,&pcmRate,0))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's sample rate due to error %s",snd_strerror(error));
	
	/* Store the actually set sample rate: */
	myFormat.framesPerSecond=pcmRate;
	
	/* Calculate the audio format's sample size in bytes: */
	sampleSize=myFormat.bytesPerSample*myFormat.samplesPerFrame;
	
	return myFormat;
	}

unsigned int ALSAAudioCaptureDevice::allocateFrameBuffers(unsigned int requestedFrameBufferSize,unsigned int requestedNumFrameBuffers)
	{
	/* Check if the device's parameters have already been locked for streaming: */
	if(pcmHwParams==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device is streaming");
	
	/* Delete all allocated frame buffers: */
	for(unsigned int i=0;i<numFrameBuffers;++i)
		delete[] frameBuffers[i];
	numFrameBuffers=0;
	delete[] frameBuffers;
	frameBuffers=0;
	
	int error;
	
	/* Set PCM device's period size: */
	snd_pcm_uframes_t pcmPeriodFrames=requestedFrameBufferSize;
	int pcmPeriodDir=0;
	if((error=snd_pcm_hw_params_set_period_size_near(pcmDevice,pcmHwParams,&pcmPeriodFrames,&pcmPeriodDir))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's period size due to error %s",snd_strerror(error));
	
	/* Remember the actual frame buffer size: */
	frameBufferSize=pcmPeriodFrames;
	
	/* Set PCM device's buffer size: */
	snd_pcm_uframes_t pcmBufferFrames=requestedNumFrameBuffers*pcmPeriodFrames;
	if((error=snd_pcm_hw_params_set_buffer_size_near(pcmDevice,pcmHwParams,&pcmBufferFrames))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's buffer size due to error %s",snd_strerror(error));
	
	/* Check if the actual buffer size is a multiple of the period size: */
	if(pcmBufferFrames%pcmPeriodFrames!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device's buffer size is not multiple of period size");
	
	/* Remember the actual number of frame buffers: */
	numFrameBuffers=pcmBufferFrames/pcmPeriodFrames;
	
	/* Allocate the frame buffer array: */
	frameBuffers=new char*[numFrameBuffers];
	for(unsigned int i=0;i<numFrameBuffers;++i)
		frameBuffers[i]=new char[frameBufferSize*sampleSize];
	
	return numFrameBuffers;
	}

void ALSAAudioCaptureDevice::startStreaming(void)
	{
	/* Check if frame buffers have been allocated: */
	if(frameBuffers==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device's frame buffers are not allocated");
	
	if(!streaming)
		{
		int error;
		
		/* Apply the potentially changed hardware parameter set to the PCM device: */
		if((error=snd_pcm_hw_params(pcmDevice,pcmHwParams))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot write hardware parameters to device due to error %s",snd_strerror(error));
		snd_pcm_hw_params_free(pcmHwParams);
		pcmHwParams=0;
		
		/* Prepare the PCM device for streaming: */
		if((error=snd_pcm_prepare(pcmDevice))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot prepare device due to error %s",snd_strerror(error));
		
		/* Start streaming: */
		nextFrameBufferIndex=0;
		if((error=snd_pcm_start(pcmDevice))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot start streaming due to error %s",snd_strerror(error));
		}
	
	/* Call the base class method: */
	AudioCaptureDevice::startStreaming();
	}

void ALSAAudioCaptureDevice::startStreaming(AudioCaptureDevice::StreamingCallback* newStreamingCallback)
	{
	/* Check if frame buffers have been allocated: */
	if(frameBuffers==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device's frame buffers are not allocated");
	
	if(!streaming)
		{
		int error;
		
		/* Apply the potentially changed hardware parameter set to the PCM device: */
		if((error=snd_pcm_hw_params(pcmDevice,pcmHwParams))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot write hardware parameters to device due to error %s",snd_strerror(error));
		snd_pcm_hw_params_free(pcmHwParams);
		pcmHwParams=0;
		
		/* Prepare the PCM device for streaming: */
		if((error=snd_pcm_prepare(pcmDevice))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot prepare device due to error %s",snd_strerror(error));
		
		/* Start streaming: */
		nextFrameBufferIndex=0;
		if((error=snd_pcm_start(pcmDevice))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot start streaming due to error %s",snd_strerror(error));
		
		/* Start the streaming thread: */
		streamingThread.start(this,&ALSAAudioCaptureDevice::streamingThreadMethod);
		}
	
	/* Call the base class method: */
	AudioCaptureDevice::startStreaming(newStreamingCallback);
	}

FrameBuffer ALSAAudioCaptureDevice::dequeueFrame(void)
	{
	/* Read the next frame: */
	FrameBuffer buffer;
	buffer.start=frameBuffers[nextFrameBufferIndex];
	snd_pcm_sframes_t result=snd_pcm_readi(pcmDevice,buffer.start,frameBufferSize);
	
	/* Check for correctness: */
	if(result>=0)
		{
		/* Mark the frame buffer as valid: */
		buffer.size=result;
		}
	else if(result==-EPIPE)
		{
		/* Mark the frame buffer as invalid: */
		buffer.size=0;
		
		/* Restart the device: */
		snd_pcm_prepare(pcmDevice);
		snd_pcm_start(pcmDevice);
		}
	
	/* Go to the next frame buffer: */
	nextFrameBufferIndex=(nextFrameBufferIndex+1)%numFrameBuffers;
	
	/* Return the buffer: */
	return buffer;
	}

void ALSAAudioCaptureDevice::enqueueFrame(const FrameBuffer& frame)
	{
	/* No need to do anything */
	}

void ALSAAudioCaptureDevice::stopStreaming(void)
	{
	if(streaming)
		{
		if(streamingThread.isJoined())
			{
			/* Stop streaming and drop all pending samples; there isn't anybody there to retrieve them: */
			int error;
			if((error=snd_pcm_drop(pcmDevice))<0)
				{
				/* Can't really throw an error here; ignore it for now */
				}
			}
		else
			{
			/* Stop streaming and let the streaming thread process all pending samples: */
			int error;
			if((error=snd_pcm_drain(pcmDevice))<0)
				{
				/* Can't really throw an error here; ignore it for now */
				}
			
			/* Signal the streaming thread to stop after the next delivered frame: */
			streaming=false;
			streamingThread.join();
			}
		}
	
	/* Call the base class method: */
	AudioCaptureDevice::stopStreaming();
	}

void ALSAAudioCaptureDevice::releaseFrameBuffers(void)
	{
	/* Delete all allocated frame buffers: */
	for(unsigned int i=0;i<numFrameBuffers;++i)
		delete[] frameBuffers[i];
	numFrameBuffers=0;
	delete[] frameBuffers;
	frameBuffers=0;
	}

}

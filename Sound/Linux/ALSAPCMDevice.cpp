/***********************************************************************
ALSAPCMDevice - Simple wrapper class around PCM devices as represented
by the Advanced Linux Sound Architecture (ALSA) library.
Copyright (c) 2009-2024 Oliver Kreylos

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

#include <Sound/Linux/ALSAPCMDevice.h>

#include <stdio.h>
#include <poll.h>
#include <Misc/StdError.h>
#include <Sound/SoundDataFormat.h>

namespace Sound {

/******************************
Methods of class ALSAPCMDevice:
******************************/

void ALSAPCMDevice::throwException(const char* prettyFunction,int error)
	{
	if(error==-EPIPE)
		{
		char buffer[1024];
		if(recording)
			throw OverrunError(Misc::makeStdErrMsg(buffer,sizeof(buffer),prettyFunction,"Overrun detected"));
		else
			throw UnderrunError(Misc::makeStdErrMsg(buffer,sizeof(buffer),prettyFunction,"Underrun detected"));
		}
	else
		throw Misc::makeStdErr(prettyFunction,"ALSA error %d (%s)",-error,snd_strerror(error));
	}

void ALSAPCMDevice::pcmEventForwarder(Threads::EventDispatcher::IOEvent& event)
	{
	ALSAPCMDevice* thisPtr=static_cast<ALSAPCMDevice*>(event.getUserData());
	
	/* Find the poll structure on whose file descriptor this event occurred: */
	int index;
	for(index=0;index<thisPtr->numPCMEventFds&&thisPtr->pcmEventListenerKeys[index]!=event.getKey();++index)
		;
	struct pollfd& pfd=thisPtr->pcmEventPolls[index];
		
	/* Update the poll structure's event mask: */
	pfd.revents=0x0;
	if(event.getEventTypeMask()&Threads::EventDispatcher::Read)
		pfd.revents|=POLLIN;
	if(event.getEventTypeMask()&Threads::EventDispatcher::Write)
		pfd.revents|=POLLOUT;
	
	/* Parse the event: */
	unsigned short pcmEvent;
	if(snd_pcm_poll_descriptors_revents(thisPtr->pcmDevice,thisPtr->pcmEventPolls,thisPtr->numPCMEventFds,&pcmEvent)==0&&(pcmEvent&(POLLIN|POLLOUT)))
		{
		/* Call the event callback: */
		thisPtr->pcmEventCallback(*thisPtr,thisPtr->pcmEventCallbackUserData);
		}
	}

ALSAPCMDevice::PCMList ALSAPCMDevice::enumeratePCMs(bool recording)
	{
	PCMList result;
	
	/* Iterate through all sound card indices: */
	int cardIndex=-1;
	while(snd_card_next(&cardIndex)==0&&cardIndex>=0)
		{
		/* Open the sound card of the current index: */
		char cardName[20];
		snprintf(cardName,sizeof(cardName),"hw:%d",cardIndex);
		snd_ctl_t* control=0;
		if(snd_ctl_open(&control,cardName,SND_CTL_NONBLOCK)==0)
			{
			/* Get the card's info structure: */
			snd_ctl_card_info_t* cardInfo=0;
			snd_ctl_card_info_alloca(&cardInfo);
			snd_ctl_card_info(control,cardInfo);
			
			/* Iterate through the card's PCM device indices: */
			int deviceIndex=-1;
			while(snd_ctl_pcm_next_device(control,&deviceIndex)==0&&deviceIndex>=0)
				{
				/* Open the PCM device of the current index: */
				char pcmName[40];
				snprintf(pcmName,sizeof(pcmName),"hw:%d,%d",cardIndex,deviceIndex);
				snd_pcm_t* pcm=0;
				if(snd_pcm_open(&pcm,pcmName,recording?SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK,SND_PCM_NONBLOCK)==0)
					{
					/* Get the PCM device's info structure: */
					snd_pcm_info_t* pcmInfo=0;
					snd_pcm_info_alloca(&pcmInfo);
					snd_pcm_info(pcm,pcmInfo);
					
					/* Add the PCM device to the list: */
					result.push_back(PCM());
					PCM& newPcm=result.back();
					newPcm.cardIndex=cardIndex;
					newPcm.deviceIndex=deviceIndex;
					const char* cardName=snd_ctl_card_info_get_name(cardInfo);
					const char* cardId=snd_ctl_card_info_get_id(cardInfo);
					const char* pcmName=snd_pcm_info_get_name(pcmInfo);
					char name[512];
					snprintf(name,sizeof(name),"%s, %s (CARD=%s,DEV=%d)",cardName,pcmName,cardId,deviceIndex);
					newPcm.name=name;
					
					/* Close the PCM device: */
					snd_pcm_close(pcm);
					}
				}
			
			/* Close the sound card: */
			snd_ctl_close(control);
			}
		}
	
	return result;
	}

ALSAPCMDevice::ALSAPCMDevice(const char* pcmDeviceName,bool sRecording,bool nonBlocking)
	:pcmDevice(0),
	 recording(sRecording),
	 pcmSampleFormat(SND_PCM_FORMAT_UNKNOWN),pcmChannels(1),pcmRate(8000),pcmBufferFrames(0),pcmPeriodFrames(0),
	 pcmConfigPending(true),
	 pcmEventCallback(0),pcmEventCallbackUserData(0),
	 numPCMEventFds(0),pcmEventPolls(0),pcmEventListenerKeys(0)
	{
	/* Open the PCM device: */
	int error=snd_pcm_open(&pcmDevice,pcmDeviceName,recording?SND_PCM_STREAM_CAPTURE:SND_PCM_STREAM_PLAYBACK,nonBlocking?SND_PCM_NONBLOCK:0);
	if(error<0)
		{
		pcmDevice=0;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot open device %s for %s due to error %s",pcmDeviceName,recording?"recording":"playback",snd_strerror(error));
		}
	
	/* Create a PCM hardware parameter context with initial values: */
	snd_pcm_hw_params_t* pcmHwParams;
	snd_pcm_hw_params_alloca(&pcmHwParams);
	snd_pcm_hw_params_any(pcmDevice,pcmHwParams);
	}

ALSAPCMDevice::~ALSAPCMDevice(void)
	{
	if(pcmDevice!=0)
		snd_pcm_close(pcmDevice);
	
	/* Delete the poll structure and listener key array just in case: */
	delete[] pcmEventPolls;
	delete[] pcmEventListenerKeys;
	}

snd_async_handler_t* ALSAPCMDevice::registerAsyncHandler(snd_async_callback_t callback,void* privateData)
	{
	snd_async_handler_t* result;
	int error;
	if((error=snd_async_add_pcm_handler(&result,pcmDevice,callback,privateData))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot register event handler due to error %s",snd_strerror(error));
	
	return result;
	}

void ALSAPCMDevice::setSoundDataFormat(const SoundDataFormat& newFormat)
	{
	/* Retrieve ALSA sample format from sound data format structure: */
	pcmSampleFormat=newFormat.getPCMFormat();
	
	/* Retrieve number of channels and sample rate: */
	pcmChannels=(unsigned int)(newFormat.samplesPerFrame);
	pcmRate=(unsigned int)(newFormat.framesPerSecond);
	
	pcmConfigPending=true;
	}

void ALSAPCMDevice::setBufferSize(size_t numBufferFrames,size_t numPeriodFrames)
	{
	/* Retrieve buffer and period sizes: */
	pcmBufferFrames=snd_pcm_uframes_t(numBufferFrames);
	pcmPeriodFrames=snd_pcm_uframes_t(numPeriodFrames);
	
	pcmConfigPending=true;
	}

size_t ALSAPCMDevice::getBufferSize(void) const
	{
	/* Return the most recent buffer size actually configured: */
	return size_t(pcmBufferFrames);
	}

size_t ALSAPCMDevice::getPeriodSize(void) const
	{
	/* Return the most recent period size actually configured: */
	return size_t(pcmPeriodFrames);
	}

void ALSAPCMDevice::setStartThreshold(size_t numStartFrames)
	{
	int error;
	
	/* Allocate a software parameter context: */
	snd_pcm_sw_params_t* pcmSwParams;
	snd_pcm_sw_params_alloca(&pcmSwParams);
	
	/* Get the PCM device's current software parameter context: */
	if((error=snd_pcm_sw_params_current(pcmDevice,pcmSwParams))<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve device's software parameter context due to error %s",snd_strerror(error));
	
	/* Set the start threshold: */
	if((error=snd_pcm_sw_params_set_start_threshold(pcmDevice,pcmSwParams,snd_pcm_uframes_t(numStartFrames)))<0)
		{
		snd_pcm_sw_params_free(pcmSwParams);
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's start threshold due to error %s",snd_strerror(error));
		}
	
	/* Write the changed software parameter set to the PCM device: */
	if((error=snd_pcm_sw_params(pcmDevice,pcmSwParams))<0)
		{
		snd_pcm_sw_params_free(pcmSwParams);
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot write softare parameters to device due to error %s",snd_strerror(error));
		}
	}

void ALSAPCMDevice::prepare(void)
	{
	int error;
	
	if(pcmConfigPending)
		{
		/* Create a PCM hardware parameter context with initial values: */
		snd_pcm_hw_params_t* pcmHwParams;
		snd_pcm_hw_params_alloca(&pcmHwParams);
		if((error=snd_pcm_hw_params_any(pcmDevice,pcmHwParams))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create device's hardware parameter context due to error %d",snd_strerror(error));
		
		/* Enable the PCM device's hardware resampling for non-native sample rates: */
		if((error=snd_pcm_hw_params_set_rate_resample(pcmDevice,pcmHwParams,1))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot enable device's hardware resampler due to error %d",snd_strerror(error));
		
		/* Set the PCM device's access method: */
		if((error=snd_pcm_hw_params_set_access(pcmDevice,pcmHwParams,SND_PCM_ACCESS_RW_INTERLEAVED))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's access method due to error %d",snd_strerror(error));
		
		/* Set the PCM device's sample format: */
		if((error=snd_pcm_hw_params_set_format(pcmDevice,pcmHwParams,pcmSampleFormat))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's sample format due to error %d",snd_strerror(error));
		
		/* Set the PCM device's number of channels: */
		if((error=snd_pcm_hw_params_set_channels(pcmDevice,pcmHwParams,pcmChannels))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's number of channels due to error %d",snd_strerror(error));
		
		/* Set the PCM device's sample rate: */
		unsigned int requestedPcmRate=pcmRate;
		if((error=snd_pcm_hw_params_set_rate(pcmDevice,pcmHwParams,pcmRate,0))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's sample rate due to error %d",snd_strerror(error));
		
		/* Check if the requested sample rate was correctly set: */
		if(pcmRate!=requestedPcmRate)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Requested sample rate %u, got %u instead",requestedPcmRate,pcmRate);
		
		/* Set PCM device's buffer size: */
		if(pcmBufferFrames!=0)
			{
			unsigned int bufferTime=(unsigned int)(size_t(pcmBufferFrames)*size_t(1000000)/size_t(pcmRate));
			int pcmBufferDir=0;
			if((error=snd_pcm_hw_params_set_buffer_time_near(pcmDevice,pcmHwParams,&bufferTime,&pcmBufferDir))<0)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's buffer size due to error %d",snd_strerror(error));
			}
		
		/* Read the configured buffer size: */
		snd_pcm_hw_params_get_buffer_size(pcmHwParams,&pcmBufferFrames);
		
		/* Set PCM device's period size: */
		int pcmPeriodDir=0;
		if(pcmPeriodFrames!=0)
			{
			unsigned int periodTime=(unsigned int)(size_t(pcmPeriodFrames)*size_t(1000000)/size_t(pcmRate));
			if((error=snd_pcm_hw_params_set_period_time_near(pcmDevice,pcmHwParams,&periodTime,&pcmPeriodDir))<0)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set device's period size due to error %d",snd_strerror(error));
			}
		
		/* Read the configured period size: */
		snd_pcm_hw_params_get_period_size(pcmHwParams,&pcmPeriodFrames,&pcmPeriodDir);
		
		/* Write the changed hardware parameter set to the PCM device: */
		if((error=snd_pcm_hw_params(pcmDevice,pcmHwParams))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot write hardware parameters to device due to error %d",snd_strerror(error));
		
		pcmConfigPending=false;
		}
	else
		{
		/* Prepare the PCM device: */
		if((error=snd_pcm_prepare(pcmDevice))<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot prepare device due to error %d",snd_strerror(error));
		}
	}

void ALSAPCMDevice::link(ALSAPCMDevice& other)
	{
	int result=snd_pcm_link(pcmDevice,other.pcmDevice);
	if(result<0)
		throwException(__PRETTY_FUNCTION__,result);
	}

void ALSAPCMDevice::unlink(void)
	{
	int result=snd_pcm_unlink(pcmDevice);
	if(result<0)
		throwException(__PRETTY_FUNCTION__,result);
	}

void ALSAPCMDevice::addPCMEventListener(Threads::EventDispatcher& dispatcher,ALSAPCMDevice::PCMEventCallback eventCallback,void* eventCallbackUserData)
	{
	/* Check if there is already a PCM event callback: */
	if(pcmEventCallback!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"PCM event listener already registered");
	
	/* Store the callback: */
	pcmEventCallback=eventCallback;
	pcmEventCallbackUserData=eventCallbackUserData;
	
	/* Retrieve the set of file descriptors that need to be watched: */
	numPCMEventFds=snd_pcm_poll_descriptors_count(pcmDevice);
	pcmEventPolls=new struct pollfd[numPCMEventFds];
	numPCMEventFds=snd_pcm_poll_descriptors(pcmDevice,pcmEventPolls,numPCMEventFds);
	
	/* Create IO event listeners for all PCM file descriptors: */
	pcmEventListenerKeys=new Threads::EventDispatcher::ListenerKey[numPCMEventFds];
	for(int i=0;i<numPCMEventFds;++i)
		{
		/* Assemble a proper event mask: */
		int eventMask=0x0;
		if(pcmEventPolls[i].events&POLLIN)
			eventMask|=Threads::EventDispatcher::Read;
		if(pcmEventPolls[i].events&POLLOUT)
			eventMask|=Threads::EventDispatcher::Write;
		pcmEventListenerKeys[i]=dispatcher.addIOEventListener(pcmEventPolls[i].fd,eventMask,pcmEventForwarder,this);
		}
	}

void ALSAPCMDevice::removePCMEventListener(Threads::EventDispatcher& dispatcher)
	{
	/* Bail out if there is no PCM event callback: */
	if(pcmEventCallback==0)
		return;
	
	/* Remove the callback: */
	pcmEventCallback=0;
	pcmEventCallbackUserData=0;
	
	/* Remove all previously created IO event listeners: */
	for(int i=0;i<numPCMEventFds;++i)
		dispatcher.removeIOEventListener(pcmEventListenerKeys[i]);
	numPCMEventFds=0;
	delete[] pcmEventPolls;
	pcmEventPolls=0;
	delete[] pcmEventListenerKeys;
	pcmEventListenerKeys=0;
	}

void ALSAPCMDevice::start(void)
	{
	int result=snd_pcm_start(pcmDevice);
	if(result<0)
		throwException(__PRETTY_FUNCTION__,result);
	}

void ALSAPCMDevice::drop(void)
	{
	int result=snd_pcm_drop(pcmDevice);
	if(result<0)
		throwException(__PRETTY_FUNCTION__,result);
	}

void ALSAPCMDevice::drain(void)
	{
	int result=snd_pcm_drain(pcmDevice);
	if(result<0)
		throwException(__PRETTY_FUNCTION__,result);
	}

}

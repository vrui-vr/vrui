/***********************************************************************
VideoDevice - Base class for video capture devices.
Copyright (c) 2009-2024 Oliver Kreylos

This file is part of the Basic Video Library (Video).

The Basic Video Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Video Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Video Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Video/VideoDevice.h>

#include <Misc/StdError.h>
#include <Misc/FunctionCalls.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/MathValueCoders.h>
#include <Video/Config.h>
#if 0
#include <Video/ImageSequenceVideoDevice.h>
#endif
#if VIDEO_CONFIG_HAVE_V4L2
#include <Video/Linux/V4L2VideoDevice.h>
#include <Video/Linux/OculusRiftDK2VideoDevice.h>
#endif
#if VIDEO_CONFIG_HAVE_DC1394
#include <Video/Linux/DC1394VideoDevice.h>
#endif

namespace Video {

/************************************
Static elements of class VideoDevice:
************************************/

VideoDevice::DeviceClass* VideoDevice::deviceClasses=0;

/****************************
Methods of class VideoDevice:
****************************/

VideoDevice::VideoDevice(void)
	:streamingCallback(0)
	{
	}

VideoDevice::~VideoDevice(void)
	{
	delete streamingCallback;
	}

void VideoDevice::registerDeviceClass(VideoDevice::EnumerateVideoDevicesFunc enumerateVideoDevices)
	{
	/* Create a new device class entry: */
	DeviceClass* newClass=new DeviceClass;
	newClass->enumerateVideoDevices=enumerateVideoDevices;
	newClass->succ=deviceClasses;
	
	/* Register the new class: */
	deviceClasses=newClass;
	}

void VideoDevice::unregisterDeviceClass(VideoDevice::EnumerateVideoDevicesFunc enumerateVideoDevices)
	{
	/* Find the enumeration function in the list of device classes: */
	DeviceClass* ptr0=0;
	for(DeviceClass* ptr1=deviceClasses;ptr1!=0;ptr0=ptr1,ptr1=ptr1->succ)
		if(ptr1->enumerateVideoDevices==enumerateVideoDevices)
			{
			/* Delete the device class entry: */
			if(ptr0!=0)
				ptr0->succ=ptr1->succ;
			else
				deviceClasses=ptr1->succ;
			delete ptr1;
			break;
			}
	}

std::vector<VideoDevice::DeviceIdPtr> VideoDevice::getVideoDevices(void)
	{
	std::vector<VideoDevice::DeviceIdPtr> result;
	
	/* Enumerate all video devices handled by all additional device classes: */
	for(DeviceClass* dcPtr=deviceClasses;dcPtr!=0;dcPtr=dcPtr->succ)
		dcPtr->enumerateVideoDevices(result);
	
	#if 0
	
	/* Enumerate all image sequence video devices in the system (THIS IS A HUGE PROBLEM): */
	ImageSequenceVideoDevice::enumerateDevices(result);
	
	#endif
	
	#if VIDEO_CONFIG_HAVE_V4L2
	
	/* Enumerate all quirky V4L2 video devices in the system: */
	OculusRiftDK2VideoDevice::enumerateDevices(result);
	
	/* Enumerate all V4L2 video devices in the system: */
	V4L2VideoDevice::enumerateDevices(result);
	
	#endif
	
	#if VIDEO_CONFIG_HAVE_DC1394
	
	/* Enumerate all DC1394 video devices in the system: */
	DC1394VideoDevice::enumerateDevices(result);
	
	#endif
	
	return result;
	}

VideoDevice* VideoDevice::createVideoDevice(VideoDevice::DeviceIdPtr deviceId)
	{
	/* Let the device ID object handle device creation: */
	return deviceId->createDevice();
	}

VideoDevice* VideoDevice::openVideoDevice(const std::string& videoDeviceName,unsigned int videoDeviceIndex)
	{
	/* Get the list of all connected video devices: */
	std::vector<DeviceIdPtr> videoDevices=getVideoDevices();
	
	/* Find the desired video device: */
	std::vector<DeviceIdPtr>::iterator vdIt;
	unsigned int vdi=videoDeviceIndex;
	for(vdIt=videoDevices.begin();vdIt!=videoDevices.end();++vdIt)
		if((*vdIt)->getName()==videoDeviceName)
			{
			/* Bail out if this is the desired video device: */
			if(vdi==0)
				break;
			
			/* Skip this device and keep searching: */
			--vdi;
			}
	if(vdIt==videoDevices.end())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Fewer than %u devices of name \"%s\" connected to the system",videoDeviceIndex,videoDeviceName.c_str());
	
	/* Open the found video device: */
	return (*vdIt)->createDevice();
	}

void VideoDevice::saveConfiguration(Misc::ConfigurationFileSection& cfg) const
	{
	/* Get the device's current video format: */
	VideoDataFormat currentFormat=getVideoFormat();
	
	/* Save the current frame size: */
	cfg.storeValue("./frameSize",currentFormat.size);
	
	/* Save the current frame rate (inverse of frame interval): */
	cfg.storeValue("./frameRate",currentFormat.frameInterval.inverse());
	
	/* Check if the current pixel format is a valid FourCC code: */
	char fourCCBuffer[5];
	currentFormat.getFourCC(fourCCBuffer);
	bool valid=true;
	for(int i=0;i<4&&valid;++i)
		valid=fourCCBuffer[i]>=32&&fourCCBuffer[i]<127&&fourCCBuffer[i]!='"';
	if(valid)
		{
		/* Save the current pixel format as a FourCC code: */
		cfg.storeValue<std::string>("./pixelFormat",fourCCBuffer);
		}
	else
		{
		/* Save the current pixel format as a hexadecimal number: */
		char hexBuffer[9];
		unsigned int pixelFormat=currentFormat.pixelFormat;
		for(int i=0;i<8;++i,pixelFormat>>=4)
			{
			if((pixelFormat&0x0fU)>=10U)
				hexBuffer[7-i]=(pixelFormat&0x0fU)+'a';
			else
				hexBuffer[7-i]=(pixelFormat&0x0fU)+'0';
			}
		hexBuffer[8]='\0';
		cfg.storeString("./pixelFormatHex",hexBuffer);
		}
	}

void VideoDevice::configure(const Misc::ConfigurationFileSection& cfg)
	{
	/* Check which components of the video format are stored in the configuration file section: */
	Size frameSize(0,0);
	bool haveFrameSize=false;
	if(cfg.hasTag("./width")&&cfg.hasTag("./height"))
		{
		/* Read the requested frame size as width and height: */
		frameSize[0]=cfg.retrieveValue<unsigned int>("./width");
		frameSize[1]=cfg.retrieveValue<unsigned int>("./height");
		
		haveFrameSize=true;
		}
	if(cfg.hasTag("./frameSize"))
		{
		/* Read the requested frame size as a Size value: */
		cfg.retrieveValue<Size>("./frameSize");
		
		haveFrameSize=true;
		}
	
	Math::Rational frameRate(0);
	bool haveFrameRate=false;
	if(cfg.hasTag("./frameRate"))
		{
		/* Read the requested frame rate: */
		frameRate=cfg.retrieveValue<Math::Rational>("./frameRate");
		
		haveFrameRate=true;
		}
	
	unsigned int pixelFormat=0;
	bool havePixelFormat=false;
	if(cfg.hasTag("./pixelFormat"))
		{
		/* Read a pixel format as a FourCC code: */
		std::string fourCC=cfg.retrieveValue<std::string>("./pixelFormat");
		if(fourCC.size()!=4)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid pixel format code \"%s\"",fourCC.c_str());
		
		/* Convert the FourCC code to a pixel format: */
		VideoDataFormat temp;
		temp.setPixelFormat(fourCC.c_str());
		pixelFormat=temp.pixelFormat;
		
		havePixelFormat=true;
		}
	if(cfg.hasTag("./pixelFormatHex"))
		{
		/* Read a pixel format as a hexadecimal number: */
		std::string hex=cfg.retrieveString("./pixelFormatHex");
		if(hex.size()!=8)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid hexadecimal pixel format code \"%s\"",hex.c_str());
		for(int i=0;i<8;++i)
			{
			if(hex[i]>='0'&&hex[i]<='9')
				pixelFormat=(pixelFormat<<4)|(hex[i]-'0');
			else if(hex[i]>='A'&&hex[i]<='F')
				pixelFormat=(pixelFormat<<4)|(hex[i]-'A'+10);
			else if(hex[i]>='a'&&hex[i]<='f')
				pixelFormat=(pixelFormat<<4)|(hex[i]-'a'+10);
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid hexadecimal pixel format code \"%s\"",hex.c_str());
			}
		
		havePixelFormat=true;
		}
	
	/* Get the list of the device's supported video formats: */
	std::vector<VideoDataFormat> deviceFormats=getVideoFormatList();
	
	/* Find the best-matching video format among the device's advertised formats: */
	std::vector<VideoDataFormat>::iterator bestFormat=deviceFormats.end();
	double bestMatch=0.0;
	for(std::vector<VideoDataFormat>::iterator dfIt=deviceFormats.begin();dfIt!=deviceFormats.end();++dfIt)
		{
		double match=1.0;
		if(haveFrameSize)
			for(int i=0;i<2;++i)
				match*=dfIt->size[i]>=frameSize[i]?double(frameSize[i])/double(dfIt->size[i]):double(dfIt->size[i])/double(frameSize[i]);
		if(haveFrameRate)
			{
			/* Calculate the smaller ratio between the device's frame rate and the requested frame rate: */
			Math::Rational rateRatio=dfIt->frameInterval*frameRate;
			if(rateRatio>Math::Rational(1))
				rateRatio.invert();
			match*=double(rateRatio);
			}
		if(havePixelFormat&&dfIt->pixelFormat!=pixelFormat)
			match*=0.75;
		
		if(bestMatch<match)
			{
			bestFormat=dfIt;
			bestMatch=match;
			}
		}
	if(bestFormat==deviceFormats.end()) // Can only happen if there are no device formats
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No matching video formats found");
	
	/* Set the selected video format: */
	setVideoFormat(*bestFormat);
	}

void VideoDevice::startStreaming(void)
	{
	/* Delete the streaming callback: */
	delete streamingCallback;
	streamingCallback=0;
	}

void VideoDevice::startStreaming(VideoDevice::StreamingCallback* newStreamingCallback)
	{
	/* Store the new streaming callback: */
	delete streamingCallback;
	streamingCallback=newStreamingCallback;
	}

void VideoDevice::stopStreaming(void)
	{
	/* Delete the streaming callback: */
	delete streamingCallback;
	streamingCallback=0;
	}

}

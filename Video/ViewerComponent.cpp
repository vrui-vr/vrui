/***********************************************************************
ViewerComponent - An application component to stream video from a camera
to an OpenGL texture for rendering, including user interfaces to select
cameras and video modes and control camera settings.
Copyright (c) 2018-2024 Oliver Kreylos

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

#include <Video/ViewerComponent.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include <iostream>
#include <Misc/StdError.h>
#include <Misc/FunctionCalls.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Math/Math.h>
#include <Math/MathValueCoders.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <Images/RGBImage.h>
#include <GLMotif/WidgetManager.icpp>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/DropdownBox.h>

namespace Video {

/******************************************
Methods of class ViewerComponent::DataItem:
******************************************/

ViewerComponent::DataItem::DataItem(const ViewerComponent* sComponent)
	:component(sComponent),
	 videoTextureId(0),
	 size(0,0),
	 videoTextureVersion(0)
	{
	/* Check whether non-power-of-two-dimension textures are supported: */
	haveNpotdt=GLARBTextureNonPowerOfTwo::isSupported();
	if(haveNpotdt)
		GLARBTextureNonPowerOfTwo::initExtension();
	
	glGenTextures(1,&videoTextureId);
	}

ViewerComponent::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&videoTextureId);
	}

void ViewerComponent::DataItem::bindVideoTexture(void)
	{
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,videoTextureId);
	
	/* Check if the cached texture is outdated: */
	if(videoTextureVersion!=component->videoFrameVersion)
		{
		/* Access the new video frame: */
		const Images::BaseImage& videoFrame=component->videoFrames.getLockedValue();
		
		/* Check if the frame size changed: */
		if(size!=videoFrame.getSize())
			{
			/* Update the frame size: */
			size=videoFrame.getSize();
			
			/* Calculate the texture coordinate rectangle: */
			Size texSize;
			if(haveNpotdt)
				texSize=size;
			else
				{
				/* Find the next larger power-of-two texture size: */
				for(int i=0;i<2;++i)
					for(texSize[i]=1U;texSize[i]<size[i];texSize[i]<<=1)
						;
				}
			
			/* Calculate texture coordinates to map the (padded) texture onto the geometry: */
			for(int i=0;i<2;++i)
				{
				texMin[i]=0.0f;
				texMax[i]=GLfloat(size[i])/GLfloat(texSize[i]);
				}
			}
		
		/* Upload the new video frame into the texture object: */
		videoFrame.glTexImage2D(GL_TEXTURE_2D,0,!haveNpotdt);
		videoTextureVersion=component->videoFrameVersion;
		}
	}

/********************************
Methods of class ViewerComponent:
********************************/

void ViewerComponent::frameCallback(const Video::FrameBuffer* frameBuffer)
	{
	/* Check whether to store incoming video frames in the input triple buffer: */
	if(storeVideoFrames)
		{
		/* Start a new value in the input triple buffer: */
		Images::BaseImage& image=videoFrames.startNewValue();
		
		/* Check if the current image is a valid RGB image of the correct size: */
		if(image.isValid()&&image.getScalarType()==GL_UNSIGNED_BYTE&&image.getNumChannels()==3&&image.getSize()==videoFormat.size)
			{
			/* Extract an RGB image from the provided frame buffer into the current image, interpreted as an RGB image: */
			videoExtractor->extractRGB(frameBuffer,image.replacePixels());
			
			/* Call the optional video frame callback: */
			{
			Threads::Spinlock::Lock videoFrameCallbackLock(videoFrameCallbackMutex);
			if(videoFrameCallback!=0)
				(*videoFrameCallback)(image);
			}
			}
		else
			{
			/* Extract an RGB image from the provided frame buffer into a new RGB image: */
			Images::RGBImage newImage(videoFormat.size);
			videoExtractor->extractRGB(frameBuffer,newImage.replacePixels());
			image=newImage;
			
			/* Call the optional video frame callback: */
			{
			Threads::Spinlock::Lock videoFrameCallbackLock(videoFrameCallbackMutex);
			if(videoFrameCallback!=0)
				(*videoFrameCallback)(newImage);
			}
			}
		
		/* Finish the new image in the input triple buffer: */
		videoFrames.postNewValue();
		}
	else
		{
		/* Check if the input image buffer is invalid or of incorrect size: */
		if(!inputVideoFrame.isValid()||inputVideoFrame.getSize()!=videoFormat.size)
			{
			/* Create a new input image: */
			inputVideoFrame=Images::RGBImage(videoFormat.size);
			}
		
		/* Extract an RGB image from the provided frame buffer into the input image buffer: */
		videoExtractor->extractRGB(frameBuffer,inputVideoFrame.replacePixels());
		
		/* Call the video frame callback: */
		{
		Threads::Spinlock::Lock videoFrameCallbackLock(videoFrameCallbackMutex);
		(*videoFrameCallback)(inputVideoFrame);
		}
		}
	}

void ViewerComponent::videoDevicesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Close the current video device: */
	closeVideoDevice();
	
	/* Open the new video device: */
	openVideoDevice(cbData->newSelectedItem,VideoDataFormat(),0x0);
	
	/* Call the optional video format change callbacks: */
	if(videoFormatChangedCallback!=0)
		(*videoFormatChangedCallback)(videoFormat);
	if(videoFormatSizeChangedCallback!=0)
		(*videoFormatSizeChangedCallback)(videoFormat);
	}

void ViewerComponent::frameSizesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Find the closest video format with the requested frame size: */
	const Size& vfs=cbData->dropdownBox->getManager()->getWidgetAttribute<Size>(cbData->getItemWidget());
	unsigned int bestFormat=~0x0U;
	unsigned int bestDist=~0x0U;
	for(unsigned int i=0;i<videoFormats.size();++i)
		if(videoFormats[i].size==vfs)
			{
			/* Calculate the mismatch in frame rate and pixel format: */
			Math::Rational ratio=videoFormats[i].frameInterval/videoFormat.frameInterval;
			unsigned int dist=Math::abs(ratio.getNumerator()-ratio.getDenominator());
			
			if(videoFormats[i].pixelFormat!=videoFormat.pixelFormat)
				++dist;
			
			if(bestDist>dist)
				{
				bestFormat=i;
				bestDist=dist;
				}
			}
	
	/* Update the video format: */
	changeVideoFormat(videoFormats[bestFormat]);
	}

void ViewerComponent::frameRatesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Find the closest video format with the requested frame interval: */
	const Math::Rational& vfi=cbData->dropdownBox->getManager()->getWidgetAttribute<Math::Rational>(cbData->getItemWidget());
	unsigned int bestFormat=~0x0U;
	unsigned int bestDist=~0x0U;
	for(unsigned int i=0;i<videoFormats.size();++i)
		if(videoFormats[i].frameInterval==vfi)
			{
			/* Calculate the mismatch in frame size and pixel format: */
			unsigned int d1=videoFormats[i].size[0]>=videoFormat.size[0]?videoFormats[i].size[0]-videoFormat.size[0]:videoFormat.size[0]-videoFormats[i].size[0];
			unsigned int d2=videoFormats[i].size[1]>=videoFormat.size[1]?videoFormats[i].size[1]-videoFormat.size[1]:videoFormat.size[1]-videoFormats[i].size[1];
			unsigned int dist=d1+d2;
			
			if(videoFormats[i].pixelFormat!=videoFormat.pixelFormat)
				++dist;
			
			if(bestDist>dist)
				{
				bestFormat=i;
				bestDist=dist;
				}
			}
	
	/* Update the video format: */
	changeVideoFormat(videoFormats[bestFormat]);
	}

void ViewerComponent::pixelFormatsValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Find the closest video format with the requested pixel format: */
	unsigned int pixelFormat=cbData->dropdownBox->getManager()->getWidgetAttribute<unsigned int>(cbData->getItemWidget());
	unsigned int bestFormat=~0x0U;
	unsigned int bestDist=~0x0U;
	for(unsigned int i=0;i<videoFormats.size();++i)
		if(videoFormats[i].pixelFormat==pixelFormat)
			{
			/* Calculate the mismatch in frame size and frame interval: */
			unsigned int d1=videoFormats[i].size[0]>=videoFormat.size[0]?videoFormats[i].size[0]-videoFormat.size[0]:videoFormat.size[0]-videoFormats[i].size[0];
			unsigned int d2=videoFormats[i].size[1]>=videoFormat.size[1]?videoFormats[i].size[1]-videoFormat.size[1]:videoFormat.size[1]-videoFormats[i].size[1];
			unsigned int dist=d1+d2;
			
			Math::Rational ratio=videoFormats[i].frameInterval/videoFormat.frameInterval;
			dist+=Math::abs(ratio.getNumerator()-ratio.getDenominator());
			
			if(bestDist>dist)
				{
				bestFormat=i;
				bestDist=dist;
				}
			}
	
	/* Update the video format: */
	changeVideoFormat(videoFormats[bestFormat]);
	}

GLMotif::PopupWindow* ViewerComponent::createVideoDevicesDialog(void)
	{
	/* Create a popup shell to hold the video device control dialog: */
	GLMotif::PopupWindow* videoDeviceDialogPopup=new GLMotif::PopupWindow("VideoDeviceDialogPopup",widgetManager,"Video Device Selection");
	// videoDeviceDialogPopup->setResizableFlags(true,false);
	videoDeviceDialogPopup->setCloseButton(true);
	videoDeviceDialogPopup->popDownOnClose();
	
	GLMotif::RowColumn* videoDeviceDialog=new GLMotif::RowColumn("VideoDeviceDialog",videoDeviceDialogPopup,false);
	videoDeviceDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	videoDeviceDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	videoDeviceDialog->setNumMinorWidgets(2);
	
	new GLMotif::Label("VideoDeviceLabel",videoDeviceDialog,"Video Device");
	
	/* Create a drop-down menu containing all connected video devices: */
	GLMotif::DropdownBox* videoDevices=new GLMotif::DropdownBox("VideoDevices",videoDeviceDialog,false);
	
	for(unsigned int i=0;i<videoDeviceList.size();++i)
		{
		/* Add the video device's name to the drop-down menu: */
		videoDevices->addItem(videoDeviceList[i]->getName().c_str());
		
		/* Associate the video device's device ID with the just-added drop-down menu entry: */
		widgetManager->setWidgetAttribute(videoDevices->getItemWidget(i),videoDeviceList[i]);
		}
	
	videoDevices->setSelectedItem(videoDeviceIndex);
	videoDevices->getValueChangedCallbacks().add(this,&ViewerComponent::videoDevicesValueChangedCallback);
	videoDevices->manageChild();
	
	new GLMotif::Label("FrameSizeLabel",videoDeviceDialog,"Frame Size");
	
	/* Create a drop-down menu containing all supported frame sizes on the currently-selected video device, which will be populated later: */
	GLMotif::DropdownBox* frameSizes=new GLMotif::DropdownBox("FrameSizes",videoDeviceDialog,true);
	frameSizes->getValueChangedCallbacks().add(this,&ViewerComponent::frameSizesValueChangedCallback);
	
	new GLMotif::Label("FrameRateLabel",videoDeviceDialog,"Frame Rate");
	
	/* Create a drop-down menu containing all supported frame rates on the currently-selected video device, which will be populated later: */
	GLMotif::DropdownBox* frameRates=new GLMotif::DropdownBox("FrameRates",videoDeviceDialog,true);
	frameRates->getValueChangedCallbacks().add(this,&ViewerComponent::frameRatesValueChangedCallback);
	
	new GLMotif::Label("PixelFormatLabel",videoDeviceDialog,"Pixel Format");
	
	/* Create a drop-down menu containing all supported pixel formats on the currently-selected video device, which will be populated later: */
	GLMotif::DropdownBox* pixelFormats=new GLMotif::DropdownBox("PixelFormats",videoDeviceDialog,true);
	pixelFormats->getValueChangedCallbacks().add(this,&ViewerComponent::pixelFormatsValueChangedCallback);
	
	videoDeviceDialog->manageChild();
	
	return videoDeviceDialogPopup;
	}

void ViewerComponent::updateVideoDevicesDialog(void)
	{
	/* Create a drop-down menu of video frame sizes: */
	GLMotif::DropdownBox* frameSizes=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameSizes"));
	frameSizes->clearItems();
	Misc::HashTable<Size,void,Size> addedFrameSizes(17);
	for(unsigned int i=0;i<videoFormats.size();++i)
		{
		if(!addedFrameSizes.setEntry(videoFormats[i].size))
			{
			/* Add the frame size to the drop-down menu: */
			char frameSizeBuffer[64];
			snprintf(frameSizeBuffer,sizeof(frameSizeBuffer),"%u x %u",videoFormats[i].size[0],videoFormats[i].size[1]);
			GLMotif::Widget* newItem=frameSizes->addItem(frameSizeBuffer);
			
			/* Associate the video frame size with the new menu entry: */
			widgetManager->setWidgetAttribute(newItem,videoFormats[i].size);
			}
		}
	
	/* Create a drop-down menu of video frame rates: */
	GLMotif::DropdownBox* frameRates=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameRates"));
	frameRates->clearItems();
	Misc::HashTable<Math::Rational,void,Math::Rational> addedFrameIntervals(17);
	for(unsigned int i=0;i<videoFormats.size();++i)
		{
		if(!addedFrameIntervals.setEntry(videoFormats[i].frameInterval))
			{
			/* Add the frame rate to the drop-down menu: */
			std::string frameRate=Misc::ValueCoder<Math::Rational>::encode(videoFormats[i].frameInterval.inverse());
			GLMotif::Widget* newItem=frameRates->addItem(frameRate.c_str());
			
			/* Associate the video frame rate with the new menu entry: */
			widgetManager->setWidgetAttribute(newItem,videoFormats[i].frameInterval);
			}
		}
	
	/* Create a drop-down menu of video pixel formats. */
	GLMotif::DropdownBox* pixelFormats=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/PixelFormats"));
	pixelFormats->clearItems();
	Misc::HashTable<unsigned int,void> addedPixelFormats(17);
	for(unsigned int i=0;i<videoFormats.size();++i)
		{
		if(!addedPixelFormats.setEntry(videoFormats[i].pixelFormat))
			{
			/* Add the video pixel format to the drop-down menu: */
			char fourCCBuffer[5];
			GLMotif::Widget* newItem=pixelFormats->addItem(videoFormats[i].getFourCC(fourCCBuffer));
			
			/* Associate the video pixel format with the new menu entry: */
			widgetManager->setWidgetAttribute(newItem,videoFormats[i].pixelFormat);
			}
		}
	}

void ViewerComponent::updateVideoDevicesDialog(const VideoDataFormat& videoFormat)
	{
	/* Select the video format's frame size: */
	GLMotif::DropdownBox* frameSizes=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameSizes"));
	for(int i=0;i<frameSizes->getNumItems();++i)
		if(widgetManager->getWidgetAttribute<Size>(frameSizes->getItemWidget(i))==videoFormat.size)
			{
			frameSizes->setSelectedItem(i);
			break;
			}
	
	/* Select the video format's frame rate: */
	GLMotif::DropdownBox* frameRates=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameRates"));
	for(int i=0;i<frameRates->getNumItems();++i)
		if(widgetManager->getWidgetAttribute<Math::Rational>(frameRates->getItemWidget(i))==videoFormat.frameInterval)
			{
			frameRates->setSelectedItem(i);
			break;
			}
	
	/* Select the video format's pixel format: */
	GLMotif::DropdownBox* pixelFormats=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/PixelFormats"));
	for(int i=0;i<pixelFormats->getNumItems();++i)
		if(widgetManager->getWidgetAttribute<unsigned int>(pixelFormats->getItemWidget(i))==videoFormat.pixelFormat)
			{
			pixelFormats->setSelectedItem(i);
			break;
			}
	}

void ViewerComponent::openVideoDevice(unsigned int newVideoDeviceIndex,const VideoDataFormat& initialFormat,int formatComponentMask)
	{
	/* Check if the video device index is out-of-bounds: */
	if(newVideoDeviceIndex>=videoDeviceList.size())
		{
		/* Show an error message and bail out: */
		Misc::formattedUserError("Video::Video::ViewerComponent: Fewer than %u connected video devices",newVideoDeviceIndex+1);
		return;
		}
	
	try
		{
		/* Open the new video device: */
		videoDeviceIndex=newVideoDeviceIndex;
		videoDevice=videoDeviceList[videoDeviceIndex]->createDevice();
		
		/* Query the new video device's supported video formats: */
		videoFormats=videoDevice->getVideoFormatList();
		
		/* Update the video devices dialog with the new device's video formats: */
		updateVideoDevicesDialog();
		
		/* Create the video device's control panel: */
		videoControlPanel=videoDevice->createControlPanel(widgetManager);
		
		/* Check if the control panel is a pop-up window; if so, add a close button: */
		GLMotif::PopupWindow* vcp=dynamic_cast<GLMotif::PopupWindow*>(videoControlPanel);
		if(vcp!=0)
			{
			/* Add a close button: */
			vcp->setCloseButton(true);
			
			/* Set it so that the popup window will pop itself down, but not destroy itself, when closed: */
			vcp->popDownOnClose();
			}
		
		/* Get the new video device's current video format: */
		videoFormat=videoDevice->getVideoFormat();
		
		/* Check if there is a requested initial video format: */
		if(formatComponentMask!=0x0)
			{
			/* Adjust the current video format based on the given initial format and component bit mask: */
			if(formatComponentMask&0x1)
				{
				/* Override frame size: */
				videoFormat.size=initialFormat.size;
				}
			if(formatComponentMask&0x2)
				{
				/* Override frame interval: */
				videoFormat.frameInterval=initialFormat.frameInterval;
				}
			if(formatComponentMask&0x4)
				{
				/* Override pixel format: */
				videoFormat.pixelFormat=initialFormat.pixelFormat;
				}
			
			/* Set the adjusted format: */
			videoDevice->setVideoFormat(videoFormat);
			}
		
		/* Update the video devices dialog with the new device's selected video format: */
		updateVideoDevicesDialog(videoFormat);
		
		/* Start streaming from the new video device: */
		startStreaming();
		}
	catch(const std::runtime_error& err)
		{
		/*******************************************************************
		Something went horribly awry; clean up as much as possible:
		*******************************************************************/
		
		/* Destroy the video device's control panel if it was created: */
		if(videoControlPanel!=0)
			{
			delete videoControlPanel;
			videoControlPanel=0;
			}
		
		/* Close the video device: */
		delete videoExtractor;
		videoExtractor=0;
		delete videoDevice;
		videoDevice=0;
		videoFormat.pixelFormat=0x0U;
		videoFormat.size=Size(0,0);
		videoFormat.frameInterval=Math::Rational::nan;
		
		/* Show an error message: */
		Misc::formattedUserError("Video::Video::ViewerComponent: Could not open video device %s due to exception %s",videoDeviceList[videoDeviceIndex]->getName().c_str(),err.what());
		}
	}

void ViewerComponent::startStreaming(void)
	{
	if(videoDevice!=0)
		{
		try
			{
			/* Create an image extractor to convert from the video device's raw image format to RGB: */
			videoExtractor=videoDevice->createImageExtractor();
			
			/* Put a placeholder frame for the new video format into the locked video frame buffer slot: */
			Images::RGBImage img(videoFormat.size);
			img.clear(Images::RGBImage::Color(128,128,128));
			videoFrames.lockNewValue();
			videoFrames.getLockedValue()=img;
			++videoFrameVersion;
			
			/* Start capturing video in the new format from the video device: */
			videoDevice->allocateFrameBuffers(5);
			videoDevice->startStreaming(Misc::createFunctionCall(this,&ViewerComponent::frameCallback));
			}
		catch(const std::runtime_error& err)
			{
			/* Clean up as much as possible: */
			delete videoExtractor;
			videoExtractor=0;
			
			/* Show an error message: */
			Misc::formattedUserError("Video::Video::ViewerComponent: Unable to stream from video device %s due to exception %s",videoDeviceList[videoDeviceIndex]->getName().c_str(),err.what());
			}
		}
	}

void ViewerComponent::stopStreaming(void)
	{
	if(videoDevice!=0)
		{
		try
			{
			/* Stop streaming on the open device: */
			videoDevice->stopStreaming();
			videoDevice->releaseFrameBuffers();
			}
		catch(const std::runtime_error& err)
			{
			/* Show an error message: */
			Misc::formattedUserWarning("ViewerComponent: Exception %s while stopping streaming from video device %s",err.what(),videoDeviceList[videoDeviceIndex]->getName().c_str());
			}
		
		delete videoExtractor;
		videoExtractor=0;
		}
	}

void ViewerComponent::changeVideoFormat(const VideoDataFormat& newVideoFormat)
	{
	if(videoDevice!=0)
		{
		try
			{
			/* Stop streaming with the current video format: */
			stopStreaming();
			
			/* Set the changed video format: */
			videoFormat=newVideoFormat;
			videoDevice->setVideoFormat(videoFormat);
			
			/* Update the video devices dialog with the new video format: */
			updateVideoDevicesDialog(videoFormat);
			
			/* Start streaming with the new video format: */
			startStreaming();
			
			/* Call the optional video format change callbacks: */
			if(videoFormatChangedCallback!=0)
				(*videoFormatChangedCallback)(videoFormat);
			if(videoFormatSizeChangedCallback!=0)
				(*videoFormatSizeChangedCallback)(videoFormat);
			}
		catch(const std::runtime_error& err)
			{
			/* Show an error message: */
			Misc::formattedUserError("Video::ViewerComponent: Unable to change video format on video device %s due to exception %s",videoDeviceList[videoDeviceIndex]->getName().c_str(),err.what());
			}
		}
	}

void ViewerComponent::closeVideoDevice(void)
	{
	/* Stop streaming on the open device: */
	stopStreaming();
	
	/* Close the video device: */
	delete videoDevice;
	videoDevice=0;
	videoFormat.pixelFormat=0x0U;
	videoFormat.size=Size(0,0);
	videoFormat.frameInterval=Math::Rational::nan;
	
	/* Delete the video device's control panel: */
	delete videoControlPanel;
	videoControlPanel=0;
	}

ViewerComponent::ViewerComponent(unsigned int sVideoDeviceIndex,const VideoDataFormat& initialFormat,int initialFormatComponentMask,GLMotif::WidgetManager* sWidgetManager)
	:videoDeviceIndex(sVideoDeviceIndex),
	 videoDevice(0),videoExtractor(0),
	 storeVideoFrames(true),
	 videoFrameVersion(0),
	 videoFrameCallback(0),videoFormatChangedCallback(0),videoFormatSizeChangedCallback(0),
	 widgetManager(sWidgetManager),
	 videoDevicesDialog(0),videoControlPanel(0)
	{
	/* Query the list of all connected video devices: */
	videoDeviceList=VideoDevice::getVideoDevices();
	if(videoDeviceList.empty())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No video devices connected to host");
	
	/* Create the video devices dialog: */
	videoDevicesDialog=createVideoDevicesDialog();
	
	/* Open the selected video device: */
	openVideoDevice(sVideoDeviceIndex,initialFormat,initialFormatComponentMask);
	}

ViewerComponent::ViewerComponent(const char* videoDeviceName,unsigned int videoDeviceNameIndex,const VideoDataFormat& initialFormat,int initialFormatComponentMask,GLMotif::WidgetManager* sWidgetManager)
	:videoDeviceIndex(0),
	 videoDevice(0),videoExtractor(0),
	 storeVideoFrames(true),
	 videoFrameVersion(0),
	 videoFrameCallback(0),videoFormatChangedCallback(0),videoFormatSizeChangedCallback(0),
	 widgetManager(sWidgetManager),
	 videoDevicesDialog(0),videoControlPanel(0)
	{
	/* Query the list of all connected video devices: */
	videoDeviceList=VideoDevice::getVideoDevices();
	if(videoDeviceList.empty())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No video devices connected to host");
	
	/* Find a video device whose name matches the given name and index: */
	videoDeviceIndex=videoDeviceList.size();
	unsigned int vdni=videoDeviceNameIndex;
	for(unsigned int i=0;i<videoDeviceIndex;++i)
		if(strcasecmp(videoDeviceList[i]->getName().c_str(),videoDeviceName)==0)
			{
			if(vdni==0U)
				{
				/* Select the matching video device and bail out: */
				videoDeviceIndex=i;
				break;
				}
			
			/* Try the next video device of the same name: */
			--vdni;
			}
	
	if(videoDeviceIndex>=videoDeviceList.size())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Fewer than %u video devices of name %s connected to host",videoDeviceNameIndex+1,videoDeviceName);
	
	/* Create the video devices dialog: */
	videoDevicesDialog=createVideoDevicesDialog();
	
	/* Open the selected video device: */
	openVideoDevice(videoDeviceIndex,initialFormat,initialFormatComponentMask);
	}

ViewerComponent::~ViewerComponent(void)
	{
	/* Close the open video device: */
	closeVideoDevice();
	
	/* Delete the callback functions: */
	delete videoFrameCallback;
	delete videoFormatChangedCallback;
	delete videoFormatSizeChangedCallback;
	
	/* Delete UI components: */
	delete videoDevicesDialog;
	delete videoControlPanel;
	}

void ViewerComponent::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem(this);
	contextData.addDataItem(this,dataItem);
	
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->videoTextureId);
	
	/* Initialize basic texture settings: */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	}

std::pair<VideoDataFormat,int> ViewerComponent::parseVideoFormat(int& argc,char**& argv)
	{
	/* Process the command line: */
	VideoDataFormat format;
	int formatComponentMask=0x0;
	for(int argi=1;argi<argc;++argi)
		{
		/* Keep track of the number of arguments to remove at the end of this iteration: */
		bool removeDangling=false;
		int removeArgs=0;
		
		/* Parse the current argument: */
		char* arg=argv[argi];
		if(arg[0]=='-')
			{
			if(strcasecmp(arg+1,"size")==0||strcasecmp(arg+1,"S")==0)
				{
				if(argi+2<argc)
					{
					/* Parse the desired video frame size: */
					int size[2];
					for(int i=0;i<2;++i)
						size[i]=atoi(argv[argi+1+i]);
					
					/* Check the frame size for validity: */
					if(size[0]>0&&size[1]>0)
						{
						format.size=Size(size[0],size[1]);
						formatComponentMask|=0x1;
						}
					else
						std::cerr<<"ViewerComponent: Ignoring invalid frame size "<<argv[argi+1]<<'x'<<argv[argi+2]<<std::endl;
					
					removeArgs=3;
					}
				else
					removeDangling=true;
				}
			else if(strcasecmp(arg+1,"rate")==0||strcasecmp(arg+1,"R")==0)
				{
				if(argi+1<argc)
					{
					/* Parse the desired video frame rate as a rational number: */
					const char* start=argv[argi+1];
					const char* end;
					for(end=start;*end!='\0';++end)
						;
					try
						{
						format.frameInterval=Misc::ValueCoder<Math::Rational>::decode(start,end,&start).invert();
						start=Misc::skipWhitespace(start,end);
						}
					catch(const std::runtime_error& err)
						{
						format.frameInterval=Math::Rational::nan;
						}
					
					/* Check the frame rate for validity: */
					if(format.frameInterval.isFinite())
						formatComponentMask|=0x2;
					else
						std::cerr<<"ViewerComponent: Ignoring invalid frame rate "<<argv[argi+1]<<std::endl;
					
					removeArgs=2;
					}
				else
					removeDangling=true;
				}
			else if(strcasecmp(arg+1,"format")==0||strcasecmp(arg+1,"F")==0)
				{
				if(argi+1<argc)
					{
					/* Parse the desired pixel format as a FourCC value: */
					format.setPixelFormat(argv[argi+1]);
					formatComponentMask|=0x4;
					
					removeArgs=2;
					}
				else
					removeDangling=true;
				}
			else if(strcasecmp(arg+1,"hexFormat")==0||strcasecmp(arg+1,"HF")==0)
				{
				if(argi+1<argc)
					{
					/* Parse the desired pixel format as a hexadecimal value: */
					long hf=strtol(argv[argi+1],0,16);
					if(hf>=0&&hf<1L<<32)
						{
						/* Set pixel format as integer: */
						format.pixelFormat=(unsigned int)hf;
						formatComponentMask|=0x4;
						}
					else
						std::cerr<<"ViewerComponent: Ignoring invalid hexadecimal pixel format "<<argv[argi+1]<<std::endl;
					
					removeArgs=2;
					}
				else
					removeDangling=true;
				}
			
			/* Check if there was a dangling video format option: */
			if(removeDangling)
				{
				std::cerr<<"ViewerComponent: Ignoring dangling "<<arg<<" option"<<std::endl;
				removeArgs=argc-argi;
				}
			}
		
		if(removeArgs>0)
			{
			/* Remove the requested number of command line arguments: */
			for(int i=argi;i<argc-removeArgs;++i)
				argv[i]=argv[i+removeArgs];
			argc-=removeArgs;
			
			--argi;
			}
		}
	
	return std::make_pair(format,formatComponentMask);
	}

GLMotif::Widget* ViewerComponent::getVideoDevicesDialog(void)
	{
	return videoDevicesDialog;
	}

GLMotif::Widget* ViewerComponent::getVideoControlPanel(void)
	{
	return videoControlPanel;
	}

void ViewerComponent::setVideoFrameCallback(ViewerComponent::VideoFrameCallback* newVideoFrameCallback,bool newStoreVideoFrames)
	{
	/* Delete the current callback and install the new one: */
	{
	Threads::Spinlock::Lock videoFrameCallbackLock(videoFrameCallbackMutex);
	delete videoFrameCallback;
	videoFrameCallback=newVideoFrameCallback;
	}
	
	/* Enable/disable automatic display of new video frames: */
	storeVideoFrames=newStoreVideoFrames||videoFrameCallback==0;
	}

void ViewerComponent::setVideoFormatChangedCallback(ViewerComponent::VideoFormatChangedCallback* newVideoFormatChangedCallback)
	{
	/* Delete the current callback and install the new one: */
	delete videoFormatChangedCallback;
	videoFormatChangedCallback=newVideoFormatChangedCallback;
	}

void ViewerComponent::setVideoFormatSizeChangedCallback(ViewerComponent::VideoFormatChangedCallback* newVideoFormatSizeChangedCallback)
	{
	/* Delete the current callback and install the new one: */
	delete videoFormatSizeChangedCallback;
	videoFormatSizeChangedCallback=newVideoFormatSizeChangedCallback;
	}

void ViewerComponent::storeVideoFrame(const Images::BaseImage& frame)
	{
	/* Store the given frame in the input triple buffer: */
	videoFrames.postNewValue(frame);
	}

void ViewerComponent::frame(void)
	{
	/* Lock the most recent video frame in the input triple buffer: */
	if(videoFrames.lockNewValue())
		{
		/* Bump up the video frame's version number to invalidate the cached texture: */
		++videoFrameVersion;
		}
	}

}

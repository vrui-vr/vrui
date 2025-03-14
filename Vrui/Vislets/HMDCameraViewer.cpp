/***********************************************************************
HMDCameraViewer - Vislet class to show a live pass-through video feed
from a mono or stereo camera attached to a head-mounted display.
Copyright (c) 2020-2024 Oliver Kreylos

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

#include <Vrui/Vislets/HMDCameraViewer.h>

#include <vector>
#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/FixedArray.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Math/MathValueCoders.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <GL/GLGeometryWrappers.h>
#include <Video/Types.h>
#include <Video/VideoDevice.h>
#include <Video/ImageExtractor.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/VisletManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Internal/Config.h>

namespace Vrui {

namespace Vislets {

/**********************************************************
Methods of class HMDCameraViewerFactory::ToggleToolFactory:
**********************************************************/

HMDCameraViewerFactory::ToggleToolFactory::ToggleToolFactory(ToolManager& toolManager,HMDCameraViewerFactory* sVisletFactory)
	:ToolFactory("HMDCameraViewerToggleTool",toolManager),
	 visletFactory(sVisletFactory)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* parentFactory=toolManager.loadClass("UtilityTool");
	parentFactory->addChildClass(this);
	addParentClass(parentFactory);
	
	/* Set tool class' factory pointer: */
	ToggleTool::factory=this;
	}

HMDCameraViewerFactory::ToggleToolFactory::~ToggleToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ToggleTool::factory=0;
	}

const char* HMDCameraViewerFactory::ToggleToolFactory::getName(void) const
	{
	return "Toggle HMD Camera Viewer";
	}

const char* HMDCameraViewerFactory::ToggleToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	return "Toggle";
	}

Tool* HMDCameraViewerFactory::ToggleToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ToggleTool(this,inputAssignment);
	}

void HMDCameraViewerFactory::ToggleToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

/***********************************************************
Static elements of class HMDCameraViewerFactory::ToggleTool:
***********************************************************/

HMDCameraViewerFactory::ToggleToolFactory* HMDCameraViewerFactory::ToggleTool::factory=0;

/***************************************************
Methods of class HMDCameraViewerFactory::ToggleTool:
***************************************************/

HMDCameraViewerFactory::ToggleTool::ToggleTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(sFactory,inputAssignment)
	{
	}

const ToolFactory* HMDCameraViewerFactory::ToggleTool::getFactory(void) const
	{
	return factory;
	}


void HMDCameraViewerFactory::ToggleTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	/* Get the active HMD camera viewer vislet: */
	HMDCameraViewer* vislet=factory->visletFactory->vislet;
	if(vislet!=0&&cbData->newButtonState) // Button has just been pressed
		{
		/* Check the current enable state of the vislet: */
		if(vislet->isActive())
			vislet->disable(false);
		else
			vislet->enable(false);
		getVisletManager()->updateVisletMenu(vislet);
		}
	}

/***************************************
Methods of class HMDCameraViewerFactory:
***************************************/

HMDCameraViewerFactory::HMDCameraViewerFactory(VisletManager& visletManager)
	:VisletFactory("HMDCameraViewer",visletManager),
	 viewer(0),vislet(0)
	{
	#if 0
	/* Insert vislet class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	
	/* Find the viewer to which the camera is attached: */
	std::string viewerName=cfs.retrieveString("./viewerName");
	viewer=findViewer(viewerName.c_str());
	if(viewer==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Viewer %s not found",viewerName.c_str());
	
	/* Retrieve the camera's video device identifier: */
	videoDeviceName=cfs.retrieveString("./videoDeviceName");
	videoDeviceIndex=cfs.retrieveValue<unsigned int>("./videoDeviceIndex",0);
	
	/* Retrieve the camera's video format: */
	videoFormat.size=cfs.retrieveValue<Video::Size>("./frameSize");
	videoFormat.frameInterval=cfs.retrieveValue("./frameRate",Math::Rational(30)).inverse();
	std::string pixelFormat=cfs.retrieveString("./pixelFormat","YUYV");
	if(pixelFormat.length()!=4)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid pixel format \"%s\"",pixelFormat.c_str());
	videoFormat.setPixelFormat(pixelFormat.c_str());
	
	/* Retrieve the camera's stereo mode and projection settings: */
	stereo=cfs.retrieveValue("./stereo",false);
	if(stereo)
		{
		subFrames[0]=cfs.retrieveValue<IRect>("./leftSubFrame");
		intrinsicsNames[0]=cfs.retrieveString("./leftIntrinsicsName");
		subFrames[1]=cfs.retrieveValue<IRect>("./rightSubFrame");
		intrinsicsNames[1]=cfs.retrieveString("./rightIntrinsicsName");
		}
	else
		{
		subFrames[0]=IRect(videoFormat.size);
		cfs.updateValue("./subFrame",subFrames[0]);
		intrinsicsNames[0]=cfs.retrieveString("./intrinsicsName");
		}
	extrinsics=cfs.retrieveValue("./extrinsics",Rotation::identity);
	sphereRadius=Video::IntrinsicParameters::Scalar(cfs.retrieveValue("./sphereRadius",getMeterFactor()));
	
	/* Read expected camera latency: */
	double latencyS=cfs.retrieveValue("./cameraLatency",double(videoFormat.frameInterval));
	cameraLatency=Realtime::TimeStamp(Realtime::TimeStamp::TSType(Math::floor(latencyS*1000000.0+0.5)));
	
	/* Set vislet class's factory pointer: */
	HMDCameraViewer::factory=this;
	
	/* Create the toggle tool factory: */
	toggleToolFactory=new ToggleToolFactory(*getToolManager(),this);
	getToolManager()->addClass(toggleToolFactory,ToolManager::defaultToolFactoryDestructor);
	}

HMDCameraViewerFactory::~HMDCameraViewerFactory(void)
	{
	/* Destroy the toggle tool factory: */
	getToolManager()->releaseClass(toggleToolFactory);
	
	/* Reset vislet class's factory pointer: */
	HMDCameraViewer::factory=0;
	}

Vislet* HMDCameraViewerFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new HMDCameraViewer(numArguments,arguments);
	}

void HMDCameraViewerFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveHMDCameraViewerDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createHMDCameraViewerFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	HMDCameraViewerFactory* factory=new HMDCameraViewerFactory(*visletManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyHMDCameraViewerFactory(VisletFactory* factory)
	{
	delete factory;
	}

/******************************************
Methods of class HMDCameraViewer::DataItem:
******************************************/

HMDCameraViewer::DataItem::DataItem(void)
	:videoTextureId(0),
	 videoTextureVersion(0)
	{
	/* Check whether non-power-of-two-dimension textures are supported: */
	haveNpotdt=GLARBTextureNonPowerOfTwo::isSupported();
	if(haveNpotdt)
		GLARBTextureNonPowerOfTwo::initExtension();
	
	/* Create the video texture object: */
	glGenTextures(1,&videoTextureId);
	}

HMDCameraViewer::DataItem::~DataItem(void)
	{
	/* Destroy the video texture object: */
	glDeleteTextures(1,&videoTextureId);
	}

/****************************************
Static elements of class HMDCameraViewer:
****************************************/

HMDCameraViewerFactory* HMDCameraViewer::factory=0;

/********************************
Methods of class HMDCameraViewer:
********************************/

void* HMDCameraViewer::streamingThreadMethod(void)
	{
	Video::VideoDevice* videoDevice=0;
	Video::ImageExtractor* videoExtractor=0;
	
	try
		{
		/* Open the requested video device: */
		videoDevice=Video::VideoDevice::openVideoDevice(factory->videoDeviceName,factory->videoDeviceIndex);
		
		/* Set the video device's video format: */
		Video::VideoDataFormat videoFormat=factory->videoFormat;
		videoDevice->setVideoFormat(videoFormat);
		
		/* Create an image extractor to convert from the video device's raw image format to RGB: */
		videoExtractor=videoDevice->createImageExtractor();
		
		/* Keep capturing video frames until interrupted: */
		while(runStreamingThread)
			{
			/* Wait until the vislet is activated: */
			{
			Threads::MutexCond::Lock activationLock(activationCond);
			while(runStreamingThread&&!isActive())
				activationCond.wait(activationLock);
			}
			
			/* Bail out if the vislet is shutting down: */
			if(!runStreamingThread)
				break;
			
			/* Start capturing video from the video device: */
			videoDevice->allocateFrameBuffers(5);
			videoDevice->startStreaming();
			
			/* Capture video frames while the vislet is active: */
			while(isActive())
				{
				/* Capture the next video frame: */
				Video::FrameBuffer* frameBuffer=videoDevice->dequeueFrame();
				
				/* Estimate the time at which the frame was captured: */
				Realtime::TimeStamp timeStamp=Realtime::TimeStamp::now();
				timeStamp-=factory->cameraLatency;
				
				/* Start a new value in the input triple buffer: */
				Frame& frame=videoFrames.startNewValue();
				frame.valid=true;
				
				/* Check if the current image needs to be created: */
				if(!frame.frame.isValid())
					{
					/* Create a new RGB image: */
					frame.frame=Images::BaseImage(factory->videoFormat.size,3,sizeof(GLubyte),GL_RGB,GL_UNSIGNED_BYTE);
					}
				
				/* Extract an RGB image from the provided frame buffer into the current image, interpreted as an RGB image: */
				videoExtractor->extractRGB(frameBuffer,frame.frame.replacePixels());
				
				/* Find the closest head orientation in the orientation buffer: */
				{
				Threads::Spinlock::Lock orientationsLock(orientationsMutex);
				int l=0;
				int r=orientationsEnd-orientations;
				while(r-l>1)
					{
					int m=(l+r)/2;
					OrientationSample* mPtr=orientationsHead+m;
					if(mPtr>=orientationsEnd)
						mPtr-=(orientationsEnd-orientations);
					
					if(mPtr->timeStamp.notAfter(timeStamp))
						l=m;
					else
						r=m;
					}
				OrientationSample* lPtr=orientationsHead+l;
				if(lPtr>=orientationsEnd)
					lPtr-=(orientationsEnd-orientations);
				frame.headOrientation=lPtr->orientation;
				frame.valid=true;
				}
				
				/* Finish the new image in the input triple buffer: */
				videoFrames.postNewValue();
				
				/* Wake up the main loop: */
				Vrui::requestUpdate();
				
				/* Put the captured frame back into the pool: */
				videoDevice->enqueueFrame(frameBuffer);
				}
			
			/* Stop capturing video from the video device: */
			videoDevice->stopStreaming();
			videoDevice->releaseFrameBuffers();
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Signal an error: */
		Misc::formattedUserError("Vrui::HMDCameraViewer: Shutting down due to exception %s",err.what());
		}
	
	/* Shut down the video device: */
	delete videoExtractor;
	delete videoDevice;
	
	return 0;
	}

HMDCameraViewer::HMDCameraViewer(int numArguments,const char* const arguments[])
	:runStreamingThread(false),
	 videoDevice(0),videoExtractor(0),
	 videoFrameVersion(0),
	 orientations(new OrientationSample[90]),orientationsEnd(orientations+90),orientationsHead(orientations)
	{
	/* Set the factory's vislet: */
	factory->vislet=this;
	
	/* Load the intrinsic camera parameter file(s): */
	unsigned int numEyes=factory->stereo?2:1;
	IO::DirectoryPtr resourceDir=IO::openDirectory(VRUI_INTERNAL_CONFIG_SHAREDIR "/Resources");
	for(unsigned int eye=0;eye<numEyes;++eye)
		{
		IO::FilePtr intrinsicsFile=resourceDir->openFile(factory->intrinsicsNames[eye].c_str());
		intrinsicsFile->setEndianness(Misc::LittleEndian);
		intrinsics[eye].read(*intrinsicsFile);
		}
	}

HMDCameraViewer::~HMDCameraViewer(void)
	{
	/* Release allocated resources: */
	delete[] orientations;
	
	/* Reset the factory's vislet: */
	if(factory->vislet==this)
		factory->vislet=0;
	}

VisletFactory* HMDCameraViewer::getFactory(void) const
	{
	return factory;
	}

void HMDCameraViewer::enable(bool startup)
	{
	/* Start the video streaming thread but don't activate the vislet on start-up: */
	if(startup)
		{
		/* Start the streaming thread: */
		runStreamingThread=true;
		streamingThread.start(this,&HMDCameraViewer::streamingThreadMethod);
		}
	else
		{
		/* Invalidate all video frames: */
		for(int i=0;i<3;++i)
			videoFrames.getBuffer(i).valid=false;
		
		/* Initialize the head orientation buffer: */
		OrientationSample o;
		o.timeStamp=Realtime::TimeStamp::now();
		o.orientation=factory->viewer->getHeadTransformation().getRotation();
		for(OrientationSample* oPtr=orientations;oPtr!=orientationsEnd;++oPtr)
			*oPtr=o;
		orientationsHead=orientations;
		
		/* Enable the vislet as far as the vislet manager is concerned: */
		Vislet::enable(false);
		
		/* Wake up the streaming thread: */
		activationCond.signal();
		}
	}

void HMDCameraViewer::disable(bool shutdown)
	{
	/* Disable the vislet as far as the vislet manager is concerned: */
	Vislet::disable(shutdown);
	
	if(shutdown)
		{
		/* Terminate the streaming thread: */
		runStreamingThread=false;
		activationCond.signal();
		streamingThread.join();
		}
	}

void HMDCameraViewer::frame(void)
	{
	/* Sample the head orientation: */
	OrientationSample o;
	o.timeStamp=Realtime::TimeStamp::now();
	o.orientation=factory->viewer->getHeadTransformation().getRotation();
	{
	Threads::Spinlock::Lock orientationsLock(orientationsMutex);
	*orientationsHead=o;
	if(++orientationsHead==orientationsEnd)
		orientationsHead=orientations;
	}
	
	/* Lock the most recent video frame in the input triple buffer: */
	if(videoFrames.lockNewValue())
		{
		/* Bump up the video frame's version number to invalidate the cached texture: */
		++videoFrameVersion;
		}
	}

void HMDCameraViewer::display(GLContextData& contextData) const
	{
	/* Access the current display state: */
	const Vrui::DisplayState& displayState=Vrui::getDisplayState(contextData);
	
	/* Bail out if this is the wrong viewer: */
	if(displayState.viewer!=factory->viewer)
		return;
	
	/* Access the most recent video frame: */
	const Frame& frame=videoFrames.getLockedValue();
	
	/* Bail out if the frame is not valid: */
	if(!frame.valid)
		return;
	
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->videoTextureId);
	
	/* Check if the cached texture is outdated: */
	if(dataItem->videoTextureVersion!=videoFrameVersion)
		{
		/* Upload the new video frame into the texture object: */
		frame.frame.glTexImage2D(GL_TEXTURE_2D,0,!dataItem->haveNpotdt);
		dataItem->videoTextureVersion=videoFrameVersion;
		}
	
	/* Go to eye space: */
	glPushMatrix();
	glLoadMatrix(displayState.mvpGl);
	glTranslate(displayState.eyePosition-Vrui::Point::origin);
	glRotate(frame.headOrientation);
	glRotate(factory->extrinsics);
	
	/* Distortion-correct and project the video frame to display space: */
	int eyeIndex=displayState.eyeIndex;
	const IRect& subFrame=factory->subFrames[eyeIndex];
	unsigned int numRows=(subFrame.size[1]-1U+15U)/16U;
	unsigned int numColumns=(subFrame.size[0]-1U+15U)/16U;
	const GLfloat* texMin=dataItem->texMin[eyeIndex];
	const GLfloat* texMax=dataItem->texMax[eyeIndex];
	for(unsigned int y=0;y<numRows;++y)
		{
		GLfloat tex0[2],tex1[2];
		tex0[1]=GLfloat(y)*(texMax[1]-texMin[1])/GLfloat(numRows)+texMin[1];
		tex1[1]=GLfloat(y+1)*(texMax[1]-texMin[1])/GLfloat(numRows)+texMin[1];
		
		ImagePoint p0,p1;
		p0[1]=Scalar(y)*Scalar(subFrame.size[1]-1U)/Scalar(numRows)+Scalar(subFrame.offset[1])+Scalar(0.5);
		p1[1]=Scalar(y+1)*Scalar(subFrame.size[1]-1U)/Scalar(numRows)+Scalar(subFrame.offset[1])+Scalar(0.5);
		
		glBegin(GL_QUAD_STRIP);
		for(unsigned int x=0;x<=numColumns;++x)
			{
			tex1[0]=tex0[0]=GLfloat(x)*(texMax[0]-texMin[0])/GLfloat(numColumns)+texMin[0];
			p1[0]=p0[0]=Scalar(x)*Scalar(subFrame.size[0]-1U)/Scalar(numColumns)+Scalar(subFrame.offset[0])+Scalar(0.5);
			
			glTexCoord2f(tex1[0],tex1[1]);
			glVertex(projectImagePoint(eyeIndex,p1));
			glTexCoord2f(tex0[0],tex0[1]);
			glVertex(projectImagePoint(eyeIndex,p0));
			}
		glEnd();
		}
	
	/* Disable eye space projection: */
	glPopMatrix();
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void HMDCameraViewer::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem();
	contextData.addDataItem(this,dataItem);
	
	/* Calculate the texture coordinate rectangle: */
	unsigned int texSize[2];
	if(dataItem->haveNpotdt)
		{
		for(int i=0;i<2;++i)
			texSize[i]=factory->videoFormat.size[i];
		}
	else
		{
		/* Find the next larger power-of-two texture size: */
		for(int i=0;i<2;++i)
			for(texSize[i]=1U;texSize[i]<factory->videoFormat.size[i];texSize[i]<<=1)
				;
		}
	
	/* Calculate the per-eye texture layout: */
	unsigned int numEyes=factory->stereo?2:1;
	for(unsigned int eye=0;eye<numEyes;++eye)
		{
		for(int i=0;i<2;++i)
			{
			dataItem->texMin[eye][i]=(GLfloat(factory->subFrames[eye].offset[i])+0.5f)/GLfloat(texSize[i]);
			dataItem->texMax[eye][i]=(GLfloat(factory->subFrames[eye].offset[i]+factory->subFrames[eye].size[i])-0.5f)/GLfloat(texSize[i]);
			}
		}
	
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->videoTextureId);
	
	/* Initialize basic texture settings: */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	}

}

}

/***********************************************************************
VRWindow - Abstract base class for OpenGL windows that are used to map
one or two eyes of a viewer onto a VR screen using a variety of mono or
stereo rendering methods.
Copyright (c) 2004-2024 Oliver Kreylos

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

#include <Vrui/VRWindow.h>

#define SAVE_SCREENSHOT_PROJECTION 0
#define RENDERFRAMETIMES 0
#define SAVE_MOUSEMOVEMENTS 0

#include <Vrui/Internal/Config.h>

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#if SAVE_SCREENSHOT_PROJECTION
#include <IO/File.h>
#include <IO/OpenFile.h>
#endif
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#if VRUI_INTERNAL_CONFIG_HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif
#if VRUI_INTERNAL_CONFIG_HAVE_XINPUT2
#include <X11/extensions/XInput2.h>
#endif
#include <GL/GLColorTemplates.h>
#include <GL/GLMiscTemplates.h>
#include <GL/GLPrintError.h>
#include <GL/GLFont.h>
#include <GL/Extensions/GLARBMultisample.h>
#include <GL/Extensions/GLEXTFramebufferSRGB.h>
#include <GL/GLValueCoders.h>
#include <GL/GLTransformationWrappers.h>
#include <Images/Config.h>
#include <Images/RGBImage.h>
#include <Images/WriteImageFile.h>
#include <Vrui/Internal/Vrui.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/InputDeviceAdapterMouse.h>
#include <Vrui/Internal/InputDeviceAdapterMultitouch.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/Internal/ToolKillZone.h>
#include <Vrui/ToolManager.h>
#include <Vrui/UIManager.h>
#include <Vrui/GetOutputConfiguration.h>
#include <Vrui/WindowProperties.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Internal/MovieSaver.h>

#include <Vrui/Internal/VRWindowMono.h>
#include <Vrui/Internal/VRWindowAnaglyph.h>
#include <Vrui/Internal/VRWindowAnaglyph2.h>
#include <Vrui/Internal/VRWindowQuadbuffer.h>
#include <Vrui/Internal/VRWindowSplitSingleViewport.h>
#include <Vrui/Internal/VRWindowCompositorClient.h>
#include <Vrui/Internal/VRWindowCubeMap.h>

namespace Vrui {

/*************************
Methods of class VRWindow:
*************************/

void VRWindow::calcPanRect(const GLWindow::Rect& rect,Scalar panRect[4])
	{
	/* Calculate the panning rectangle from the given rectangle and the output configuration's panning dommain: */
	panRect[0]=Scalar(rect.offset[0]-outputConfiguration.domain.offset[0])/Scalar(outputConfiguration.domain.size[0]);
	panRect[1]=Scalar(rect.offset[0]+rect.size[0]-outputConfiguration.domain.offset[0])/Scalar(outputConfiguration.domain.size[0]);
	panRect[2]=Scalar(1)-Scalar(rect.offset[1]+rect.size[1]-outputConfiguration.domain.offset[1])/Scalar(outputConfiguration.domain.size[1]);
	panRect[3]=Scalar(1)-Scalar(rect.offset[1]-outputConfiguration.domain.offset[1])/Scalar(outputConfiguration.domain.size[1]);
	}

void VRWindow::placeToolKillZone(void)
	{
	/* Bail out if the window does not have exactly one screen: */
	if(getNumVRScreens()!=1)
		return;
	
	/* Retrieve the window's screen, its size, and its transformation: */
	VRScreen* screen=getVRScreen(0);
	Scalar screenW=screen->getWidth();
	Scalar screenH=screen->getHeight();
	ONTransform screenT=screen->getScreenTransformation();
	
	/* Get the kill zone's size: */
	ToolKillZone* killZone=getToolManager()->getToolKillZone();
	Vrui::Size killZoneSize=killZone->getSize();
	
	/* Move the kill zone to its new window-relative position: */
	Scalar hw=Math::div2(killZoneSize[0]);
	Scalar hh=Math::div2(killZoneSize[1]);
	Point center(panRect[0]*screenW+hw+((panRect[1]-panRect[0])*screenW-hw-hw)*toolKillZonePos[0],panRect[2]*screenH+hh+((panRect[3]-panRect[2])*screenH-hh-hh)*toolKillZonePos[1],0);
	killZone->setCenter(screenT.transform(center));
	vruiState->navigationTransformationChangedMask|=0x4;
	}

Scalar* VRWindow::writePanRect(const VRScreen* screen,Scalar screenRect[4]) const
	{
	/* Scale the panning rectangle by the given screen's size: */
	for(int i=0;i<2;++i)
		{
		screenRect[0+i]=panRect[0+i]*screen->getWidth();
		screenRect[2+i]=panRect[2+i]*screen->getHeight();
		}
	
	return screenRect;
	}

void VRWindow::rectChangedCallback(GLWindow::RectChangedCallbackData* cbData)
	{
	/* Forward to the virtual method: */
	rectChanged(cbData->oldRect,cbData->newRect);
	}

void VRWindow::enableButtonCallback(Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	enabled=cbData->newButtonState;
	if(invertEnableButton)
		enabled=!enabled;
	}

void VRWindow::setRectCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	VRWindow* thisPtr=static_cast<VRWindow*>(userData);
	
	/* Parse the argument list: */
	Rect newRect;
	for(int index=0;index<4;++index)
		{
		/* Find the start of the next argument: */
		for(;argumentsBegin!=argumentsEnd&&isspace(*argumentsBegin);++argumentsBegin)
			;
		
		/* Throw an exception if there are no more arguments: */
		if(argumentsBegin==argumentsEnd)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Not enough arguments");
		
		/* Get and store the next argument: */
		if(index<2)
			newRect.offset[index]=Misc::ValueCoder<int>::decode(argumentsBegin,argumentsEnd,&argumentsBegin);
		else
			newRect.size[index-2]=Misc::ValueCoder<unsigned int>::decode(argumentsBegin,argumentsEnd,&argumentsBegin);
		}
	
	/* Set the window position: */
	thisPtr->setRect(newRect);
	}

void VRWindow::toggleMovieSaverCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	/* Toggle the movie recording flag: */
	VRWindow* thisPtr=static_cast<VRWindow*>(userData);
	thisPtr->movieSaverRecording=!thisPtr->movieSaverRecording;
	Misc::formattedLogNote("VRWindow: Movie recording %s",thisPtr->movieSaverRecording?"active":"paused");
	}

void VRWindow::rectChanged(const Rect& oldRect,const Rect& newRect)
	{
	if(panningViewport)
		{
		/* Calculate the window's new panning rectangle: */
		Scalar newPanRect[4];
		calcPanRect(newRect,newPanRect);
		
		/* The rest of panning functionality makes no sense if the window doesn't have exactly one VR screen: */
		if(getNumVRScreens()==1)
			{
			/* Retrieve the screen, its size, and its transformation: */
			VRScreen* screen=getVRScreen(0);
			Scalar screenW=screen->getWidth();
			Scalar screenH=screen->getHeight();
			ONTransform screenT=screen->getScreenTransformation();
			
			/* Calculate the screen's old and new center and size: */
			Point oldCenter=screenT.transform(Point(Math::mid(panRect[0],panRect[1])*screenW,Math::mid(panRect[2],panRect[3])*screenH,0));
			Scalar oldSize=Math::sqrt(Math::sqr((panRect[1]-panRect[0])*screenW)+Math::sqr((panRect[3]-panRect[2])*screenH));
			Point newCenter=screenT.transform(Point(Math::mid(newPanRect[0],newPanRect[1])*screenW,Math::mid(newPanRect[2],newPanRect[3])*screenH,0));
			Scalar newSize=Math::sqrt(Math::sqr((newPanRect[1]-newPanRect[0])*screenW)+Math::sqr((newPanRect[3]-newPanRect[2])*screenH));
			
			/* Calculate an incremental physical-space window transformation: */
			NavTransform navUpdate=NavTransform::translateFromOriginTo(newCenter);
			navUpdate*=NavTransform::scale(newSize/oldSize);
			navUpdate*=NavTransform::translateToOriginFrom(oldCenter);
			
			if(navigate)
				{
				/* Update Vrui's display center and size: */
				setDisplayCenter(newCenter,getDisplaySize()*newSize/oldSize);
				
				/* Try activating a fake navigation tool: */
				if(activateNavigationTool(reinterpret_cast<Tool*>(this)))
					{
					/* Apply the incremental transformation: */
					concatenateNavigationTransformationLeft(navUpdate);
					
					/* Deactivate the fake navigation tool again: */
					deactivateNavigationTool(reinterpret_cast<Tool*>(this));
					}
				}
			
			if(movePrimaryWidgets)
				{
				/* Move all popped-up primary widgets with the window: */
				GLMotif::WidgetManager* wm=getWidgetManager();
				for(GLMotif::WidgetManager::PoppedWidgetIterator wIt=wm->beginPrimaryWidgets();wIt!=wm->endPrimaryWidgets();++wIt)
					{
					/* Calculate the widget's hot spot in physical coordinates: */
					Point hotSpot=wIt.getWidgetToWorld().transform(Point((*wIt)->calcHotSpot().getXyzw()));
					
					/* Translate the widget from the old to the new hot spot: */
					wm->setWidgetTransformation(wIt,GLMotif::WidgetManager::Transformation::translate(navUpdate.transform(hotSpot)-hotSpot)*wIt.getWidgetToWorld());
					}
				}
			}
		
		/* Update the window's panning rectangle: */
		for(int i=0;i<4;++i)
			panRect[i]=newPanRect[i];
		}
	
	/* Place the tool kill zone if it is being tracked: */
	if(trackToolKillZone)
		placeToolKillZone();
	
	if(windowGroup!=0)
		{
		/* Notify the Vrui run-time that the window size has changed: */
		resizeWindow(windowGroup,this,getViewportSize(),getFramebufferSize());
		}
	
	/* Remember that the window was resized: */
	resized=true;
	}

void VRWindow::prepareRender(void)
	{
	/* Continue setting up the display state object: */
	displayState->window=this;
	displayState->windowIndex=windowIndex;
	displayState->resized=resized;
	
	/* Initialize standard OpenGL settings: */
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_ALWAYS,0.0f);
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ZERO);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	if(clearBufferMask&GL_STENCIL_BUFFER_BIT)
		{
		glDisable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS,0,~0x0U);
		glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
		glStencilMask(~0x0U);
		}
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
	if(multisamplingLevel>1)
		glEnable(GL_MULTISAMPLE_ARB);
	else
		glDisable(GL_MULTISAMPLE_ARB);
	if(getContext().isNonlinear())
		glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	else
		glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	}

void VRWindow::render(void)
	{
	/*********************************************************************
	Step one: Clear all relevant buffers.
	*********************************************************************/
	
	/* Clear all relevant buffers: */
	glClearColor(getBackgroundColor());
	glClearDepth(1.0); // Clear depth is "infinity"
	if(clearBufferMask&GL_STENCIL_BUFFER_BIT)
		glClearStencil(0x0);
	if(clearBufferMask&GL_ACCUM_BUFFER_BIT)
		glClearAccum(0.0f,0.0f,0.0f,0.0f);
	glClear(clearBufferMask);
	
	/*********************************************************************
	Step two: Calculate projection and modelview transformations.
	*********************************************************************/
	
	/* Get the current screen's size and inverse transformation: */
	Scalar screenW=displayState->screen->getWidth();
	Scalar screenH=displayState->screen->getHeight();
	ONTransform invScreenT=displayState->screen->getScreenTransformation();
	invScreenT.doInvert();
	
	/* Transform the current eye position to screen coordinates: */
	Point screenEyePos=invScreenT.transform(displayState->eyePosition);
	
	/* Calculate the frustum transformation defined by the (eye, screen) pair: */
	Scalar left=(panRect[0]*screenW-screenEyePos[0])/screenEyePos[2];
	Scalar right=(panRect[1]*screenW-screenEyePos[0])/screenEyePos[2];
	Scalar bottom=(panRect[2]*screenH-screenEyePos[1])/screenEyePos[2];
	Scalar top=(panRect[3]*screenH-screenEyePos[1])/screenEyePos[2];
	Scalar near=getFrontplaneDist();
	Scalar far=getBackplaneDist();
	PTransform::Matrix& pm=displayState->projection.getMatrix();
	pm(0,0)=Scalar(2)/(right-left);
	pm(0,1)=Scalar(0);
	pm(0,2)=(right+left)/(right-left);
	pm(0,3)=Scalar(0);
	pm(1,0)=Scalar(0);
	pm(1,1)=Scalar(2)/(top-bottom);
	pm(1,2)=(top+bottom)/(top-bottom);
	pm(1,3)=Scalar(0);
	pm(2,0)=Scalar(0);
	pm(2,1)=Scalar(0);
	pm(2,2)=-(far+near)/(far-near);
	pm(2,3)=-Scalar(2)*far*near/(far-near);
	pm(3,0)=Scalar(0);
	pm(3,1)=Scalar(0);
	pm(3,2)=-Scalar(1);
	pm(3,3)=Scalar(0);
	
	/* Apply the screen's correction homography if it is projected off-axis: */
	if(displayState->screen->isOffAxis())
		displayState->projection.leftMultiply(displayState->screen->getInverseClipHomography());
	
	/* Upload the projection matrix to OpenGL: */
	glMatrixMode(GL_PROJECTION);
	glLoadMatrix(displayState->projection);
	
	/* Calculate the physical and navigational modelview matrices: */
	displayState->modelviewPhysical=OGTransform::translateToOriginFrom(screenEyePos);
	displayState->modelviewPhysical*=OGTransform(invScreenT);
	displayState->modelviewNavigational=displayState->modelviewPhysical;
	displayState->modelviewNavigational*=getNavigationTransformation();
	
	/* Convert the physical and navigational modelview matrices to column-major OpenGL format: */
	displayState->modelviewPhysical.renormalize();
	{
	Geometry::Matrix<Scalar,4,4> mvpMatrix(Scalar(1));
	displayState->modelviewPhysical.writeMatrix(mvpMatrix);
	Scalar* tPtr=displayState->mvpGl;
	for(int j=0;j<4;++j)
		for(int i=0;i<4;++i,++tPtr)
			*tPtr=mvpMatrix(i,j);
	}
	
	displayState->modelviewNavigational.renormalize();
	{
	Geometry::Matrix<Scalar,4,4> mvnMatrix(Scalar(1));
	displayState->modelviewNavigational.writeMatrix(mvnMatrix);
	Scalar* tPtr=displayState->mvnGl;
	for(int j=0;j<4;++j)
		for(int i=0;i<4;++i,++tPtr)
			*tPtr=mvnMatrix(i,j);
	}
	
	/*********************************************************************
	Step three: Render Vrui state.
	*********************************************************************/
	
	/* Call Vrui's main rendering function: */
	vruiState->display(displayState,getContextData());
	
	/*********************************************************************
	Step four: Render fps counter.
	*********************************************************************/
	
	if(showFps&&burnMode)
		{
		/* Set OpenGL matrices to pixel-based: */
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0,displayState->viewport.size[0],0,displayState->viewport.size[1],0,1);
		
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		#if RENDERFRAMETIMES
		
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		
		/* Render EKG of recent frame rates: */
		glBegin(GL_LINES);
		glColor3f(0.0f,1.0f,0.0f);
		for(int i=0;i<numFrameTimes;++i)
			if(i!=frameTimeIndex)
				{
				glVertex2i(i,0);
				glVertex2i(i,int(floor(frameTimes[i]*1000.0+0.5)));
				}
		glColor3f(1.0f,0.0f,0.0f);
		glVertex2i(frameTimeIndex,0);
		glVertex2i(frameTimeIndex,int(floor(frameTimes[frameTimeIndex]*1000.0+0.5)));
		glEnd();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		
		#else
		
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Print the current frame time: */
		unsigned int fps=(unsigned int)(10.0/vruiState->currentFrameTime+0.5);
		char buffer[20];
		char* bufPtr=buffer+15;
		*(--bufPtr)=char(fps%10+'0');
		fps/=10;
		*(--bufPtr)='.';
		do
			{
			*(--bufPtr)=char(fps%10+'0');
			fps/=10;
			}
		while(bufPtr>buffer&&fps!=0);
		buffer[15]=' ';
		buffer[16]='f';
		buffer[17]='p';
		buffer[18]='s';
		buffer[19]='\0';
		
		/* Draw the current frame time: */
		getPixelFont()->setHAlignment(GLFont::Right);
		getPixelFont()->setVAlignment(GLFont::Bottom);
		getPixelFont()->drawString(GLFont::Vector(getPixelFont()->getCharacterWidth()*9.5f+2.0f,2.0f,0.0f),bufPtr);
		
		/* Restore OpenGL state: */
		glPopAttrib();
		
		#endif
		
		/* Reset the OpenGL matrices: */
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		}
	
	/* Check for OpenGL errors: */
	glPrintError();
	}

void VRWindow::renderComplete(void)
	{
	/* Take a screen shot if requested: */
	if(saveScreenshot)
		{
		try
			{
			/* Create an RGB image of the same size as the window: */
			Images::RGBImage image(getWindowSize());
			
			/* Read the window contents into an RGB image: */
			image.glReadPixels(Images::Offset(0,0));
			
			/* Save the image buffer to the given image file: */
			Images::writeImageFile(image,screenshotImageFileName.c_str());
			
			#if SAVE_SCREENSHOT_PROJECTION
			
			/* Temporarily load the navigation-space modelview matrix: */
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glMultMatrix(displayState->mvnGl);
			
			/* Query the current projection and modelview matrices: */
			GLdouble proj[16],mv[16];
			glGetDoublev(GL_PROJECTION_MATRIX,proj);
			glGetDoublev(GL_MODELVIEW_MATRIX,mv);
			
			glPopMatrix();
			
			/* Write the matrices to a projection file: */
			{
			IO::FilePtr projFile(IO::openFile((screenshotImageFileName+".proj").c_str(),IO::File::WriteOnly));
			projFile->setEndianness(IO::File::LittleEndian);
			projFile->write(proj,16);
			projFile->write(mv,16);
			}
			
			#endif
			}
		catch(const std::runtime_error& err)
			{
			/* Show an error message: */
			Misc::formattedUserError("Vrui::VRWindow: Cannot save screenshot to file %s due to exception %s",screenshotImageFileName.c_str(),err.what());
			}
		
		saveScreenshot=false;
		}
	
	/* Check if the window is currently saving a movie: */
	if(movieSaverRecording)
		{
		/* Get a fresh frame buffer: */
		MovieSaver::FrameBuffer& frameBuffer=movieSaver->startNewFrame();
		
		/* Update the frame buffer's size and prepare it for writing: */
		frameBuffer.setFrameSize(getWindowSize());
		frameBuffer.prepareWrite();
		
		/* Read the window contents into the movie saver's frame buffer: */
		glPixelStorei(GL_PACK_ALIGNMENT,1);
		glPixelStorei(GL_PACK_SKIP_PIXELS,0);
		glPixelStorei(GL_PACK_ROW_LENGTH,0);
		glPixelStorei(GL_PACK_SKIP_ROWS,0);
		glReadPixels(getWindowSize(),GL_RGB,GL_UNSIGNED_BYTE,frameBuffer.getBuffer());
		
		/* Post the new frame: */
		movieSaver->postNewFrame();
		}
	
	/* Check if the window is in burn mode: */
	if(burnMode)
		{
		/* Check if the spin-up time has passed and burn mode is collecting stats: */
		if(burnModeNumFrames>0U)
			{
			/* Measure the minimum and maximum frame interval: */
			double time=getFrameTime();
			if(burnModeMax<time)
				burnModeMax=time;
			if(burnModeMin>time)
				burnModeMin=time;
			
			++burnModeNumFrames;
			}
		else if(getApplicationTime()>=burnModeStartTime)
			{
			/* Start collecting stats: */
			burnModeFirstFrameTime=getApplicationTime();
			burnModeNumFrames=1U;
			}
		
		/* Request another Vrui frame immediately: */
		requestUpdate();
		}
	
	/* Window is now up-to-date: */
	dirty=false;
	resized=false;
	}

void VRWindow::updateScreenDeviceCommon(const Scalar windowPos[2],const GLWindow::Rect& viewport,const Point& physEyePos,const VRScreen* screen,InputDevice* device) const
	{
	/* Transform the given mouse position from window coordinates to screen coordinates using the given viewport and the window's current panning rectangle: */
	Scalar vpX=(windowPos[0]-Scalar(viewport.offset[0]))/Scalar(viewport.size[0]);
	Scalar vpY=Scalar(1)-(windowPos[1]-Scalar(viewport.offset[1]))/Scalar(viewport.size[1]);
	Geometry::Point<Scalar,2> screenPos((vpX*(panRect[1]-panRect[0])+panRect[0])*screen->getWidth(),(vpY*(panRect[3]-panRect[2])+panRect[2])*screen->getHeight());
	
	/* If the screen is projected off-axis, transform the screen position from rectified screen coordinates to projected screen coordinates: */
	if(screen->isOffAxis())
		screenPos=screen->getScreenHomography().transform(screenPos);
	
	/* Get the current screen transformation: */
	ONTransform screenT=screen->getScreenTransformation();
	
	/* Calculate the screen device's initial position and orientation (rotate by 90 degrees around negative x axis to have y axis point into screen): */
	ONTransform deviceT(screenT.transform(Point(screenPos[0],screenPos[1],Scalar(0)))-Point::origin,screenT.getRotation()*Rotation::rotateX(Math::rad(-90.0)));
	
	/* Transform the physical-space eye position to device coordinates: */
	Point deviceEyePos=deviceT.inverseTransform(physEyePos);
	
	/* Calculate the ray direction and ray origin offset in device coordinates: */
	Vector deviceRayDir=Point::origin-deviceEyePos;
	Scalar deviceRayDirLen=Geometry::mag(deviceRayDir);
	deviceRayDir/=deviceRayDirLen;
	Scalar deviceRayStart=-(deviceEyePos[1]+getFrontplaneDist())*deviceRayDirLen/deviceEyePos[1];
	
	/* Update the device's ray: */
	device->setDeviceRay(deviceRayDir,deviceRayStart);
	
	/* Let the UI manager modify the initial screen device position and orientation: */
	getUiManager()->projectDevice(device,deviceT);
	}

namespace {

/***************
Helper functions:
****************/

int stripDisplayScreenSuffix(std::string& displayName) // Strips the ".<screen>" suffix from a display string and returns the stripped screen index as an integer
	{
	int result=-1;
	
	/* Find the X server name component of the display name: */
	std::string::iterator colonIt;
	for(colonIt=displayName.begin();colonIt!=displayName.end()&&*colonIt!=':';++colonIt)
		;
	if(colonIt!=displayName.end())
		{
		/* Find the screen suffix: */
		std::string::iterator periodIt;
		for(periodIt=colonIt+1;periodIt!=displayName.end()&&*periodIt!='.';++periodIt)
			;
		if(periodIt!=displayName.end())
			{
			/* Read the screen index: */
			result=0;
			std::string::iterator screenIt;
			for(screenIt=periodIt+1;screenIt!=displayName.end()&&isdigit(*screenIt);++screenIt)
				result=result*10+(int(*screenIt)-'0');
			if(screenIt!=displayName.end())
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Malformed X display string");
			
			/* Strip the screen suffix: */
			displayName.erase(periodIt,displayName.end());
			}
		}
	
	return result;
	}

}

std::pair<std::string,int> VRWindow::getDisplayName(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Retrieve the name of the default X display from the environment: */
	std::string displayName;
	const char* envDisplayName=getenv("DISPLAY");
	if(envDisplayName!=0)
		displayName=envDisplayName;
	
	/* Update the default with the configured display name and remove an optional ".<screen>" suffix: */
	configFileSection.updateString("./display",displayName);
	int screen=stripDisplayScreenSuffix(displayName);
	
	/* Override the screen index extracted from the display name: */
	configFileSection.updateValue("./screen",screen);
	
	return std::make_pair(displayName,screen);
	}

void VRWindow::updateContextProperties(GLContext::Properties& contextProperties,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Query the window's type: */
	std::string windowType=configFileSection.retrieveString("./windowType");
	
	/* Determine whether this window renders directly to its implicit framebuffer, or uses an intermediate frame buffer: */
	bool renderToBuffer=windowType=="Anaglyph2"||windowType=="ExtendedModeHMD"||windowType=="CompositorClient"||windowType=="CubeMap"; // Some of these aren't implemented yet...
	
	/* Query visual flags from the given configuration file section: */
	bool vsync=configFileSection.retrieveValue("./vsync",false);
	bool frontBufferRendering=vsync&&renderToBuffer&&!configFileSection.retrieveValue("./useBackBuffer",false);
	
	if(renderToBuffer)
		{
		if(!frontBufferRendering)
			{
			/* Request a back buffer: */
			contextProperties.backbuffer=true;
			}
		}
	else
		{
		/* Request a direct-rendering visual, i.e., one with a back buffer and a depth buffer: */
		contextProperties.direct=true;
		contextProperties.backbuffer=true;
		
		/* Check if multisampling is requested: */
		int multisamplingLevel=configFileSection.retrieveValue<int>("./multisamplingLevel",1);
		if(contextProperties.numSamples<multisamplingLevel)
			contextProperties.numSamples=multisamplingLevel;
		
		/* Check if quadbuffer stereo is requested: */
		std::string windowType=configFileSection.retrieveString("./windowType");
		if(windowType=="Quadbuffer"||windowType=="QuadbufferStereo")
			contextProperties.stereo=true;
		}
	}

VRWindow* VRWindow::createWindow(GLContext& context,const char* windowName,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Get the display name and screen index: */
	std::pair<std::string,int> displayName=getDisplayName(configFileSection);
	
	/* Get a default output configuration for the window: */
	std::string outputName;
	configFileSection.updateString("./outputName",outputName);
	OutputConfiguration outputConfiguration=getOutputConfiguration(context.getDisplay(),displayName.second,outputName.c_str());
	
	/* Determine an initial window size and position: */
	IRect initialRect;
	
	/* Let the window cover a quarter of its selected output: */
	initialRect.size=ISize(outputConfiguration.domain.size[0]/2,outputConfiguration.domain.size[1]/2);
	
	/* Override the size from the configuration: */
	configFileSection.updateValue("./windowSize",initialRect.size);
	
	/* Limit the window's size to the output: */
	initialRect.size.min(outputConfiguration.domain.size);
	
	/* Center the window on its selected output: */
	for(int i=0;i<2;++i)
		initialRect.offset[i]=outputConfiguration.domain.offset[i]+(outputConfiguration.domain.size[i]-initialRect.size[i])/2;
	
	/* Override the offset and size from the configuration: */
	configFileSection.updateValue("./windowPos",initialRect);
	
	/* Check if the window is supposed to be decorated by the window manager: */
	bool decorate=true;
	configFileSection.updateValue("./decorate",decorate);
	
	/* Read the window's type from the given configuration file section: */
	std::string windowType=configFileSection.retrieveString("./windowType");
	
	/* Create a window of the appropriate derived class: */
	if(windowType=="Mono"||windowType=="LeftEye"||windowType=="RightEye")
		return new VRWindowMono(context,outputConfiguration,windowName,initialRect,decorate,configFileSection);
	else if(windowType=="Anaglyph"||windowType=="Anaglyphic")
		return new VRWindowAnaglyph(context,outputConfiguration,windowName,initialRect,decorate,configFileSection);
	else if(windowType=="Anaglyph2"||windowType=="Anaglyphic2")
		return new VRWindowAnaglyph2(context,outputConfiguration,windowName,initialRect,decorate,configFileSection);
	else if(windowType=="Quadbuffer"||windowType=="QuadbufferStereo")
		return new VRWindowQuadbuffer(context,outputConfiguration,windowName,initialRect,decorate,configFileSection);
	else if(windowType=="SplitViewport"||windowType=="SplitViewportStereo")
		return new VRWindowSplitSingleViewport(context,outputConfiguration,windowName,initialRect,decorate,configFileSection);
	else if(windowType=="CompositorClient")
		return new VRWindowCompositorClient(context,outputConfiguration,windowName,initialRect,decorate,configFileSection);
	else if(windowType=="CubeMap")
		return new VRWindowCubeMap(context,outputConfiguration,windowName,initialRect,decorate,configFileSection);
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unrecognized window type %s",windowType.c_str());
	}

VRWindow::VRWindow(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection)
	:GLWindow(&sContext,sOutputConfiguration.screen,windowName,initialRect,decorate),
	 outputConfiguration(sOutputConfiguration),
	 outputName(configFileSection.retrieveString("./outputName",std::string())),xrandrEventBase(-1),
	 vruiState(0),
	 windowIndex(-1),windowGroup(0),
	 protectScreens(configFileSection.retrieveValue("./protectScreens",true)),
	 panningViewport(configFileSection.retrieveValue("./panningViewport",false)),
	 navigate(configFileSection.retrieveValue("./navigate",false)),
	 movePrimaryWidgets(configFileSection.retrieveValue("./movePrimaryWidgets",false)),
	 trackToolKillZone(false),
	 exitKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./exitKey","Esc"))),
	 homeKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./homeKey","Super+Home"))),
	 screenshotKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./screenshotKey","Super+Print"))),
	 fullscreenToggleKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./fullscreenToggleKey","F11"))),
	 burnModeToggleKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./burnModeToggleKey","Super+ScrollLock"))),
	 pauseMovieSaverKey(KeyMapper::getQualifiedKey(configFileSection.retrieveString("./pauseMovieSaverKey","Super+Pause"))),
	 mouseAdapter(0),multitouchAdapter(0),xinput2Opcode(0),
	 enableButtonDevice(0),enableButtonIndex(-1),invertEnableButton(false),
	 multisamplingLevel(configFileSection.retrieveValue<int>("./multisamplingLevel",1)),
	 displayState(0),
	 clearBufferMask(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT),
	 frontBufferRendering(false),
	 dirty(true),resized(true),enabled(true),
	 disabledColor(0.5f,0.5f,0.5f,1.0f),
	 haveSync(GLARBSync::isSupported()),drawFence(0),
	 vsync(configFileSection.retrieveValue("./vsync",false)),
	 synchronize(false),
	 lowLatency(configFileSection.retrieveValue("./lowLatency",false)),
	 saveScreenshot(false),movieSaver(0),movieSaverRecording(configFileSection.retrieveValue("./saveMovieAutostart",false)),
	 showFps(configFileSection.retrieveValue<bool>("./showFps",false)),burnMode(false)
	{
	/* Update the window's X event mask: */
	{
	XWindowAttributes wa;
	XGetWindowAttributes(getContext().getDisplay(),getWindow(),&wa);
	XSetWindowAttributes swa;
	swa.event_mask=wa.your_event_mask|FocusChangeMask;
	XChangeWindowAttributes(getContext().getDisplay(),getWindow(),CWEventMask,&swa);
	}
	
	#if VRUI_INTERNAL_CONFIG_HAVE_XRANDR
	
	if(!outputName.empty())
		{
		/* Determine whether XRANDR is supported on the X server: */
		int xrandrErrorBase;
		if(XRRQueryExtension(getContext().getDisplay(),&xrandrEventBase,&xrandrErrorBase))
			{
			/* Request XRANDR events to keep the window in the screen area assigned to the requested output: */
			XRRSelectInput(getContext().getDisplay(),getWindow(),RRScreenChangeNotifyMask);
			}
		else
			xrandrEventBase=-1;
		}
	
	#endif
	
	/* Make the window full screen if requested: */
	if(configFileSection.retrieveValue("./windowFullscreen",false)&&!makeFullscreen()&&vruiVerbose)
		std::cout<<"\tUnable to switch window to fullscreen mode"<<std::endl;
	
	/* Attempt to bypass the compositor if requested: */
	if(configFileSection.retrieveValue("./bypassCompositor",false)&&!bypassCompositor()&&vruiVerbose)
		std::cout<<"\tUnable to bypass the compositor; compositing may be disabled"<<std::endl;
	
	/* Install a callback to be notified when the window changes position or size: */
	getRectChangedCallbacks().add(this,&VRWindow::rectChangedCallback);
	
	/* Check if there is an input device button to enable or disable rendering into this window: */
	if(configFileSection.hasTag("./enableButtonDevice"))
		{
		/* Retrieve the enable button input device, button index, and mode: */
		std::string enableButtonDeviceName=configFileSection.retrieveString("./enableButtonDevice");
		enableButtonDevice=findInputDevice(enableButtonDeviceName.c_str());
		if(enableButtonDevice==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Enable button device %s not found",enableButtonDeviceName.c_str());
		std::string enableButtonName=configFileSection.retrieveString("./enableButton");
		int enableButtonFeatureIndex=getInputDeviceManager()->getFeatureIndex(enableButtonDevice,enableButtonName.c_str());
		if(enableButtonFeatureIndex<0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Feature %s on device %s not found",enableButtonName.c_str(),enableButtonDeviceName.c_str());
		if(!enableButtonDevice->isFeatureButton(enableButtonFeatureIndex))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Feature %s on device %s is not a button",enableButtonName.c_str(),enableButtonDeviceName.c_str());
		enableButtonIndex=enableButtonDevice->getFeatureTypeIndex(enableButtonFeatureIndex);
		configFileSection.updateValue("./invertEnableButton",invertEnableButton);
		
		/* Get the enable button's initial value, and install a callback to be notified of changes: */
		enabled=enableButtonDevice->getButtonState(enableButtonIndex);
		if(invertEnableButton)
			enabled=!enabled;
		enableButtonDevice->getButtonCallbacks(enableButtonIndex).add(this,&VRWindow::enableButtonCallback);
		}
	
	/* Update the disabled color: */
	configFileSection.updateValue("./disabledColor",disabledColor);
	
	/* Initialize the panning rectangle to the entire screen area: */
	panRect[2]=panRect[0]=Scalar(0);
	panRect[3]=panRect[1]=Scalar(1);
	
	if(vruiVerbose)
		{
		if(panningViewport)
			std::cout<<"\tPanning domain "<<outputConfiguration.domain.size[0]<<'x'<<outputConfiguration.domain.size[1]<<'+'<<outputConfiguration.domain.offset[0]<<'+'<<outputConfiguration.domain.offset[1]<<", aspect ratio "<<double(outputConfiguration.domain.size[0])/double(outputConfiguration.domain.size[1])<<std::endl;
		const Rect& r=getRect();
		std::cout<<"\tWindow position "<<r.size[0]<<'x'<<r.size[1]<<'+'<<r.offset[0]<<'+'<<r.offset[1]<<", aspect ratio "<<double(r.size[0])/double(r.size[1])<<std::endl;
		if(outputConfiguration.frameInterval>0U)
			std::cout<<"\tRefresh rate "<<1000000000.0/double(outputConfiguration.frameInterval)<<" Hz"<<std::endl;
		}
	
	/* Check if the window is supposed to save a movie: */
	if(configFileSection.retrieveValue("./saveMovie",false))
		{
		/* Create a movie saver object: */
		movieSaver=MovieSaver::createMovieSaver(configFileSection);
		
		/* Announce the movie saver's initial state: */
		if(movieSaverRecording)
			Misc::formattedLogNote("VRWindow: Movie recording active; press %s to pause recording",configFileSection.retrieveString("./pauseMovieSaverKey","Super+Pause").c_str());
		else
			Misc::formattedLogNote("VRWindow: Movie recording paused; press %s to resume recording",configFileSection.retrieveString("./pauseMovieSaverKey","Super+Pause").c_str());
		}
	}

VRWindow::~VRWindow(void)
	{
	/* Remove a registered enable button callback from the enable button device: */
	if(enableButtonDevice!=0)
		enableButtonDevice->getButtonCallbacks(enableButtonIndex).remove(this,&VRWindow::enableButtonCallback);
	
	delete movieSaver;
	}

void VRWindow::setWindowIndex(int newWindowIndex)
	{
	/* Store the window index: */
	windowIndex=newWindowIndex;
	
	/* Register a pipe command to set the window's position and size: */
	char setRectCommand[64];
	snprintf(setRectCommand,sizeof(setRectCommand),"Window(%d).setRect",windowIndex);
	getCommandDispatcher().addCommandCallback(setRectCommand,&VRWindow::setRectCallback,this,"<x> <y> <width> <height>","Sets the window's position and size");
	
	/* Check if the window is supposed to save a movie: */
	if(movieSaver!=0)
		{
		/* Register pipe command callbacks: */
		char toggleMovieSaverCommand[64];
		snprintf(toggleMovieSaverCommand,sizeof(toggleMovieSaverCommand),"Window(%d).toggleMovieSaver",windowIndex);
		getCommandDispatcher().addCommandCallback(toggleMovieSaverCommand,&VRWindow::toggleMovieSaverCallback,this,0,"Toggles the window's movie saver between paused and active");
		}
	}

void VRWindow::setWindowGroup(VruiWindowGroup* newWindowGroup)
	{
	/* Store the window group association: */
	windowGroup=newWindowGroup;
	
	/* Inform the new group of the current viewport and frame buffer sizes: */
	resizeWindow(windowGroup,this,getViewportSize(),getFramebufferSize());
	}

void VRWindow::setVruiState(VruiState* newVruiState,bool newSynchronize)
	{
	vruiState=newVruiState;
	synchronize=newSynchronize;
	
	/* Enable vsync if this is the synchronization window: */
	vsync=vsync||synchronize;
	if(vruiVerbose)
		{
		std::cout<<"\tVsync "<<(vsync?"enabled":"disabled");
		if(synchronize)
			std::cout<<", Vrui synchronization window";
		std::cout<<std::endl;
		}
	
	/* Update the clear buffer bit mask based on the window properties requested of Vrui: */
	if(vruiState->windowProperties.stencilBufferSize>0)
		clearBufferMask|=GL_STENCIL_BUFFER_BIT;
	if(vruiState->windowProperties.accumBufferSize[0]>0||vruiState->windowProperties.accumBufferSize[1]>0||vruiState->windowProperties.accumBufferSize[2]>0||vruiState->windowProperties.accumBufferSize[3]>0)
		clearBufferMask|=GL_ACCUM_BUFFER_BIT;
	}

void VRWindow::setMouseAdapter(InputDeviceAdapterMouse* newMouseAdapter,const Misc::ConfigurationFileSection& configFileSection)
	{
	mouseAdapter=newMouseAdapter;
	}

void VRWindow::setMultitouchAdapter(InputDeviceAdapterMultitouch* newMultitouchAdapter,const Misc::ConfigurationFileSection& configFileSection)
	{
	#if VRUI_INTERNAL_CONFIG_HAVE_XINPUT2
	
	/*********************************************************************
	Check if the X server has a direct-mode multitouch-capable screen by
	performing a sequence of tests:
	*********************************************************************/
	
	/* Check if display supports XInput extension: */
	int xiEvent,xiError;
	if(!XQueryExtension(getContext().getDisplay(),"XInputExtension",&xinput2Opcode,&xiEvent,&xiError))
		{
		Misc::consoleError("VRWindow::VRWindow: XInput extension not supported");
		return;
		}
	
	/* Query the version of XInput supported by the display: */
	int xiVersion[2]={2,2};
	if((XIQueryVersion(getContext().getDisplay(),&xiVersion[0],&xiVersion[1])!=Success||xiVersion[0]<2||(xiVersion[0]==2&&xiVersion[1]<2)))
		{
		Misc::consoleError("VRWindow::VRWindow: XInput extension does not support multitouch");
		return;
		}
	
	/* Search the X server for a direct-mode multitouch device: */
	int numDeviceInfos;
	XIDeviceInfo* deviceInfos=XIQueryDevice(getContext().getDisplay(),XIAllDevices,&numDeviceInfos);
	XIDeviceInfo* diPtr=deviceInfos;
	// int maxNumTouches=0;
	int touchDeviceId=-1;
	for(int i=0;i<numDeviceInfos&&touchDeviceId<0;++i,++diPtr)
		{
		XIAnyClassInfo** ciPtr=diPtr->classes;
		for(int j=0;j<diPtr->num_classes;++j,++ciPtr)
			{
			/* Check if the device is a direct-mode multitouch device: */
			if((*ciPtr)->type==XITouchClass)
				{
				XITouchClassInfo* ti=reinterpret_cast<XITouchClassInfo*>(*ciPtr);
				if(ti->mode==XIDirectTouch)
					{
					/* Use this touch device: */
					touchDeviceId=diPtr->deviceid;
					// maxNumTouches=ti->num_touches;
					}
				}
			}
		}
	XIFreeDeviceInfo(deviceInfos);
	if(touchDeviceId<0)
		{
		Misc::consoleError("VRWindow::VRWindow: No direct-mode multitouch-capable devices found");
		return;
		}
	
	/* Listen for touch events from the selected direct touch device: */
	XIEventMask eventMask;
	eventMask.deviceid=XIAllDevices; // touchDeviceId;
	eventMask.mask_len=XIMaskLen(XI_TouchOwnership);
	eventMask.mask=static_cast<unsigned char*>(malloc(eventMask.mask_len));
	memset(eventMask.mask,0,eventMask.mask_len);
	XISetMask(eventMask.mask,XI_TouchBegin);
	XISetMask(eventMask.mask,XI_TouchUpdate);
	XISetMask(eventMask.mask,XI_TouchEnd);
	// XISetMask(eventMask.mask,XI_TouchOwnership);
	Status selectEventsStatus=XISelectEvents(getContext().getDisplay(),getWindow(),&eventMask,1);
	free(eventMask.mask);
	if(selectEventsStatus!=Success)
		{
		Misc::consoleError("VRWindow::VRWindow: Unable to listen for multitouch events");
		return;
		}
	
	/* All tests were successful; remember the new multitouch input device adapter: */
	multitouchAdapter=newMultitouchAdapter;
	
	#else
	
	Misc::consoleError("VRWindow::VRWindow: Multitouch input devices not supported");
	
	#endif
	}

void VRWindow::setDisplayState(DisplayState* newDisplayState,const Misc::ConfigurationFileSection& configFileSection)
	{
	displayState=newDisplayState;
	
	/* Try to enable multisampling if requested: */
	if(multisamplingLevel>1)
		{
		/* Initialize the GL_ARB_multisample extension if supported; otherwise, disable multisampling: */
		if(GLARBMultisample::isSupported())
			GLARBMultisample::initExtension();
		else
			{
			if(vruiVerbose)
				std::cout<<"\tGL_ARB_multisample OpenGL extension not supported; falling back to single-sample rendering"<<std::endl;
			multisamplingLevel=1;
			}
		}
	
	/* Init the GL_ARB_sync OpenGL extension if supported: */
	if(haveSync)
		GLARBSync::initExtension();
	else if(vruiVerbose)
		std::cout<<"\tGL_ARB_sync OpenGL extension not supported"<<std::endl;
	
	/* Setup vertical retrace synchronization if rendering to backbuffer: */
	if(!frontBufferRendering)
		{
		/* Enable or disable vsync if application control is supported: */
		if(canVsync(false))
			setVsyncInterval(vsync?1:0);
		else
			Misc::consoleError("VRWindow::VRWindow: Vertical retrace synchronization control not supported");
		}
	
	#if VRUI_INTERNAL_CONFIG_VRWINDOW_USE_SWAPGROUPS
	
	/* Join a swap group if requested: */
	if(configFileSection.retrieveValue("./joinSwapGroup",false))
		{
		GLuint maxSwapGroupName,maxSwapBarrierName;
		glXQueryMaxSwapGroupsNV(getContext().getDisplay(),getScreen(),&maxSwapGroupName,&maxSwapBarrierName);
		GLuint swapGroupName=configFileSection.retrieveValue<GLuint>("./swapGroupName",0);
		if(swapGroupName>maxSwapGroupName)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Swap group name %u larger than maximum %u",swapGroupName,maxSwapGroupName);
		GLuint swapBarrierName=configFileSection.retrieveValue<GLuint>("./swapBarrierName",0);
		if(swapBarrierName>maxSwapBarrierName)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Swap barrier name %u larger than maximum %u",swapBarrierName,maxSwapBarrierName);
		if(!glXJoinSwapGroupNV(getContext().getDisplay(),getWindow(),swapGroupName))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to join swap group %u",swapGroupName);
		if(!glXBindSwapBarrierNV(getContext().getDisplay(),swapGroupName,swapBarrierName))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to bind swap barrier %u",swapBarrierName);
		}
	
	#endif
	}

void VRWindow::init(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Initialize the window's panning viewport state: */
	if(panningViewport)
		{
		/* Override the panning domain from the configuration file: */
		IRect& d=outputConfiguration.domain;
		configFileSection.updateValue("./panningDomain",d);
		
		/* Initialize the panning rectangle from the window's current position and size relative to its panning domain: */
		calcPanRect(getRect(),panRect);
		
		/* The rest of panning functionality makes no sense if the window doesn't have exactly one VR screen: */
		if(getNumVRScreens()==1)
			{
			/* Retrieve the screen, its size, and its transformation: */
			VRScreen* screen=getVRScreen(0);
			Scalar screenW=screen->getWidth();
			Scalar screenH=screen->getHeight();
			ONTransform screenT=screen->getScreenTransformation();
			
			if(navigate)
				{
				/* Calculate the full screen's and panned screen's center and size: */
				Point fullCenter=screenT.transform(Point(Math::div2(screenW),Math::div2(screenH),0));
				Scalar fullSize=Math::sqrt(Math::sqr(screenW)+Math::sqr(screenH));
				Point center=screenT.transform(Point(Math::mid(panRect[0],panRect[1])*screenW,Math::mid(panRect[2],panRect[3])*screenH,0));
				Scalar size=Math::sqrt(Math::sqr((panRect[1]-panRect[0])*screenW)+Math::sqr((panRect[3]-panRect[2])*screenH));
				
				/* Calculate an incremental physical-space window transformation: */
				NavTransform navUpdate=NavTransform::translateFromOriginTo(center);
				navUpdate*=NavTransform::scale(size/fullSize);
				navUpdate*=NavTransform::translateToOriginFrom(fullCenter);
				
				/* Update Vrui's display center and size: */
				setDisplayCenter(center,getDisplaySize()*size/fullSize);
				
				/* Try activating a fake navigation tool: */
				if(activateNavigationTool(reinterpret_cast<Tool*>(this)))
					{
					/* Apply the incremental transformation: */
					setNavigationTransformation(navUpdate*getNavigationTransformation());
					
					/* Deactivate the fake navigation tool again: */
					deactivateNavigationTool(reinterpret_cast<Tool*>(this));
					}
				}
			}
		}
	
	/* Check if the window is supposed to track the tool manager's tool kill zone: */
	if(configFileSection.hasTag("./toolKillZonePos"))
		{
		/* Read the tool kill zone's relative window position: */
		Geometry::Point<Scalar,2> tkzp=configFileSection.retrieveValue<Geometry::Point<Scalar,2> >("./toolKillZonePos");
		for(int i=0;i<2;++i)
			toolKillZonePos[i]=tkzp[i];
		trackToolKillZone=true;
		
		/* Place the tool kill zone: */
		placeToolKillZone();
		}
	
	/* Hide mouse cursor and ignore mouse events if the mouse is not used as an input device: */
	if(mouseAdapter==0||!mouseAdapter->needMouseCursor())
		{
		hideCursor();
		if(mouseAdapter==0)
			disableMouseEvents();
		}
	}

void VRWindow::releaseGLState(void)
	{
	}

bool VRWindow::processEvent(const XEvent& event)
	{
	bool stopProcessing=false;
	
	#if VRUI_INTERNAL_CONFIG_HAVE_XRANDR
	
	/* Check for xrandr events: */
	if(xrandrEventBase!=0&&event.type==xrandrEventBase+RRScreenChangeNotify)
		{
		/* Tell Xlib that the screen resolution or layout changed: */
		XRRUpdateConfiguration(const_cast<XEvent*>(&event));
		
		/* Query the new screen layout: */
		OutputConfiguration newOc=getOutputConfiguration(getContext().getDisplay(),getScreen(),outputName.c_str());
		
		/* Do something clever by moving the window from the old to the new display position etc.: */
		const GLWindow::Rect& oldRect=getRect();
		GLWindow::Rect newRect;
		for(int i=0;i<2;++i)
			{
			newRect.offset[i]=((oldRect.offset[i]-outputConfiguration.domain.offset[i])*newOc.domain.size[i]+outputConfiguration.domain.size[i]/2)/outputConfiguration.domain.size[i]+newOc.domain.offset[i];
			newRect.size[i]=(oldRect.size[i]*newOc.domain.size[i]+outputConfiguration.domain.size[i]/2)/outputConfiguration.domain.size[i];
			}
		
		/* Store the new output configuration: */
		outputConfiguration=newOc;
		
		/* Request the window's new position and size: */
		setRect(newRect);
		
		/* Inform derived classes that the window has moved and/or changed size: */
		rectChanged(oldRect,newRect);
		
		return stopProcessing;
		}
	
	#endif
	
	#if VRUI_INTERNAL_CONFIG_HAVE_XINPUT2
	
	/* Check for xinput2 events: */
	if(multitouchAdapter!=0&&event.xcookie.type==GenericEvent&&event.xcookie.extension==xinput2Opcode)
		{
		/* Get the event's detail data: */
		XGenericEventCookie cookie=event.xcookie;
		if(XGetEventData(getContext().getDisplay(),&cookie))
			{
			XIDeviceEvent* de=static_cast<XIDeviceEvent*>(cookie.data);
			
			/* Extract relevant touch event state: */
			InputDeviceAdapterMultitouch::TouchEvent te;
			te.id=de->detail;
			te.x=Scalar(de->event_x);
			te.y=Scalar(de->event_y);
			
			/* Extract the event's raw valuators: */
			te.ellipseMask=0x0U;
			te.majorAxis=Scalar(0);
			te.minorAxis=Scalar(0);
			te.orientation=Scalar(0);
			int valueIndex=0;
			for(int i=0;i<de->valuators.mask_len;++i)
				for(int j=0;j<8;++j)
					if(de->valuators.mask[i]&(0x1<<j))
						{
						switch(i*8+j)
							{
							case 2:
								te.ellipseMask|=0x1U;
								te.majorAxis=Scalar(de->valuators.values[valueIndex]);
								break;
							
							case 3:
								te.ellipseMask|=0x2U;
								te.minorAxis=Scalar(de->valuators.values[valueIndex]);
								break;
							
							case 4:
								te.ellipseMask|=0x4U;
								te.orientation=Scalar(de->valuators.values[valueIndex]);
								break;
							}
						++valueIndex;
						}
			
			/* Forward the event to the multitouch adapter: */
			switch(cookie.evtype)
				{
				case XI_TouchBegin:
					multitouchAdapter->touchBegin(this,te);
					stopProcessing=true;
					break;
				
				case XI_TouchUpdate:
					multitouchAdapter->touchUpdate(this,te);
					break;
				
				case XI_TouchEnd:
					multitouchAdapter->touchEnd(this,te);
					stopProcessing=true;
					break;
				}
			
			XFreeEventData(getContext().getDisplay(),&cookie);
			}
		
		return stopProcessing;
		}
	
	#endif
	
	/* Check for "regular" X events: */
	switch(event.type)
		{
		case Expose:
		case GraphicsExpose:
			/* Window must be redrawn: */
			dirty=true;
			break;
		
		case MotionNotify:
			if(mouseAdapter!=0)
				{
				#if SAVE_MOUSEMOVEMENTS
				if(mouseMovementsFile!=0)
					{
					/* Write event time and new mouse position: */
					mouseMovementsFile->write<Misc::UInt32>(0); // Motion event
					mouseMovementsFile->write<Misc::UInt64>(event.xmotion.time);
					mouseMovementsFile->write<Misc::SInt32>(event.xmotion.x);
					mouseMovementsFile->write<Misc::SInt32>(event.xmotion.y);
					}
				#endif
				
				/* Set mouse position in input device adapter: */
				mouseAdapter->setMousePosition(this,GLWindow::Offset(event.xmotion.x,event.xmotion.y));
				}
			break;
		
		case ButtonPress:
		case ButtonRelease:
			if(mouseAdapter!=0)
				{
				#if SAVE_MOUSEMOVEMENTS
				if(mouseMovementsFile!=0)
					{
					/* Write event time and new mouse position: */
					mouseMovementsFile->write<Misc::UInt32>(1); // Button event
					mouseMovementsFile->write<Misc::UInt64>(event.xbutton.time);
					mouseMovementsFile->write<Misc::SInt32>(event.xbutton.x);
					mouseMovementsFile->write<Misc::SInt32>(event.xbutton.y);
					}
				#endif
				
				/* Set mouse position in input device adapter: */
				mouseAdapter->setMousePosition(this,GLWindow::Offset(event.xbutton.x,event.xbutton.y));
				
				/* Set the state of the appropriate button in the input device adapter: */
				bool newState=event.type==ButtonPress;
				if(event.xbutton.button<4)
					stopProcessing=mouseAdapter->setButtonState(event.xbutton.button-1,newState);
				else if(event.xbutton.button==4)
					{
					if(newState)
						mouseAdapter->incMouseWheelTicks();
					}
				else if(event.xbutton.button==5)
					{
					if(newState)
						mouseAdapter->decMouseWheelTicks();
					}
				else if(event.xbutton.button>5)
					stopProcessing=mouseAdapter->setButtonState(event.xbutton.button-3,newState);
				}
			break;
		
		case KeyPress:
		case KeyRelease:
			{
			if(mouseAdapter!=0)
				{
				#if SAVE_MOUSEMOVEMENTS
				if(mouseMovementsFile!=0)
					{
					/* Write event time and new mouse position: */
					mouseMovementsFile->write<Misc::UInt32>(2); // Key event
					mouseMovementsFile->write<Misc::UInt64>(event.xkey.time);
					mouseMovementsFile->write<Misc::SInt32>(event.xkey.x);
					mouseMovementsFile->write<Misc::SInt32>(event.xkey.y);
					}
				#endif
				
				/* Set mouse position in input device adapter: */
				mouseAdapter->setMousePosition(this,GLWindow::Offset(event.xkey.x,event.xkey.y));
				}
			
			/* Convert event key index to keysym: */
			char keyString[20];
			KeySym keySym;
			XKeyEvent keyEvent=event.xkey;
			
			/* Use string lookup method to get proper key value for text events: */
			int keyStringLen=XLookupString(&keyEvent,keyString,sizeof(keyString),&keySym,0);
			keyString[keyStringLen]='\0';
			
			/* Use keysym lookup a second time to get raw key code ignoring modifier keys: */
			keySym=XLookupKeysym(&keyEvent,0);
			
			if(event.type==KeyPress)
				{
				/* Handle Vrui application keys: */
				if(exitKey.matches(keySym,keyEvent.state))
					{
					/* Call the window close callbacks: */
					Misc::CallbackData cbData;
					getCloseCallbacks().call(&cbData);
					stopProcessing=true;
					}
				else if(homeKey.matches(keySym,keyEvent.state))
					{
					/* Reset the navigation transformation: */
					resetNavigation();
					stopProcessing=true;
					}
				else if(screenshotKey.matches(keySym,keyEvent.state))
					{
					saveScreenshot=true;
					char numberedFileName[256];
					#if IMAGES_CONFIG_HAVE_PNG
					/* Save the screenshot as a PNG file: */
					screenshotImageFileName=Misc::createNumberedFileName("VruiScreenshot.png",4,numberedFileName);
					#else
					/* Save the screenshot as a PPM file: */
					screenshotImageFileName=Misc::createNumberedFileName("VruiScreenshot.ppm",4,numberedFileName);
					#endif
					
					/* Write a confirmation message: */
					Misc::formattedLogNote("Saving window contents as %s",screenshotImageFileName.c_str());
					}
				else if(fullscreenToggleKey.matches(keySym,keyEvent.state))
					{
					/* Toggle fullscreen mode: */
					toggleFullscreen();
					}
				else if(burnModeToggleKey.matches(keySym,keyEvent.state))
					{
					/* Enter or leave burn mode: */
					if(burnMode)
						{
						if(burnModeNumFrames>0)
							{
							double burnModeTime=getApplicationTime()-burnModeFirstFrameTime;
							Misc::formattedLogNote("Leaving burn mode: %u frames in %f ms,averaging %f fps, frame time range [%f ms, %f ms]",burnModeNumFrames,burnModeTime*1000.0,double(burnModeNumFrames)/burnModeTime,burnModeMin*1000.0,burnModeMax*1000.0);
							}
						else
							Misc::logNote("Leaving burn mode during spin-up phase");
						burnMode=false;
						}
					else
						{
						Misc::logNote("Entering burn mode");
						burnMode=true;
						burnModeStartTime=Vrui::getApplicationTime()+2.0; // Allow two seconds of spin-up time
						burnModeFirstFrameTime=burnModeStartTime; // Just an estimate that will be corrected later
						burnModeNumFrames=0U;
						burnModeMin=Math::Constants<double>::max;
						burnModeMax=0.0;
						}
					}
				else if(pauseMovieSaverKey.matches(keySym,keyEvent.state))
					{
					/* Toggle the movie saver's activation state if it exists: */
					if(movieSaver!=0)
						{
						movieSaverRecording=!movieSaverRecording;
						Misc::formattedLogNote("VRWindow: Movie recording %s",movieSaverRecording?"active":"paused");
						}
					}
				
				if(mouseAdapter!=0)
					{
					/* Send key event to input device adapter: */
					stopProcessing=mouseAdapter->keyPressed(keySym,keyEvent.state,keyString);
					}
				}
			else
				{
				if(mouseAdapter!=0)
					{
					/* Send key event to input device adapter: */
					stopProcessing=mouseAdapter->keyReleased(keySym);
					}
				}
			break;
			}
		
		case FocusIn:
			if(panningViewport&&getNumVRScreens()==1)
				{
				/* Retrieve the screen, its size, and its transformation: */
				VRScreen* screen=getVRScreen(0);
				Scalar screenW=screen->getWidth();
				Scalar screenH=screen->getHeight();
				ONTransform screenT=screen->getScreenTransformation();
				
				/* Calculate the screen's center: */
				Point center=screenT.transform(Point(Math::mid(panRect[0],panRect[1])*screenW,Math::mid(panRect[2],panRect[3])*screenH,0));
				
				/* Update Vrui's display center: */
				setDisplayCenter(center,getDisplaySize());
				}
			
			if(trackToolKillZone)
				placeToolKillZone();
			
			if(mouseAdapter!=0)
				{
				/* Create a fake XKeymap event: */
				XKeymapEvent keymapEvent;
				keymapEvent.type=KeymapNotify;
				keymapEvent.serial=event.xcrossing.serial;
				keymapEvent.send_event=event.xcrossing.send_event;
				keymapEvent.display=event.xcrossing.display;
				keymapEvent.window=event.xcrossing.window;
				
				/* Query the current key map: */
				XQueryKeymap(getContext().getDisplay(),keymapEvent.key_vector);
				
				/* Reset the input device adapter's key states: */
				mouseAdapter->resetKeys(this,keymapEvent);
				}
			break;
		
		default:
			/* Call base class method: */
			GLWindow::processEvent(event);
		}
	
	return stopProcessing;
	}

void VRWindow::requestScreenshot(const char* newScreenshotImageFileName)
	{
	/* Set the screenshot flag and remember the image file name: */
	saveScreenshot=true;
	screenshotImageFileName=newScreenshotImageFileName;
	}

}

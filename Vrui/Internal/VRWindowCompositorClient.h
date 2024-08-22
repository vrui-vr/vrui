/***********************************************************************
VRWindowCompositorClient - Class for OpenGL windows that drive head-
mounted displays via an external VR compositing client.
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

#ifndef VRUI_VRWINDOWCOMPOSITORCLIENT_INCLUDED
#define VRUI_VRWINDOWCOMPOSITORCLIENT_INCLUDED

#include <Realtime/SharedMemory.h>
#include <Comm/UNIXPipe.h>
#include <Vrui/KeyMapper.h>
#include <Vrui/VRWindow.h>
#include <Vrui/Internal/VRCompositorProtocol.h>

/* Forward declarations: */
namespace Vrui {
class Viewer;
class VRScreen;
class HMDConfiguration;
class HMDConfigurationUpdater;
}

namespace Vrui {

class VRWindowCompositorClient:public VRWindow,public VRCompositorProtocol
	{
	/* Embedded classes: */
	private:
	struct CompositorInfo // Structure holding state retrieved from establishing a connection to the VR compositing server
		{
		/* Elements: */
		public:
		int sharedMemoryBlockFd; // File descriptor for the server's communication shared memory block
		int imageMemoryBlockFd; // File descriptor for the server's shared GPU image memory block
		size_t imageMemoryBlockSize; // Total size of the server's shared GPU image memory block
		size_t imageMemorySizes[3]; // Sizes of the server's three input images in its shared GPU image memory block
		ptrdiff_t imageMemoryOffsets[3]; // Offsets of the server's three input images in its shared GPU image memory block
		
		/* Constructors and destructors: */
		CompositorInfo(const char* source,Comm::UNIXPipe& pipe); // Reads VR compositor information from the given pipe
		};
	
	/* Elements: */
	Comm::UNIXPipe compositorPipe; // UNIX domain pipe connected to the VR compositor server process
	CompositorInfo compositorInfo; // VR compositor's connection state
	Realtime::SharedMemory sharedMemory; // Shared memory block backing the shared triple buffer of texture images and ancillary data
	SharedMemorySegment* sharedMemorySegment; // Pointer to the VR compositing server's shared memory segment
	HMDConfiguration hmdConfiguration; // Local copy of the compositor's HMD configuration
	Viewer* viewer; // Pointer to the viewer representing the head-mounted display
	HMDConfigurationUpdater* hmdConfigurationUpdater; // Helper object to react to asynchronous HMD configuration changes
	VRScreen* screens[2]; // Pointer to the VR screens representing the head-mounted display's left and right screens
	GLuint predistortionFrameBufferId; // ID of the pre-distortion frame buffer
	GLuint memoryObjectId; // ID of the memory object exported by the VR compositor backing the three color buffer textures
	GLuint predistortionColorBufferIds[3]; // IDs of the three color buffer textures backed by the VR compositor
	GLuint predistortionMultisamplingColorBufferId; // ID of the shared pre-distortion multisampling color buffer
	GLuint predistortionDepthStencilBufferId; // ID of the pre-distortion depth buffer, potentially interleaved with a stencil buffer
	GLuint multisamplingFrameBufferId; // ID of a frame buffer to "fix" a multisampled image texture into a regular image texture
	bool mirrorHmd; // Flag whether to mirror the pre-distortion image to the window
	int mirrorEyeIndex; // Index of the eye whose pre-distortion image to mirror to the window
	Scalar mirrorFov; // Field of view of the mirroring camera in tangent space
	bool mirrorFollowAzimuth; // Flag if the mirroring camera follows the viewer's azimuth
	bool mirrorFollowElevation; // Flag if the mirroring camera follows the viewer's elevation
	
	/* Interaction state: */
	KeyMapper::QualifiedKey mirrorModeKey; // Key to cycle through HMD mirroring modes
	KeyMapper::QualifiedKey mirrorFollowModeKey; // Key to cycle through HMD mirroring viewer following modes
	
	/* Protected methods from class VRWindow: */
	virtual ISize getViewportSize(void) const;
	virtual ISize getFramebufferSize(void) const;
	
	/* Private methods: */
	void hmdConfigurationUpdatedCallback(const Vrui::HMDConfiguration& hmdConfiguration); // Callback called when the HMD configuration changed asynchronously
	
	/* Constructors and destructors: */
	public:
	VRWindowCompositorClient(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~VRWindowCompositorClient(void);
	
	/* Methods from class VRWindow: */
	virtual void setDisplayState(DisplayState* newDisplayState,const Misc::ConfigurationFileSection& configFileSection);
	virtual void init(const Misc::ConfigurationFileSection& configFileSection);
	virtual void releaseGLState(void);
	virtual int getNumVRScreens(void) const;
	virtual VRScreen* getVRScreen(int index);
	virtual VRScreen* replaceVRScreen(int index,VRScreen* newVRScreen);
	virtual int getNumViewers(void) const;
	virtual Viewer* getViewer(int index);
	virtual Viewer* replaceViewer(int index,Viewer* newViewer);
	virtual InteractionRectangle getInteractionRectangle(void);
	virtual int getNumViews(void) const;
	virtual View getView(int index);
	virtual bool processEvent(const XEvent& event);
	virtual void updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const;
	virtual void draw(void);
	virtual void waitComplete(void);
	virtual void present(void);
	};

}

#endif

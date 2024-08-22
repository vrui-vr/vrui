/***********************************************************************
VRWindow - Abstract base class for OpenGL windows that are used to bind
together any numbers of viewers and VR screens to render VR
environments.
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

#ifndef VRUI_VRWINDOW_INCLUDED
#define VRUI_VRWINDOW_INCLUDED

#include <utility>
#include <string>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLContext.h>
#include <GL/Extensions/GLARBSync.h>
#include <GL/GLWindow.h>
#include <Vrui/Types.h>
#include <Vrui/InputDevice.h>
#include <Vrui/KeyMapper.h>
#include <Vrui/GetOutputConfiguration.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class InputDeviceAdapterMouse;
class InputDeviceAdapterMultitouch;
class VRScreen;
class Viewer;
class DisplayState;
class VruiState;
struct VruiWindowGroup;
class MovieSaver;
}

namespace Vrui {

class VRWindow:public GLWindow
	{
	/* Embedded classes: */
	public:
	struct InteractionRectangle // Structure describing a rectangle in 3D space that can be used for mapping 2D input devices
		{
		/* Elements: */
		public:
		ONTransform transformation; // Transformation from rectangle space, where x points right and y points up, to physical space
		Scalar size[2]; // Width and height of interaction rectangle
		};
	
	struct View // Structure describing one of potentially several 3D views used by a window
		{
		/* Elements: */
		public:
		Rect viewport; // Viewport in window coordinates
		Viewer* viewer; // Viewer responsible for this view
		Point eye; // Eye position used for this view in viewer coordinates
		VRScreen* screen; // Screen responsible for this view
		Scalar screenRect[4]; // Left, right, bottom, and top edges of screen rectangle used in this view in screen coordinates
		};
	
	/* Elements: */
	protected:
	OutputConfiguration outputConfiguration; // Size and panning domain of output to which window is bound
	std::string outputName; // Optional name of output connector or connected monitor to which to bind this window to keep track of changes to Xrandr's output configuration at run-time; not used if empty
	int xrandrEventBase; // Base index for XRANDR events
	
	/* Vrui integration state: */
	VruiState* vruiState; // Pointer to the Vrui state object to which this window belongs
	int windowIndex; // Index of this window in the environment's total window list, i.e., across all cluster nodes
	VruiWindowGroup* windowGroup; // Pointer to the window group to which this window belongs
	public:
	bool protectScreens; // Flag whether this window needs to render environmental protection boundaries
	
	/* Panning state: */
	private:
	Scalar panRect[4]; // Relative left, right, bottom, and top boundaries of window inside its panning domain normalized to [0.0, 1.0]^2; always full range for non-panning windows
	bool panningViewport; // Flag whether the window should only draw to the part of its screen that matches the window's relative inside its panning domain
	bool navigate; // Flag if the window should drag navigation space along when it is moved/resized
	bool movePrimaryWidgets; // Flag if the window should drag along primary popped-up widgets when it is moved/resized
	bool trackToolKillZone; // Flag if the tool manager's tool kill zone should follow the window when moved/resized
	Scalar toolKillZonePos[2]; // Position of tool kill zone in relative window coordinates (0.0-1.0 in both directions)
	
	/* Interaction state: */
	KeyMapper::QualifiedKey exitKey; // Key to exit the Vrui application
	KeyMapper::QualifiedKey homeKey; // Key to reset the navigation transformation to the application's default
	KeyMapper::QualifiedKey screenshotKey; // Key to save the window contents as an image
	KeyMapper::QualifiedKey fullscreenToggleKey; // Key to toggle between windowed and fullscreen modes
	KeyMapper::QualifiedKey burnModeToggleKey; // Key to toggle "burn mode"
	KeyMapper::QualifiedKey pauseMovieSaverKey; // Key to pause/unpause a movie saver
	InputDeviceAdapterMouse* mouseAdapter; // Pointer to the mouse input device adapter (if one exists; 0 otherwise)
	InputDeviceAdapterMultitouch* multitouchAdapter; // Pointer to the multitouch input device adapter (if one exists; 0 otherwise)
	int xinput2Opcode; // Opcode for XINPUT2 events
	InputDevice* enableButtonDevice; // Pointer to an input device containing a button to enable and disable rendering to this window
	int enableButtonIndex; // Index of the enable button on the input device
	bool invertEnableButton; // Flag if the enable button's state needs to be inverted
	
	/* Rendering state: */
	protected:
	int multisamplingLevel; // Level of multisampling (FSAA) for this window (1 == multisampling disabled)
	DisplayState* displayState; // The display state object associated with this window's OpenGL context; updated before each rendering pass
	GLbitfield clearBufferMask; // Bit mask of OpenGL buffers that need to be cleared before rendering to this window
	bool frontBufferRendering; // Flag whether the window renders directly into the front buffer
	bool dirty; // Flag whether the window needs to be redrawn
	bool resized; // Flag whether the window has changed size since the last draw() call
	bool enabled; // Flag whether it is currently possible to render into this window
	Color disabledColor; // Color with which to clear the window when it is disabled
	bool haveSync; // Flag whether OpenGL supports the GL_ARB_sync extension
	GLsync drawFence; // If GL_ARB_sync is supported, fence object to wait on completion of the most recent draw() call
	bool vsync; // Flag if the window uses vertical retrace synchronization
	bool synchronize; // Flag if the window is responsible for synchronizing Vrui's frame loop
	bool lowLatency; // Flag whether the window calls glFinish() after glXSwapBuffers() to reduce display latency at some CPU busy-waiting cost
	private:
	bool saveScreenshot; // Flag if the window is to save its contents after the next draw() call
	std::string screenshotImageFileName; // Name of the image file into which to save the next screen shot
	MovieSaver* movieSaver; // Pointer to a movie saver object if the window is supposed to write contents to a movie
	bool movieSaverRecording; // Flag whether the movie saver is currently recording
	bool showFps; // Flag whether this window displays an FPS counter in "burn mode"
	bool burnMode; // Flag if the window is currently in burn mode, i.e., if it runs Vrui frames as quickly as possible and displays smoothed FPS
	double burnModeStartTime; // Application time after which burn mode statistics will be collected to allow for spin-up
	double burnModeFirstFrameTime; // Application time at which the window started collecting burn mode statistics
	unsigned int burnModeNumFrames; // Number of frames rendered in burn mode
	double burnModeMin,burnModeMax; // Minimum and maximum frame times while burn mode was active
	
	/* Private methods: */
	private:
	void calcPanRect(const Rect& rect,Scalar panRect[4]); // Calculates the panning rectangle after the window or panning domain changed
	void placeToolKillZone(void); // Updates the position of the tool kill zone if it is being tracked
	void rectChangedCallback(RectChangedCallbackData* cbData); // Callback called when the window changes position or size
	void enableButtonCallback(Vrui::InputDevice::ButtonCallbackData* cbData); // Callback called when the button used to enable/disable this window changes state
	static void setRectCallback(const char* argumentBegin,const char* argumentEnd,void* userData); // Handles a "setRect" command on the command pipe
	static void toggleMovieSaverCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData); // Handles a "toggleMovieSaver" command on the command pipe
	
	/* Protected methods: */
	protected:
	Scalar* writePanRect(const VRScreen* screen,Scalar screenRect[4]) const; // Writes the window's panning rectangle for the given screen into the given array in screen coordinates in order left, right, bottom, top; returns a pointer to the written array
	virtual ISize getViewportSize(void) const =0; // Returns the largest viewport size currently used by the window
	virtual ISize getFramebufferSize(void) const =0; // Returns the largest implicit or explicit framebuffer size currently used by the window
	virtual void rectChanged(const Rect& oldRect,const Rect& newRect); // Notifies a derived window class that the window changed position or size
	void prepareRender(void); // Prepares the display state and OpenGL context for subsequent rendering
	void render(void); // Renders the current state of the VR environment after display state has been initialized; will be called multiple times for non-monoscopic windows
	void renderComplete(void); // Called by derived classes after all rendering on this window is done and completed by the GPU
	void updateScreenDeviceCommon(const Scalar windowPos[2],const Rect& viewport,const Point& physEyePos,const VRScreen* screen,InputDevice* device) const; // Common function to update a 3D input device from a mouse position in a window
	
	/* Constructors and destructors: */
	public:
	static std::pair<std::string,int> getDisplayName(const Misc::ConfigurationFileSection& configFileSection); // Returns the name of the X display and the index of the screen requested in the given window configuration section
	static void updateContextProperties(GLContext::Properties& contextProperties,const Misc::ConfigurationFileSection& configFileSection); // Updates the given OpenGL context properties based on the given window configuration file section
	static VRWindow* createWindow(GLContext& context,const char* windowName,const Misc::ConfigurationFileSection& configFileSection); // Creates a VR window using the given OpenGL context and settings from the given configuration file section
	protected:
	VRWindow(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const Rect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection); // Creates a VR window for the given OpenGL context's display connection and screen index at the given initial position and size
	public:
	virtual ~VRWindow(void);
	
	/* Methods: */
	void setWindowIndex(int newWindowIndex); // Sets the window's index in the total window list
	void setWindowGroup(VruiWindowGroup* newWindowGroup); // Sets the window's window group
	virtual void setVruiState(VruiState* newVruiState,bool newSynchronize); // Associates the window with a Vrui state object; flag indicates whether this window is responsible for synchronizing Vrui frames with its display's vertical retrace period
	virtual void setMouseAdapter(InputDeviceAdapterMouse* newMouseAdapter,const Misc::ConfigurationFileSection& configFileSection); // Associates the window with the given mouse input device adapter
	virtual void setMultitouchAdapter(InputDeviceAdapterMultitouch* newMultitouchAdapter,const Misc::ConfigurationFileSection& configFileSection); // Associates the window with the given multitouch input device adapter
	virtual void setDisplayState(DisplayState* newDisplayState,const Misc::ConfigurationFileSection& configFileSection); // Sets the window's (shared) display state object after the (shared) OpenGL context has been fully initialized
	virtual void init(const Misc::ConfigurationFileSection& configFileSection); // Called after a window object has been fully constructed
	virtual void releaseGLState(void); // Called before a window's OpenGL context will be destroyed
	
	const Scalar* getPanRect(void) const // Returns the window's normalized panning rectangle
		{
		return panRect;
		}
	virtual int getNumVRScreens(void) const =0; // Returns the number of VR screens used by the window
	virtual VRScreen* getVRScreen(int index) =0; // Returns the VR screen of the given index
	virtual VRScreen* replaceVRScreen(int index,VRScreen* newScreen) =0; // Replaces one of the window's VR screens and returns the previous VR screen
	virtual int getNumViewers(void) const =0; // Returns the number of viewers used by the window
	virtual Viewer* getViewer(int index) =0; // Returns the viewer of the given index
	virtual Viewer* replaceViewer(int index,Viewer* newViewer) =0; // Replaces one of the window's viewers and returns the previous viewer
	virtual InteractionRectangle getInteractionRectangle(void) =0; // Returns an interaction rectangle that can be used to map 2D input devices inside this window
	virtual int getNumViews(void) const =0; // Returns the number of views used by this window
	virtual View getView(int index) =0; // Returns the view of the given index
	
	virtual bool processEvent(const XEvent& event); // Processes an X event; returns true if the Vrui mainloop should stop processing events and start continue with the frame sequence
	Offset getWindowCenterPos(void) const // Returns the center of the window in window coordinates
		{
		return Offset(Math::div2(getWindowSize()[0]),Math::div2(getWindowSize()[1]));
		}
	virtual void updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const =0; // Positions a 3D device based on the given pointer position in window coordinates
	
	bool isDirty(void) const // Returns true if the window needs to be redrawn
		{
		return dirty;
		}
	void requestScreenshot(const char* newScreenshotImageFileName); // Asks the window to save its contents to an image file of the given name on the next render pass
	
	/* Abstract rendering interface: */
	virtual void draw(void) =0; // Draws the current state of the VR environment into this window
	virtual void waitComplete(void) =0; // Blocks the caller until all operations on the window's framebuffer(s) have completed
	virtual void present(void) =0; // Presents the results of the most recent draw() call to the user
	};

}

#endif

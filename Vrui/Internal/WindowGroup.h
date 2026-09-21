/***********************************************************************
WindowGroup - Class representing a group of windows sharing the same
OpenGL context and X11 display connection.
Copyright (c) 2000-2026 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_WINDOWGROUP_INCLUDED
#define VRUI_INTERNAL_WINDOWGROUP_INCLUDED

#include <string>
#include <vector>
#include <Misc/ConfigurationFile.h>
#include <X11/Xlib.h>
#include <GL/GLContext.h>
#include <Vrui/Types.h>

/* Forward declarations: */
namespace Vrui {
class InputDeviceAdapterMouse;
class InputDeviceAdapterMultitouch;
class VRWindow;
class DisplayState;
}

namespace Vrui {

class WindowGroup
	{
	/* Embedded classes: */
	public:
	struct Window
		{
		/* Elements: */
		public:
		VRWindow* window; // Pointer to the window
		ISize viewportSize; // Window's current maximal viewport size
		ISize frameSize; // Window's current maximal frame buffer size
		};
	
	struct Creator // Structure to collect windows to be put into the same window group
		{
		/* Embedded classes: */
		public:
		struct Window // Structure defining a window inside a window group
			{
			/* Elements: */
			public:
			int windowIndex; // Index of the window in Vrui's main window array
			Misc::ConfigurationFileSection windowConfigFileSection; // Configuration file section for the window
			};
		
		/* Elements: */
		public:
		const char* applicationName; // Name of the Vrui application
		int numWindows; // Total number of windows on this node
		VRWindow** resultWindows; // Array of pointers to all windows on this node
		int groupId; // ID of the window group
		std::string displayName; // Display name for this window group
		int screen; // Screen index for this window group
		std::vector<Window> windows; // List of the windows in this group
		GLContext::Properties contextProperties; // OpenGL context properties for this window group
		
		/* Constructors and destructors: */
		Creator(const char* sApplicationName,int sNumWindows,VRWindow** sResultWindows,int sGroupId)
			:applicationName(sApplicationName),numWindows(sNumWindows),resultWindows(sResultWindows),
			 groupId(sGroupId),
			 screen(-1)
			{
			}
		};
	
	/* Elements: */
	private:
	Display* display; // Display connection shared by all windows in the window group
	int displayFd; // File descriptor for the display connection
	int numEventsInQueue; // Number of unhandled events in the display connection's event queue
	bool socketReady; // Flag if the display connection's socket has data available to read
	GLContextPtr context; // OpenGL context shared by all windows in the group
	DisplayState* displayState; // Display state structure shared by all windows in the group
	std::vector<Window> windows; // List of pointers to windows in the window group
	ISize maxViewportSize; // Maximum current viewport size of all windows in the group
	ISize maxFrameSize; // Maximum current frame buffer size of all windows in the group
	
	/* Constructors and destructors: */
	public:
	WindowGroup(void); // Creates an empty window group
	~WindowGroup(void); // Destroys this window group
	
	/* Methods: */
	bool initialize(const Creator& creator,const std::string& syncWindowName,InputDeviceAdapterMouse* mouseAdapter,InputDeviceAdapterMultitouch* multitouchAdapter); // Initializes this window group from the given group creator and additional information
	int getDisplayFd(void) const // Returns the file descriptor of the display connection's socket
		{
		return displayFd;
		}
	GLContext& getContext(void) // Returns this window group's shared OpenGL context
		{
		return *context;
		}
	void resizeWindow(const VRWindow* window,const ISize& newViewportSize,const ISize& newFrameSize); // Adjusts the window group's display state after the given window changes size
	void getMaxWindowSizes(ISize& viewportSize,ISize& frameSize) const // Returns the maximum viewport and frame sizes of all windows in this window group
		{
		viewportSize=maxViewportSize;
		frameSize=maxFrameSize;
		}
	bool hasPendingEvents(void) const // Returns true if this window group's display connection has pending events
		{
		return numEventsInQueue>0;
		}
	void setSocketReady(bool newSocketReady) // Sets the ready state of the display connection's socket
		{
		socketReady=newSocketReady;
		}
	bool dispatchXEvents(void); // Dispatches all X11 events in the display connection's event queue and on its socket to the windows in this window group; returns true if any "real" events were dispatched
	void draw(void); // Draws all windows in this window group
	void wait(void); // Waits until all windows in this window group are done rendering
	void present(void); // Presents the most recent rendering results of all windows in this group
	void releaseGLState(void); // Releases all OpenGL state held by the window group and any of its windows
	};

}

#endif

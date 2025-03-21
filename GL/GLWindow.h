/***********************************************************************
GLWindow - Class to encapsulate details of the underlying window system
implementation from an application wishing to use OpenGL windows.
Copyright (c) 2001-2022 Oliver Kreylos

This file is part of the OpenGL/GLX Support Library (GLXSupport).

The OpenGL/GLX Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL/GLX Support Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL/GLX Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLWINDOW_INCLUDED
#define GLWINDOW_INCLUDED

#include <Misc/Rect.h>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <X11/X.h>
#include <GL/GLContext.h>

/************************************
Needed declarations from GL/glxext.h:
************************************/

#ifndef GLX_SGI_video_sync
#define GLX_SGI_video_sync 1
typedef int (*PFNGLXGETVIDEOSYNCSGIPROC)(unsigned int* count);
typedef int (*PFNGLXWAITVIDEOSYNCSGIPROC)(int divisor,int remainder,unsigned int* count);
#ifdef GLX_GLXEXT_PROTOTYPES
int glXGetVideoSyncSGI(unsigned int* count);
int glXWaitVideoSyncSGI(int divisor,int remainder,unsigned int* count);
#endif
#endif

#ifndef GLX_EXT_swap_control
#define GLX_EXT_swap_control 1
#define GLX_SWAP_INTERVAL_EXT 0x20F1
#define GLX_MAX_SWAP_INTERVAL_EXT 0x20F2
typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display* dpy,GLXDrawable drawable,int interval);
#ifdef GLX_GLXEXT_PROTOTYPES
void glXSwapIntervalEXT(Display* dpy,GLXDrawable drawable,int interval);
#endif
#endif

#ifndef GLX_MESA_swap_control
#define GLX_MESA_swap_control 1
typedef int (*PFNGLXSWAPINTERVALMESAPROC)(unsigned int interval);
typedef int (*PFNGLXGETSWAPINTERVALMESAPROC)(void);
#ifdef GLX_GLXEXT_PROTOTYPES
int glXSwapIntervalMESA(unsigned int interval);
int glXGetSwapIntervalMESA(void);
#endif
#endif

#ifndef GLX_NV_delay_before_swap
#define GLX_NV_delay_before_swap 1
typedef Bool (*PFNGLXDELAYBEFORESWAPNVPROC)(Display* dpy,GLXDrawable drawable,GLfloat seconds);
#ifdef GLX_GLXEXT_PROTOTYPES
Bool glXDelayBeforeSwapNV(Display* dpy,GLXDrawable drawable,GLfloat seconds);
#endif
#endif

class GLWindow
	{
	/* Embedded classes: */
	public:
	typedef Misc::Rect<2> Rect; // Type for window positions and sizes
	typedef Rect::Offset Offset; // Type for window positions
	typedef Rect::Size Size; // Type for window sizes
	
	struct CallbackData:public Misc::CallbackData // Base class for window callback data structures
		{
		/* Elements: */
		public:
		GLWindow* window; // Window that caused the callback
		
		/* Constructors and destructors: */
		CallbackData(GLWindow* sWindow)
			:window(sWindow)
			{
			}
		};
	
	struct RectChangedCallbackData:public CallbackData // Callback data structure when the window's position and/or size change
		{
		/* Elements: */
		public:
		Rect oldRect; // Old window position and size
		Rect newRect; // New window position and size
		
		/* Constructors and destructors: */
		RectChangedCallbackData(GLWindow* sWindow,const Rect& sOldRect,const Rect& sNewRect)
			:CallbackData(sWindow),
			 oldRect(sOldRect),newRect(sNewRect)
			{
			}
		
		/* Methods: */
		bool isOffsetChanged(void) const // Returns true if the window changed offset
			{
			return newRect.offset!=oldRect.offset;
			}
		bool isSizeChanged(void) const // Returns true if the window changed size
			{
			return newRect.size!=oldRect.size;
			}
		};
	
	/* Elements: */
	private:
	GLContextPtr context; // Pointer to a GL context object
	int screen; // Screen this window belongs to
	Window root; // Handle of the screen's root window
	Colormap colorMap; // Colormap used in window
	Window window; // X window handle
	Window parent; // Handle of window's parent, to query on-screen position of decorated windows
	Offset parentOffset; // Position of window's top-left corner in its parent's coordinate system
	Atom wmProtocolsAtom,wmDeleteWindowAtom; // Atoms needed for window manager communication
	
	/* Entry points for required/optional GLX extensions: */
	PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXTProc; // Selects vertical retrace synchronization interval
	PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESAProc; // Ditto, for Mesa-derived OpenGL implementations
	PFNGLXWAITVIDEOSYNCSGIPROC glXWaitVideoSyncSGIProc; // Waits for next vertical retrace synchronization pulse
	PFNGLXDELAYBEFORESWAPNVPROC glXDelayBeforeSwapNVProc; // Waits for a specified time *before* next vertical retrace synchronization pulse
	
	Rect rect; // Current position and size of output window
	bool fullscreen; // Flag if the window occupies the full screen (and has no decoration)
	Misc::CallbackList rectChangedCallbacks; // List of callbacks to be called when the window changes position and/or size
	Misc::CallbackList closeCallbacks; // List of callbacks to be called when the user attempts to close the window
	
	/* Private methods: */
	void initWindow(const char* windowName,bool decorate); // Common part of all constructors
	
	/* Constructors and destructors: */
	public:
	GLWindow(GLContext* sContext,int sScreen,const char* windowName,const Rect& sRect,bool decorate); // Creates a window using the given OpenGL context
	GLWindow(const char* displayName,const char* windowName,const Rect& sRect,bool decorate,const GLContext::Properties& contextProperties); // Creates a window by connecting to the given X display
	GLWindow(const char* windowName,const Rect& sRect,bool decorate,const GLContext::Properties& contextProperties); // Ditto; gets the default display name from the environment
	GLWindow(GLWindow* source,int sScreen,const char* windowName,const Rect& sRect,bool decorate); // Creates a window using the same GL context as the given source window, on the given screen of the source window's display
	GLWindow(GLWindow* source,const char* windowName,const Rect& sRect,bool decorate); // Ditto; uses the same screen as the source window
	virtual ~GLWindow(void); // Destroys the window and all associated resources
	
	/* Methods: */
	GLContext& getContext(void) // Returns the window's OpenGL context
		{
		return *context;
		}
	int getConnectionNumber(void) const // Returns a file descriptor for the window's event pipe
		{
		return ConnectionNumber(context->getDisplay());
		}
	GLExtensionManager& getExtensionManager(void) // Returns the window's extension manager
		{
		return context->getExtensionManager();
		}
	GLContextData& getContextData(void) // Returns the window's context data
		{
		return context->getContextData();
		}
	int getScreen(void) const // Returns the window's screen index
		{
		return screen;
		}
	Window getRoot(void) const
		{
		return root;
		}
	Window getWindow(void) const
		{
		return window;
		}
	const Rect& getRect(void) const
		{
		return rect;
		}
	const Offset& getWindowOffset(void) const
		{
		return rect.offset;
		}
	const Size& getWindowSize(void) const
		{
		return rect.size;
		}
	unsigned int getWindowWidth(void) const
		{
		return rect.size[0];
		}
	unsigned int getWindowHeight(void) const
		{
		return rect.size[1];
		}
	Rect getRootWindowRect(void) const; // Returns the position and size of the root window containing this window
	double getScreenWidthMM(void) const; // Returns the physical width of the window's screen in mm
	double getScreenHeightMM(void) const; // Returns the physical height of the window's screen in mm
	Misc::CallbackList& getRectChangedCallbacks(void) // Returns the list of position/size change callbacks
		{
		return rectChangedCallbacks;
		}
	Misc::CallbackList& getCloseCallbacks(void) // Returns the list of close callbacks
		{
		return closeCallbacks;
		}
	void setRect(const Rect& newRect); // Sets the window's position and size
	bool bypassCompositor(void); // Asks the window manager to disable compositing for this window to (hopefully) reduce latency; returns true if the request was made successfully (does not guarantee request was granted)
	bool makeFullscreen(void); // Asks the window manager to switch the window to fullscreen mode; returns true if request was made successfully
	bool toggleFullscreen(void); // Attempts to toggle the window's fullscreen state by communicating with the window manager; returns true if request was made successfully
	bool canVsync(bool frontBufferRendering) const; // Returns true if the local GLX has the capability to sync with vertical retrace in front- or backbuffer rendering mode
	bool canPreVsync(void) const; // Returns true if the local GLX has the capability to sync to a time delta before vertical retrace
	bool setVsyncInterval(int newInterval); // Sets the vertical retrace synchronization for buffer swaps; 0 disables synchronization
	void disableMouseEvents(void); // Tells the window to ignore mouse events (pointer motion, button click and release) from that point on
	void hideCursor(void); // Hides the cursor while inside the window
	void showCursor(void); // Resets the cursor to the one used by the parent window
	bool grabPointer(void); // Grabs the mouse pointer to redirect all following mouse and keyboard events to this window; returns true if grab successful
	void releasePointer(void); // Releases the mouse pointer after a successful grab
	void setCursorPos(const Offset& newCursorPos); // Sets the cursor to the given position in window coordinates
	void redraw(void); // Signals a window that it should redraw itself (can be sent from outside window processing thread)
	void makeCurrent(void) // Sets the window's GL context as the current context
		{
		context->makeCurrent(window);
		}
	void swapBuffers(void) // Swaps front and back buffer
		{
		context->swapBuffers(window);
		}
	void waitForVsync(void); // Waits for the next vertical synchronization pulse
	bool waitForPreVsync(GLfloat delta); // Waits for the specified time before the next vertical synchronization pulse; returns true if the call had to wait
	bool pendingEvents(void) // Returns true if there are pending events on this window's X display connection
		{
		return XPending(context->getDisplay());
		}
	void peekEvent(XEvent& event) // Waits for and returns the next event intended for this window without removing it from the event queue
		{
		XPeekEvent(context->getDisplay(),&event);
		}
	void nextEvent(XEvent& event) // Waits for and returns the next event intended for this window
		{
		XNextEvent(context->getDisplay(),&event);
		}
	bool isEventForWindow(const XEvent& event) const // Returns true if the given event is intended for this window
		{
		return event.xany.type==GenericEvent||event.xany.window==window;
		}
	void processEvent(const XEvent& event); // Sends an X event to the window for processing
	};

#endif

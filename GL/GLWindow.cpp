/***********************************************************************
GLWindow - Class to encapsulate details of the underlying window system
implementation from an application wishing to use OpenGL windows.
Copyright (c) 2001-2024 Oliver Kreylos

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

#include <GL/GLWindow.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdexcept>
#include <X11/cursorfont.h>
#include <GL/glx.h>
#include <GL/GLExtensionManager.h>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <Misc/CallbackData.h>
#include <Misc/MessageLogger.h>

namespace {

/****************
Helper functions:
****************/

GLWindow::Offset XGetOffset(const XConfigureEvent& event) // Retrieves window offset from an X Configure event
	{
	return GLWindow::Offset(event.x,event.y);
	}

GLWindow::Size XGetSize(const XConfigureEvent& event) // Retrieves window size from an X Configure event
	{
	return GLWindow::Size(event.width,event.height);
	}

GLWindow::Rect XGetRect(const XConfigureEvent& event) // Retrieves window position and size from an X Configure event
	{
	return GLWindow::Rect(XGetOffset(event),XGetSize(event));
	}

GLWindow::Rect XGetRect(GLContext& context,Window window) // Convenience wrapper for XGetGeometry
	{
	GLWindow::Rect result;
	
	/* Call the real function: */
	Window root;
	unsigned int borderWidth,depth;
	XGetGeometry(context.getDisplay(),window,&root,&result.offset[0],&result.offset[1],&result.size[0],&result.size[1],&borderWidth,&depth);
	
	return result;
	}

}

/**************************
Methods of class GLWindow:
**************************/

void GLWindow::initWindow(const char* windowName,bool decorate)
	{
	/* Check if the window's screen matches the context's: */
	if(screen!=context->getScreen())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot share OpenGL contexts between screens %d and %d on display %s",screen,context->getScreen(),context->getDisplayName());
	
	/* Get a handle to the root window: */
	root=RootWindow(context->getDisplay(),screen);
	
	/* Create an X colormap (visual might not be default): */
	colorMap=XCreateColormap(context->getDisplay(),root,context->getVisual(),AllocNone);
	
	/* Create an X window with the selected visual: */
	XSetWindowAttributes swa;
	swa.colormap=colorMap;
	swa.border_pixel=0;
	swa.override_redirect=False;
	if(fullscreen) // Create a fullscreen window
		{
		rect.offset=Offset(0,0);
		rect.size=Size(DisplayWidth(context->getDisplay(),screen),DisplayHeight(context->getDisplay(),screen));
		// decorate=false;
		swa.override_redirect=True;
		}
	swa.event_mask=PointerMotionMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|KeyReleaseMask|ExposureMask|StructureNotifyMask;
	unsigned long attributeMask=CWBorderPixel|CWColormap|CWOverrideRedirect|CWEventMask;
	window=XCreateWindow(context->getDisplay(),root,
	                     rect.offset[0],rect.offset[1],rect.size[0],rect.size[1],
	                     0,context->getDepth(),InputOutput,context->getVisual(),attributeMask,&swa);
	XSetStandardProperties(context->getDisplay(),window,windowName,windowName,None,0,0,0);
	
	/* Start by assuming that the window is not parented: */
	parent=window;
	parentOffset=Offset(0,0);
	
	if(!decorate&&!fullscreen)
		{
		/*******************************************************************
		Ask the window manager not to decorate this window:
		*******************************************************************/
		
		/* Create and fill in window manager hint structure inherited from Motif: */
		struct MotifHints // Structure to pass hints to window managers
			{
			/* Elements: */
			public:
			Misc::UInt32 flags;
			Misc::UInt32 functions;
			Misc::UInt32 decorations;
			Misc::SInt32 inputMode;
			Misc::UInt32 status;
			} hints;
		hints.flags=2U; // Only change decorations bit
		hints.functions=0U;
		hints.decorations=0U;
		hints.inputMode=0;
		hints.status=0U;
		
		/* Get the X atom to set hint properties: */
		Atom hintProperty=XInternAtom(context->getDisplay(),"_MOTIF_WM_HINTS",True);
		if(hintProperty!=None)
			{
			/* Set the window manager hint property: */
			XChangeProperty(context->getDisplay(),window,hintProperty,hintProperty,32,PropModeReplace,reinterpret_cast<unsigned char*>(&hints),5);
			}
		else
			Misc::sourcedUserError(__PRETTY_FUNCTION__,"Cannot disable window decorations");
		}
	
	if(fullscreen)
		{
		/* Get relevant window manager protocol atoms: */
		Atom netwmBypassCompositorAtom=XInternAtom(context->getDisplay(),"_NET_WM_BYPASS_COMPOSITOR",True);
		if(netwmBypassCompositorAtom!=None)
			{
			/* Ask the window manager to let this window bypass the compositor: */
			XEvent bypassCompositorEvent;
			memset(&bypassCompositorEvent,0,sizeof(XEvent));
			bypassCompositorEvent.xclient.type=ClientMessage;
			bypassCompositorEvent.xclient.serial=0;
			bypassCompositorEvent.xclient.send_event=True;
			bypassCompositorEvent.xclient.display=context->getDisplay();
			bypassCompositorEvent.xclient.window=window;
			bypassCompositorEvent.xclient.message_type=netwmBypassCompositorAtom;
			bypassCompositorEvent.xclient.format=32;
			bypassCompositorEvent.xclient.data.l[0]=1; // Bypass compositor
			XSendEvent(context->getDisplay(),RootWindow(context->getDisplay(),screen),False,SubstructureRedirectMask|SubstructureNotifyMask,&bypassCompositorEvent);
			XFlush(context->getDisplay());
			}
		else
			Misc::sourcedUserError(__PRETTY_FUNCTION__,"Cannot bypass desktop compositor");
		}
	
	/* Initiate window manager communication: */
	wmProtocolsAtom=XInternAtom(context->getDisplay(),"WM_PROTOCOLS",False);
	wmDeleteWindowAtom=XInternAtom(context->getDisplay(),"WM_DELETE_WINDOW",False);
	XSetWMProtocols(context->getDisplay(),window,&wmDeleteWindowAtom,1);
	
	/* Display the window on the screen: */
	XMapWindow(context->getDisplay(),window);
	
	/* Flush the X queue in case there are events in the receive queue from opening a previous window: */
	XFlush(context->getDisplay());
	
	/* Process events up until the first Expose event to determine the initial window position and size: */
	bool receivedConfigureNotify=false;
	while(true)
		{
		XEvent event;
		XWindowEvent(context->getDisplay(),window,ExposureMask|StructureNotifyMask,&event);
		
		if(event.type==ConfigureNotify)
			{
			/* Check if this is a real event: */
			if(!event.xconfigure.send_event)
				{
				/* The event's position is this window's offset inside its parent: */
				parentOffset=XGetOffset(event.xconfigure);
				}
			
			/* Retrieve the window size: */
			rect.size=XGetSize(event.xconfigure);
			receivedConfigureNotify=true;
			}
		else if(event.type==ReparentNotify)
			{
			/* Retrieve the window's new parent: */
			parent=event.xreparent.parent;
			}
		else if(event.type==Expose)
			{
			/* Put the event back into the queue: */
			XPutBackEvent(context->getDisplay(),&event);
			
			/* We're done here: */
			break;
			}
		}
	
	if(receivedConfigureNotify)
		{
		/*********************************************************************
		Since modern window managers ignore window positions when opening
		windows, we now need to move the window to its requested position.
		Fix suggested by William Sherman.
		*********************************************************************/
		
		/* As this request will go to the redirected parent window, calculate its intended position by taking this window's parent offset into account: */
		XMoveWindow(context->getDisplay(),window,rect.offset[0]-parentOffset[0],rect.offset[1]-parentOffset[1]);
		
		/* Wait for the final ConfigureNotify event to determine the final window position and size: */
		while(true)
			{
			XEvent event;
			XWindowEvent(context->getDisplay(),window,StructureNotifyMask,&event);
		
			if(event.type==ConfigureNotify)
				{
				/* Retrieve the final window position and size: */
				rect=XGetRect(event.xconfigure);
				
				break;
				}
			}
		}
	
	if(fullscreen)
		{
		/* Grab pointer and keyboard: */
		XGrabPointer(context->getDisplay(),window,True,0,GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
		XGrabKeyboard(context->getDisplay(),window,True,GrabModeAsync,GrabModeAsync,CurrentTime);
		}
	
	/* Initialize the OpenGL context: */
	context->init(window);
	
	/* Query needed GLX extension entry points: */
	glXSwapIntervalEXTProc=GLExtensionManager::getFunction<PFNGLXSWAPINTERVALEXTPROC>("glXSwapIntervalEXT");
	glXSwapIntervalMESAProc=GLExtensionManager::getFunction<PFNGLXSWAPINTERVALMESAPROC>("glXSwapIntervalMESA");
	glXWaitVideoSyncSGIProc=GLExtensionManager::getFunction<PFNGLXWAITVIDEOSYNCSGIPROC>("glXWaitVideoSyncSGI");
	glXDelayBeforeSwapNVProc=GLExtensionManager::getFunction<PFNGLXDELAYBEFORESWAPNVPROC>("glXDelayBeforeSwapNV");
	}

GLWindow::GLWindow(GLContext* sContext,int sScreen,const char* windowName,const GLWindow::Rect& sRect,bool decorate)
	:context(sContext),
	 screen(sScreen),
	 rect(sRect),fullscreen(rect.volume()==0)
	{
	/* Call common part of window initialization routine: */
	initWindow(windowName,decorate);
	}

GLWindow::GLWindow(const char* displayName,const char* windowName,const GLWindow::Rect& sRect,bool decorate,const GLContext::Properties& contextProperties)
	:context(new GLContext(displayName)),
	 screen(context->getDefaultScreen()),
	 rect(sRect),fullscreen(rect.volume()==0)
	{
	/* Initialize the new OpenGL context: */
	context->initialize(screen,contextProperties);
	
	/* Call common part of window initialization routine: */
	initWindow(windowName,decorate);
	}

GLWindow::GLWindow(const char* windowName,const GLWindow::Rect& sRect,bool decorate,const GLContext::Properties& contextProperties)
	:context(new GLContext(0)),
	 screen(context->getDefaultScreen()),
	 rect(sRect),fullscreen(rect.volume()==0)
	{
	/* Initialize the new OpenGL context: */
	context->initialize(screen,contextProperties);
	
	/* Call common part of window initialization routine: */
	initWindow(windowName,decorate);
	}

GLWindow::GLWindow(GLWindow* source,int sScreen,const char* windowName,const GLWindow::Rect& sRect,bool decorate)
	:context(source->context),
	 screen(sScreen),
	 rect(sRect),fullscreen(rect.volume()==0)
	{
	/* Call common part of window initialization routine: */
	initWindow(windowName,decorate);
	}

GLWindow::GLWindow(GLWindow* source,const char* windowName,const GLWindow::Rect& sRect,bool decorate)
	:context(source->context),
	 screen(source->screen),
	 rect(sRect),fullscreen(rect.volume()==0)
	{
	/* Call common part of window initialization routine: */
	initWindow(windowName,decorate);
	}

GLWindow::~GLWindow(void)
	{
	if(fullscreen)
		{
		/* Release the pointer and keyboard grab: */
		XUngrabPointer(context->getDisplay(),CurrentTime);
		XUngrabKeyboard(context->getDisplay(),CurrentTime);
		}
	
	/* Close the window: */
	XUnmapWindow(context->getDisplay(),window);
	context->release();
	XDestroyWindow(context->getDisplay(),window);
	XFreeColormap(context->getDisplay(),colorMap);
	
	/* Context pointer's destructor will detach from GL context and possibly destroy it */
	}

GLWindow::Rect GLWindow::getRootWindowRect(void) const
	{
	return Rect(Offset(0,0),Size(DisplayWidth(context->getDisplay(),screen),DisplayHeight(context->getDisplay(),screen)));
	}

double GLWindow::getScreenWidthMM(void) const
	{
	return double(DisplayWidthMM(context->getDisplay(),screen));
	}

double GLWindow::getScreenHeightMM(void) const
	{
	return double(DisplayHeightMM(context->getDisplay(),screen));
	}

void GLWindow::setRect(const GLWindow::Rect& newRect)
	{
	#if 0
	
	/* Query the window tree to get the window's parent (the one containing the decorations): */
	Window win_root,win_parent;
	Window* win_children;
	unsigned int win_numChildren;
	XQueryTree(context->getDisplay(),window,&win_root,&win_parent,&win_children,&win_numChildren);
	
	/* Check if the window has decorations: */
	Offset delta(0,0);
	if(win_parent!=win_root)
		{
		/* Query the window's and the parent's geometry to calculate the window's offset inside its parent: */
		Rect parentRect=XGetRect(*context,win_parent);
		Rect winRect=XGetRect(*context,window);
		delta=winRect.offset-parentRect.offset;
		}
	
	/* Move the window's interior's top-left corner to the requested position: */
	XMoveResizeWindow(context->getDisplay(),window,newRect.offset[0]-delta[0],newRect.offset[1]-delta[1],newRect.size[0],newRect.size[1]);
	XFlush(context->getDisplay());
	
	/* Clean up: */
	XFree(win_children);
	
	#else
	
	/* As this request will go to the redirected parent window, calculate its intended position by taking this window's parent offset into account: */
	XMoveResizeWindow(context->getDisplay(),window,newRect.offset[0]-parentOffset[0],newRect.offset[1]-parentOffset[1],newRect.size[0],newRect.size[1]);
	
	#endif
	
	/* Don't update the window size structure yet; this method will cause a ConfigureNotify event which will handle it */
	}

bool GLWindow::bypassCompositor(void)
	{
	/* Get relevant window manager protocol atoms: */
	Atom netwmBypassCompositorAtom=XInternAtom(context->getDisplay(),"_NET_WM_BYPASS_COMPOSITOR",True);
	if(netwmBypassCompositorAtom!=None)
		{
		/* Ask the window manager to let this window bypass the compositor: */
		XEvent bypassCompositorEvent;
		memset(&bypassCompositorEvent,0,sizeof(XEvent));
		bypassCompositorEvent.xclient.type=ClientMessage;
		bypassCompositorEvent.xclient.serial=0;
		bypassCompositorEvent.xclient.send_event=True;
		bypassCompositorEvent.xclient.display=context->getDisplay();
		bypassCompositorEvent.xclient.window=window;
		bypassCompositorEvent.xclient.message_type=netwmBypassCompositorAtom;
		bypassCompositorEvent.xclient.format=32;
		bypassCompositorEvent.xclient.data.l[0]=1; // Bypass compositor
		XSendEvent(context->getDisplay(),RootWindow(context->getDisplay(),screen),False,SubstructureRedirectMask|SubstructureNotifyMask,&bypassCompositorEvent);
		XFlush(context->getDisplay());
		
		return true;
		}
	else
		return false;
	}

bool GLWindow::makeFullscreen(void)
	{
	/*********************************************************************
	"Sane" version of fullscreen switch: Use the window manager protocol
	when supported; otherwise, fall back to hacky method.
	*********************************************************************/
	
	/* Get relevant window manager protocol atoms: */
	Atom netwmStateAtom=XInternAtom(context->getDisplay(),"_NET_WM_STATE",True);
	Atom netwmStateFullscreenAtom=XInternAtom(context->getDisplay(),"_NET_WM_STATE_FULLSCREEN",True);
	if(netwmStateAtom!=None&&netwmStateFullscreenAtom!=None)
		{
		/* Ask the window manager to make this window fullscreen: */
		XEvent fullscreenEvent;
		memset(&fullscreenEvent,0,sizeof(XEvent));
		fullscreenEvent.xclient.type=ClientMessage;
		fullscreenEvent.xclient.serial=0;
		fullscreenEvent.xclient.send_event=True;
		fullscreenEvent.xclient.display=context->getDisplay();
		fullscreenEvent.xclient.window=window;
		fullscreenEvent.xclient.message_type=netwmStateAtom;
		fullscreenEvent.xclient.format=32;
		fullscreenEvent.xclient.data.l[0]=1; // Should be _NET_WM_STATE_ADD, but that doesn't work for some reason
		fullscreenEvent.xclient.data.l[1]=netwmStateFullscreenAtom;
		fullscreenEvent.xclient.data.l[2]=0;
		fullscreenEvent.xclient.data.l[3]=1; // Request source is application
		XSendEvent(context->getDisplay(),RootWindow(context->getDisplay(),screen),False,SubstructureRedirectMask|SubstructureNotifyMask,&fullscreenEvent);
		XFlush(context->getDisplay());
		}
	else
		{
		/*******************************************************************
		Use hacky method of adjusting window size just beyond the root
		window. Only method available if there is no window manager, like on
		dedicated cluster rendering nodes.
		*******************************************************************/
		
		/* Query the window's geometry to calculate its offset inside its parent window (the window manager decoration): */
		Rect winRect=XGetRect(*context,window);
		
		/* Set the window's position and size such that the window manager decoration falls outside the root window: */
		XMoveResizeWindow(context->getDisplay(),window,-winRect.offset[0],-winRect.offset[1],DisplayWidth(context->getDisplay(),screen),DisplayHeight(context->getDisplay(),screen));
		}
	
	/* Raise the window to the top of the stacking hierarchy: */
	XRaiseWindow(context->getDisplay(),window);
	
	/* Always return true for now, due to hacky method: */
	return true;
	}

bool GLWindow::toggleFullscreen(void)
	{
	/* Get relevant window manager protocol atoms: */
	Atom netwmStateAtom=XInternAtom(context->getDisplay(),"_NET_WM_STATE",True);
	Atom netwmStateFullscreenAtom=XInternAtom(context->getDisplay(),"_NET_WM_STATE_FULLSCREEN",True);
	if(netwmStateAtom!=None&&netwmStateFullscreenAtom!=None)
		{
		/* Ask the window manager to make this window fullscreen: */
		XEvent fullscreenEvent;
		memset(&fullscreenEvent,0,sizeof(XEvent));
		fullscreenEvent.xclient.type=ClientMessage;
		fullscreenEvent.xclient.serial=0;
		fullscreenEvent.xclient.send_event=True;
		fullscreenEvent.xclient.display=context->getDisplay();
		fullscreenEvent.xclient.window=window;
		fullscreenEvent.xclient.message_type=netwmStateAtom;
		fullscreenEvent.xclient.format=32;
		fullscreenEvent.xclient.data.l[0]=2; // Should be _NET_WM_STATE_TOGGLE, but that doesn't work for some reason
		fullscreenEvent.xclient.data.l[1]=netwmStateFullscreenAtom;
		fullscreenEvent.xclient.data.l[2]=0;
		fullscreenEvent.xclient.data.l[3]=1; // Request source is application
		XSendEvent(context->getDisplay(),RootWindow(context->getDisplay(),screen),False,SubstructureRedirectMask|SubstructureNotifyMask,&fullscreenEvent);
		XFlush(context->getDisplay());
		
		return true;
		}
	else
		return false;
	}

bool GLWindow::canVsync(bool frontBufferRendering) const
	{
	if(frontBufferRendering)
		return glXWaitVideoSyncSGIProc!=0&&context->isDirect();
	else
		return glXSwapIntervalEXTProc!=0||glXSwapIntervalMESAProc!=0;
	}

bool GLWindow::canPreVsync(void) const
	{
	return glXDelayBeforeSwapNVProc!=0;
	}

bool GLWindow::setVsyncInterval(int newInterval)
	{
	/* Check if the GLX_EXT_swap_control or GLX_MESA_swap_control extensions are supported: */
	if(glXSwapIntervalEXTProc!=0)
		{
		/* Set the vsync interval: */
		glXSwapIntervalEXTProc(context->getDisplay(),window,newInterval);
		
		return true;
		}
	else if(glXSwapIntervalMESAProc!=0)
		{
		/* Set the vsync interval: */
		int result=glXSwapIntervalMESAProc(newInterval);
		if(result!=0)
			{
			Misc::sourcedUserError(__PRETTY_FUNCTION__,"Cannot set synch interval to %d due to GL error %d",newInterval,result);
			return false;
			}
		else
			return true;
		}
	else
		return false;
	}

void GLWindow::disableMouseEvents(void)
	{
	/* Get the window's current event mask: */
	XWindowAttributes wa;
	XGetWindowAttributes(context->getDisplay(),window,&wa);
	
	/* Disable mouse-related events: */
	XSetWindowAttributes swa;
	swa.event_mask=wa.your_event_mask&~(PointerMotionMask|EnterWindowMask|LeaveWindowMask|ButtonPressMask|ButtonReleaseMask);
	XChangeWindowAttributes(context->getDisplay(),window,CWEventMask,&swa);
	}

void GLWindow::hideCursor(void)
	{
	/* Why is it so goshdarn complicated to just hide the friggin' cursor? */
	static char emptyCursorBits[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	Pixmap emptyCursorPixmap=XCreatePixmapFromBitmapData(context->getDisplay(),window,emptyCursorBits,16,16,1,0,1);
	XColor black,white; // Actually, both are dummy colors
	Cursor emptyCursor=XCreatePixmapCursor(context->getDisplay(),emptyCursorPixmap,emptyCursorPixmap,&black,&white,0,0);
	XDefineCursor(context->getDisplay(),window,emptyCursor);
	XFreeCursor(context->getDisplay(),emptyCursor);
	XFreePixmap(context->getDisplay(),emptyCursorPixmap);
	XFlush(context->getDisplay());
	}

void GLWindow::showCursor(void)
	{
	XUndefineCursor(context->getDisplay(),window);
	XFlush(context->getDisplay());
	}

bool GLWindow::grabPointer(void)
	{
	/* Do nothing if the window is in fullscreen mode: */
	if(fullscreen)
		return true;
	
	bool result;
	
	/* Try grabbing the pointer: */
	result=XGrabPointer(context->getDisplay(),window,False,
	                    ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
	                    GrabModeAsync,GrabModeAsync,None,None,CurrentTime)==GrabSuccess;
	if(result)
		{
		/* Try grabbing the keyboard as well: */
		result=XGrabKeyboard(context->getDisplay(),window,False,GrabModeAsync,GrabModeAsync,CurrentTime)==GrabSuccess;
		if(!result)
			{
			/* Release the pointer again: */
			XUngrabPointer(context->getDisplay(),CurrentTime);
			}
		}
	
	return result;
	}

void GLWindow::releasePointer(void)
	{
	/* Do nothing if the window is in fullscreen mode: */
	if(fullscreen)
		return;
	
	XUngrabPointer(context->getDisplay(),CurrentTime);
	XUngrabKeyboard(context->getDisplay(),CurrentTime);
	}

void GLWindow::setCursorPos(const GLWindow::Offset& newCursorPos)
	{
	XWarpPointer(context->getDisplay(),None,window,0,0,0,0,newCursorPos[0],newCursorPos[1]);
	}

void GLWindow::redraw(void)
	{
	/* Send an expose X event for the entire area to this window: */
	XEvent event;
	memset(&event,0,sizeof(XEvent));
	event.type=Expose;
	event.xexpose.display=context->getDisplay();
	event.xexpose.window=window;
	event.xexpose.x=0;
	event.xexpose.y=0;
	event.xexpose.width=rect.size[0];
	event.xexpose.height=rect.size[1];
	event.xexpose.count=0;
	XSendEvent(context->getDisplay(),window,False,0x0,&event);
	XFlush(context->getDisplay());
	}

void GLWindow::waitForVsync(void)
	{
	/* Check if the GLX_SGI_video_sync extension is supported: */
	if(glXWaitVideoSyncSGIProc!=0)
		{
		/* Wait for the next vertical retrace synchronization pulse: */
		unsigned int count;
		glXWaitVideoSyncSGIProc(1,0,&count);
		}
	}

bool GLWindow::waitForPreVsync(GLfloat delta)
	{
	/* Check if the GLX_NV_delay_before_swap extension is supported: */
	if(glXDelayBeforeSwapNVProc!=0)
		{
		/* Wait for a time delta before the next vertical retrace synchronization pulse: */
		return glXDelayBeforeSwapNVProc(context->getDisplay(),window,delta);
		}
	else
		return false;
	}

void GLWindow::processEvent(const XEvent& event)
	{
	switch(event.type)
		{
		case ReparentNotify:
			/* Retrieve the window's new parent: */
			parent=event.xreparent.parent;
			
			break;
		
		case ConfigureNotify:
			{
			/* Retrieve the event's window rectangle: */
			Rect newRect=XGetRect(event.xconfigure);
			
			/* Check whether this is a real (parent-relative coordinates) or synthetic (root-relative coordinates) event: */
			if(!event.xconfigure.send_event) // Real event
				{
				/* Update this window's parent offset, just in case: */
				parentOffset=newRect.offset;
				
				/* Query the parent's geometry to find the absolute window position: */
				Rect parentRect=XGetRect(*context,parent);
				
				/* Calculate the absolute window position: */
				newRect.offset+=parentRect.offset;
				}
			
			/* Update the window position and size and then call the position/size changed callbacks: */
			RectChangedCallbackData cbData(this,rect,newRect);
			rect=newRect;
			rectChangedCallbacks.call(&cbData);
			
			break;
			}
		
		case ClientMessage:
			if(event.xclient.message_type==wmProtocolsAtom&&event.xclient.format==32&&(Atom)(event.xclient.data.l[0])==wmDeleteWindowAtom)
				{
				/* Call the close callbacks: */
				CallbackData cbData(this);
				closeCallbacks.call(&cbData);
				}
			break;
		}
	}

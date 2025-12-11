/***********************************************************************
XBackground - Utility to draw one of several calibration patterns.
Copyright (c) 2004-2025 Oliver Kreylos

This file is part of the Vrui calibration utility package.

The Vrui calibration utility package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Vrui calibration utility package is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui calibration utility package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <Misc/StdError.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CommandLineParser.icpp>

/*********************************************************
Helper class to convert floating-point colors to X colors:
*********************************************************/

class ColorConverter
	{
	/* Elements: */
	private:
	int colorMask[3]; // Mask indicating which bits in an X color belong to red, green, and blue components, respectively
	int colorShift[3]; // Position of red, green, and blue component in X color
	int colorScale[3]; // Maximum values of red, green, and blue compoennts in X color
	int colorBits[3]; // Number of bits for red, green, and blue components
	
	/* Methods: */
	public:
	void init(const Visual* visual) // Initializes the color converter for the given visual
		{
		/* Retrieve color masks from the given visual: */
		colorMask[0]=visual->red_mask;
		colorMask[1]=visual->green_mask;
		colorMask[2]=visual->blue_mask;
		
		/* Calculate shift, scale, and number of bits per component : */
		for(int i=0;i<3;++i)
			{
			/* colorShift is the number of zero bits to the right of colorMask: */
			int cm=colorMask[i];
			for(colorShift[i]=0;(cm&0x1)==0x0;++colorShift[i],cm>>=1)
				;
			
			/* Remember the maximum color component value: */
			colorScale[i]=cm;
			
			/* colorBits is the number of one bits in colorMask: */
			for(colorBits[i]=0;cm!=0x0;++colorBits[i],cm>>=1)
				;
			}
		}
	unsigned long operator()(const float color[3]) const // Converts the given color with [0.0, 1.0] component range to an X color
		{
		unsigned long result=0x000000;
		for(int i=0;i<3;++i)
			{
			/* Conceptually, limit color to the range [0, 1) (with 1.0 being excluded), and convert to range [0, colorScale+1): */
			int comp=int(floorf(color[i]*float(colorScale[i]+1)));
			if(comp<0)
				comp=0;
			else if(comp>=colorScale[i])
				comp=colorScale[i];
			result|=(unsigned long)(comp<<colorShift[i]);
			}
		return result;
		}
	unsigned long operator()(const unsigned char color[3]) const // Ditto, for colors in the range [0, 255]
		{
		unsigned long result=0x000000;
		for(int i=0;i<3;++i)
			{
			/* Shift 8-bit color component to number of bits in X color: */
			int comp=int(color[i])>>(8-colorBits[i]);
			result|=(unsigned long)(comp<<colorShift[i]);
			}
		return result;
		}
	unsigned long operator()(unsigned char red,unsigned char green,unsigned char blue) const // Ditto, for individual components
		{
		unsigned long result=(unsigned long)((int(red)>>(8-colorBits[0]))<<colorShift[0]);
		result|=(unsigned long)((int(green)>>(8-colorBits[1]))<<colorShift[1]);
		result|=(unsigned long)((int(blue)>>(8-colorBits[2]))<<colorShift[2]);
		return result;
		}
	};

/**************************************************
Helper function to load an RGB image in PPM format:
**************************************************/

unsigned char* loadPPMFile(const char* ppmFileName,int ppmSize[2])
	{
	/* Open PPM file: */
	FILE* ppmFile=fopen(ppmFileName,"rb");
	if(ppmFile==0)
		{
		std::ostringstream str;
		str<<"loadPPMFile: Unable to open input file "<<ppmFileName;
		throw std::runtime_error(str.str());
		}
	
	/* Parse PPM file header: */
	char line[256];
	if(fgets(line,sizeof(line),ppmFile)==0||strcmp(line,"P6\n")!=0)
		{
		fclose(ppmFile);
		std::ostringstream str;
		str<<"loadPPMFile: Input file "<<ppmFileName<<" is not a binary RGB PPM file";
		throw std::runtime_error(str.str());
		}
	
	/* Skip all comment lines: */
	do
		{
		if(fgets(line,sizeof(line),ppmFile)==0)
			break;
		}
	while(line[0]=='#');
	
	/* Read image size: */
	if(sscanf(line,"%d %d",&ppmSize[0],&ppmSize[1])!=2)
		{
		fclose(ppmFile);
		std::ostringstream str;
		str<<"loadPPMFile: Input file "<<ppmFileName<<" has a malformed PPM header";
		throw std::runtime_error(str.str());
		}
	
	/* Read (and ignore) maxvalue: */
	if(fgets(line,sizeof(line),ppmFile)==0)
		{
		fclose(ppmFile);
		std::ostringstream str;
		str<<"loadPPMFile: Input file "<<ppmFileName<<" has a malformed PPM header";
		throw std::runtime_error(str.str());
		}
	
	/* Read image data: */
	unsigned char* result=new unsigned char[ppmSize[1]*ppmSize[0]*3];
	#if 1
	if(fread(result,sizeof(unsigned char)*3,ppmSize[1]*ppmSize[0],ppmFile)!=size_t(ppmSize[1]*ppmSize[0]))
		{
		fclose(ppmFile);
		delete[] result;
		std::ostringstream str;
		str<<"loadPPMFile: Error while reading from input file "<<ppmFileName;
		throw std::runtime_error(str.str());
		}
	#else
	for(int y=ppmSize[1]-1;y>=0;--y)
		if(fread(&result[y*ppmSize[0]*3],sizeof(unsigned char)*3,ppmSize[0],ppmFile)!=ppmSize[0])
			{
			fclose(ppmFile);
			delete[] result;
			std::ostringstream str;
			str<<"loadPPMFile: Error while reading from input file "<<ppmFileName;
			throw std::runtime_error(str.str());
			}
	#endif
	
	fclose(ppmFile);
	return result;
	}

/***********************************************
Helper class to represent X11 window geometries:
***********************************************/

struct XWindowGeometry
	{
	/* Elements: */
	public:
	unsigned int setMask; // Bit mask of geometry components that were set
	unsigned int size[2]; // Window's width and height in pixels
	int position[2]; // Position of window's top-left corner relative to its parent in pixels
	
	/* Constructors and destructors: */
	XWindowGeometry(void) // Creates a dummy window geometry with no set parameters
		:setMask(0x0U)
		{
		}
	XWindowGeometry(unsigned int width,unsigned int height) // Creates a window geometry with the given window size
		:setMask(0x1U)
		{
		size[0]=width;
		size[1]=height;
		}
	XWindowGeometry(unsigned int width,unsigned int height,int x, int y) // Creates a window geometry with the given window size and position
		:setMask(0x3U)
		{
		size[0]=width;
		size[1]=height;
		position[0]=x;
		position[1]=y;
		}
	
	/* Methods: */
	void setSize(unsigned int newWidth,unsigned int newHeight) // Sets the window size
		{
		setMask|=0x1U;
		size[0]=newWidth;
		size[1]=newHeight;
		}
	void setPosition(int newX,int newY) // Sets the window position
		{
		setMask|=0x2U;
		position[0]=newX;
		position[1]=newY;
		}
	bool hasSize(void) const // Returns true if the window geometry's size component was set
		{
		return (setMask&0x1U)!=0x0U;
		}
	bool hasPosition(void) const // Returns true if the window geometry's position component was set
		{
		return (setMask&0x2U)!=0x0U;
		}
	};

/*************************************
Helper class to represent X11 windows:
*************************************/

struct WindowState
	{
	/* Elements: */
	public:
	Display* display; // The display connection
	int screen; // The screen containing the window
	Window root; // Handle of the screen's root window
	Window window; // X11 window handle
	Window parent; // Handle of window's parent, to query on-screen position of decorated windows
	int parentOffset[2]; // Position of window's top-left corner in its parent's coordinate system
	Atom wmProtocolsAtom,wmDeleteWindowAtom; // Atoms needed for window manager communication
	XWindowGeometry geometry; // Window position and size
	GC gc; // Graphics context for the window
	ColorConverter colorConverter; // Class to convert colors to X colors for the window's visual
	XImage* image; // Image to display in the window (or 0 if no image was given)
	bool fullscreened; // Flag if the window is currently in full-screen mode
	unsigned long background,foreground; // Window's background and foreground colors
	
	/* Constructors and destructors: */
	public:
	WindowState(void)
		:display(0),screen(0),
		 image(0),
		 fullscreened(false)
		{
		}
	~WindowState(void)
		{
		if(image!=0)
			{
			delete[] (int*)image->data;
			delete image;
			}
		XFreeGC(display,gc);
		XDestroyWindow(display,window);
		}
	
	/* Methods: */
	bool init(Display* sDisplay,int sScreen,bool makeFullscreen,bool decorate)
		{
		/* Store the display connection: */
		display=sDisplay;
		screen=sScreen;
		
		/* Get the root window of this screen: */
		root=RootWindow(display,screen);
		
		/* Get the root window's size: */
		XWindowAttributes rootAttr;
		XGetWindowAttributes(display,root,&rootAttr);
		
		/* Create the new window: */
		if(!geometry.hasSize())
			geometry.setSize(640,480);
		if(!geometry.hasPosition())
			geometry.setPosition((int(rootAttr.width)-int(geometry.size[0]))/2,(int(rootAttr.height)-int(geometry.size[1]))/2);
		window=XCreateSimpleWindow(display,root,geometry.position[0],geometry.position[1],geometry.size[0],geometry.size[1],0,WhitePixel(display,screen),BlackPixel(display,screen));
		XSetStandardProperties(display,window,"XBackground","XBackground",None,0,0,0);
		XSelectInput(display,window,ExposureMask|StructureNotifyMask|KeyPressMask);
		
		/* Start by assuming that the window is not parented: */
		parent=window;
		parentOffset[1]=parentOffset[0]=0;
		
		if(!decorate&&!makeFullscreen)
			{
			/*******************************************************************
			Ask the window manager not to decorate this window:
			*******************************************************************/
			
			/* Create and fill in window manager hint structure inherited from Motif: */
			struct MotifHints // Structure to pass hints to window managers
				{
				/* Elements: */
				public:
				unsigned int flags;
				unsigned int functions;
				unsigned int decorations;
				int inputMode;
				unsigned int status;
				} hints;
			hints.flags=2U; // Only change decorations bit
			hints.functions=0U;
			hints.decorations=0U;
			hints.inputMode=0;
			hints.status=0U;
			
			/* Get the X atom to set hint properties: */
			Atom hintProperty=XInternAtom(display,"_MOTIF_WM_HINTS",True);
			if(hintProperty!=None)
				{
				/* Set the window manager hint property: */
				XChangeProperty(display,window,hintProperty,hintProperty,32,PropModeReplace,reinterpret_cast<unsigned char*>(&hints),5);
				}
			else
				return false;
			}
		
		/* Initiate window manager communication: */
		wmProtocolsAtom=XInternAtom(display,"WM_PROTOCOLS",False);
		wmDeleteWindowAtom=XInternAtom(display,"WM_DELETE_WINDOW",False);
		XSetWMProtocols(display,window,&wmDeleteWindowAtom,1);
		
		/* Map the window onto the screen: */
		XMapRaised(display,window);
		
		/* Flush the X queue in case there are events in the receive queue from opening a previous window: */
		XFlush(display);
		
		/* Process events up until the first Expose event to determine the initial window position and size: */
		bool receivedConfigureNotify=false;
		while(true)
			{
			XEvent event;
			XWindowEvent(display,window,ExposureMask|StructureNotifyMask,&event);
			
			if(event.type==ConfigureNotify)
				{
				/* Check if this is a real event: */
				if(decorate&&!event.xconfigure.send_event)
					{
					/* The event's position is this window's offset inside its parent: */
					parentOffset[0]=event.xconfigure.x;
					parentOffset[1]=event.xconfigure.y;
					}
				
				/* Retrieve the window size: */
				geometry.setSize(event.xconfigure.width,event.xconfigure.height);
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
				XPutBackEvent(display,&event);
				
				/* We're done here: */
				break;
				}
			}
		
		XWindowGeometry actualGeometry;
		for(int trial=0;trial<2;++trial) // Try this at most two times
			{
			/* As this request will go to the redirected parent window, calculate its intended position by taking this window's parent offset into account: */
			XMoveWindow(display,window,geometry.position[0]-parentOffset[0],geometry.position[1]-parentOffset[1]);
			
			/* Wait for the final ConfigureNotify event to determine the final window position and size: */
			XEvent event;
			XWindowEvent(display,window,StructureNotifyMask,&event);
			if(event.type==ConfigureNotify)
				{
				/* Retrieve the final window position and size: */
				actualGeometry=XWindowGeometry(event.xconfigure.width,event.xconfigure.height,event.xconfigure.x,event.xconfigure.y);
				}
			while(XCheckWindowEvent(display,window,StructureNotifyMask,&event))
				{
				if(event.type==ConfigureNotify)
					{
					/* Retrieve the final window position and size: */
					actualGeometry=XWindowGeometry(event.xconfigure.width,event.xconfigure.height,event.xconfigure.x,event.xconfigure.y);
					}
				}
			
			/* Check if the window actually ended up where we wanted: */
			if(actualGeometry.position[0]==geometry.position[0]&&actualGeometry.position[1]==geometry.position[1])
				break;
			
			/* Adjust the parent offset and try again: */
			for(int i=0;i<2;++i)
				parentOffset[i]+=actualGeometry.position[i]-geometry.position[i];
			}
		
		/* Store the final window rectangle: */
		geometry=actualGeometry;
		
		if(makeFullscreen)
			{
			/* Switch the window to full-screen mode: */
			toggleFullscreen();
			}
		
		/* Raise the window to the top of the stacking hierarchy: */
		XRaiseWindow(display,window);
		
		/* Hide the mouse cursor: */
		static char emptyCursorBits[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		Pixmap emptyCursorPixmap=XCreatePixmapFromBitmapData(display,window,emptyCursorBits,16,16,1,0,1);
		XColor black,white; // Actually, both are dummy colors
		Cursor emptyCursor=XCreatePixmapCursor(display,emptyCursorPixmap,emptyCursorPixmap,&black,&white,0,0);
		XDefineCursor(display,window,emptyCursor);
		XFreeCursor(display,emptyCursor);
		XFreePixmap(display,emptyCursorPixmap);
		
		/* Create a graphics context for the window: */
		gc=XCreateGC(display,window,0x0,0);
		
		/* Set up a color converter for the window: */
		XWindowAttributes windowAttr;
		XGetWindowAttributes(display,window,&windowAttr);
		colorConverter.init(windowAttr.visual);
		
		/* Initialize background and foreground colors: */
		setBackground(0,0,0);
		setForeground(255,255,255);
		
		return true;
		}
	void loadImage(const char* ppmFileName,const char* components)
		{
		/* Parse components string: */
		bool useRed=false;
		bool useGreen=false;
		bool useBlue=false;
		for(const char* compPtr=components;*compPtr!='\0';++compPtr)
			switch(*compPtr)
				{
				case 'r':
				case 'R':
					useRed=true;
					break;
				
				case 'g':
				case 'G':
					useGreen=true;
					break;
				
				case 'b':
				case 'B':
					useBlue=true;
					break;
				}
		
		/* Get window's attributes: */
		XWindowAttributes windowAttr;
		XGetWindowAttributes(display,window,&windowAttr);
		
		/* Allocate an image data buffer: */
		int bitsPerPixel=32;
		int bytesPerLine=((bitsPerPixel*windowAttr.width+31)/32)*4;
		int* imageData=new int[windowAttr.width*windowAttr.height];
		
		/* Read the image file: */
		int ppmSize[2];
		unsigned char* ppmData=loadPPMFile(ppmFileName,ppmSize);
		
		/* Initialize the image data: */
		int* imgPtr=imageData;
		const unsigned char* ppmPtr=ppmData;
		for(int y=0;y<windowAttr.height;++y)
			for(int x=0;x<windowAttr.width;++x,++imgPtr)
				if(x<ppmSize[0]&&y<ppmSize[1])
					{
					*imgPtr=colorConverter(useRed?ppmPtr[0]:0,useGreen?ppmPtr[1]:0,useBlue?ppmPtr[2]:0);
					ppmPtr+=3;
					}
		
		/* Delete the image file data: */
		delete[] ppmData;
		
		/* Create an appropriate XImage structure: */
		image=new XImage;
		image->width=windowAttr.width;
		image->height=windowAttr.height;
		image->xoffset=0;
		image->format=ZPixmap;
		image->data=(char*)imageData;
		image->byte_order=ImageByteOrder(display);
		image->bitmap_unit=BitmapUnit(display);
		image->bitmap_bit_order=BitmapBitOrder(display);
		image->bitmap_pad=BitmapPad(display);
		image->depth=windowAttr.depth;
		image->bytes_per_line=bytesPerLine;
		image->bits_per_pixel=bitsPerPixel;
		image->red_mask=windowAttr.visual->red_mask;
		image->green_mask=windowAttr.visual->green_mask;
		image->blue_mask=windowAttr.visual->blue_mask;
		XInitImage(image);
		}
	void toggleFullscreen(void) // Switches the window in or out of full-screen mode
		{
		/* Get relevant window manager protocol atoms: */
		Atom netwmStateAtom=XInternAtom(display,"_NET_WM_STATE",True);
		Atom netwmStateFullscreenAtom=XInternAtom(display,"_NET_WM_STATE_FULLSCREEN",True);
		
		if(fullscreened)
			{
			if(netwmStateAtom!=None&&netwmStateFullscreenAtom!=None)
				{
				/* Ask the window manager to make this window fullscreen: */
				XEvent fullscreenEvent;
				fullscreenEvent.xclient.type=ClientMessage;
				fullscreenEvent.xclient.serial=0;
				fullscreenEvent.xclient.send_event=True;
				fullscreenEvent.xclient.display=display;
				fullscreenEvent.xclient.window=window;
				fullscreenEvent.xclient.message_type=netwmStateAtom;
				fullscreenEvent.xclient.format=32;
				fullscreenEvent.xclient.data.l[0]=0; // Should be _NET_WM_STATE_REMOVE, but that doesn't work for some reason
				fullscreenEvent.xclient.data.l[1]=netwmStateFullscreenAtom;
				fullscreenEvent.xclient.data.l[2]=0;
				XSendEvent(display,RootWindow(display,screen),False,SubstructureRedirectMask|SubstructureNotifyMask,&fullscreenEvent);
				XFlush(display);
				}
			}
		else
			{
			if(netwmStateAtom!=None&&netwmStateFullscreenAtom!=None)
				{
				/* Ask the window manager to make this window fullscreen: */
				XEvent fullscreenEvent;
				fullscreenEvent.xclient.type=ClientMessage;
				fullscreenEvent.xclient.serial=0;
				fullscreenEvent.xclient.send_event=True;
				fullscreenEvent.xclient.display=display;
				fullscreenEvent.xclient.window=window;
				fullscreenEvent.xclient.message_type=netwmStateAtom;
				fullscreenEvent.xclient.format=32;
				fullscreenEvent.xclient.data.l[0]=1; // Should be _NET_WM_STATE_ADD, but that doesn't work for some reason
				fullscreenEvent.xclient.data.l[1]=netwmStateFullscreenAtom;
				fullscreenEvent.xclient.data.l[2]=0;
				XSendEvent(display,RootWindow(display,screen),False,SubstructureRedirectMask|SubstructureNotifyMask,&fullscreenEvent);
				XFlush(display);
				}
			else
				{
				/*******************************************************************
				Use hacky method of adjusting window size just beyond the root
				window.
				*******************************************************************/
				
				/* Set the window's position and size such that the window manager decoration falls outside the root window: */
				XMoveResizeWindow(display,window,-parentOffset[0],-parentOffset[1],DisplayWidth(display,screen),DisplayHeight(display,screen));
				}
			}
		fullscreened=!fullscreened;
		}
	void setBackground(const unsigned char color[3])
		{
		background=colorConverter(color);
		XSetBackground(display,gc,background);
		}
	void setBackground(unsigned char red,unsigned char green,unsigned char blue)
		{
		background=colorConverter(red,green,blue);
		XSetBackground(display,gc,background);
		}
	void setForeground(const unsigned char color[3])
		{
		foreground=colorConverter(color);
		XSetForeground(display,gc,foreground);
		}
	void setForeground(unsigned char red,unsigned char green,unsigned char blue)
		{
		foreground=colorConverter(red,green,blue);
		XSetForeground(display,gc,foreground);
		}
	};

void redraw(const WindowState& ws,int winOriginX,int winOriginY,int winWidth,int winHeight,int patternType,int squareSize)
	{
	if(ws.image!=0)
		{
		/* Draw the image: */
		XPutImage(ws.display,ws.window,ws.gc,ws.image,0,0,winOriginX,winOriginY,winWidth,winHeight);
		}
	else
		{
		switch(patternType)
			{
			case 0: // Calibration grid
				{
				/* Draw a set of vertical lines: */
				for(int hl=0;hl<=20;++hl)
					{
					int x=int(floor(double(hl)*double(winWidth-1)/20.0+0.5))+winOriginX;
					XDrawLine(ws.display,ws.window,ws.gc,x,winOriginY,x,winOriginY+winHeight-1);
					}
				
				/* Draw a set of horizontal lines: */
				for(int vl=0;vl<=16;++vl)
					{
					int y=int(floor(double(vl)*double(winHeight-1)/16.0+0.5))+winOriginY;
					XDrawLine(ws.display,ws.window,ws.gc,winOriginX,y,winOriginX+winWidth-1,y);
					}
				
				/* Draw some circles: */
				int r=winHeight/2;
				if(r>winWidth/2)
					r=winWidth/2;
				XDrawArc(ws.display,ws.window,ws.gc,winOriginX+winWidth/2-r,winOriginY+winHeight/2-r,r*2,r*2,0,360*64);
				r=(winHeight*2)/15;
				XDrawArc(ws.display,ws.window,ws.gc,winOriginX,winOriginY,r*2,r*2,0,360*64);
				XDrawArc(ws.display,ws.window,ws.gc,winOriginX+winWidth-1-r*2,winOriginY,r*2,r*2,0,360*64);
				XDrawArc(ws.display,ws.window,ws.gc,winOriginX+winWidth-1-r*2,winOriginY+winHeight-1-r*2,r*2,r*2,0,360*64);
				XDrawArc(ws.display,ws.window,ws.gc,winOriginX,winOriginY+winHeight-1-r*2,r*2,r*2,0,360*64);
				
				/* Draw a fence of vertical lines to check pixel tracking: */
				int fenceYMin=winOriginY+winHeight/2-winHeight/20;
				int fenceYMax=winOriginY+winHeight/2+winHeight/20;
				XSetForeground(ws.display,ws.gc,ws.foreground);
				for(int x=0;x<winWidth;x+=2)
					XDrawLine(ws.display,ws.window,ws.gc,x,fenceYMin,x,fenceYMax);
				XSetForeground(ws.display,ws.gc,ws.background);
				for(int x=1;x<winWidth;x+=2)
					XDrawLine(ws.display,ws.window,ws.gc,x,fenceYMin,x,fenceYMax);
				break;
				}
			
			case 1: // Pixel tracking test
				/* Draw a set of vertical lines: */
				for(int x=winOriginX;x<winOriginX+winWidth;x+=2)
					XDrawLine(ws.display,ws.window,ws.gc,x,winOriginY,x,winOriginY+winHeight-1);
				break;
			
			case 2: // Calibration grid for TotalStation
				{
				/* Draw a set of vertical lines: */
				int offsetX=((winWidth-1)%squareSize)/2;
				for(int x=winOriginX+offsetX;x<winOriginX+winWidth;x+=squareSize)
					XDrawLine(ws.display,ws.window,ws.gc,x,winOriginY,x,winOriginY+winHeight-1);
				
				/* Draw a set of horizontal lines: */
				int offsetY=((winHeight-1)%squareSize)/2;
				for(int y=winOriginY+offsetY;y<winOriginY+winHeight;y+=squareSize)
					XDrawLine(ws.display,ws.window,ws.gc,winOriginX,y,winOriginX+winWidth-1,y);
				
				break;
				}
			
			case 3: // Checkerboard for camera calibration
				{
				/* Determine the offset for the top-left square: */
				int offsetX=((winWidth-1)%squareSize)/2;
				int offsetY=((winHeight-1)%squareSize)/2;
				
				/* Fill the window white: */
				XSetForeground(ws.display,ws.gc,ws.foreground);
				XFillRectangle(ws.display,ws.window,ws.gc,winOriginX,winOriginY,winWidth,winHeight);
				
				/* Draw a checkerboard of black squares: */
				XSetForeground(ws.display,ws.gc,ws.background);
				for(int y=offsetY;y+squareSize<winHeight;y+=squareSize)
					for(int x=offsetX;x+squareSize<winWidth;x+=squareSize)
						if(((x-offsetX)/squareSize+(y-offsetY)/squareSize)%2==0)
							{
							XFillRectangle(ws.display,ws.window,ws.gc,x,y,squareSize,squareSize);
							}
				
				break;
				}
			
			case 4: // Blank screen
				break;
			}
		}
	}

namespace Misc {

template <>
class ValueCoder<XWindowGeometry>
	{
	/* Methods: */
	public:
	static XWindowGeometry decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		try
			{
			XWindowGeometry result;
			const char* sPtr=start;
			
			/* Check if the size component is present: */
			if(sPtr!=end&&*sPtr!='+'&&*sPtr!='-')
				{
				/* Parse the window width: */
				unsigned int width=Misc::ValueCoder<unsigned int>::decode(sPtr,end,&sPtr);
				
				/* Check for the 'x': */
				if(sPtr==end||(*sPtr!='x'&&*sPtr!='X'))
					throw Misc::DecodingError("missing x separator");
				++sPtr;
				
				/* Parse the window height: */
				unsigned int height=Misc::ValueCoder<unsigned int>::decode(sPtr,end,&sPtr);
				
				/* Update the result: */
				result.setSize(width,height);
				}
			
			/* Check if the position component is present: */
			if(sPtr!=end&&(*sPtr=='+'||*sPtr=='-'))
				{
				/* Parse the window x position: */
				int x=Misc::ValueCoder<int>::decode(sPtr,end,&sPtr);
				
				/* Parse the window y position: */
				int y=Misc::ValueCoder<int>::decode(sPtr,end,&sPtr);
				
				/* Update the result: */
				result.setPosition(x,y);
				}
			
			/* Set the decode end pointer: */
			if(decodeEnd!=0)
				*decodeEnd=sPtr;
			
			return result;
			}
		catch(std::runtime_error& err)
			{
			throw Misc::makeStdErr(0,"Unable to convert %s to XWindowGeometry due to %s",std::string(start,end).c_str(),err.what());
			}
		}
	};

}

int main(int argc,char* argv[])
	{
	try
		{
		/* Build a command line parser: */
		Misc::CommandLineParser cmdLine;
		cmdLine.setDescription("Utility to display a variety of calibration patterns or images.");
		cmdLine.setArguments("[ <image file name> [ [r|R][g|G][b|B] ] ]","Loads an image file of the given name in PPM format and applies the optional color mask as a subset of RGB.");
		char* displayNameEnv=getenv("DISPLAY");
		std::string displayName=displayNameEnv!=0?displayNameEnv:":0";
		cmdLine.addValueOption("display","display",displayName,"<X display connection name>","Sets the name of the X display on which to display the calibration image.");
		XWindowGeometry geometry;
		cmdLine.addValueOption("geometry","geometry",geometry,"[<width>x<height>][(+|-)<x>(+|-)<y>]","Sets the size and/or position of the calibration window.");
		bool fullscreen=false;
		cmdLine.addEnableOption("fullscreen","f",fullscreen,"Ask the window manager to make the calibration window full-screen.");
		bool decorate=true;
		cmdLine.addDisableOption("noDecorate","nd",decorate,"Do not add window manager decorations around the calibration window.");
		const char* patternTypeNames[]=
			{
			"TV","Phase","Grid","Checkerboard","Blank"
			};
		unsigned int patternType=0;
		cmdLine.addCategoryOption("patternType","pt",5,patternTypeNames,patternType,"Selects the calibration pattern type.");
		cmdLine.addValueOption("type","type",patternType,"<calibration type index>","Selects the calibration pattern type: 0=TV, 1=Phase, 2=Grid, 3=Checkerboard, 4=Blank.");
		int squareSize=300;
		cmdLine.addValueOption("size","s",squareSize,"<calibration grid size>","Sets the size of a square calibration grid cell in pixels.");
		std::string patternChannels="rgb";
		cmdLine.addValueOption("color","c",patternChannels,"[r|R][g|G][b|B]","Sets the channel mask for the calibration pattern to a subset of RGB.");
		bool splitStereo=false;
		cmdLine.addEnableOption("stereo","stereo",splitStereo,"Displays the calibration pattern in side-by-side stereo.");
		const char* imageFileName=0;
		const char* imageChannels=0;
		
		/* Parse the command line: */
		char** argPtr=argv;
		char** argEnd=argv+argc;
		while(true)
			{
			/* Parse the next chunk of options: */
			argPtr=cmdLine.parse(argPtr,argEnd);
			if(argPtr==argEnd)
				break;
			
			/* Parse a non-option argument: */
			if(imageFileName==0)
				imageFileName=*argPtr;
			else if(imageChannels==0)
				imageChannels=*argPtr;
			else
				throw Misc::makeStdErr(0,"Extra argument %s",*argPtr);
			
			++argPtr;
			}
		
		/* Bail out if help was given: */
		if(cmdLine.hadHelp())
			return 0;
		
		/* Assign default image channel components if none were given: */
		if(imageFileName!=0&&imageChannels==0)
			imageChannels="rgb";
		
		/* Open a connection to the X server: */
		Display* display=XOpenDisplay(displayName.c_str());
		if(display==0)
			throw Misc::makeStdErr(0,"Cannot open connection to display %s",displayName.c_str());
		
		/* Check if the display name contains a screen name: */
		bool haveColon=false;
		const char* periodPtr=0;
		for(const char* dnPtr=displayName.c_str();*dnPtr!='\0';++dnPtr)
			{
			if(*dnPtr==':')
				haveColon=true;
			else if(haveColon&&*dnPtr=='.')
				periodPtr=dnPtr;
			}
		
		/* Open one or more windows: */
		int numWindows;
		WindowState* ws=0;
		if(periodPtr!=0)
			{
			/* Create a window for the given screen: */
			numWindows=1;
			ws=new WindowState[numWindows];
			ws[0].geometry=geometry;
			int screen=atoi(periodPtr+1);
			ws[0].init(display,screen,fullscreen,decorate);
			
			/* Load an image, if given: */
			if(imageFileName!=0&&strcasecmp(imageFileName,"Grid")!=0)
				ws[0].loadImage(imageFileName,imageChannels);
			}
		else
			{
			/* Create a window for each screen: */
			numWindows=ScreenCount(display);
			ws=new WindowState[numWindows];
			for(int screen=0;screen<numWindows;++screen)
				{
				ws[screen].geometry=geometry;
				ws[screen].init(display,screen,fullscreen,decorate);
				
				/* Load an image, if given: */
				if(imageFileName!=0&&strcasecmp(imageFileName,"Grid")!=0)
					ws[screen].loadImage(imageFileName,imageChannels);
				}
			}
		
		/* Set the stereo pattern rendering colors: */
		unsigned char stereoColors[2][3]={{0x00,0xdf,0x00},{0xff,0x20,0xff}};
		
		/* Calculate the mono pattern rendering color: */
		unsigned char monoColor[3]={0,0,0};
		for(const char* pcPtr=patternChannels.c_str();*pcPtr!='\0';++pcPtr)
			{
			switch(*pcPtr)
				{
				case 'r':
				case 'R':
					monoColor[0]=255;
					break;
				
				case 'g':
				case 'G':
					monoColor[1]=255;
					break;
				
				case 'b':
				case 'B':
					monoColor[2]=255;
					break;
				}
			}
		
		/* Process X events: */
		bool goOn=true;
		while(goOn)
			{
			XEvent event;
			XNextEvent(display,&event);
			
			/* Find the target window of this event: */
			int i;
			for(i=0;i<numWindows&&event.xany.window!=ws[i].window;++i)
				;
			if(i<numWindows)
				switch(event.type)
					{
					case ConfigureNotify:
						{
						/* Check whether this is a real (parent-relative coordinates) or synthetic (root-relative coordinates) event: */
						if(event.xconfigure.send_event) // Synthetic event
							{
							/* Update the window position and size: */
							ws[i].geometry.setSize(event.xconfigure.width,event.xconfigure.height);
							ws[i].geometry.setPosition(event.xconfigure.x,event.xconfigure.y);
							}
						else // Real event
							{
							/* Update this window's parent offset, just in case: */
							ws[i].parentOffset[0]=event.xconfigure.x;
							ws[i].parentOffset[1]=event.xconfigure.y;
							
							/* Update the window size: */
							ws[i].geometry.setSize(event.xconfigure.width,event.xconfigure.height);
							
							/* Query the parent's geometry to find the absolute window position: */
							Window root;
							int x,y;
							unsigned int width,height,borderWidth,depth;
							XGetGeometry(ws[i].display,ws[i].parent,&root,&x,&y,&width,&height,&borderWidth,&depth);
							
							/* Calculate the window position: */
							ws[i].geometry.setPosition(x+ws[i].parentOffset[0],y+ws[i].parentOffset[1]);
							}
						
						break;
						}
					
					case KeyPress:
						{
						XKeyEvent keyEvent=event.xkey;
						KeySym keySym=XLookupKeysym(&keyEvent,0);
						if(keySym==XK_F11)
							ws[i].toggleFullscreen();
						
						goOn=keySym!=XK_Escape;
						break;
						}
					
					case Expose:
						if(splitStereo)
							{
							/* Render test pattern for double-wide split-stereo screen: */
							ws->setForeground(stereoColors[0]);
							redraw(ws[i],0,0,ws[i].geometry.size[0]/2,ws[i].geometry.size[1],patternType,squareSize);
							ws->setForeground(stereoColors[1]);
							redraw(ws[i],ws[i].geometry.size[0]/2,0,ws[i].geometry.size[0]/2,ws[i].geometry.size[1],patternType,squareSize);
							}
						else
							{
							/* Render test pattern for regular-size screen: */
							ws->setForeground(monoColor);
							redraw(ws[i],0,0,ws[i].geometry.size[0],ws[i].geometry.size[1],patternType,squareSize);
							}
						break;
						
					case ClientMessage:
						if(event.xclient.message_type==ws[i].wmProtocolsAtom&&event.xclient.format==32&&(Atom)(event.xclient.data.l[0])==ws[i].wmDeleteWindowAtom)
							goOn=false;
						break;
					}
			}
		
		/* Clean up: */
		delete[] ws;
		XCloseDisplay(display);
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"Terminating with error "<<err.what()<<std::endl;
		return 1;
		}
	
	return 0;
	}

/***********************************************************************
GLContext - Class to encapsulate state relating to a single OpenGL
context, to facilitate context sharing between windows.
Copyright (c) 2013-2025 Oliver Kreylos

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

#include <GL/GLContext.h>

#include <stdexcept>
#include <Misc/Utility.h>
#include <Misc/StdError.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBMultisample.h>
#include <GL/Extensions/GLEXTFramebufferSRGB.h>
#include <GL/Extensions/GLEXTTextureSRGB.h>
#include <GL/GLContextData.h>

/**************************************
Methods of class GLContext::Properties:
**************************************/

GLContext::Properties::Properties(void)
	:nonlinear(false),
	 depthBufferSize(16),stencilBufferSize(0),
	 numAuxBuffers(0),
	 numSamples(1),
	 direct(true),backbuffer(true),stereo(false)
	{
	for(int i=0;i<3;++i)
		colorBufferSize[i]=8;
	colorBufferSize[3]=0;
	for(int i=0;i<4;++i)
		accumBufferSize[i]=0;
	}

void GLContext::Properties::setColorBufferSize(int rgbSize,int alphaSize)
	{
	for(int i=0;i<3;++i)
		colorBufferSize[i]=rgbSize;
	colorBufferSize[3]=alphaSize;
	}

void GLContext::Properties::setAccumBufferSize(int rgbSize,int alphaSize)
	{
	for(int i=0;i<3;++i)
		accumBufferSize[i]=rgbSize;
	accumBufferSize[3]=alphaSize;
	}

void GLContext::Properties::merge(const GLContext::Properties& other)
	{
	/* Take the maximum of all requested numerical properties: */
	for(int i=0;i<4;++i)
		colorBufferSize[i]=Misc::max(colorBufferSize[i],other.colorBufferSize[i]);
	depthBufferSize=Misc::max(depthBufferSize,other.depthBufferSize);
	stencilBufferSize=Misc::max(stencilBufferSize,other.stencilBufferSize);
	numAuxBuffers=Misc::max(numAuxBuffers,other.numAuxBuffers);
	for(int i=0;i<4;++i)
		accumBufferSize[i]=Misc::max(accumBufferSize[i],other.accumBufferSize[i]);
	numSamples=Misc::max(numSamples,other.numSamples);
	
	/* Take the disjuntion of all requested boolean properties: */
	nonlinear=nonlinear||other.nonlinear;
	direct=direct||other.direct;
	backbuffer=backbuffer||other.backbuffer;
	stereo=stereo||other.stereo;
	}

/**************************
Methods of class GLContext:
**************************/

GLContext::GLContext(const char* sDisplayName)
	:displayName(sDisplayName!=0?sDisplayName:"default"),display(0),screen(-1),
	 visual(0),context(None),
	 depth(-1),nonlinear(false),
	 extensionManager(0),contextData(0)
	{
	/* Open connection to the X server: */
	display=XOpenDisplay(sDisplayName);
	if(display==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot open display %s",displayName.c_str());
	
	/* Query for GLX extension: */
	int errorBase,eventBase;
	if(!glXQueryExtension(display,&errorBase,&eventBase))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"GLX extension not supported on display %s",displayName.c_str());
	
	/* Initialize the version number: */
	version[1]=version[0]=0;
	}

GLContext::~GLContext(void)
	{
	/* Release this GLX context if it is the current one: */
	if(glXGetCurrentContext()==context)
		glXMakeCurrent(display,None,0);
	
	if(context!=None)
		{
		/* Destroy this GLX context: */
		glXDestroyContext(display,context);
		}
	
	/* Close the X server connection: */
	XCloseDisplay(display);
	}

void GLContext::initialize(int sScreen,const GLContext::Properties& properties)
	{
	/* Select a screen: */
	screen=sScreen;
	if(screen<0)
		screen=XDefaultScreen(display);
	else if(screen>=XScreenCount(display))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Requested screen index %d on display %s larger than %d",screen,displayName.c_str(),XScreenCount(display)-1);
	
	/* Create a list of visual properties matching the requested context properties: */
	int visualProperties[256];
	int* propPtr=visualProperties;
	
	/* Add standard properties first: */
	*(propPtr++)=GLX_RGBA;
	
	/* Check if double buffering is required: */
	if(properties.backbuffer)
		*(propPtr++)=GLX_DOUBLEBUFFER;
	
	/* Ask for the requested main buffer channel sizes: */
	*(propPtr++)=GLX_RED_SIZE;
	*(propPtr++)=properties.colorBufferSize[0];
	*(propPtr++)=GLX_GREEN_SIZE;
	*(propPtr++)=properties.colorBufferSize[1];
	*(propPtr++)=GLX_BLUE_SIZE;
	*(propPtr++)=properties.colorBufferSize[2];
	*(propPtr++)=GLX_ALPHA_SIZE;
	*(propPtr++)=properties.colorBufferSize[3];
	
	/* Ask for sRGB support if requested: */
	if(properties.nonlinear)
		{
		*(propPtr++)=GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT;
		*(propPtr++)=1;
		}
	
	/* All other properties apply to the render buffer, not necessarily the window's visual: */
	if(properties.direct)
		{
		/* Ask for the requested depth buffer size: */
		*(propPtr++)=GLX_DEPTH_SIZE;
		*(propPtr++)=properties.depthBufferSize;
		
		if(properties.numAuxBuffers>0)
			{
			/* Ask for auxiliary buffers: */
			*(propPtr++)=GLX_AUX_BUFFERS;
			*(propPtr++)=properties.numAuxBuffers;
			}
		
		if(properties.stencilBufferSize>0)
			{
			/* Ask for a stencil buffer: */
			*(propPtr++)=GLX_STENCIL_SIZE;
			*(propPtr++)=properties.stencilBufferSize;
			}
		
		/* Check for multisample requests: */
		if(properties.numSamples>1)
			{
			*(propPtr++)=GLX_SAMPLE_BUFFERS_ARB;
			*(propPtr++)=1;
			*(propPtr++)=GLX_SAMPLES_ARB;
			*(propPtr++)=properties.numSamples;
			}
		}
	
	if(properties.accumBufferSize[0]>0||properties.accumBufferSize[1]>0||properties.accumBufferSize[2]>0||properties.accumBufferSize[3]>0)
		{
		/* Ask for an accumulation buffer of the requested channel sizes: */
		*(propPtr++)=GLX_ACCUM_RED_SIZE;
		*(propPtr++)=properties.accumBufferSize[0];
		*(propPtr++)=GLX_ACCUM_GREEN_SIZE;
		*(propPtr++)=properties.accumBufferSize[1];
		*(propPtr++)=GLX_ACCUM_BLUE_SIZE;
		*(propPtr++)=properties.accumBufferSize[2];
		*(propPtr++)=GLX_ACCUM_ALPHA_SIZE;
		*(propPtr++)=properties.accumBufferSize[3];
		}
	
	/* Check for quad buffering (active stereo) requests: */
	if(properties.stereo)
		*(propPtr++)=GLX_STEREO;
	
	/* Terminate the property list: */
	*(propPtr++)=None;
	
	/* Look for a matching visual: */
	XVisualInfo* visInfo=glXChooseVisual(display,screen,visualProperties);
	if(visInfo==0)
		{
		/* Reduce any requested color channel sizes, and try again: */
		for(int* propPtr=visualProperties;*propPtr!=None;++propPtr)
			if(*propPtr==GLX_RED_SIZE||*propPtr==GLX_GREEN_SIZE||*propPtr==GLX_BLUE_SIZE)
				{
				/* Ask for at least one bit per channel: */
				++propPtr;
				*propPtr=1;
				}
		
		/* Search again: */
		visInfo=glXChooseVisual(display,screen,visualProperties);
		if(visInfo==0)
			{
			/* Reduce any requested depth channel sizes, and try yet again: */
			for(int* propPtr=visualProperties;*propPtr!=None;++propPtr)
				if(*propPtr==GLX_DEPTH_SIZE)
					{
					/* Ask for at least one bit of depth channel: */
					++propPtr;
					*propPtr=1;
					}
			
			/* Search one last time: */
			visInfo=glXChooseVisual(display,screen,visualProperties);
			if(visInfo==0)
				{
				/* Now fail: */
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No suitable visual found on display %s",displayName.c_str());
				}
			}
		}
	
	/* Create an OpenGL context: */
	context=glXCreateContext(display,visInfo,0,GL_TRUE);
	if(context==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot create OpenGL context on display %s",displayName.c_str());
	
	/* Remember the chosen visual and display bit depth: */
	visual=visInfo->visual;
	depth=visInfo->depth;
	
	/* Remember if the context is set up for non-linear compressed color values: */
	nonlinear=properties.nonlinear;
	
	/* Delete the visual information structure: */
	XFree(visInfo);
	}

bool GLContext::isDirect(void) const
	{
	return glXIsDirect(display,context);
	}

void GLContext::init(GLXDrawable drawable)
	{
	/* Check if the extension manager already exists: */
	if(extensionManager==0)
		{
		/* Associate the GLX context with the current thread and the given drawable: */
		if(!glXMakeCurrent(display,drawable,context))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot bind GLX context");
		
		/* Query the OpenGL version: */
		const GLubyte* versionString=glGetString(GL_VERSION);
		const GLubyte* vsPtr=versionString;
		
		/* Extract the major version number: */
		version[0]=0U;
		while(*vsPtr>='0'&&*vsPtr<='9')
			version[0]=version[0]*10U+(unsigned int)(*(vsPtr++)-'0');
		
		/* Check for the separating period: */
		if(*(vsPtr++)!='.')
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid OpenGL version string");
		
		/* Extract the minor version number: */
		version[1]=0U;
		while(*vsPtr>='0'&&*vsPtr<='9')
			version[1]=version[1]*10U+(unsigned int)(*(vsPtr++)-'0');
		
		/* Create and initialize the extension manager: */
		extensionManager=new GLExtensionManager;
		
		/* Install this context's GL extension manager: */
		GLExtensionManager::makeCurrent(extensionManager);
		
		if(nonlinear)
			{
			/* Initialize the sRGB OpenGL extensions: */
			GLEXTTextureSRGB::initExtension();
			GLEXTFramebufferSRGB::initExtension();
			}
		
		/* Create a context data manager: */
		contextData=new GLContextData(*this,101);
		
		/* Query initial context state: */
		retrieveViewport();
		}
	}

void GLContext::deinit(void)
	{
	/* Delete the context data and extension managers: */
	GLContextData::makeCurrent(0);
	delete contextData;
	contextData=0;
	GLExtensionManager::makeCurrent(0);
	delete extensionManager;
	extensionManager=0;
	}

void GLContext::makeCurrent(GLXDrawable drawable)
	{
	/* Associate the GLX context with the current thread and the given drawable: */
	if(!glXMakeCurrent(display,drawable,context))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set current GLX context");
	
	/* Install this context's GL extension manager: */
	GLExtensionManager::makeCurrent(extensionManager);
	
	/* Install the this context's GL context data manager: */
	GLContextData::makeCurrent(contextData);
	}

void GLContext::swapBuffers(GLXDrawable drawable)
	{
	/* Swap buffers in the given drawable: */
	glXSwapBuffers(display,drawable);
	}

void GLContext::release(void)
	{
	/* Check if this GLX context is the current one: */
	if(glXGetCurrentContext()==context)
		{
		/* Release this context's context data and extension managers: */
		GLContextData::makeCurrent(0);
		GLExtensionManager::makeCurrent(0);
		
		/* Release the GLX context: */
		glXMakeCurrent(display,None,0);
		}
	}

const GLContext::Viewport& GLContext::retrieveViewport(void)
	{
	/* Query the current viewport from OpenGL: */
	GLint viewportRect[4];
	glGetIntegerv(GL_VIEWPORT,viewportRect);
	for(int i=0;i<2;++i)
		{
		viewport.offset[i]=viewportRect[i];
		viewport.size[i]=viewportRect[2+i];
		}
	
	return viewport;
	}

void GLContext::setViewport(const GLContext::Viewport& newViewport)
	{
	/* Only update the viewport if it actually changed: */
	if(viewport!=newViewport)
		{
		viewport=newViewport;
		
		/* Set the new viewport in OpenGL: */
		glViewport(viewport.offset[0],viewport.offset[1],viewport.size[0],viewport.size[1]);
		}
	}

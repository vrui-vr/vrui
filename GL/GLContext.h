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

#ifndef GLCONTEXT_INCLUDED
#define GLCONTEXT_INCLUDED

#include <string>
#include <Misc/Autopointer.h>
#include <Misc/Rect.h>
#include <Threads/RefCounted.h>
#include <X11/X.h>
#include <GL/glx.h>

/* Forward declarations: */
class GLExtensionManager;
class GLContextData;

class GLContext:public Threads::RefCounted
	{
	/* Embedded classes: */
	public:
	typedef Misc::Rect<2> Viewport; // Type for viewport rectangles
	
	struct Properties // Structure to hold context creation properties
		{
		/* Elements: */
		public:
		int colorBufferSize[4]; // Minimum required number of bits in each of the main color buffer's channels (R, G, B, Alpha)
		bool nonlinear; // Flag if the main color buffer will store non-linear compressed color values
		int depthBufferSize; // Minimum required number of bits in the depth buffer
		int stencilBufferSize; // Minimum required number of bits in the stencil buffer
		int numAuxBuffers; // Minimum required number of auxiliary buffers
		int accumBufferSize[4]; // Minimum required number of bits in each of the accumulation buffer's channels (R, G, B, Alpha)
		int numSamples; // Minimum required number of multisampling samples
		bool direct; // Flag if the context's rendering target will be the main frame buffer
		bool backbuffer; // Flag if the context requires a back buffer
		bool stereo; // Flag if the context requires left/right stereo buffers
		
		/* Constructors and destructors: */
		Properties(void); // Creates a default property set
		
		/* Methods: */
		void setColorBufferSize(int rgbSize,int alphaSize =0); // Sets the bit sizes of the main color buffer's channels
		void setAccumBufferSize(int rgbSize,int alphaSize =0); // Sets the bit sizes of the accumulation buffer's channels
		void merge(const Properties& other); // Merges this property set with the given property set
		};
	
	/* Elements: */
	private:
	std::string displayName; // The name of the display connection for this context
	Display* display; // Display connection for this context
	int screen; // Screen for which the GLX context was created
	Visual* visual; // Pointer to the visual for which the GLX context was created
	GLXContext context; // GLX context handle
	unsigned int version[2]; // Major and minor version number of local OpenGL
	int depth; // Bit depth of the visual associated with the GLX context
	bool nonlinear; // Flag if the context is set up for non-linear compressed color components in the main color buffer
	GLExtensionManager* extensionManager; // Pointer to an extension manager for this GLX context
	GLContextData* contextData; // Pointer to an object associating per-context application state with this GLX context
	
	/* Context state tracking: */
	Viewport viewport; // The context's current viewport
	
	/* Constructors and destructors: */
	public:
	GLContext(const char* sDisplayName); // Prepares an OpenGL context for the given display name
	private:
	GLContext(const GLContext& source); // Prohibit copy constructor
	GLContext& operator=(const GLContext& source); // Prohibit assignment operator
	public:
	virtual ~GLContext(void); // Destroys the OpenGL context and all associated data
	
	/* Methods: */
	const char* getDisplayName(void) const // Returns the name of the display connection
		{
		return displayName.c_str();
		}
	Display* getDisplay(void) // Returns the context's display connection
		{
		return display;
		}
	int getDefaultScreen(void) // Returns the context's default screen
		{
		return XDefaultScreen(display);
		}
	bool isValid(void) const // Returns true if the context has been created
		{
		return context!=None;
		}
	void initialize(int sScreen,const Properties& properties); // Creates an OpenGL context on the given screen using the context properties
	GLXContext getContext(void) const // Returns the context's GLX handle
		{
		return context;
		}
	unsigned int getMajorVersion(void) const // Returns the local OpenGL's major version number
		{
		return version[0];
		}
	unsigned int getMinorVersion(void) const // Returns the local OpenGL's minor version number
		{
		return version[1];
		}
	bool isVersionLargerEqual(unsigned int major,unsigned int minor) const // Returns true if the local OpenGL is at least the given major.minor version
		{
		return version[0]>major||(version[0]==major&&version[1]>=minor);
		}
	int getScreen(void) const // Returns the screen for which the context was created
		{
		return screen;
		}
	Visual* getVisual(void) // Returns the context's visual
		{
		return visual;
		}
	int getDepth(void) // Returns the context's bit depth
		{
		return depth;
		}
	bool isNonlinear(void) const // Returns true if the context uses non-linear compressed color values in the main color buffer
		{
		return nonlinear;
		}
	bool isDirect(void) const; // Returns true if the OpenGL context supports direct rendering
	void init(GLXDrawable drawable); // Creates the context's extension and context data managers; context will be bound to the given drawable
	void deinit(void); // Destroys the context's extension and context data managers; context must be current on some drawable
	GLExtensionManager& getExtensionManager(void) // Returns the context's extension manager
		{
		return *extensionManager;
		}
	GLContextData& getContextData(void) // Returns the context's context data manager
		{
		return *contextData;
		}
	void makeCurrent(GLXDrawable drawable); // Makes this OpenGL context current in the current thread and the given drawable (window or off-screen buffer)
	void swapBuffers(GLXDrawable drawable); // Swaps front and back buffers in the given drawable
	void release(void); // Detaches the OpenGL context from the current thread and drawable if it is the current context
	
	/* State tracking methods: */
	const Viewport& retrieveViewport(void); // Updates the viewport from OpenGL; returns updated viewport
	const Viewport& getViewport(void) const // Returns the current viewport
		{
		return viewport;
		}
	void setViewport(const Viewport& newViewport); // Sets the viewport
	};

typedef Misc::Autopointer<GLContext> GLContextPtr; // Type for automatic pointers to GLContext objects

#endif

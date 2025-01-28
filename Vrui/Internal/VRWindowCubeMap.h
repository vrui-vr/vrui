/***********************************************************************
VRWindowCubeMap - Class for OpenGL windows that use off-screen rendering
into a cube map to create pre-distorted panoramic display images for
planetarium projectors, panoramic video, etc.
Copyright (c) 2025 Oliver Kreylos

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

#ifndef VRUI_VRWINDOWCUBEMAP_INCLUDED
#define VRUI_VRWINDOWCUBEMAP_INCLUDED

#include <GL/gl.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <Vrui/VRWindow.h>

/* Forward declarations: */
namespace Vrui {
class Viewer;
class VRScreen;
}

namespace Vrui {

class VRWindowCubeMap:public VRWindow
	{
	/* Elements: */
	private:
	Viewer* viewer; // Pointer to the viewer
	Scalar cubeSize; // Size of the cube around the viewer in physical coordinate units
	VRScreen* screens[6]; // Pointers to the six VR screens representing the cube around the viewer
	ISize cubeMapSize; // Size of each of the cube map's faces
	GLuint frameBufferId; // ID of the cube map rendering frame buffer
	GLuint colorBufferId; // ID of the cube map texture representing the faces of the cube map
	GLuint multisamplingColorBufferId; // ID of the shared multisampling color buffer
	GLuint depthStencilBufferId; // ID of the shared depth buffer, potentially interleaved with a stencil buffer
	GLuint multisamplingFrameBufferId; // ID of a frame buffer to "fix" a multisampled image texture into a regular image texture
	GLhandleARB reprojectionShader; // Handle of the shader program to reproject a rendered cube map into the final output window
	GLint reprojectionShaderUniforms[1]; // Locations of the reprojection shader's uniform variables
	
	/* Protected methods from class VRWindow: */
	protected:
	virtual ISize getViewportSize(void) const;
	virtual ISize getFramebufferSize(void) const;
	
	/* Constructors and destructors: */
	public:
	VRWindowCubeMap(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~VRWindowCubeMap(void);
	
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
	virtual void updateScreenDevice(const Scalar windowPos[2],InputDevice* device) const;
	virtual void draw(void);
	virtual void waitComplete(void);
	virtual void present(void);
	};

}

#endif

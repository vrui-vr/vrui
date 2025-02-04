/***********************************************************************
VRWindowAnaglyph2 - Class for OpenGL windows that render an anaglyph 
stereoscopic view with saturation and cross-talk reduction.
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

#ifndef VRUI_VRWINDOWANAGLYPH2_INCLUDED
#define VRUI_VRWINDOWANAGLYPH2_INCLUDED

#include <GL/gl.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <Vrui/Internal/VRWindowSingleViewport.h>

/* Forward declarations: */
namespace Vrui {
class VRScreen;
}

namespace Vrui {

class VRWindowAnaglyph2:public VRWindowSingleViewport
	{
	/* Elements: */
	private:
	GLuint frameBufferId; // ID of the per-eye rendering frame buffer
	GLuint colorBufferIds[2]; // IDs of the per-eye rendering color textures
	GLuint multisamplingColorBufferId; // ID of the shared multisampling color buffer
	GLuint depthStencilBufferId; // ID of the shared depth buffer, potentially interleaved with a stencil buffer
	GLuint multisamplingFrameBufferId; // ID of a frame buffer to "fix" a multisampled image texture into a regular image texture
	Size frameBufferSize; // Current sizes of the rendering frame buffers and textures
	GLhandleARB combiningShader; // Handle of the shader program to combine left/right views into a single anaglyph
	GLfloat colorMatrix[3][3]; // Matrix to reduce the saturation of left/right input colors for better stereo perception in column-major format
	GLint combiningShaderUniforms[3]; // Locations of the combining shader's uniform variables
	
	/* Protected methods from class VRWindowSingleViewport: */
	virtual void drawInner(bool canDraw);
	
	/* Constructors and destructors: */
	public:
	VRWindowAnaglyph2(GLContext& sContext,const OutputConfiguration& sOutputConfiguration,const char* windowName,const IRect& initialRect,bool decorate,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~VRWindowAnaglyph2(void);
	
	/* Methods from class VRWindow: */
	virtual void setDisplayState(DisplayState* newDisplayState,const Misc::ConfigurationFileSection& configFileSection);
	virtual void releaseGLState(void);
	virtual int getNumViews(void) const;
	virtual View getView(int index);
	};

}

#endif

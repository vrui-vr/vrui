/***********************************************************************
GLEXTTextureSRGB - OpenGL extension class for the GL_EXT_texture_sRGB
extension.
Copyright (c) 2022 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLEXTENSIONS_GLEXTTEXTURESRGB_INCLUDED
#define GLEXTENSIONS_GLEXTTEXTURESRGB_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_texture_sRGB
#define GL_EXT_texture_sRGB 1

/* Extension-specific constants: */
#define GL_SRGB_EXT                            0x8C40
#define GL_SRGB8_EXT                           0x8C41
#define GL_SRGB_ALPHA_EXT                      0x8C42
#define GL_SRGB8_ALPHA8_EXT                    0x8C43
#define GL_SLUMINANCE_ALPHA_EXT                0x8C44
#define GL_SLUMINANCE8_ALPHA8_EXT              0x8C45
#define GL_SLUMINANCE_EXT                      0x8C46
#define GL_SLUMINANCE8_EXT                     0x8C47
#define GL_COMPRESSED_SRGB_EXT                 0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT           0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT           0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT     0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT       0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F

#endif

/* Forward declarations of friend functions: */

class GLEXTTextureSRGB:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTTextureSRGB*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	
	/* Constructors and destructors: */
	private:
	GLEXTTextureSRGB(void);
	public:
	virtual ~GLEXTTextureSRGB(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif

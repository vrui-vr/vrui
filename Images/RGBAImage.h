/***********************************************************************
RGBAImage - Specialized image class to represent RGBA images with 8-bit
color depth.
Copyright (c) 2007-2022 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IMAGES_RGBAIMAGE_INCLUDED
#define IMAGES_RGBAIMAGE_INCLUDED

#include <GL/gl.h>
#include <Images/Image.h>

namespace Images {

class RGBAImage:public Image<GLubyte,4>
	{
	/* Embedded classes: */
	public:
	typedef Image<GLubyte,4> Base; // Type of base class
	
	/* Constructors and destructors: */
	public:
	RGBAImage(void) // Creates an invalid image
		{
		}
	RGBAImage(const Size& sSize) // Creates an uninitialized image of the given size
		:Base(sSize,GL_RGBA)
		{
		}
	explicit RGBAImage(const BaseImage& source) // Copies an existing base image (does not copy image representation); throws exception if base image format does not match pixel type
		:Base(source)
		{
		}
	RGBAImage(const RGBAImage& source) // Copies an existing image (does not copy image representation)
		:Base(source)
		{
		}
	RGBAImage& operator=(const BaseImage& source) // Assigns an existing base image (does not copy image representation); throws exception if base image format does not match pixel type
		{
		Base::operator=(source);
		return *this;
		}
	RGBAImage& operator=(const RGBAImage& source) // Assigns an existing image (does not copy image representation)
		{
		Base::operator=(source);
		return *this;
		}
	
	/* Methods: */
	using Base::glReadPixels;
	static RGBAImage glReadPixels(const Offset& offset,const Size& size) // Returns a new image created by reading from the frame buffer
		{
		/* Create a new image of the requested size: */
		RGBAImage result(size);
		
		/* Read from the frame buffer: */
		result.glReadPixels(offset);
		
		return result;
		}
	};

}

#endif

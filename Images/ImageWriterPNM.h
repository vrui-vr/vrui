/***********************************************************************
ImageWriterPNM - Class to write images to files in Portable aNyMap
format.
Copyright (c) 2024 Oliver Kreylos

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

#ifndef IMAGES_IMAGEWRITERPNM_INCLUDED
#define IMAGES_IMAGEWRITERPNM_INCLUDED

#include <Images/ImageWriter.h>

namespace Images {

class ImageWriterPNM:public ImageWriter
	{
	/* Constructors and destructors: */
	public:
	ImageWriterPNM(IO::File& sFile); // Creates a PNM image writer for the given file
	
	/* Methods from ImageWriter: */
	virtual void writeImage(const BaseImage& image);
	};

}

#endif

/***********************************************************************
ReadPNGImage - Functions to read RGB or RGBA images from image files in
PNG formats over an IO::File abstraction.
Copyright (c) 2011-2022 Oliver Kreylos

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

#include <Images/ReadPNGImage.h>

#include <Misc/MessageLogger.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>
#include <Images/ImageReaderPNG.h>

namespace Images {

RGBImage readPNGImage(IO::File& source)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file through deprecated RGBImage readPNGImage(IO::File& file) function");
	
	/* This is a legacy function; read generic image and convert it to 8-bit unsigned RGB: */
	ImageReaderPNG reader(source);
	return RGBImage(reader.readImage().dropAlpha().toRgb().toUInt8());
	}

RGBAImage readTransparentPNGImage(IO::File& source)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file through deprecated RGBAImage readTransparentPNGImage(IO::File& file) function");
	
	/* This is a legacy function; read generic image and convert it to 8-bit unsigned RGBA: */
	ImageReaderPNG reader(source);
	return RGBAImage(reader.readImage().toRgb().toUInt8().addAlpha(1.0));
	}

BaseImage readGenericPNGImage(IO::File& source)
	{
	/* Create a PNG image reader: */
	ImageReaderPNG reader(source);
	
	/* Read the image contained in the file: */
	return reader.readImage();
	}

}

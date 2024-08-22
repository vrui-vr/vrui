/***********************************************************************
ReadPNMImage - Functions to read RGB images from image files in PNM
(Portable AnyMap) formats over an IO::File abstraction.
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

#include <Images/ReadPNMImage.h>

#include <Misc/MessageLogger.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/ImageReaderPNM.h>

namespace Images {

RGBImage readPNMImage(IO::File& source)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file through deprecated RGBImage readPNMImage(IO::File& file) function");
	
	/* This is a legacy function; read generic image and convert it to 8-bit unsigned RGB: */
	ImageReaderPNM reader(source);
	return RGBImage(reader.readImage().dropAlpha().toRgb().toUInt8());
	}

BaseImage readGenericPNMImage(IO::File& source)
	{
	/* Create a PNM image reader: */
	ImageReaderPNM reader(source);
	
	/* Read the image contained in the file: */
	return reader.readImage();
	}

}

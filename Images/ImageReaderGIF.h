/***********************************************************************
ImageReaderGIF - Class to read images from files in GIF format.
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

#ifndef IMAGES_IMAGEREADERGIF_INCLUDED
#define IMAGES_IMAGEREADERGIF_INCLUDED

#include <Misc/SizedTypes.h>
#include <Misc/RGB.h>
#include <Images/ImageReader.h>

namespace Images {

class ImageReaderGIF:public ImageReader
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt8 PixelValue; // Pixel value type used in GIF files
	typedef Misc::RGB<Misc::UInt8> Color; // Color type used in GIF files
	
	/* Elements: */
	private:
	unsigned int bitsPerPixel; // Number of bits per pixel value, between 1 and 8
	PixelValue backgroundColorIndex; // Index of the canvas's background color in the global color map
	float pixelAspectRatio; // Pixel aspect ratio, width/height
	Color* globalColorMap; // Global color map; null if there is no global color map
	bool done; // Flag indicating that there are no more images in the GIF file
	
	/* Private methods: */
	void readNextImageBlock(void); // Reads the header of the next image block in the GIF file and process all intermediate blocks
	
	/* Constructors and destructors: */
	public:
	ImageReaderGIF(IO::File& sFile); // Creates a GIF image reader for the given file
	virtual ~ImageReaderGIF(void); // Destroys the reader
	
	/* Methods from class ImageReader: */
	virtual bool eof(void) const;
	virtual BaseImage readImage(void);
	};

}

#endif

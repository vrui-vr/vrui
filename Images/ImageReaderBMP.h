/***********************************************************************
ImageReaderBMP - Class to read images from files in Windows BMP format.
Copyright (c) 2022 Oliver Kreylos

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

#ifndef IMAGES_IMAGEREADERBMP_INCLUDED
#define IMAGES_IMAGEREADERBMP_INCLUDED

#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructArray.h>
#include <Images/ImageReader.h>

namespace Images {

class ImageReaderBMP:public ImageReader
	{
	/* Embedded classes: */
	private:
	enum BMPCompressionMethods // Enumerated type for image compression methods used in BMP files
		{
		BMP_RGB=0, // Uncompressed RGB color space
		BMP_RLE8,BMP_RLE4, // 8-bit and 4-bit run-length encoded RGB
		BMP_BitFields,
		BMP_JPEG,BMP_PNG, // JPEG or PNG images embedded in PNG files
		BMP_AlphaBitFields,
		BMP_CMYK, // Uncompressed CMYK color space
		BMP_CMYK_RLE8,BMP_CMYK_RLE4 // 8-bit and 4-bit run-length encoded CMYK
		};
	
	enum BMPResolutionUnits // Enumerated type for units of measurement used in image resolution
		{
		BMP_PixelsPerMeter=0 // Only defined value
		};
	
	/* Elements: */
	private:
	bool bottomUp; // Flag whether the image is stored in bottom-up order, default for BMP
	unsigned int numBitsPerPixel; // Number of bits per pixel (usually 1, 4, 8, 16, 24, 32)
	Misc::UInt32 rgbaMasks[4]; // Bit mask for the 3 or 4 pixel channels
	unsigned int compressionMethod; // Compression method
	int resolution[2]; // Image resolution in the given resolution unit
	unsigned int resolutionUnit; // Unit of measurement for image resolution
	unsigned int numPaletteColors; // Number of colors in the image's color palette
	Misc::SelfDestructArray<Misc::UInt32> palette; // Color map used for indexed image files
	bool done; // Flag set after the only image in the image file has been read
	
	/* Constructors and destructors: */
	public:
	ImageReaderBMP(IO::File& sFile); // Creates a PNM image reader for the given file
	
	/* Methods from ImageReader: */
	virtual bool eof(void) const;
	virtual BaseImage readImage(void);
	};

}

#endif

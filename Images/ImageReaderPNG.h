/***********************************************************************
ImageReaderPNG - Class to read images from files in PNG format.
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

#ifndef IMAGES_IMAGEREADERPNG_INCLUDED
#define IMAGES_IMAGEREADERPNG_INCLUDED

#include <png.h>
#include <Images/ImageReader.h>

namespace Images {

class ImageReaderPNG:public ImageReader
	{
	/* Elements: */
	private:
	png_structp pngReadStruct; // Structure representing state of an open PNG image file inside the PNG library
	png_infop pngInfoStruct; // Structure containing information about the image in an open PNG image file
	bool done; // Flag set to true after the image has been read
	
	/* Private methods: */
	static void readDataFunction(png_structp pngReadStruct,png_bytep buffer,png_size_t size); // Called by the PNG library to read additional data from the source
	static void errorFunction(png_structp pngReadStruct,png_const_charp errorMsg); // Called by the PNG library to report an error
	static void warningFunction(png_structp pngReadStruct,png_const_charp warningMsg); // Called by the PNG library to report a recoverable error
	
	/* Constructors and destructors: */
	public:
	ImageReaderPNG(IO::File& sFile); // Creates a PNG image reader for the given file
	virtual ~ImageReaderPNG(void); // Destroys the reader
	
	/* Methods from class ImageReader: */
	virtual bool eof(void) const;
	virtual BaseImage readImage(void);
	};

}

#endif

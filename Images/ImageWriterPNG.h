/***********************************************************************
ImageWriterPNG - Class to write images to files in PNG format.
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

#ifndef IMAGES_IMAGEWRITERPNG_INCLUDED
#define IMAGES_IMAGEWRITERPNG_INCLUDED

#include <png.h>
#include <Images/ImageWriter.h>

namespace Images {

class ImageWriterPNG:public ImageWriter
	{
	/* Elements: */
	private:
	png_structp pngWriteStruct; // Structure representing state of an open PNG image file inside the PNG library
	png_infop pngInfoStruct; // Structure containing information about the image in an open PNG image file
	int interlaceType; // PNG interlacing type for the next image to be written
	int compressionLevel; // PNG compression level for the next image to be written in [0, 0]
	
	/* Private methods: */
	static void writeDataFunction(png_structp pngWriteStruct,png_bytep buffer,png_size_t size); // Called by the PNG library to write additional data to the sink
	static void flushSinkFunction(png_structp pngWriteStruct); // Called by the PNG library to flush the sink
	static void errorFunction(png_structp pngReadStruct,png_const_charp errorMsg); // Called by the PNG library to report an error
	static void warningFunction(png_structp pngReadStruct,png_const_charp warningMsg); // Called by the PNG library to report a recoverable error
	
	/* Constructors and destructors: */
	public:
	ImageWriterPNG(IO::File& sFile); // Creates a PNG image writer for the given file
	virtual ~ImageWriterPNG(void); // Destroys the writer
	
	/* Methods from class ImageWriter: */
	virtual void writeImage(const BaseImage& image);
	
	/* New methods: */
	bool isInterlaced(void) const // Returns true if the next image will be written interlaced
		{
		return interlaceType!=PNG_INTERLACE_NONE;
		}
	int getCompressionLevel(void) const // Returns the compression level for the next image to be written
		{
		return compressionLevel;
		}
	void setInterlaced(bool interlaced); // Enables or disables interlacing for the next image to be written
	void setCompressionLevel(int newCompressionLevel); // Sets the compression level for the next image to be written in [0, 9]
	};

}

#endif

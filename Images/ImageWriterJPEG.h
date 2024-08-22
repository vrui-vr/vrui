/***********************************************************************
ImageWriterJPEG - Class to write images to files in JPEG format.
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

#ifndef IMAGES_IMAGEWRITERJPEG_INCLUDED
#define IMAGES_IMAGEWRITERJPEG_INCLUDED

#include <stddef.h>
#include <stdio.h>
#include <jpeglib.h>
#include <jconfig.h>
#include <GL/gl.h>
#include <Images/ImageWriter.h>

namespace Images {

class ImageWriterJPEG:public ImageWriter,public jpeg_compress_struct
	{
	/* Embedded classes: */
	private:
	class ExceptionErrorManager:public jpeg_error_mgr // Class to manage errors when writing a JPEG file
		{
		/* Private methods: */
		private:
		static void errorExitFunction(j_common_ptr cinfo);
		
		/* Constructors and destructors: */
		public:
		ExceptionErrorManager(void);
		};
	
	class FileDestinationManager:public jpeg_destination_mgr // Class to manage writing to an IO::File sink
		{
		/* Elements: */
		private:
		IO::File& dest; // Reference to the destination stream
		size_t bufferSize; // Size of the currently used output buffer
		
		/* Private methods: */
		void initBuffer(void);
		static void initDestinationFunction(j_compress_ptr cinfo);
		static boolean emptyOutputBufferFunction(j_compress_ptr cinfo);
		static void termDestinationFunction(j_compress_ptr cinfo);
		
		/* Constructors and destructors: */
		public:
		FileDestinationManager(IO::File& sDest);
		};
	
	/* Elements: */
	private:
	ExceptionErrorManager exceptionErrorManager; // Exception manager
	FileDestinationManager fileDestinationManager; // File destination manager
	int quality; // Compression quality for the next image to be written in [0, 100]
	
	/* Constructors and destructors: */
	public:
	ImageWriterJPEG(IO::File& sFile); // Creates a JPEG image writer for the given file
	virtual ~ImageWriterJPEG(void); // Destroys the writer
	
	/* Methods from class ImageWriter: */
	virtual void writeImage(const BaseImage& image);
	
	/* New methods: */
	unsigned int getRequiredChannelSize(void) const // Returns the channel size required by the JPEG library in bytes
		{
		return (unsigned int)(BITS_IN_JSAMPLE/8);
		}
	GLenum getRequiredScalarType(void) const; // Returns the sample scalar type required by the JPEG library
	int getQuality(void) const // Returns the current compression quality setting
		{
		return quality;
		}
	void setQuality(int newQuality); // Sets the compression quality for the next image to be written, in [0, 100]
	};

}

#endif

/***********************************************************************
ImageReaderJPEG - Class to read images from files in JPEG format.
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

#ifndef IMAGES_IMAGEREADERJPEG_INCLUDED
#define IMAGES_IMAGEREADERJPEG_INCLUDED

#include <stddef.h>
#include <stdio.h>
#include <jpeglib.h>
#include <jconfig.h>
#include <Images/ImageReader.h>

namespace Images {

class ImageReaderJPEG:public ImageReader,public jpeg_decompress_struct
	{
	/* Embedded classes: */
	private:
	class ExceptionErrorManager:public jpeg_error_mgr // Class to manage errors when reading a JPEG file
		{
		/* Private methods: */
		private:
		static void errorExitFunction(j_common_ptr cinfo);
		
		/* Constructors and destructors: */
		public:
		ExceptionErrorManager(void);
		};
	
	class FileSourceManager:public jpeg_source_mgr // Class to manage reading from an IO::File source
		{
		/* Elements: */
		private:
		IO::File& source; // Reference to the source stream
		
		/* Private methods: */
		static void initSourceFunction(j_decompress_ptr cinfo);
		static boolean fillInputBufferFunction(j_decompress_ptr cinfo);
		static void skipInputDataFunction(j_decompress_ptr cinfo,long count);
		static void termSourceFunction(j_decompress_ptr cinfo);
		
		/* Constructors and destructors: */
		public:
		FileSourceManager(IO::File& sSource);
		};
	
	/* Elements: */
	private:
	ExceptionErrorManager exceptionErrorManager; // Exception manager
	FileSourceManager fileSourceManager; // File source manager
	bool mustFinishDecompress; // Flag if jpeg_start_decompress has been called without jpeg_finish_decompress
	bool done; // Flag set to true after the image has been read
	
	/* Constructors and destructors: */
	public:
	ImageReaderJPEG(IO::File& sFile); // Creates a JPEG image reader for the given file
	virtual ~ImageReaderJPEG(void); // Destroys the reader
	
	/* Methods from class ImageReader: */
	virtual bool eof(void) const;
	virtual BaseImage readImage(void);
	};

}

#endif

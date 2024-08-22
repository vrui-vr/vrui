/***********************************************************************
ImageWriter - Abstract base class to write images to files in a variety
of image file formats.
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

#include <Images/ImageWriter.h>

#include <Misc/StdError.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <Images/Config.h>
#include <Images/ImageWriterPNM.h>
#if IMAGES_CONFIG_HAVE_PNG
#include <Images/ImageWriterPNG.h>
#endif
#if IMAGES_CONFIG_HAVE_JPEG
#include <Images/ImageWriterJPEG.h>
#endif
#if IMAGES_CONFIG_HAVE_TIFF
#include <Images/ImageWriterTIFF.h>
#endif

namespace Images {

/****************************
Methods of class ImageWriter:
****************************/

ImageWriter* ImageWriter::create(ImageFileFormat imageFileFormat,IO::File& imageFile)
	{
	/* Create an image writer object of the approprate derived class based on the given image file format: */
	switch(imageFileFormat)
		{
		case IFF_PNM:
			return new ImageWriterPNM(imageFile);
		
		case IFF_PNG:
			#if IMAGES_CONFIG_HAVE_PNG
			return new ImageWriterPNG(imageFile);
			#else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"PNG image file format not supported");
			#endif
		
		case IFF_JPEG:
			#if IMAGES_CONFIG_HAVE_JPEG
			return new ImageWriterJPEG(imageFile);
			#else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"JPEG image file format not supported");
			#endif
		
		case IFF_TIFF:
			#if IMAGES_CONFIG_HAVE_TIFF
			return new ImageWriterTIFF(imageFile);
			#else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"TIFF image file format not supported");
			#endif
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported image file format");
		}
	}

ImageWriter* ImageWriter::create(const char* imageFileName)
	{
	/* Determine the type of the given image file: */
	ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
	
	/* Open the file of the given name and delegate to the other method: */
	return create(imageFileFormat,*IO::openFile(imageFileName,IO::File::WriteOnly));
	}

ImageWriter* ImageWriter::create(const IO::Directory& directory,const char* imageFileName)
	{
	/* Determine the type of the given image file: */
	ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
	
	/* Open the file of the given name and delegate to the other method: */
	return create(imageFileFormat,*directory.openFile(imageFileName,IO::File::WriteOnly));
	}

ImageWriter::ImageWriter(IO::File& sFile)
	:file(&sFile)
	{
	}

ImageWriter::~ImageWriter(void)
	{
	}


}

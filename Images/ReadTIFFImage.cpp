/***********************************************************************
ReadTIFFImage - Functions to read RGB images from image files in TIFF
formats over an IO::SeekableFile abstraction.
Copyright (c) 2011-2024 Oliver Kreylos

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

#include <Images/ReadTIFFImage.h>

#include <Images/Config.h>

#if IMAGES_CONFIG_HAVE_TIFF

#include <tiffio.h>
#include <stdexcept>
#include <Misc/SelfDestructArray.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>
#include <Images/TIFFReader.h>
#include <Images/ImageReaderTIFF.h>

namespace Images {

RGBImage readTIFFImage(IO::File& source)
	{
	/* Create a TIFF image reader for the given source file: */
	TIFFReader reader(source);
	
	/* Create the result image: */
	RGBImage result(Size(reader.getWidth(),reader.getHeight()));
	
	/* Read the TIFF image into a temporary RGBA buffer: */
	Misc::SelfDestructArray<uint32_t> rgbaBuffer(reader.getHeight()*reader.getWidth());
	reader.readRgba(rgbaBuffer);
	
	/* Copy the RGB image data into the result image: */
	uint32_t* sPtr=rgbaBuffer;
	RGBImage::Color* dPtr=result.replacePixels();
	for(uint32_t y=0;y<reader.getHeight();++y)
		for(uint32_t x=0;x<reader.getWidth();++x,++sPtr,++dPtr)
			{
			(*dPtr)[0]=RGBImage::Color::Scalar(TIFFGetR(*sPtr));
			(*dPtr)[1]=RGBImage::Color::Scalar(TIFFGetG(*sPtr));
			(*dPtr)[2]=RGBImage::Color::Scalar(TIFFGetB(*sPtr));
			}
	
	return result;
	}

RGBAImage readTransparentTIFFImage(IO::File& source)
	{
	/* Create a TIFF image reader for the given source file: */
	TIFFReader reader(source);
	
	/* Create the result image: */
	RGBAImage result(Size(reader.getWidth(),reader.getHeight()));
	
	/* Read the TIFF image into a temporary RGBA buffer: */
	Misc::SelfDestructArray<uint32_t> rgbaBuffer(reader.getHeight()*reader.getWidth());
	reader.readRgba(rgbaBuffer);
	
	/* Copy the RGB image data into the result image: */
	uint32_t* sPtr=rgbaBuffer;
	RGBAImage::Color* dPtr=result.replacePixels();
	for(uint32_t y=0;y<reader.getHeight();++y)
		for(uint32_t x=0;x<reader.getWidth();++x,++sPtr,++dPtr)
			{
			(*dPtr)[0]=RGBAImage::Color::Scalar(TIFFGetR(*sPtr));
			(*dPtr)[1]=RGBAImage::Color::Scalar(TIFFGetG(*sPtr));
			(*dPtr)[2]=RGBAImage::Color::Scalar(TIFFGetB(*sPtr));
			(*dPtr)[3]=RGBAImage::Color::Scalar(TIFFGetA(*sPtr));
			}
	
	return result;
	}

BaseImage readGenericTIFFImage(IO::File& source,GeoTIFFMetadata* metadata)
	{
	/* Create a TIFF image reader for the source file: */
	ImageReaderTIFF reader(source);
	
	/* Check if the caller wants to retrieve metadata: */
	if(metadata!=0)
		{
		/* Extract GeoTIFF metadata from the TIFF file: */
		*metadata=reader.getMetadata();
		}
	
	/* Read the first image contained in the TIFF file: */
	return reader.readImage();
	}

}

#endif

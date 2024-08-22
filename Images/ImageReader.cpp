/***********************************************************************
ImageReader - Abstract base class to read images from files in a variety
of image file formats.
Copyright (c) 2012-2024 Oliver Kreylos

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

#include <Images/ImageReader.h>

#include <Misc/StdError.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <Images/Config.h>
#include <Images/BaseImage.h>
#include <Images/ImageReaderPNM.h>
#include <Images/ImageReaderGIF.h>
#include <Images/ImageReaderBIL.h>
#if IMAGES_CONFIG_HAVE_PNG
#include <Images/ImageReaderPNG.h>
#endif
#if IMAGES_CONFIG_HAVE_JPEG
#include <Images/ImageReaderJPEG.h>
#endif
#if IMAGES_CONFIG_HAVE_TIFF
#include <Images/ImageReaderTIFF.h>
#endif
#include <Images/ImageReaderIFF.h>
#include <Images/ImageReaderBMP.h>

namespace Images {

/****************************
Methods of class ImageReader:
****************************/

GLenum ImageReader::getFormat(void) const
	{
	/* Determine an image format based on the color space, the existence of an alpha channel, and the number of channels: */
	if(imageSpec.colorSpace==Grayscale)
		{
		if(imageSpec.hasAlpha&&imageSpec.numChannels==2)
			return GL_LUMINANCE_ALPHA;
		else if(!imageSpec.hasAlpha&&imageSpec.numChannels==1)
			return GL_LUMINANCE;
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported number of channels in grayscale image");
		}
	else if(imageSpec.colorSpace==RGB)
		{
		if(imageSpec.hasAlpha&&imageSpec.numChannels==4)
			return GL_RGBA;
		else if(!imageSpec.hasAlpha&&imageSpec.numChannels==3)
			return GL_RGB;
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported number of channels in RGB image");
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported color space");
	}

GLenum ImageReader::getScalarType(void) const
	{
	/* Determine the scalar type from the sample size in bytes and the value type: */
	if(imageSpec.numFieldBytes==1)
		{
		if(imageSpec.valueType==UnsignedInt)
			return GL_UNSIGNED_BYTE;
		else if(imageSpec.valueType==SignedInt)
			return GL_BYTE;
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported 8-bit sample format");
		}
	else if(imageSpec.numFieldBytes==2)
		{
		if(imageSpec.valueType==UnsignedInt)
			return GL_UNSIGNED_SHORT;
		else if(imageSpec.valueType==SignedInt)
			return GL_SHORT;
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported 16-bit sample format");
		}
	else if(imageSpec.numFieldBytes==4)
		{
		if(imageSpec.valueType==UnsignedInt)
			return GL_UNSIGNED_INT;
		else if(imageSpec.valueType==SignedInt)
			return GL_INT;
		else if(imageSpec.valueType==Float)
			return GL_FLOAT;
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported 32-bit sample format");
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported sample bit deptb");
	}

BaseImage ImageReader::createImage(void) const
	{
	return BaseImage(imageSpec.rect.size,imageSpec.numChannels,imageSpec.numFieldBytes,getFormat(),getScalarType());
	}

ImageReader* ImageReader::create(ImageFileFormat imageFileFormat,IO::File& imageFile)
	{
	/* Create an image reader object of the approprate derived class based on the given image file format: */
	switch(imageFileFormat)
		{
		case IFF_PNM:
			return new ImageReaderPNM(imageFile);
		
		case IFF_GIF:
			return new ImageReaderGIF(imageFile);
			
		case IFF_BIL:
			/* Can't read BIL files through an already-open file, as we need a header file: */
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot read BIP/BIL/BSQ image files through an already-open file");
		
		case IFF_PNG:
			#if IMAGES_CONFIG_HAVE_PNG
			return new ImageReaderPNG(imageFile);
			#else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"PNG image file format not supported");
			#endif
		
		case IFF_JPEG:
			#if IMAGES_CONFIG_HAVE_JPEG
			return new ImageReaderJPEG(imageFile);
			#else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"JPEG/JFIF image file format not supported");
			#endif
		
		case IFF_TIFF:
			#if IMAGES_CONFIG_HAVE_TIFF
			return new ImageReaderTIFF(imageFile);
			#else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"TIFF image file format not supported");
			#endif
		
		case IFF_IFF:
			return new ImageReaderIFF(imageFile);
		
		case IFF_BMP:
			return new ImageReaderBMP(imageFile);
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported image file format");
		}
	}

ImageReader* ImageReader::create(const char* imageFileName)
	{
	/* Determine the type of the given image file: */
	ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
	
	/* Check if the image is a BIL/BIP/BSQ file: */
	if(imageFileFormat==IFF_BIL)
		{
		/* Create a BIL image reader for the given image file name: */
		return new ImageReaderBIL(imageFileName);
		}
	else
		{
		/* Open the file of the given name and delegate to the other method: */
		return create(imageFileFormat,*IO::openFile(imageFileName));
		}
	}

ImageReader* ImageReader::create(const IO::Directory& directory,const char* imageFileName)
	{
	/* Determine the type of the given image file: */
	ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
	
	/* Check if the image is a BIL/BIP/BSQ file: */
	if(imageFileFormat==IFF_BIL)
		{
		/* Create a BIL image reader for the given image file name inside the given directory: */
		return new ImageReaderBIL(directory,imageFileName);
		}
	else
		{
		/* Open the file of the given name and delegate to the other method: */
		return create(imageFileFormat,*directory.openFile(imageFileName));
		}
	}

ImageReader::ImageReader(IO::File& sFile)
	:file(&sFile)
	{
	/* Initialize the canvas size: */
	canvasSize=Size(0,0);
	
	/* Initialize the image specification: */
	imageSpec.rect=Rect(Size(0,0));
	imageSpec.colorSpace=InvalidColorSpace;
	imageSpec.hasAlpha=false;
	imageSpec.numChannels=0;
	imageSpec.valueType=InvalidChannelValueType;
	imageSpec.numFieldBits=0;
	imageSpec.numValueBits=0;
	}

ImageReader::~ImageReader(void)
	{
	}

}

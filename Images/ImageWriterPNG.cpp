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

#include <Images/ImageWriterPNG.h>

#include <Misc/Utility.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Images/BaseImage.h>

namespace Images {

/*******************************
Methods of class ImageWriterPNG:
*******************************/

void ImageWriterPNG::writeDataFunction(png_structp pngWriteStruct,png_bytep buffer,png_size_t size)
	{
	/* Get the pointer to the IO::File object: */
	IO::File* sink=static_cast<IO::File*>(png_get_io_ptr(pngWriteStruct));
	
	/* Write the requested number of bytes to the sink, and let the sink handle errors: */
	sink->writeRaw(buffer,size);
	}

void ImageWriterPNG::flushSinkFunction(png_structp pngWriteStruct)
	{
	/* Get the pointer to the IO::File object: */
	IO::File* sink=static_cast<IO::File*>(png_get_io_ptr(pngWriteStruct));
	
	/* Flush the sink buffer: */
	sink->flush();
	}

void ImageWriterPNG::errorFunction(png_structp pngReadStruct,png_const_charp errorMsg)
	{
	/* Throw an exception: */
	throw Misc::makeStdErr("Images::ImageWriterPNG",errorMsg);
	}

void ImageWriterPNG::warningFunction(png_structp pngReadStruct,png_const_charp warningMsg)
	{
	/* Show warning to user: */
	Misc::userWarning(warningMsg);
	}

ImageWriterPNG::ImageWriterPNG(IO::File& sFile)
	:ImageWriter(sFile),
	 pngWriteStruct(0),pngInfoStruct(0),
	 interlaceType(PNG_INTERLACE_NONE),compressionLevel(6)
	{
	/* Allocate the PNG library data structures: */
	if((pngWriteStruct=png_create_write_struct(PNG_LIBPNG_VER_STRING,0,errorFunction,warningFunction))==0
	   ||(pngInfoStruct=png_create_info_struct(pngWriteStruct))==0)
	  {
	  /* Clean up and throw an exception: */
		png_destroy_write_struct(&pngWriteStruct,0);
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Internal error in PNG library");
		}
	
	/* Initialize PNG I/O to write to the supplied data sink: */
	png_set_write_fn(pngWriteStruct,file.getPointer(),writeDataFunction,flushSinkFunction);
	}

ImageWriterPNG::~ImageWriterPNG(void)
	{
	/* Clean up: */
	png_destroy_write_struct(&pngWriteStruct,&pngInfoStruct);
	}

void ImageWriterPNG::writeImage(const BaseImage& image)
	{
	/* Determine the PNG image format compatible with the image: */
	int colorType=0;
	unsigned int numRequiredChannels=0U;
	switch(image.getFormat())
		{
		case GL_LUMINANCE:
			colorType=PNG_COLOR_TYPE_GRAY;
			numRequiredChannels=1;
			break;
		
		case GL_LUMINANCE_ALPHA:
			colorType=PNG_COLOR_TYPE_GRAY_ALPHA;
			numRequiredChannels=2;
			break;
		
		case GL_RGB:
			colorType=PNG_COLOR_TYPE_RGB;
			numRequiredChannels=3;
			break;
		
		case GL_RGBA:
			colorType=PNG_COLOR_TYPE_RGB_ALPHA;
			numRequiredChannels=4;
			break;
		}
	int bitDepth=image.getChannelSize()*8;
	GLenum requiredScalarType=0;
	switch(image.getChannelSize())
		{
		case 1:
			requiredScalarType=GL_UNSIGNED_BYTE;
			break;
		
		case 2:
			requiredScalarType=GL_UNSIGNED_SHORT;
			break;
		}
	if(image.getNumChannels()!=numRequiredChannels||image.getScalarType()!=requiredScalarType)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Incompatible image format");
	
	/* Set the PNG image information structure: */
	png_set_IHDR(pngWriteStruct,pngInfoStruct,image.getWidth(),image.getHeight(),bitDepth,colorType,interlaceType,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
	png_write_info(pngWriteStruct,pngInfoStruct);
	png_set_compression_level(pngWriteStruct,compressionLevel);
	
	/* Write all image rows in reverse order: */
	ptrdiff_t rowStride=image.getRowStride();
	const png_byte* rowPtr=static_cast<const png_byte*>(image.getPixels())+(image.getHeight()-1)*rowStride;
	for(unsigned int row=image.getHeight();row>0;--row,rowPtr-=rowStride)
		png_write_row(pngWriteStruct,rowPtr);
	
	/* Finish writing image: */
	png_write_end(pngWriteStruct,0);
	}

void ImageWriterPNG::setInterlaced(bool interlaced)
	{
	/* Set the interlacing type: */
	interlaceType=interlaced?PNG_INTERLACE_ADAM7:PNG_INTERLACE_NONE;
	}

void ImageWriterPNG::setCompressionLevel(int newCompressionLevel)
	{
	/* Clamp and set the compression level: */
	compressionLevel=Misc::clamp(newCompressionLevel,0,9);
	}

}

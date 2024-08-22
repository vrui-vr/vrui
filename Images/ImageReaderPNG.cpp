/***********************************************************************
ImageReaderPNG - Class to read images from files in PNG format.
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

#include <Images/ImageReaderPNG.h>

#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>

namespace Images {

/*******************************
Methods of class ImageReaderPNG:
*******************************/

void ImageReaderPNG::readDataFunction(png_structp pngReadStruct,png_bytep buffer,png_size_t size)
	{
	/* Get the pointer to the IO::File object: */
	IO::File* source=static_cast<IO::File*>(png_get_io_ptr(pngReadStruct));
	
	/* Read the requested number of bytes from the source, and let the source handle errors: */
	source->read(buffer,size);
	}

void ImageReaderPNG::errorFunction(png_structp pngReadStruct,png_const_charp errorMsg)
	{
	/* Throw an exception: */
	throw Misc::makeStdErr("Images::ImageReaderPNG","%s",errorMsg);
	}

void ImageReaderPNG::warningFunction(png_structp pngReadStruct,png_const_charp warningMsg)
	{
	/* Print a warning to the console: */
	Misc::sourcedConsoleWarning("Images::ImageReaderPNG","%s",warningMsg);
	}

ImageReaderPNG::ImageReaderPNG(IO::File& sFile)
	:ImageReader(sFile),
	 pngReadStruct(0),pngInfoStruct(0),
	 done(false)
	{
	/* Check for PNG file signature: */
	unsigned char pngSignature[8];
	file->read(pngSignature,8);
	if(!png_check_sig(pngSignature,8))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Illegal PNG header");
	
	/* Allocate the PNG library data structures: */
	if((pngReadStruct=png_create_read_struct(PNG_LIBPNG_VER_STRING,0,errorFunction,warningFunction))==0
	   ||(pngInfoStruct=png_create_info_struct(pngReadStruct))==0)
	  {
	  /* Clean up and throw an exception: */
		png_destroy_read_struct(&pngReadStruct,0,0);
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Internal error in PNG library");
		}
	
	/* Initialize PNG I/O to read from the supplied data source: */
	png_set_read_fn(pngReadStruct,file.getPointer(),readDataFunction);
	
	/* Tell the PNG library that the signature has been read: */
	png_set_sig_bytes(pngReadStruct,8);
	
	/* Read the PNG image header: */
	png_read_info(pngReadStruct,pngInfoStruct);
	png_uint_32 imageSize[2];
	int elementSize;
	int colorType;
	png_get_IHDR(pngReadStruct,pngInfoStruct,&imageSize[0],&imageSize[1],&elementSize,&colorType,0,0,0);
	canvasSize=Size(imageSize[0],imageSize[1]);
	imageSpec.rect=Rect(canvasSize);
	
	/* Determine image format and set up image processing: */
	imageSpec.colorSpace=(colorType==PNG_COLOR_TYPE_GRAY||colorType==PNG_COLOR_TYPE_GRAY_ALPHA)?Grayscale:RGB;
	imageSpec.hasAlpha=colorType==PNG_COLOR_TYPE_GRAY_ALPHA||colorType==PNG_COLOR_TYPE_RGB_ALPHA;
	if(colorType==PNG_COLOR_TYPE_PALETTE)
		{
		/* Expand paletted images to RGB: */
		png_set_palette_to_rgb(pngReadStruct);
		}
	if(imageSpec.colorSpace==Grayscale&&elementSize<8)
		{
		/* Expand bitmaps to 8-bit grayscale: */
		png_set_expand_gray_1_2_4_to_8(pngReadStruct);
		}
	#if __BYTE_ORDER==__LITTLE_ENDIAN
	if(elementSize==16)
		{
		/* Swap 16-bit pixels from big endian network order to little endian host order: */
		png_set_swap(pngReadStruct);
		}
	#endif
	if(png_get_valid(pngReadStruct,pngInfoStruct,PNG_INFO_tRNS))
		{
		/* Create a full alpha channel from tRNS chunk: */
		png_set_tRNS_to_alpha(pngReadStruct);
		imageSpec.hasAlpha=true;
		}
	
	/* Apply the image's stored gamma curve: */
	double gamma;
	if(png_get_gAMA(pngReadStruct,pngInfoStruct,&gamma))
		png_set_gamma(pngReadStruct,2.2,gamma);
	
	/* Update the PNG processor and retrieve the potentially changed image format: */
	png_read_update_info(pngReadStruct,pngInfoStruct);
	imageSpec.numChannels=png_get_channels(pngReadStruct,pngInfoStruct);
	elementSize=png_get_bit_depth(pngReadStruct,pngInfoStruct);
	setValueSpec(UnsignedInt,elementSize);
	}

ImageReaderPNG::~ImageReaderPNG(void)
	{
	/* Destroy the PNG library structures: */
	png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
	}

bool ImageReaderPNG::eof(void) const
	{
	return done;
	}

BaseImage ImageReaderPNG::readImage(void)
	{
	/* Create the result image: */
	Size& size=imageSpec.rect.size;
	BaseImage result=createImage();
	
	/* Set up the row pointers array: */
	Misc::SelfDestructArray<png_byte*> rowPointers(new png_byte*[size[1]]);
	ptrdiff_t rowStride=result.getRowStride();
	rowPointers[0]=reinterpret_cast<png_byte*>(result.replacePixels())+(size[1]-1)*rowStride;
	for(unsigned int y=1;y<size[1];++y)
		rowPointers[y]=rowPointers[y-1]-rowStride;
	
	/* Read the PNG image: */
	png_read_image(pngReadStruct,rowPointers.getArray());
	
	/* Finish reading the image: */
	png_read_end(pngReadStruct,0);
	
	/* There can be only one image in a PNG file: */
	done=true;
	
	return result;
	}

}

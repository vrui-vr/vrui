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

#include <Images/ImageWriterJPEG.h>

#include <Misc/Utility.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Images/BaseImage.h>

namespace Images {

/*******************************************************
Methods of class ImageWriterJPEG::ExceptionErrorManager:
*******************************************************/

void ImageWriterJPEG::ExceptionErrorManager::errorExitFunction(j_common_ptr cinfo)
	{
	ExceptionErrorManager* thisPtr=static_cast<ExceptionErrorManager*>(cinfo->err);
	
	/* Throw an exception: */
	throw Misc::makeStdErr("Images::ImageWriterJPEG",thisPtr->jpeg_message_table[thisPtr->msg_code],thisPtr->msg_parm.i[0],thisPtr->msg_parm.i[1],thisPtr->msg_parm.i[2],thisPtr->msg_parm.i[3],thisPtr->msg_parm.i[4],thisPtr->msg_parm.i[5],thisPtr->msg_parm.i[6],thisPtr->msg_parm.i[7]);
	}

ImageWriterJPEG::ExceptionErrorManager::ExceptionErrorManager(void)
	{
	/* Set the method pointer(s) in the base class object: */
	jpeg_std_error(this);
	error_exit=errorExitFunction;
	}

/********************************************************
Methods of class ImageWriterJPEG::FileDestinationManager:
********************************************************/

void ImageWriterJPEG::FileDestinationManager::initBuffer(void)
	{
	/* Set the output buffer to the destination file's output buffer: */
	void* destBuffer;
	free_in_buffer=bufferSize=dest.writeInBufferPrepare(destBuffer);
	next_output_byte=static_cast<JOCTET*>(destBuffer);
	}

void ImageWriterJPEG::FileDestinationManager::initDestinationFunction(j_compress_ptr cinfo)
	{
	FileDestinationManager* thisPtr=static_cast<FileDestinationManager*>(cinfo->dest);
	
	/* Clear the output buffer: */
	thisPtr->initBuffer();
	}

boolean ImageWriterJPEG::FileDestinationManager::emptyOutputBufferFunction(j_compress_ptr cinfo)
	{
	FileDestinationManager* thisPtr=static_cast<FileDestinationManager*>(cinfo->dest);
	
	/* Write the JPEG encoder's output buffer to the destination file: */
	thisPtr->dest.writeInBufferFinish(thisPtr->bufferSize);
	
	/* Clear the output buffer: */
	thisPtr->initBuffer();
	
	return TRUE;
	}

void ImageWriterJPEG::FileDestinationManager::termDestinationFunction(j_compress_ptr cinfo)
	{
	FileDestinationManager* thisPtr=static_cast<FileDestinationManager*>(cinfo->dest);
	
	/* Write the JPEG encoder's final (partial) output buffer to the destination file: */
	thisPtr->dest.writeInBufferFinish(thisPtr->bufferSize-thisPtr->free_in_buffer);
	}

ImageWriterJPEG::FileDestinationManager::FileDestinationManager(IO::File& sDest)
	:dest(sDest)
	{
	/* Install the hook functions: */
	init_destination=initDestinationFunction;
	empty_output_buffer=emptyOutputBufferFunction;
	term_destination=termDestinationFunction;
	}

/********************************
Methods of class ImageWriterJPEG:
********************************/

ImageWriterJPEG::ImageWriterJPEG(IO::File& sFile)
	:ImageWriter(sFile),
	 fileDestinationManager(sFile),
	 quality(90)
	{
	/* Initialize the JPEG library parts of this object: */
	err=&exceptionErrorManager;
	client_data=0;
	jpeg_create_compress(this);
	dest=&fileDestinationManager;
	}

ImageWriterJPEG::~ImageWriterJPEG(void)
	{
	/* Clean up: */
	jpeg_destroy_compress(this);
	}

void ImageWriterJPEG::writeImage(const BaseImage& image)
	{
	/* Check that the image has a compatible format: */
	if(image.getNumChannels()!=3||image.getChannelSize()!=getRequiredChannelSize()||image.getScalarType()!=getRequiredScalarType())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Incompatible image format");
	
	/* Set JPEG library image descriptor: */
	image_width=image.getWidth();
	image_height=image.getHeight();
	input_components=3;
	in_color_space=JCS_RGB;
	
	/* Create default compression parameters: */
	jpeg_set_defaults(this);
	
	/* Set selected compression parameters: */
	jpeg_set_quality(this,quality,TRUE);
	arith_code=FALSE;
	dct_method=JDCT_FASTEST;
	optimize_coding=FALSE;
	
	/* Start compressing to a complete JPEG interchange datastream: */
	jpeg_start_compress(this,TRUE);
	
	/* Create row pointers to flip the image during writing: */
	Misc::SelfDestructArray<const JSAMPLE*> rowPointers(image_height);
	const JSAMPLE* pixels=static_cast<const JSAMPLE*>(image.getPixels());
	ptrdiff_t rowStride=ptrdiff_t(image.getWidth())*3;
	rowPointers[0]=pixels+(image_height-1)*rowStride;
	for(JDIMENSION y=1;y<image_height;++y)
		rowPointers[y]=rowPointers[y-1]-rowStride;
	
	/* Write the JPEG image's scan lines: */
	JDIMENSION scanline=0;
	while(scanline<image_height)
		scanline+=jpeg_write_scanlines(this,const_cast<JSAMPARRAY>(rowPointers+scanline),image_height-scanline);
	
	/* Finish writing the image: */
	jpeg_finish_compress(this);
	}

GLenum ImageWriterJPEG::getRequiredScalarType(void) const
	{
	#if BITS_IN_JSAMPLE==8
	return GL_UNSIGNED_BYTE;
	#elif BITS_IN_JSAMPLE==16
	return GL_UNSIGNED_SHORT;
	#else
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported bit depth in JPEG library");
	#endif
	}

void ImageWriterJPEG::setQuality(int newQuality)
	{
	/* Clamp the given quality to the valid range and set it: */
	quality=Misc::clamp(newQuality,0,100);
	}

}

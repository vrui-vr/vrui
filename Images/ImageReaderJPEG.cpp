/***********************************************************************
ImageReaderJPEG - Class to read images from files in JPEG format.
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

#include <Images/ImageReaderJPEG.h>

#include <stdexcept>
#include <Misc/StdError.h>
#include <Misc/SelfDestructArray.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>

namespace Images {

/*******************************************************
Methods of class ImageReaderJPEG::ExceptionErrorManager:
*******************************************************/

void ImageReaderJPEG::ExceptionErrorManager::errorExitFunction(j_common_ptr cinfo)
	{
	ExceptionErrorManager* thisPtr=static_cast<ExceptionErrorManager*>(cinfo->err);
	
	/* Throw an exception: */
	throw Misc::makeStdErr("Images::ImageReaderJPEG",thisPtr->jpeg_message_table[thisPtr->msg_code],thisPtr->msg_parm.i[0],thisPtr->msg_parm.i[1],thisPtr->msg_parm.i[2],thisPtr->msg_parm.i[3],thisPtr->msg_parm.i[4],thisPtr->msg_parm.i[5],thisPtr->msg_parm.i[6],thisPtr->msg_parm.i[7]);
	}

ImageReaderJPEG::ExceptionErrorManager::ExceptionErrorManager(void)
	{
	/* Set the method pointer(s) in the base class object: */
	jpeg_std_error(this);
	error_exit=errorExitFunction;
	}

/***************************************************
Methods of class ImageReaderJPEG::FileSourceManager:
***************************************************/


void ImageReaderJPEG::FileSourceManager::initSourceFunction(j_decompress_ptr cinfo)
	{
	// FileSourceManager* thisPtr=static_cast<FileSourceManager*>(cinfo->src);
	}

boolean ImageReaderJPEG::FileSourceManager::fillInputBufferFunction(j_decompress_ptr cinfo)
	{
	FileSourceManager* thisPtr=static_cast<FileSourceManager*>(cinfo->src);
	
	/* Fill the JPEG decoder's input buffer directly from the file's read buffer: */
	void* buffer;
	size_t bufferSize=thisPtr->source.readInBuffer(buffer);
	thisPtr->bytes_in_buffer=bufferSize;
	thisPtr->next_input_byte=static_cast<JOCTET*>(buffer);
	
	/* Return true if all data has been read: */
	return bufferSize!=0;
	}

void ImageReaderJPEG::FileSourceManager::skipInputDataFunction(j_decompress_ptr cinfo,long count)
	{
	FileSourceManager* thisPtr=static_cast<FileSourceManager*>(cinfo->src);
	
	size_t skip=size_t(count);
	if(skip<thisPtr->bytes_in_buffer)
		{
		/* Skip inside the decompressor's read buffer: */
		thisPtr->next_input_byte+=skip;
		thisPtr->bytes_in_buffer-=skip;
		}
	else
		{
		/* Flush the decompressor's read buffer and skip in the source: */
		skip-=thisPtr->bytes_in_buffer;
		thisPtr->bytes_in_buffer=0;
		thisPtr->source.skip<JOCTET>(skip);
		}
	}

void ImageReaderJPEG::FileSourceManager::termSourceFunction(j_decompress_ptr cinfo)
	{
	FileSourceManager* thisPtr=static_cast<FileSourceManager*>(cinfo->src);
	
	/* Put remaining data in the JPEG read buffer back into the source file: */
	thisPtr->source.putBackInBuffer(thisPtr->bytes_in_buffer);
	}

ImageReaderJPEG::FileSourceManager::FileSourceManager(IO::File& sSource)
	:source(sSource)
	{
	/* Install the hook functions: */
	init_source=initSourceFunction;
	fill_input_buffer=fillInputBufferFunction;
	skip_input_data=skipInputDataFunction;
	resync_to_restart=jpeg_resync_to_restart; // Use default function
	term_source=termSourceFunction;
	
	/* Clear the input buffer: */
	bytes_in_buffer=0;
	next_input_byte=0;
	}

/********************************
Methods of class ImageReaderJPEG:
********************************/

ImageReaderJPEG::ImageReaderJPEG(IO::File& sFile)
	:ImageReader(sFile),
	 fileSourceManager(sFile),
	 mustFinishDecompress(false),
	 done(false)
	{
	/* Throw an exception if the JPEG library produces anything but 8-bit samples: */
	#if BITS_IN_JSAMPLE!=8
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported bit depth in JPEG library");
	#endif
	
	/* Initialize the JPEG library parts of this object: */
	err=&exceptionErrorManager;
	client_data=0;
	jpeg_create_decompress(this);
	src=&fileSourceManager;
	
	/* Read the JPEG file header: */
	jpeg_read_header(this,TRUE);
	
	/* Set up image processing: */
	if(out_color_space!=JCS_GRAYSCALE)
		{
		/* Force output color space to RGB: */
		out_color_space=JCS_RGB;
		setFormatSpec(RGB,false);
		}
	else
		{
		/* Use grayscale colorspace: */
		setFormatSpec(Grayscale,false);
		}
	setValueSpec(UnsignedInt,8);
	
	/* Prepare for decompression: */
	jpeg_start_decompress(this);
	mustFinishDecompress=true;
	canvasSize=Size(output_width,output_height);
	imageSpec.rect=Rect(canvasSize);
	}

ImageReaderJPEG::~ImageReaderJPEG(void)
	{
	/* Clean up: */
	if(mustFinishDecompress)
		jpeg_finish_decompress(this);
	jpeg_destroy_decompress(this);
	}

bool ImageReaderJPEG::eof(void) const
	{
	return done;
	}

BaseImage ImageReaderJPEG::readImage(void)
	{
	/* Create the result image: */
	Size& size=imageSpec.rect.size;
	BaseImage result=createImage();
	
	/* Create row pointers to flip the image during reading: */
	Misc::SelfDestructArray<JSAMPLE*> rowPointers(new JSAMPLE*[size[1]]);
	ptrdiff_t rowStride=result.getRowStride();
	rowPointers[0]=reinterpret_cast<JSAMPLE*>(result.replacePixels())+(size[1]-1)*rowStride;
	for(unsigned int y=1;y<size[1];++y)
		rowPointers[y]=rowPointers[y-1]-rowStride;
	
	/* Read the JPEG image's scan lines: */
	JDIMENSION scanline=0;
	while(scanline<output_height)
		scanline+=jpeg_read_scanlines(this,rowPointers+scanline,output_height-scanline);
	
	/* Finish reading the image: */
	jpeg_finish_decompress(this);
	mustFinishDecompress=false;
	
	/* There can be only one image in a JPEG file: */
	done=true;
	
	return result;
	}

}

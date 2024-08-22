/***********************************************************************
ImageWriterPNM - Class to write images to files in Portable aNyMap
format.
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

#include <Images/ImageWriterPNM.h>

#include <stdio.h>
#include <Misc/PrintInteger.h>
#include <Misc/StdError.h>
#include <Images/BaseImage.h>

namespace Images {

/*******************************
Methods of class ImageWriterPNM:
*******************************/

namespace {

/****************
Helper functions:
****************/

template <class ScalarParam>
inline
void writeImageASCII(const BaseImage& image,IO::File& file)
	{
	/* Write the image data row-by-row top to bottom: */
	unsigned int rowLength=image.getWidth()*image.getNumChannels();
	ptrdiff_t rowStride=rowLength;
	const ScalarParam* rowPtr=static_cast<const ScalarParam*>(image.getPixels())+(image.getHeight()-1U)*rowStride;
	char buffer[16];
	char* bufferEnd=buffer+(sizeof(buffer)-1);
	for(unsigned int row=0;row<image.getHeight();++row,rowPtr-=rowStride)
		{
		const ScalarParam* rPtr=rowPtr;
		for(unsigned int i=1;i<rowLength;++i,++rowPtr)
			{
			/* Print the pixel component value into the buffer and put a space behind it: */
			char* numberStart=Misc::print(*rPtr,bufferEnd);
			*bufferEnd=' ';
			file.writeRaw(numberStart,bufferEnd-numberStart+1);
			}
		
		/* Print the pixel component value into the buffer and put a newline behind it: */
		char* numberStart=Misc::print(*rPtr,bufferEnd);
		*bufferEnd='\n';
		file.writeRaw(numberStart,bufferEnd-numberStart+1);
		}
	}

}

ImageWriterPNM::ImageWriterPNM(IO::File& sFile)
	:ImageWriter(sFile)
	{
	}

void ImageWriterPNM::writeImage(const BaseImage& image)
	{
	/* Determine the PNM sub-format compatible with the image: */
	int pnmFormat=0;
	unsigned int maxValue=0;
	bool binary=true;
	if(image.getChannelSize()==1&&image.getScalarType()==GL_UNSIGNED_BYTE)
		{
		if(image.getNumChannels()==1&&image.getFormat()==GL_LUMINANCE)
			pnmFormat=5;
		else if(image.getNumChannels()==3&&image.getFormat()==GL_RGB)
			pnmFormat=6;
		maxValue=255U;
		binary=true;
		}
	else if(image.getChannelSize()==2&&image.getScalarType()==GL_UNSIGNED_SHORT)
		{
		if(image.getNumChannels()==1&&image.getFormat()==GL_LUMINANCE)
			pnmFormat=2;
		else if(image.getNumChannels()==3&&image.getFormat()==GL_RGB)
			pnmFormat=3;
		maxValue=65535U;
		binary=false;
		}
	if(pnmFormat==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Incompatible image format");
	
	/* Write the appropriate PNM header to file: */
	char header[1024];
	int headerSize=snprintf(header,sizeof(header),"P%d\n%u %u\n%u\n",pnmFormat,image.getWidth(),image.getHeight(),maxValue);
	file->writeRaw(header,headerSize);
	
	if(binary)
		{
		/* Write the image data row-by-row top to bottom: */
		ptrdiff_t rowStride=image.getRowStride();
		const unsigned char* rowPtr=static_cast<const unsigned char*>(image.getPixels())+(image.getHeight()-1U)*rowStride;
		for(unsigned int row=0;row<image.getHeight();++row,rowPtr-=rowStride)
			file->writeRaw(rowPtr,rowStride);
		}
	else
		{
		/* Write the image in ASCII format depending on its scalar type: */
		if(image.getScalarType()==GL_UNSIGNED_BYTE)
			writeImageASCII<GLubyte>(image,*file);
		else
			writeImageASCII<GLushort>(image,*file);
		}
	}

}

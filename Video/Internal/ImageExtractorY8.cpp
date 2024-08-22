/***********************************************************************
ImageExtractorY8 - Class to extract images from raw video frames encoded
in 8-bit greyscale format.
Copyright (c) 2015-2022 Oliver Kreylos

This file is part of the Basic Video Library (Video).

The Basic Video Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Video Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Video Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Video/Internal/ImageExtractorY8.h>

#include <string.h>
#include <Video/FrameBuffer.h>
#include <Video/Colorspaces.h>

namespace Video {

/*********************************
Methods of class ImageExtractorY8:
*********************************/

ImageExtractorY8::ImageExtractorY8(const Size& sSize)
	:ImageExtractor(size)
	{
	}

void ImageExtractorY8::extractGrey(const FrameBuffer* frame,void* image)
	{
	/* Do a straight-up copy: */
	memcpy(image,frame->start,size.volume());
	}

void ImageExtractorY8::extractRGB(const FrameBuffer* frame,void* image)
	{
	/* Convert pixels to RBG: */
	const unsigned char* fPtr=frame->start;
	const unsigned char* fEnd=fPtr+size.volume();
	unsigned char* iPtr=static_cast<unsigned char*>(image);
	for(;fPtr!=fEnd;++fPtr,iPtr+=3)
		iPtr[2]=iPtr[1]=iPtr[0]=*fPtr;
	}

void ImageExtractorY8::extractYpCbCr(const FrameBuffer* frame,void* image)
	{
	/* Convert pixels to Y'CbCr: */
	const unsigned char* fPtr=frame->start;
	const unsigned char* fEnd=fPtr+size.volume();
	unsigned char* iPtr=static_cast<unsigned char*>(image);
	for(;fPtr!=fEnd;++fPtr,iPtr+=3)
		{
		iPtr[0]=*fPtr;
		iPtr[2]=iPtr[1]=0U;
		}
	}

void ImageExtractorY8::extractYpCbCr420(const FrameBuffer* frame,void* yp,unsigned int ypStride,void* cb,unsigned int cbStride,void* cr,unsigned int crStride)
	{
	/* Copy pixels directly to Y': */
	const unsigned char* fRowPtr=frame->start;
	unsigned char* ypRowPtr=static_cast<unsigned char*>(yp);
	for(unsigned int y=0;y<size[1];++y,fRowPtr+=size[0],ypRowPtr+=ypStride)
		memcpy(ypRowPtr,fRowPtr,size[0]);
	
	/* Reset the Cb and Cr planes to zero: */
	unsigned char* cbRowPtr=static_cast<unsigned char*>(cb);
	unsigned char* crRowPtr=static_cast<unsigned char*>(cr);
	for(unsigned int y=0;y<size[1];y+=2,cbRowPtr+=cbStride,crRowPtr+=crStride)
		{
		memset(cbRowPtr,0,size[0]/2);
		memset(crRowPtr,0,size[0]/2);
		}
	}

}

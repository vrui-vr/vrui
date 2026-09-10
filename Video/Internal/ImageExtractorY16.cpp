/***********************************************************************
ImageExtractorY16 - Class to extract images from raw video frames
encoded in 16-bit little-endian greyscale format.
Copyright (c) 2026 Oliver Kreylos

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

#include <Video/Internal/ImageExtractorY16.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <Video/FrameBuffer.h>
#include <Video/Colorspaces.h>

namespace Video {

/**********************************
Methods of class ImageExtractorY16:
**********************************/

ImageExtractorY16::ImageExtractorY16(const Size& sSize)
	:ImageExtractor(sSize)
	{
	}

void ImageExtractorY16::extractGrey(const FrameBuffer* frame,void* image)
	{
	/* Downshift 16-bit pixel values to 8-bit pixel values: */
	const Misc::UInt16* fPtr=reinterpret_cast<const Misc::UInt16*>(frame->start);
	const Misc::UInt16* frameEnd=fPtr+size.volume();
	Misc::UInt8* iPtr=static_cast<Misc::UInt8*>(image);
	for(;fPtr!=frameEnd;++fPtr,++iPtr)
		*iPtr=Misc::UInt8(*fPtr>>8);
	}

void ImageExtractorY16::extractRGB(const FrameBuffer* frame,void* image)
	{
	/* Downshift 16-bit pixel values to 8-bit pixel values and replicate for greyscale RGB: */
	const Misc::UInt16* fPtr=reinterpret_cast<const Misc::UInt16*>(frame->start);
	const Misc::UInt16* frameEnd=fPtr+size.volume();
	Misc::UInt8* iPtr=static_cast<Misc::UInt8*>(image);
	for(;fPtr!=frameEnd;++fPtr,iPtr+=3)
		iPtr[2]=iPtr[1]=iPtr[0]=Misc::UInt8(*fPtr>>8);
	}

void ImageExtractorY16::extractYpCbCr(const FrameBuffer* frame,void* image)
	{
	/* Downshift 16-bit pixel values to 8-bit pixel values and assign straight to Y': */
	const Misc::UInt16* fPtr=reinterpret_cast<const Misc::UInt16*>(frame->start);
	const Misc::UInt16* frameEnd=fPtr+size.volume();
	Misc::UInt8* iPtr=static_cast<Misc::UInt8*>(image);
	for(;fPtr!=frameEnd;++fPtr,iPtr+=3)
		{
		iPtr[0]=Misc::UInt8(*fPtr>>8);
		iPtr[2]=iPtr[1]=Misc::UInt8(128);
		}
	}

void ImageExtractorY16::extractYpCbCr420(const FrameBuffer* frame,void* yp,unsigned int ypStride,void* cb,unsigned int cbStride,void* cr,unsigned int crStride)
	{
	/* Downshift 16-bit pixel values to 8-bit pixel values and assign straight to Y': */
	const Misc::UInt16* fPtr=reinterpret_cast<const Misc::UInt16*>(frame->start);
	Misc::UInt8* ypRowPtr=static_cast<Misc::UInt8*>(yp);
	for(unsigned int y=0;y<size[1];++y,ypRowPtr+=ypStride)
		{
		const Misc::UInt16* fRowEnd=fPtr+size[0];
		Misc::UInt8* ypPtr=ypRowPtr;
		for(;fPtr!=fRowEnd;++fPtr,++ypPtr)
			*ypPtr=Misc::UInt8(*fPtr>>8);
		}
	
	/* Reset the Cb and Cr planes to zero: */
	Misc::UInt8* cbRowPtr=static_cast<Misc::UInt8*>(cb);
	Misc::UInt8* crRowPtr=static_cast<Misc::UInt8*>(cr);
	for(unsigned int y=0;y<size[1];y+=2,cbRowPtr+=cbStride,crRowPtr+=crStride)
		{
		memset(cbRowPtr,128,size[0]/2);
		memset(crRowPtr,128,size[0]/2);
		}
	}

}

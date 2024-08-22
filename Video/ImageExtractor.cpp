/***********************************************************************
ImageExtractor - Abstract base class for processors that can extract
image data in a variety of formats from raw video streams.
Copyright (c) 2009-2024 Oliver Kreylos

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

#include <Video/ImageExtractor.h>

#include <Misc/StdError.h>
#include <Video/VideoDataFormat.h>
#include <Video/Internal/ImageExtractorY8.h>
#include <Video/Internal/ImageExtractorY10B.h>
#include <Video/Internal/ImageExtractorBA81.h>
#include <Video/Internal/ImageExtractorYUYV.h>
#include <Video/Internal/ImageExtractorUYVY.h>
#include <Video/Internal/ImageExtractorYV12.h>
#include <Video/Internal/ImageExtractorRGB8.h>
#include <Video/Internal/ImageExtractorMJPG.h>

namespace Video {

/*******************************
Methods of class ImageExtractor:
*******************************/

ImageExtractor* ImageExtractor::createExtractor(const VideoDataFormat& format)
	{
	/* Create an extractor compatible with the given video data format's pixel format: */
	switch(format.pixelFormat)
		{
		case 0x20203859: // "Y8  "
		case 0x59455247: // "GREY"
			return new ImageExtractorY8(format.size);
		
		case 0x42303159: // "Y10B"
			return new ImageExtractorY10B(format.size);
		
		case 0x47425247: // "GRBG"
			return new ImageExtractorBA81(format.size,BAYER_GRBG);
			
		case 0x56595559: // "YUYV"
			return new ImageExtractorYUYV(format.size);
		
		case 0x59565955: // "UYVY"
			return new ImageExtractorUYVY(format.size);
		
		case 0x32315559: // "YU12"
			{
			ptrdiff_t yStride=ptrdiff_t(format.size[0])*sizeof(unsigned char);
			ptrdiff_t ySize=ptrdiff_t(format.size[1])*yStride;
			ptrdiff_t cbcrStride=ptrdiff_t((format.size[0]+1)/2)*sizeof(unsigned char);
			ptrdiff_t cbcrSize=ptrdiff_t((format.size[1]+1)/2)*cbcrStride;
			return new ImageExtractorYV12(format.size,0,yStride,ySize,cbcrStride,ySize+cbcrSize,cbcrStride);
			}
		
		case 0x32315659: // "YV12"
			{
			ptrdiff_t yStride=ptrdiff_t(format.size[0])*sizeof(unsigned char);
			ptrdiff_t ySize=ptrdiff_t(format.size[1])*yStride;
			ptrdiff_t cbcrStride=ptrdiff_t((format.size[0]+1)/2)*sizeof(unsigned char);
			ptrdiff_t cbcrSize=ptrdiff_t((format.size[1]+1)/2)*cbcrStride;
			return new ImageExtractorYV12(format.size,0,yStride,ySize+cbcrSize,cbcrStride,ySize,cbcrStride);
			}
		
		case 0x38424752: // "RGB8"
			return new ImageExtractorRGB8(format.size);
		
		case 0x47504a4d: // "MJPG"
			return new ImageExtractorMJPG(format.size);
		
		default:
			{
			char fourcc[5];
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported pixel format %s",format.getFourCC(fourcc));
			}
		}
	}

}

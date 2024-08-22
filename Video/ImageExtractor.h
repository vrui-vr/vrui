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

#ifndef VIDEO_IMAGEEXTRACTOR_INCLUDED
#define VIDEO_IMAGEEXTRACTOR_INCLUDED

#include <Video/Types.h>

/* Forward declarations: */
namespace Video {
struct VideoDataFormat;
class FrameBuffer;
}

namespace Video {

class ImageExtractor
	{
	/* Elements: */
	protected:
	Size size; // Size of video frames extracted by this extractor
	
	/* Constructors and destructors: */
	public:
	static ImageExtractor* createExtractor(const VideoDataFormat& format); // Returns a new-allocated image extractor for the given video data format
	ImageExtractor(const Size& sSize) // Creates an image extractor for the given frame size
		:size(sSize)
		{
		}
	virtual ~ImageExtractor(void)
		{
		}
	
	/* Methods: */
	const Size& getSize(void) const // Returns the extractor's frame size
		{
		return size;
		}
	virtual void extractGrey(const FrameBuffer* frame,void* image) =0; // Extracts an 8-bit greyscale image from the given video buffer; image buffer must hold 1 byte per pixel
	virtual void extractRGB(const FrameBuffer* frame,void* image) =0; // Extracts an 8-bit RGB image from the given video buffer; image buffer must hold 3 bytes per pixel
	virtual void extractYpCbCr(const FrameBuffer* frame,void* image) =0; // Extracts an 8-bit Y'CbCr image from the given video buffer; image buffer must hold 3 bytes per pixel
	virtual void extractYpCbCr420(const FrameBuffer* frame,void* yp,unsigned int ypStride,void* cb,unsigned int cbStride,void* cr,unsigned int crStride) =0; // Extracts a Y'CbCr image using 4:2:0 downsampling from the given video buffer; each plane must hold 1 byte per pixel
	};

}

#endif

/***********************************************************************
VideoDataFormatSelector - Helper class to set parts of a video data
format.
Copyright (c) 2025 Oliver Kreylos

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

#ifndef VIDEO_VIDEODATAFORMATSELECTOR_INCLUDED
#define VIDEO_VIDEODATAFORMATSELECTOR_INCLUDED

#include <Video/VideoDataFormat.h>

/* Forward declarations: */
namespace Misc {
class CommandLineParser;
}

namespace Video {

class VideoDataFormatSelector:public VideoDataFormat
	{
	/* Elements: */
	private:
	unsigned int setMask; // Mask of video data format components that have been set
	
	/* Constructors and destructors: */
	public:
	VideoDataFormatSelector(void) // Creates a video data format selector with no set components
		:setMask(0x0U)
		{
		}
	
	/* Methods: */
	void setPixelFormat(const char* newFourCC) // Selects the format's pixel format
		{
		/* Set the pixel format using the base class method and remember that a pixel format has been set: */
		VideoDataFormat::setPixelFormat(newFourCC);
		setMask|=0x1U;
		}
	void setSize(const Size& newSize) // Selects the format's frame size
		{
		/* Set the frame size and remember that a frame size has been set: */
		size=newSize;
		setMask|=0x2U;
		}
	void setFrameInterval(const Math::Rational& newFrameInterval) // Selects the format's frame interval
		{
		/* Set the frame interval and remember that a frame interval has been set: */
		frameInterval=newFrameInterval;
		setMask|=0x4;
		}
	bool hasPixelFormat(void) const // Returns true if a pixel format has been set
		{
		return (setMask&0x1U)!=0x0U;
		}
	bool hasSize(void) const // Returns true if a frame size has been set
		{
		return (setMask&0x2U)!=0x0U;
		}
	bool hasFrameInterval(void) const // Returns true if a frame interval has been set
		{
		return (setMask&0x4U)!=0x0U;
		}
	void addToParser(Misc::CommandLineParser& commandLineParser); // Adds options to set video data format components to the given command line parser
	};

}

#endif

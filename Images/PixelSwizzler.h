/***********************************************************************
PixelSwizzler - Helper class to convert pixels with up to four unsigned
integer channels comprising up to 32 bits total into canonical unsigned
8-bit RGB(A) representations.
Copyright (c) 2022 Oliver Kreylos

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

#ifndef IMAGES_PIXELSWIZZLER_INCLUDED
#define IMAGES_PIXELSWIZZLER_INCLUDED

#include <Misc/SizedTypes.h>

namespace Images {

class PixelSwizzler
	{
	/* Embedded classes: */
	public:
	typedef Misc::UInt32 InputPixel; // Type for input pixels with up to 32 bits
	typedef Misc::UInt8 OutputChannel; // Type for output channel values
	
	private:
	struct ChannelExtractor // Structure holding information on how to extract a single 8-bit channel from an input pixel
		{
		/* Elements: */
		public:
		InputPixel mask; // Bit mask to extract a channel's bits
		InputPixel factor; // Factor by which to multiply the channel's masked bits to promote them to 8-bit values
		unsigned int shift; // Amount of right shift to limit the promoted channel value to 8 bits
		
		/* Constructors and destructors: */
		ChannelExtractor(void) // Dummy constructor
			:mask(0x0U),factor(0),shift(0)
			{
			}
		
		/* Methods: */
		void init(InputPixel inputMask) // Initializes the channel extractor based on the given input mask
			{
			/* Ignore channels with empty input masks: */
			if(inputMask!=0x0U)
				{
				/* Store the channel's input mask: */
				mask=inputMask;
				
				/* Find the position of the channel's LSB and shift the channel's input mask: */
				for(shift=0;(inputMask&0x1U)==0x0U;++shift,inputMask>>=1)
					;
				
				/* Count the number of bits in the channel's input mask: */
				unsigned int numBits;
				for(numBits=0;inputMask!=0x0U;++numBits,inputMask>>=1)
					;
				
				/* Calculate a multiplication factor to promote the channel's raw values to 8 bits: */
				factor=1U;
				unsigned int outputNumBits=numBits;
				while(outputNumBits<8)
					{
					/* Append another copy of the raw channel value to the current result's bits: */
					factor=(factor<<numBits)+1;
					outputNumBits+=numBits;
					}
				
				/* Calculate a right-shift amount to reduce the replicated raw value to 8 bits: */
				shift+=outputNumBits-8;
				}
			}
		OutputChannel extract(InputPixel inputPixel) const // Returns the input pixel's channel value promoted to an 8-bit output value
			{
			return OutputChannel((Misc::UInt64(inputPixel&mask)*Misc::UInt64(factor))>>shift);
			}
		};
	
	/* Elements: */
	ChannelExtractor channelExtractors[4]; // Channel extractors for the up to four channels of an input pixel, in order red, green, blue, alpha
	
	/* Constructors and destructors: */
	public:
	PixelSwizzler(void) // Creates an identity swizzler for a standard RGB(A)8 pixel layout
		{
		/* Create default channel extractors: */
		for(int channel=0;channel<4;++channel)
			channelExtractors[channel].init(0xffU<<(channel*8));
		}
	PixelSwizzler(const InputPixel inputMasks[4]) // Constructs a swizzler from four input bit masks for the red, green, blue, and alpha channels, in that order
		{
		/* Convert the input masks into channel extractors: */
		for(int channel=0;channel<4;++channel)
			channelExtractors[channel].init(inputMasks[channel]);
		}
	
	/* Methods: */
	void swizzle(unsigned int numComponents,OutputChannel destChannels[],InputPixel inputPixel) const // Extracts the given number of the input pixel's RGBA components into the destination pixel
		{
		/* Extract the given number of channels into the destination pixel: */
		for(unsigned int channel=0;channel<numComponents;++channel)
			destChannels[channel]=channelExtractors[channel].extract(inputPixel);
		}
	};

}

#endif

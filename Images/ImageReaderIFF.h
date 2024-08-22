/***********************************************************************
ImageReaderIFF - Class to read images from files in IFF (Interchange
File Format) format.
Copyright (c) 2021-2022 Oliver Kreylos

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

#ifndef IMAGES_IMAGEREADERIFF_INCLUDED
#define IMAGES_IMAGEREADERIFF_INCLUDED

#include <Misc/SelfDestructArray.h>
#include <IO/IFFChunk.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <Images/ImageReader.h>

namespace Images {

class ImageReaderIFF:public ImageReader
	{
	/* Embedded classes: */
	private:
	typedef GLColor<GLubyte,3> ColorMapEntry; // Type for color map entries
	
	enum Masking // Enumerated type for image masking types
		{
		Opaque=0,Interleaved=1,TransparentColor=2,Lassoed=3
		};
	
	/* Elements: */
	private:
	IO::IFFChunkPtr form; // The IFF image file's root FORM chunk
	unsigned int numBitPlanes; // Number of bit planes in the image body chunk
	unsigned int masking; // The image's masking mode
	bool compress; // Flag whether the image is compressed
	unsigned int transparentColorIndex; // Index of the transparent color if masking==TransparentColor
	Misc::SelfDestructArray<ColorMapEntry> colorMap; // The image's color map
	IO::IFFChunkPtr body; // The next image body chunk in the root FORM chunk
	
	/* Private methods: */
	void readImageHeaders(void); // Reads an image specification and IFF-specific image layout from sub-chunks of the root FORM chunk
	void readScanline(GLubyte* scanline,unsigned int scanLineSize); // Reads a scanline from the current BODY chunk
	
	/* Constructors and destructors: */
	public:
	ImageReaderIFF(IO::File& sFile); // Creates an IFF image reader for the given file
	
	/* Methods from ImageReader: */
	virtual bool eof(void) const;
	virtual BaseImage readImage(void);
	};

}

#endif

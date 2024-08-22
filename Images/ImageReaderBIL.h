/***********************************************************************
ImageReaderBIL - Class to read images from files in BIL (Band
Interleaved by Line), BIP (Band Interleaved by Pixel), or BSQ (Band
Sequential) formats.
Copyright (c) 2018-2022 Oliver Kreylos

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

#ifndef IMAGES_IMAGEREADERBIL_INCLUDED
#define IMAGES_IMAGEREADERBIL_INCLUDED

#include <stddef.h>
#include <Misc/Endianness.h>
#include <IO/File.h>
#include <Images/Types.h>
#include <Images/ImageReader.h>

/* Forward declarations: */
namespace IO {
class Directory;
}

namespace Images {

class ImageReaderBIL:public ImageReader
	{
	/* Embedded classes: */
	public:
	struct Metadata // Structure to represent metadata commonly associated with BIL images
		{
		/* Elements: */
		public:
		bool haveMap; // Flag whether the image defines map coordinates
		double map[2]; // Map coordinates of center of upper-left pixel
		bool haveDim; // Flag whether the image file defines pixel dimensions
		double dim[2]; // Pixel dimension in map coordinates
		bool haveNoData; // Flag whether the image file defines an invalid pixel value
		double noData; // Pixel value indicating an invalid pixel
		};
	
	struct FileLayout // Structure describing the data layout of a BIL file, typically extracted from its associated header file
		{
		/* Embedded classes: */
		public:
		enum BandLayout
			{
			BIP,BIL,BSQ
			};
		
		/* Elements: */
		Size size; // Image width and height
		unsigned int numBands; // Number of bands
		unsigned int numBits; // Number of bits per band per pixel
		bool pixelSigned; // Flag if pixels are signed integers
		Misc::Endianness byteOrder; // File's byte order
		BandLayout bandLayout; // File's band layout
		size_t skipBytes; // Number of bytes to skip at beginning of image file
		size_t bandRowBytes; // Number of bytes per band per image row
		size_t totalRowBytes; // Number of bytes per image row
		size_t bandGapBytes; // Number of bytes between bands in a BSQ layout
		Metadata metadata; // Optional metadata extracted from the header file
		};
	
	/* Elements: */
	private:
	FileLayout layout; // Layout of the BIL image file
	bool done; // Flag whether the BIL image files single image has been read
	
	/* Private methods: */
	void setImageSpec(void); // Sets the image reader's image specification based on the image file's layout
	
	/* Constructors and destructors: */
	public:
	static FileLayout readHeaderFile(const char* imageFileName); // Reads the header file associated with the BIL image file of the given name and returns a file layout structure
	static FileLayout readHeaderFile(const IO::Directory& directory,const char* imageFileName); // Ditto, but the image file name is relative to the given directory
	ImageReaderBIL(const FileLayout& sLayout,IO::File& imageFile); // Creates a BIL image reader for the given file layout and open file
	ImageReaderBIL(const char* imageFileName); // Creates a BIL image reader for the image file of the given name, needed to access the associated header file
	ImageReaderBIL(const IO::Directory& directory,const char* imageFileName); // Ditto, but the image file name is relative to the given directory
	
	/* Methods from class ImageReader: */
	virtual bool eof(void) const;
	virtual BaseImage readImage(void);
	
	/* New methods: */
	const Metadata& getMetadata(void) const // Returns the optional metadata structure extracted from the image's header file
		{
		return layout.metadata;
		}
	};

}

#endif

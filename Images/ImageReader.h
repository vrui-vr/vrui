/***********************************************************************
ImageReader - Abstract base class to read images from files in a variety
of image file formats.
Copyright (c) 2012-2022 Oliver Kreylos

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

#ifndef IMAGES_IMAGEREADER_INCLUDED
#define IMAGES_IMAGEREADER_INCLUDED

#include <IO/File.h>
#include <GL/gl.h>
#include <Images/Types.h>
#include <Images/ImageFileFormats.h>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace Images {
class BaseImage;
}

namespace Images {

class ImageReader
	{
	/* Embedded classes: */
	public:
	enum ColorSpace // Enumerated type for image color spaces
		{
		Grayscale, // Image is grayscale
		RGB, // Image is in RGB color space
		InvalidColorSpace
		};
	
	enum ChannelValueType // Enumerated type for image channel data types
		{
		UnsignedInt, // Channel values are unsigned integers
		SignedInt, // Channel values are two's complement signed integers
		Float, // Channel values are IEEE floating-point numbers
		InvalidChannelValueType
		};
	
	struct ImageSpec // Specification for a sub-image inside the image file
		{
		/* Elements: */
		public:
		Rect rect; // Image's position and size inside the image file's canvas
		ColorSpace colorSpace; // Color space of the image
		bool hasAlpha; // Flag if the image has an alpha channel
		unsigned int numChannels; // Number of channels in image; typically 1 (grayscale), 2 (grayscale+alpha), 3 (RGB), or 4 (RGB+alpha)
		ChannelValueType valueType; // Data type for channel values
		unsigned int numFieldBits; // Number of bits for channel values, usually a multiple of 8 for byte-aligned values
		unsigned int numFieldBytes; // Number of bytes required to hold each channel value
		unsigned int numValueBits; // Number of used bits LSB-aligned inside each channel value field; <= numFieldBits
		};
	
	/* Elements: */
	protected:
	IO::FilePtr file; // Pointer to the image file
	Size canvasSize; // Size of the image canvas, i.e., the bounding box of all sub-images
	ImageSpec imageSpec; // Specification structure for the next image to be read from the file
	
	/* Protected methods: */
	void setFormatSpec(ColorSpace newColorSpace,bool newHasAlpha,unsigned int newNumChannels) // Sets the image specification's image format
		{
		imageSpec.colorSpace=newColorSpace;
		imageSpec.hasAlpha=newHasAlpha;
		imageSpec.numChannels=newNumChannels;
		}
	void setFormatSpec(ColorSpace newColorSpace,bool newHasAlpha) // Ditto, determines number of channels from color space
		{
		imageSpec.colorSpace=newColorSpace;
		imageSpec.hasAlpha=newHasAlpha;
		imageSpec.numChannels=imageSpec.colorSpace==Grayscale?1:3;
		if(imageSpec.hasAlpha)
			++imageSpec.numChannels;
		}
	void setValueSpec(ChannelValueType newValueType,unsigned int newNumFieldBits,unsigned int newNumValueBits) // Sets the image specification's value layout
		{
		imageSpec.valueType=newValueType;
		imageSpec.numFieldBits=newNumFieldBits;
		imageSpec.numFieldBytes=(imageSpec.numFieldBits+7)>>3;
		imageSpec.numValueBits=newNumValueBits;
		}
	void setValueSpec(ChannelValueType newValueType,unsigned int newNumValueBits) // Ditto, where field width is equal to value width
		{
		imageSpec.valueType=newValueType;
		imageSpec.numFieldBits=newNumValueBits;
		imageSpec.numFieldBytes=(imageSpec.numFieldBits+7)>>3;
		imageSpec.numValueBits=newNumValueBits;
		}
	GLenum getFormat(void) const; // Returns a BaseImage-compatible image format based on the current image specification
	GLenum getScalarType(void) const; // Returns a BaseImage-compatible scalar type based on the current image specification
	BaseImage createImage(void) const; // Returns a BaseImage object based on the current image specification
	
	/* Constructors and destructors: */
	public:
	static ImageReader* create(ImageFileFormat imageFileFormat,IO::File& imageFile); // Returns an image reader to read an image file of the given format via the given file abstraction
	static ImageReader* create(const char* imageFileName); // Returns an image reader to read the image file of the given name
	static ImageReader* create(const IO::Directory& directory,const char* imageFileName); // Returns an image reader to read the image file of the given name relative to the given directory
	ImageReader(IO::File& sFile); // Creates an image reader for the given file
	virtual ~ImageReader(void);
	
	/* Methods: */
	const Size& getCanvasSize(void) const // Returns the size of the image canvas
		{
		return canvasSize;
		}
	unsigned int getCanvasSize(int dimension) const // Ditto, for single dimension
		{
		return canvasSize[dimension];
		}
	virtual bool eof(void) const =0; // Returns true if there are no more images to read in the image file
	
	/* The following methods must not be called if eof() returns true: */
	const ImageSpec& getImageSpec(void) const // Returns the specification of the next image to be returned by the image reader methods
		{
		return imageSpec;
		}
	virtual BaseImage readImage(void) =0; // Reads the next image from the image file
	};

}

#endif

/***********************************************************************
ImageWriter - Abstract base class to write images to files in a variety
of image file formats.
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

#ifndef IMAGES_IMAGEWRITER_INCLUDED
#define IMAGES_IMAGEWRITER_INCLUDED

#include <IO/File.h>
#include <Images/ImageFileFormats.h>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace Images {
class BaseImage;
}

namespace Images {

class ImageWriter
	{
	/* Elements: */
	protected:
	IO::FilePtr file; // Pointer to the image file
	
	/* Constructors and destructors: */
	public:
	static ImageWriter* create(ImageFileFormat imageFileFormat,IO::File& imageFile); // Returns an image writer to write an image file of the given format via the given file abstraction
	static ImageWriter* create(const char* imageFileName); // Returns an image writer to write the image file of the given name
	static ImageWriter* create(const IO::Directory& directory,const char* imageFileName); // Returns an image writer to write the image file of the given name relative to the given directory
	ImageWriter(IO::File& sFile); // Creates an image writer for the given file
	virtual ~ImageWriter(void);
	
	/* Methods: */
	virtual void writeImage(const BaseImage& image) =0; // Writes the given image to the image file
	};

}

#endif

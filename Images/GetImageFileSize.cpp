/***********************************************************************
GetImageFileSize - Functions to extract the image size from a variety of
file formats reading the minimal required amount of data.
Copyright (c) 2007-2022 Oliver Kreylos

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

#include <Images/GetImageFileSize.h>

#include <Misc/SelfDestructPointer.h>
#include <Images/ImageReader.h>

namespace Images {

/*****************************************************************************
Function to extract image sizes from image files in several supported formats:
*****************************************************************************/

Size getImageFileSize(const char* imageFileName)
	{
	/* Create a reader for the image file: */
	Misc::SelfDestructPointer<ImageReader> reader(ImageReader::create(imageFileName));
	
	/* Return the size of the first image in the image file: */
	return reader->getImageSpec().rect.size;
	}

}

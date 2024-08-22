/***********************************************************************
ReadBILImage - Functions to read RGB images from image files in BIL
(Band Interleaved by Line), BIP (Band Interleaved by Pixel), or BSQ
(Band Sequential) formats over an IO::File abstraction.
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

#include <Images/ReadBILImage.h>

#include <Images/BaseImage.h>

namespace Images {

BaseImage readGenericBILImage(IO::File& file,const BILFileLayout& fileLayout)
	{
	/* Create a BIL image reader: */
	ImageReaderBIL reader(fileLayout,file);
	
	/* Read the image contained in the file: */
	return reader.readImage();
	}

BaseImage readGenericBILImage(const char* imageFileName,BILMetadata* metadata)
	{
	/* Create a BIL image reader: */
	ImageReaderBIL reader(imageFileName);
	
	/* Fill in the metadata structure if given: */
	if(metadata!=0)
		*metadata=reader.getMetadata();
	
	/* Read the image contained in the file: */
	return reader.readImage();
	}

BaseImage readGenericBILImage(const IO::Directory& directory,const char* imageFileName,BILMetadata* metadata)
	{
	/* Create a BIL image reader: */
	ImageReaderBIL reader(directory,imageFileName);
	
	/* Fill in the metadata structure if given: */
	if(metadata!=0)
		*metadata=reader.getMetadata();
	
	/* Read the image contained in the file: */
	return reader.readImage();
	}

}

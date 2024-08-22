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

#ifndef IMAGES_READBILIMAGE_INCLUDED
#define IMAGES_READBILIMAGE_INCLUDED

#include <Images/ImageReaderBIL.h>

/* Forward declarations: */
namespace IO {
class File;
class Directory;
}
namespace Images {
class BaseImage;
}

namespace Images {

typedef ImageReaderBIL::Metadata BILMetadata; // Structure to represent metadata commonly associated with BIL images

typedef ImageReaderBIL::FileLayout BILFileLayout; // Structure describing the data layout of a BIL file

BaseImage readGenericBILImage(IO::File& file,const BILFileLayout& fileLayout); // Reads a generic image in BIL/BIP/BSQ format from the given opened file and the provided file layout structure
BaseImage readGenericBILImage(const char* imageFileName,BILMetadata* metadata =0); // Reads a generic image in BIL/BIP/BSQ format from the file of the given name; fills in metadata structure if provided
BaseImage readGenericBILImage(const IO::Directory& directory,const char* imageFileName,BILMetadata* metadata =0); // Reads a generic image in BIL/BIP/BSQ format from the file of the given name inside the given directory; fills in metadata structure if provided

}

#endif

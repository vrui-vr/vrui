/***********************************************************************
ImageReaderTIFF - Class to read images from files in TIFF format.
Copyright (c) 2011-2022 Oliver Kreylos

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

#ifndef IMAGES_IMAGEREADERTIFF_INCLUDED
#define IMAGES_IMAGEREADERTIFF_INCLUDED

#include <IO/SeekableFile.h>
#include <tiffio.h>
#include <Images/GeoTIFFMetadata.h>
#include <Images/ImageReader.h>

namespace Images {

class ImageReaderTIFF:public ImageReader
	{
	/* Embedded classes: */
	private:
	enum TIFFColorSpace // Enumerated type for color spaces supported by TIFF
		{
		TIFF_WhiteIsZero=0,TIFF_BlackIsZero,TIFF_RGB,
		TIFF_TransparencyMask=4,TIFF_CMYK,TIFF_YCbCr,
		TIFF_CIE_Lab=8,TIFF_ICC_Lab,TIFF_ITU_Lab,
		TIFF_ColorSpaceInvalid
		};
	
	/* Elements: */
	IO::SeekableFilePtr seekableFile; // Seekable wrapper around the image file
	TIFF* tiff; // Pointer to the TIFF library object used to read the image file
	bool indexed; // Flag if pixel values are indices into a color map
	TIFFColorSpace colorSpace; // Color space to interpret pixel values
	bool planar; // Flag whether sample data is stored separately by sample, or interleaved by pixel
	bool tiled; // Flag whether image data is organized in tiles instead of in strips
	Size tileSize; // Width and height of an image tile if image data is organized in tiles
	unsigned int rowsPerStrip; // Number of rows per image strip if image data is organized in strips
	GeoTIFFMetadata metadata; // GeoTIFF metadata extracted from the current TIFF directory
	bool done; // Flag set to true after all images in the image file have been read
	
	/* Private methods: */
	
	/* Error handling callbacks for the TIFF library: */
	static void tiffErrorFunction(const char* module,const char* fmt,va_list ap);
	static void tiffWarningFunction(const char* module,const char* fmt,va_list ap);
	
	/* Methods to let the TIFF library access image files through an IO::File abstraction: */
	static tsize_t tiffReadFunction(thandle_t handle,tdata_t buffer,tsize_t size);
	static tsize_t tiffWriteFunction(thandle_t handle,tdata_t buffer,tsize_t size);
	static toff_t tiffSeekFunction(thandle_t handle,toff_t offset,int whence);
	static int tiffCloseFunction(thandle_t handle);
	static toff_t tiffSizeFunction(thandle_t handle);
	static int tiffMapFileFunction(thandle_t handle,tdata_t* buffer,toff_t* size);
	static void tiffUnmapFileFunction(thandle_t handle,tdata_t buffer,toff_t size);
	
	void readDirectory(void); // Reads the next TIFF directory and extracts the next image's specification
	void readTiles(uint8_t* image,ptrdiff_t rowStride); // Reads a tiled image
	void readStrips(uint8_t* image,ptrdiff_t rowStride); // Reads a stripped image
	
	/* Constructors and destructors: */
	public:
	ImageReaderTIFF(IO::File& sFile); // Creates a TIFF image reader for the given file
	virtual ~ImageReaderTIFF(void); // Destroys the reader
	
	/* Methods from class ImageReader: */
	virtual bool eof(void) const;
	virtual BaseImage readImage(void);
	
	/* New methods: */
	const GeoTIFFMetadata& getMetadata(void) const // Returns optional GeoTIFF metadata extracted from the current TIFF file directory
		{
		return metadata;
		}
	};

}

#endif

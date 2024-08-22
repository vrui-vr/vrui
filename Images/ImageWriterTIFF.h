/***********************************************************************
ImageWriterTIFF - Class to write images to files in TIFF format.
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

#ifndef IMAGES_IMAGEWRITERTIFF_INCLUDED
#define IMAGES_IMAGEWRITERTIFF_INCLUDED

#include <tiffio.h>
#include <Images/ImageWriter.h>
#include <Images/GeoTIFFMetadata.h>

namespace Images {

class ImageWriterTIFF:public ImageWriter
	{
	/* Embedded classes: */
	public:
	enum CompressionMode // Enumerated type for TIFF compression modes
		{
		Uncompressed=0,
		LZW,JPEG
		};
	
	/* Elements: */
	private:
	TIFF* tiff; // Pointer to the TIFF library object used to write the image file
	CompressionMode compressionMode; // Compression mode for the next image
	int jpegQuality; // Quality for JPEG compression in [0, 100]
	GeoTIFFMetadata geoTIFFMetadata; // GeoTIFF metadata to write to the next image
	
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
	
	/* Constructors and destructors: */
	public:
	ImageWriterTIFF(IO::File& sFile); // Creates a TIFF image writer for the given file
	virtual ~ImageWriterTIFF(void); // Destroys the writer
	
	/* Methods from class ImageWriter: */
	virtual void writeImage(const BaseImage& image);
	
	/* New methods: */
	CompressionMode getCompressionMode(void) const // Returns the compression mode for the next image
		{
		return compressionMode;
		}
	int getJPEGQuality(void) const // Returns the JPEG compression quality for the next image
		{
		return jpegQuality;
		}
	const GeoTIFFMetadata& getGeoTIFFMetadata(void) const // Returns the GeoTIFF metadata structure written to the next image
		{
		return geoTIFFMetadata;
		}
	GeoTIFFMetadata& getGeoTIFFMetadata(void) // Ditto
		{
		return geoTIFFMetadata;
		}
	void setCompressionMode(CompressionMode newCompressionMode); // Sets the compression mode for the next image
	void setJPEGQuality(int newJPEGQuality); // Sets the JPEG compression quality for the next image in [0, 100]
	};

}

#endif

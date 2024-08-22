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

#include <Images/ImageWriterTIFF.h>

#include <stdarg.h>
#include <string.h>
#include <Misc/Utility.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <IO/SeekableFile.h>
#include <Images/BaseImage.h>

namespace Images {

/********************************
Methods of class ImageWriterTIFF:
********************************/

void ImageWriterTIFF::tiffErrorFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Throw an exception with the error message: */
	char msg[1024];
	vsnprintf(msg,sizeof(msg),fmt,ap);
	throw Misc::makeStdErr("Images::ImageWriterTIF",msg);
	}

void ImageWriterTIFF::tiffWarningFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Print a warning message to the console: */
	char msg[1024];
	vsnprintf(msg,sizeof(msg),fmt,ap);
	Misc::consoleWarning(msg);
	}

tsize_t ImageWriterTIFF::tiffReadFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	/* Ignore silently */
	return size;
	}

tsize_t ImageWriterTIFF::tiffWriteFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	IO::SeekableFile* file=static_cast<IO::SeekableFile*>(handle);
	
	/* Libtiff expects to always write the amount of data it wants: */
	file->writeRaw(buffer,size);
	return size;
	}

toff_t ImageWriterTIFF::tiffSeekFunction(thandle_t handle,toff_t offset,int whence)
	{
	IO::SeekableFile* file=static_cast<IO::SeekableFile*>(handle);
	
	/* Seek to the requested position: */
	file->flush();
	switch(whence)
		{
		case SEEK_SET:
			file->setWritePosAbs(offset);
			break;

		case SEEK_CUR:
			file->setWritePosRel(offset);
			break;

		case SEEK_END:
			file->setWritePosAbs(file->getSize()-offset);
			break;
		}
	
	return file->getWritePos();
	}

int ImageWriterTIFF::tiffCloseFunction(thandle_t handle)
	{
	IO::SeekableFile* file=static_cast<IO::SeekableFile*>(handle);
	
	file->flush();
	return 0;
	}

toff_t ImageWriterTIFF::tiffSizeFunction(thandle_t handle)
	{
	IO::SeekableFile* file=static_cast<IO::SeekableFile*>(handle);
	
	return file->getSize();
	}

int ImageWriterTIFF::tiffMapFileFunction(thandle_t handle,tdata_t* buffer,toff_t* size)
	{
	/* Ignore silently */
	return -1;
	}

void ImageWriterTIFF::tiffUnmapFileFunction(thandle_t handle,tdata_t buffer,toff_t size)
	{
	/* Ignore silently */
	}

ImageWriterTIFF::ImageWriterTIFF(IO::File& sFile)
	:ImageWriter(sFile),
	 tiff(0),
	 compressionMode(Uncompressed),jpegQuality(75)
	{
	/* Check if the sink file is seekable: */
	if(dynamic_cast<IO::SeekableFile*>(file.getPointer())==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to write TIFF images to non-seekable files");
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	TIFFSetWarningHandler(tiffWarningFunction);
	
	/* Pretend to open the TIFF file and register the hook functions: */
	tiff=TIFFClientOpen("Foo.tif","wm",file.getPointer(),tiffReadFunction,tiffWriteFunction,tiffSeekFunction,tiffCloseFunction,tiffSizeFunction,tiffMapFileFunction,tiffUnmapFileFunction);
	if(tiff==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to initialize TIFF library");
	}

ImageWriterTIFF::~ImageWriterTIFF(void)
	{
	/* Close the TIFF file: */
	TIFFClose(tiff);
	}

void ImageWriterTIFF::writeImage(const BaseImage& image)
	{
	/* Retrieve image parameters: */
	uint32_t width=image.getWidth();
	uint32_t height=image.getHeight();
	unsigned int numChannels=image.getNumChannels();
	unsigned int channelSize=image.getChannelSize();
	GLenum format=image.getFormat();
	GLenum scalarType=image.getScalarType();
	
	/* Set the image specification: */
	bool isOkay=true;
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_IMAGEWIDTH,width)!=0;
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_IMAGELENGTH,height)!=0;
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,numChannels)!=0;
	int sampleFormat=0;
	switch(scalarType)
		{
		case GL_BYTE:
		case GL_SHORT:
		case GL_INT:
			sampleFormat=SAMPLEFORMAT_INT;
			break;
		
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT:
		case GL_UNSIGNED_INT:
			sampleFormat=SAMPLEFORMAT_UINT;
			break;
		
		case GL_FLOAT:
		case GL_DOUBLE:
			sampleFormat=SAMPLEFORMAT_IEEEFP;
		}
	int bitsPerSample=0;
	unsigned int requiredChannelSize=0;
	switch(scalarType)
		{
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			bitsPerSample=8;
			requiredChannelSize=1;
			break;
		
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			bitsPerSample=16;
			requiredChannelSize=2;
			break;
		
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
			bitsPerSample=32;
			requiredChannelSize=4;
			break;
		
		case GL_DOUBLE:
			bitsPerSample=64;
			requiredChannelSize=8;
			break;
		}
	if(channelSize!=requiredChannelSize)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Incompatible image format");
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_SAMPLEFORMAT,sampleFormat)!=0;
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_BITSPERSAMPLE,bitsPerSample)!=0;
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT)!=0;
	static const int compressionModes[3]=
		{
		COMPRESSION_NONE,COMPRESSION_LZW,COMPRESSION_JPEG
		};
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_COMPRESSION,compressionModes[compressionMode])!=0;
	if(compressionMode==JPEG)
		isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_JPEGQUALITY,jpegQuality)!=0;
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG)!=0;
	int photometric=0;
	switch(format)
		{
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
			photometric=PHOTOMETRIC_MINISBLACK;
			break;
		
		case GL_RGB:
		case GL_RGBA:
			photometric=PHOTOMETRIC_RGB;
			break;
		}
	if(photometric!=0)
		isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_PHOTOMETRIC,photometric)!=0;
	if(format==GL_LUMINANCE_ALPHA||format==GL_RGBA)
		{
		uint16_t extraSamples[1];
		extraSamples[0]=EXTRASAMPLE_UNASSALPHA;
		isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_EXTRASAMPLES,1U,extraSamples)!=0;
		}
	ptrdiff_t rowStride=image.getRowStride();
	isOkay=isOkay&&TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,TIFFDefaultStripSize(tiff,rowStride))!=0;
	if(!isOkay)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to write image specification");
	isOkay=isOkay&&writeGeoTIFFMetadata(tiff,geoTIFFMetadata);
	if(!isOkay)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to write GeoTIFF metadata");
	
	/* Write the image top to bottom: */
	Misc::SelfDestructArray<unsigned char*> rowBuffer(Misc::max(size_t(rowStride),size_t(TIFFScanlineSize(tiff))));
	const unsigned char* rowPtr=static_cast<const unsigned char*>(image.getPixels())+(height-1U)*rowStride;
	for(uint32_t row=0;row<height&&isOkay;++row,rowPtr-=rowStride)
		{
		memcpy(rowBuffer,rowPtr,rowStride);
		isOkay=TIFFWriteScanline(tiff,rowBuffer,row,0)>=0;
		}
	
	if(!isOkay)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unable to write image data");
	}

void ImageWriterTIFF::setCompressionMode(ImageWriterTIFF::CompressionMode newCompressionMode)
	{
	compressionMode=newCompressionMode;
	}

void ImageWriterTIFF::setJPEGQuality(int newJPEGQuality)
	{
	/* Clamp and set the JPEG compression quality: */
	jpegQuality=Misc::clamp(newJPEGQuality,0,100);
	}

}

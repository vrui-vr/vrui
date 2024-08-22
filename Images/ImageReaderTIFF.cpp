/***********************************************************************
ImageReaderTIFF - Class to read images from files in TIFF format.
Copyright (c) 2011-2024 Oliver Kreylos

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

#include <Images/ImageReaderTIFF.h>

#include <stdint.h>
#include <string.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <IO/SeekableFilter.h>
#include <Images/BaseImage.h>

namespace Images {

/********************************
Methods of class ImageReaderTIFF:
********************************/

void ImageReaderTIFF::tiffErrorFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Throw an exception with the error message: */
	throw Misc::makeStdErr("Images::ImageReaderTIFF",fmt,ap);
	}

void ImageReaderTIFF::tiffWarningFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Print a warning message to the console: */
	Misc::sourcedConsoleWarning("Images::ImageReaderTIFF",fmt,ap);
	}

tsize_t ImageReaderTIFF::tiffReadFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	IO::SeekableFile* file=static_cast<IO::SeekableFile*>(handle);
	
	/* Libtiff expects to always get the amount of data it wants: */
	file->readRaw(buffer,size);
	return size;
	}

tsize_t ImageReaderTIFF::tiffWriteFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	/* Ignore silently */
	return size;
	}

toff_t ImageReaderTIFF::tiffSeekFunction(thandle_t handle,toff_t offset,int whence)
	{
	IO::SeekableFile* file=static_cast<IO::SeekableFile*>(handle);
	
	/* Seek to the requested position: */
	switch(whence)
		{
		case SEEK_SET:
			file->setReadPosAbs(offset);
			break;

		case SEEK_CUR:
			file->setReadPosRel(offset);
			break;

		case SEEK_END:
			file->setReadPosAbs(file->getSize()-offset);
			break;
		}
	
	return file->getReadPos();
	}

int ImageReaderTIFF::tiffCloseFunction(thandle_t handle)
	{
	/* Ignore silently */
	return 0;
	}

toff_t ImageReaderTIFF::tiffSizeFunction(thandle_t handle)
	{
	IO::SeekableFile* file=static_cast<IO::SeekableFile*>(handle);
	
	return file->getSize();
	}

int ImageReaderTIFF::tiffMapFileFunction(thandle_t handle,tdata_t* buffer,toff_t* size)
	{
	/* Ignore silently */
	return -1;
	}

void ImageReaderTIFF::tiffUnmapFileFunction(thandle_t handle,tdata_t buffer,toff_t size)
	{
	/* Ignore silently */
	}

void ImageReaderTIFF::readDirectory(void)
	{
	/* Get the image's size: */
	uint32_t width,height;
	TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&width);
	TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&height);
	imageSpec.rect=Rect(Size(width,height));
	
	/* Get the image's format: */
	uint16_t samplesPerPixel,sampleFormat,bitsPerSample;
	TIFFGetField(tiff,TIFFTAG_SAMPLESPERPIXEL,&samplesPerPixel);
	if(samplesPerPixel<=2)
		setFormatSpec(Grayscale,samplesPerPixel==2);
	else if(samplesPerPixel<=4)
		setFormatSpec(RGB,samplesPerPixel==4);
	else
		{
		imageSpec.colorSpace=InvalidColorSpace;
		imageSpec.numChannels=samplesPerPixel;
		}
	TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLEFORMAT,&sampleFormat);
	TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&bitsPerSample);
	switch(sampleFormat)
		{
		case SAMPLEFORMAT_UINT:
			imageSpec.valueType=UnsignedInt;
			break;
		
		case SAMPLEFORMAT_INT:
			imageSpec.valueType=SignedInt;
			break;
		
		case SAMPLEFORMAT_IEEEFP:
			imageSpec.valueType=Float;
			break;
		
		default:
			imageSpec.valueType=InvalidChannelValueType;
		}
	setValueSpec(imageSpec.valueType,bitsPerSample);
	
	/* Check if pixel values are color map indices: */
	int16_t indexedTagValue;
	bool haveIndexedTag=TIFFGetField(tiff,TIFFTAG_INDEXED,&indexedTagValue)!=0;
	indexed=haveIndexedTag&&indexedTagValue!=0;
	int16_t photometricTagValue;
	bool havePhotometricTag=TIFFGetField(tiff,TIFFTAG_PHOTOMETRIC,&photometricTagValue)!=0;
	colorSpace=TIFF_ColorSpaceInvalid;
	if(havePhotometricTag&&photometricTagValue==PHOTOMETRIC_PALETTE)
		{
		if(!indexed)
			colorSpace=TIFF_RGB;
		indexed=true;
		}
	else if(havePhotometricTag&&photometricTagValue<=10&&photometricTagValue!=7)
		colorSpace=TIFFColorSpace(photometricTagValue);
	
	/* Query whether samples are layed out in planes or interleaved per pixel: */
	uint16_t planarConfig;
	TIFFGetFieldDefaulted(tiff,TIFFTAG_PLANARCONFIG,&planarConfig);
	planar=planarConfig==PLANARCONFIG_SEPARATE;
	
	/* Query whether the image is organized in strips or tiles: */
	tiled=TIFFIsTiled(tiff);
	if(tiled)
		{
		/* Get the image's tile layout: */
		uint32_t tw,th;
		TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&tw);
		TIFFGetField(tiff,TIFFTAG_TILELENGTH,&th);
		tileSize=Size(tw,th);
		}
	else
		{
		/* Get the image's strip layout: */
		uint32_t rps;
		TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rps);
		rowsPerStrip=rps;
		}
	
	/* Read optional GeoTIFF metadata: */
	readGeoTIFFMetadata(tiff,metadata);
	}

namespace {

/****************
Helper functions:
****************/

template <class ScalarParam>
inline
void
copyRowChannel( // Helper function to read a single channel for an image row
	uint32_t width,
	uint16_t numChannels,
	uint16_t channel,
	ScalarParam* rowPtr,
	ScalarParam* stripPtr)
	{
	rowPtr+=channel;
	for(uint32_t x=0;x<width;++x,rowPtr+=numChannels,++stripPtr)
		*rowPtr=*stripPtr;
	}

}

void ImageReaderTIFF::readTiles(uint8_t* image,ptrdiff_t rowStride)
	{
	Size& size=imageSpec.rect.size;
	
	/* Create a buffer to hold a tile of image data: */
	tmsize_t tileFileSize=TIFFTileSize(tiff);
	Misc::SelfDestructArray<uint8_t> tileBuffer(tileFileSize);
	tmsize_t tileRowStride=TIFFTileRowSize(tiff);
	tmsize_t pixelSize=tmsize_t(imageSpec.numChannels*imageSpec.numFieldBytes);
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Read image data by channels: */
		uint32_t tileIndex=0;
		for(unsigned int channel=0;channel<imageSpec.numChannels;++channel)
			{
			/* Read channel data by tiles: */
			for(unsigned int ty=0;ty<size[1];ty+=tileSize[1])
				{
				/* Determine actual tile height in this tile row: */
				unsigned int th=Misc::min(size[1]-ty,tileSize[1]);
				
				/* Read all tiles in this tile row: */
				for(unsigned int tx=0;tx<size[0];tx+=tileSize[0],++tileIndex)
					{
					/* Read the next tile: */
					TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileFileSize);
					
					/* Determine actual tile width in this tile column: */
					unsigned int tw=Misc::min(size[0]-tx,tileSize[0]);
					
					/* Copy tile data by row: */
					uint8_t* rowPtr=image+(size[1]-1-ty)*rowStride+tx*pixelSize;
					uint8_t* tilePtr=tileBuffer;
					switch(imageSpec.numFieldBytes)
						{
						case 1:
							for(unsigned int y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,imageSpec.numChannels,channel,rowPtr,tilePtr);
							break;
						
						case 2:
							for(unsigned int y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,imageSpec.numChannels,channel,reinterpret_cast<uint16_t*>(rowPtr),reinterpret_cast<uint16_t*>(tilePtr));
							break;
						
						case 4:
							for(unsigned int y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,imageSpec.numChannels,channel,reinterpret_cast<uint32_t*>(rowPtr),reinterpret_cast<uint32_t*>(tilePtr));
							break;
						}
					}
				}
			}
		}
	else
		{
		/* Read image data by tiles: */
		uint32_t tileIndex=0;
		for(unsigned int ty=0;ty<size[1];ty+=tileSize[1])
			{
			/* Determine actual tile height in this tile row: */
			unsigned int th=Misc::min(size[1]-ty,tileSize[1]);
			
			/* Read all tiles in this tile row: */
			for(unsigned int tx=0;tx<size[0];tx+=tileSize[0],++tileIndex)
				{
				/* Read the next tile: */
				TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileFileSize);
				
				/* Determine actual tile width in this tile column: */
				unsigned int tw=Misc::min(size[0]-tx,tileSize[0]);
				tmsize_t ts=tw*pixelSize;
				
				/* Copy tile data by row: */
				uint8_t* rowPtr=image+(size[1]-1-ty)*rowStride+tx*pixelSize;
				uint8_t* tilePtr=tileBuffer;
				for(unsigned int y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
					{
					/* Copy the tile row: */
					memcpy(rowPtr,tilePtr,ts);
					}
				}
			}
		}
	}

void ImageReaderTIFF::readStrips(uint8_t* image,ptrdiff_t rowStride)
	{
	Size& size=imageSpec.rect.size;
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Create a buffer to hold a strip of image data: */
		Misc::SelfDestructArray<uint8_t> stripBuffer(TIFFStripSize(tiff));
		
		/* Read image data by channels: */
		for(unsigned int channel=0;channel<imageSpec.numChannels;++channel)
			{
			/* Read channel data by rows: */
			uint8_t* rowPtr=image+(size[1]-1)*rowStride;
			unsigned int rowStart=0;
			for(unsigned int strip=0;rowStart<size[1];++strip)
				{
				/* Read the next strip of image data: */
				uint8_t* stripPtr=stripBuffer;
				TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
				
				/* Copy image data from the strip buffer into the result image: */
				unsigned int rowEnd=Misc::min(size[1],rowStart+rowsPerStrip);
				switch(imageSpec.numFieldBytes)
					{
					case 1:
						for(unsigned int row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(size[0],imageSpec.numChannels,channel,rowPtr,stripPtr);
						break;
					
					case 2:
						for(unsigned int row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(size[0],imageSpec.numChannels,channel,reinterpret_cast<uint16_t*>(rowPtr),reinterpret_cast<uint16_t*>(stripPtr));
						break;
					
					case 4:
						for(unsigned int row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(size[0],imageSpec.numChannels,channel,reinterpret_cast<uint32_t*>(rowPtr),reinterpret_cast<uint32_t*>(stripPtr));
						break;
					}
				
				/* Prepare for the next strip: */
				rowStart=rowEnd;
				}
			}
		}
	else
		{
		/* Read image data by rows directly into the result image: */
		unsigned int rowEnd=size[1];
		for(unsigned int strip=0;rowEnd>0;++strip)
			{
			/* Read the next strip of image data into the appropriate region of the result image: */
			unsigned int rowStart=rowEnd>=rowsPerStrip?rowEnd-rowsPerStrip:0;
			uint8_t* stripPtr=image+rowStart*rowStride;
			TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
			
			/* Flip rows in the just-read image region in-place: */
			unsigned int row0=rowStart;
			unsigned int row1=rowEnd-1;
			while(row0<row1)
				{
				uint8_t* row0Ptr=image+row0*rowStride;
				uint8_t* row1Ptr=image+row1*rowStride;
				for(size_t i=size_t(rowStride);i>0;--i,++row0Ptr,++row1Ptr)
					{
					uint8_t t=*row0Ptr;
					*row0Ptr=*row1Ptr;
					*row1Ptr=t;
					}
				++row0;
				--row1;
				}
			
			/* Prepare for the next strip: */
			rowEnd=rowStart;
			}
		}
	}

ImageReaderTIFF::ImageReaderTIFF(IO::File& sFile)
	:ImageReader(sFile),
	 tiff(0),
	 indexed(false),colorSpace(TIFF_ColorSpaceInvalid),planar(false),
	 tiled(false),tileSize(0,0),rowsPerStrip(0),
	 done(false)
	{
	/* Check if the source file is seekable: */
	seekableFile=IO::SeekableFilePtr(file);
	if(seekableFile==0)
		{
		/* Create a seekable filter for the source file: */
		seekableFile=new IO::SeekableFilter(file);
		}
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	TIFFSetWarningHandler(tiffWarningFunction);
	
	/* Pretend to open the TIFF file and register the hook functions: */
	tiff=TIFFClientOpen("Foo.tif","rm",seekableFile.getPointer(),tiffReadFunction,tiffWriteFunction,tiffSeekFunction,tiffCloseFunction,tiffSizeFunction,tiffMapFileFunction,tiffUnmapFileFunction);
	if(tiff==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot initialize TIFF library");
	
	/* Read the first directory in the TIFF file: */
	readDirectory();
	}

ImageReaderTIFF::~ImageReaderTIFF(void)
	{
	/* Close the TIFF file: */
	TIFFClose(tiff);
	}

bool ImageReaderTIFF::eof(void) const
	{
	return done;
	}

BaseImage ImageReaderTIFF::readImage(void)
	{
	/* Create the result image: */
	BaseImage result=createImage();
	uint8_t* image=static_cast<uint8_t*>(result.replacePixels());
	ptrdiff_t rowStride=result.getRowStride();
	
	/* Check if the TIFF image is tiled: */
	if(tiled)
		{
		/* Read the image in tiles: */
		readTiles(image,rowStride);
		}
	else
		{
		/* Read the image in strips: */
		readStrips(image,rowStride);
		}
	
	/* Read the next TIFF directory: */
	if(TIFFReadDirectory(tiff)==1)
		{
		/* Read the next directory: */
		readDirectory();
		}
	else
		{
		/* No more directories and images in the file: */
		done=true;
		}
	
	return result;
	}

}

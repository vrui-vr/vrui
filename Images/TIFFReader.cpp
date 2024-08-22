/***********************************************************************
TIFFReader - Helper class for low-level access to image files in TIFF
formats over an IO::SeekableFile abstraction.
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

#include <Images/TIFFReader.h>

#if IMAGES_CONFIG_HAVE_TIFF

#include <stdlib.h>
#include <string.h>
#include <Misc/Utility.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <IO/File.h>
#include <IO/SeekableFilter.h>

namespace Images {

/***************************
Methods of class TIFFReader:
***************************/

void TIFFReader::tiffErrorFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Throw an exception with the error message: */
	throw Misc::makeStdErr("Images::ImageReaderTIFF",fmt,ap);
	}

void TIFFReader::tiffWarningFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Print a warning message to the console: */
	Misc::sourcedConsoleWarning("Images::ImageReaderTIFF",fmt,ap);
	}

tsize_t TIFFReader::tiffReadFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	/* Libtiff expects to always get the amount of data it wants: */
	source->readRaw(buffer,size);
	return size;
	}

tsize_t TIFFReader::tiffWriteFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	/* Ignore silently */
	return size;
	}

toff_t TIFFReader::tiffSeekFunction(thandle_t handle,toff_t offset,int whence)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	/* Seek to the requested position: */
	switch(whence)
		{
		case SEEK_SET:
			source->setReadPosAbs(offset);
			break;

		case SEEK_CUR:
			source->setReadPosRel(offset);
			break;

		case SEEK_END:
			source->setReadPosAbs(source->getSize()-offset);
			break;
		}
	
	return source->getReadPos();
	}

int TIFFReader::tiffCloseFunction(thandle_t handle)
	{
	/* Ignore silently */
	return 0;
	}

toff_t TIFFReader::tiffSizeFunction(thandle_t handle)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	return source->getSize();
	}

int TIFFReader::tiffMapFileFunction(thandle_t handle,tdata_t* buffer,toff_t* size)
	{
	/* Ignore silently */
	return -1;
	}

void TIFFReader::tiffUnmapFileFunction(thandle_t handle,tdata_t buffer,toff_t size)
	{
	/* Ignore silently */
	}

TIFFReader::TIFFReader(IO::File& source,unsigned int imageIndex)
	:tiff(0),
	 rowsPerStrip(0),tileWidth(0),tileHeight(0)
	{
	/* Check if the source file is seekable: */
	seekableSource=IO::SeekableFilePtr(&source);
	if(seekableSource==0)
		{
		/* Create a seekable filter for the source file: */
		seekableSource=new IO::SeekableFilter(&source);
		}
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	TIFFSetWarningHandler(tiffWarningFunction);
	
	/* Pretend to open the TIFF file and register the hook functions: */
	tiff=TIFFClientOpen("Foo.tif","rm",seekableSource.getPointer(),tiffReadFunction,tiffWriteFunction,tiffSeekFunction,tiffCloseFunction,tiffSizeFunction,tiffMapFileFunction,tiffUnmapFileFunction);
	if(tiff==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot initialize TIFF library");
	
	/* Select the requested image: */
	if(imageIndex!=0)
		{
		/* Select directory; errors will be handled by the TIFF error handler: */
		TIFFSetDirectory(tiff,uint16_t(imageIndex));
		}
	
	/* Get the image size and format: */
	TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&width);
	TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&height);
	TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&numBits);
	TIFFGetField(tiff,TIFFTAG_SAMPLESPERPIXEL,&numSamples);
	TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLEFORMAT,&sampleFormat);
	
	/* Check if pixel values are color map indices: */
	int16_t indexedTagValue;
	bool haveIndexedTag=TIFFGetField(tiff,TIFFTAG_INDEXED,&indexedTagValue)!=0;
	indexed=haveIndexedTag&&indexedTagValue!=0;
	int16_t photometricTagValue;
	bool havePhotometricTag=TIFFGetField(tiff,TIFFTAG_PHOTOMETRIC,&photometricTagValue)!=0;
	colorSpace=Invalid;
	if(havePhotometricTag&&photometricTagValue==PHOTOMETRIC_PALETTE)
		{
		if(!indexed)
			colorSpace=RGB;
		indexed=true;
		}
	else if(havePhotometricTag&&photometricTagValue<=10&&photometricTagValue!=7)
		colorSpace=ColorSpace(photometricTagValue);
	
	/* Query whether samples are layed out in planes or interleaved per pixel: */
	uint16_t planarConfig;
	TIFFGetFieldDefaulted(tiff,TIFFTAG_PLANARCONFIG,&planarConfig);
	planar=planarConfig==PLANARCONFIG_SEPARATE;
	
	/* Query whether the image is organized in strips or tiles: */
	tiled=TIFFIsTiled(tiff);
	if(tiled)
		{
		/* Get the image's tile layout: */
		TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&tileWidth);
		TIFFGetField(tiff,TIFFTAG_TILELENGTH,&tileHeight);
		}
	else
		{
		/* Get the image's strip layout: */
		TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rowsPerStrip);
		}
	}

TIFFReader::~TIFFReader(void)
	{
	/* Close the TIFF file: */
	TIFFClose(tiff);
	}

bool TIFFReader::getColorMap(uint16_t*& red,uint16_t*& green,uint16_t*& blue)
	{
	/* Get the color map: */
	bool result=TIFFGetField(tiff,TIFFTAG_COLORMAP,&red,&green,&blue)!=0;
	if(!result)
		{
		/* Null the given pointers, just in case: */
		blue=green=red=0;
		}
	
	return result;
	}

bool TIFFReader::getCMYKColorMap(uint16_t*& cyan,uint16_t*& magenta,uint16_t*& yellow,uint16_t*& black)
	{
	/* Get the color map: */
	bool result=TIFFGetField(tiff,TIFFTAG_COLORMAP,&cyan,&magenta,&yellow,&black)!=0;
	if(!result)
		{
		/* Null the given pointers, just in case: */
		black=yellow=magenta=cyan=0;
		}
	
	return result;
	}

void TIFFReader::readRgba(uint32_t* rgbaBuffer)
	{
	if(!TIFFReadRGBAImage(tiff,width,height,rgbaBuffer))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot read image");
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

void TIFFReader::readStrips(uint8_t* image,ptrdiff_t rowStride)
	{
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Create a buffer to hold a strip of image data: */
		Misc::SelfDestructArray<uint8_t> stripBuffer(TIFFStripSize(tiff));
		
		/* Read image data by channels: */
		for(uint16_t channel=0;channel<numSamples;++channel)
			{
			/* Read channel data by rows: */
			uint8_t* rowPtr=image+(height-1)*rowStride;
			uint32_t rowStart=0;
			for(uint32_t strip=0;rowStart<height;++strip)
				{
				/* Read the next strip of image data: */
				uint8_t* stripPtr=stripBuffer;
				TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
				
				/* Copy image data from the strip buffer into the result image: */
				uint32_t rowEnd=rowStart+rowsPerStrip;
				if(rowEnd>height)
					rowEnd=height;
				switch(numBits)
					{
					case 8:
						for(uint32_t row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(width,numSamples,channel,rowPtr,stripPtr);
						break;
					
					case 16:
						for(uint32_t row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(width,numSamples,channel,reinterpret_cast<uint16_t*>(rowPtr),reinterpret_cast<uint16_t*>(stripPtr));
						break;
					
					case 32:
						for(uint32_t row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(width,numSamples,channel,reinterpret_cast<uint32_t*>(rowPtr),reinterpret_cast<uint32_t*>(stripPtr));
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
		uint32_t rowEnd=height;
		for(uint32_t strip=0;rowEnd>0;++strip)
			{
			/* Read the next strip of image data into the appropriate region of the result image: */
			uint32_t rowStart=rowEnd>=rowsPerStrip?rowEnd-rowsPerStrip:0;
			uint8_t* stripPtr=image+rowStart*rowStride;
			TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
			
			/* Flip rows in the just-read image region in-place: */
			uint32_t row0=rowStart;
			uint32_t row1=rowEnd-1;
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

namespace {

/****************
Helper functions:
****************/

template <class ScalarParam>
inline
void
copyTileChannel( // Helper function to read a single channel for an image tile
	uint32_t width,
	uint32_t height,
	uint16_t numChannels,
	uint16_t channel,
	ScalarParam* rowPtr,
	ptrdiff_t rowStride,
	ScalarParam* tilePtr,
	ptrdiff_t tileStride)
	{
	rowPtr+=channel;
	for(uint32_t y=0;y<height;++y,rowPtr+=rowStride,tilePtr+=tileStride)
		{
		ScalarParam* rPtr=rowPtr;
		ScalarParam* tPtr=tilePtr;
		for(uint32_t x=0;x<width;++x,rPtr+=numChannels,++tPtr)
			*rPtr=*tPtr;
		}
	}

}

uint8_t* TIFFReader::createTileBuffer(void)
	{
	/* Return a buffer to hold a tile of image data: */
	return new uint8_t[TIFFTileSize(tiff)];
	}

void TIFFReader::readTile(uint32_t tileIndexX,uint32_t tileIndexY,uint8_t* tileBuffer,uint8_t* image,ptrdiff_t rowStride)
	{
	/* Query tile memory layout: */
	tmsize_t tileSize=TIFFTileSize(tiff);
	tmsize_t tileRowStride=TIFFTileRowSize(tiff);
	tmsize_t pixelSize=tmsize_t(numSamples*((numBits+7)/8));
	uint32_t tilesPerRow=(width+tileWidth-1)/tileWidth;
	
	/* Calculate the index of the tile to read, or the tile to read in the first plane of a planar image: */
	uint32_t tileIndex=tileIndexY*tilesPerRow+tileIndexX;
	
	/* Determine actually used tile size of the requested tile: */
	uint32_t tw=Misc::min(width-tileIndexX*tileWidth,tileWidth);
	tmsize_t ts=tw*pixelSize;
	uint32_t th=Misc::min(height-tileIndexY*tileHeight,tileHeight);
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Calculate the tile index increment to the next plane: */
		uint32_t tilesPerPlane=((height+tileHeight-1)/tileHeight)*tilesPerRow;
		
		/* Read tile data by channels: */
		for(uint16_t channel=0;channel<numSamples;++channel,tileIndex+=tilesPerPlane)
			{
			/* Read the requested tile into the tile buffer: */
			TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
			
			/* Copy tile data by row: */
			uint8_t* rowPtr=image;
			uint8_t* tilePtr=tileBuffer;
			switch(numBits)
				{
				case 8:
					for(uint32_t y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
						copyRowChannel(tw,numSamples,channel,rowPtr,tilePtr);
					break;
				
				case 16:
					for(uint32_t y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
						copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint16_t*>(rowPtr),reinterpret_cast<uint16_t*>(tilePtr));
					break;
				
				case 32:
					for(uint32_t y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
						copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint32_t*>(rowPtr),reinterpret_cast<uint32_t*>(tilePtr));
					break;
				}
			}
		}
	else
		{
		/* Read the requested tile into the tile buffer: */
		TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
		
		/* Copy tile data by row: */
		uint8_t* rowPtr=image;
		uint8_t* tilePtr=tileBuffer;
		for(uint32_t y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
			{
			/* Copy the tile row: */
			memcpy(rowPtr,tilePtr,ts);
			}
		}
	}

void TIFFReader::readTiles(uint8_t* image,ptrdiff_t rowStride)
	{
	/* Create a buffer to hold a tile of image data: */
	tmsize_t tileSize=TIFFTileSize(tiff);
	Misc::SelfDestructArray<uint8_t> tileBuffer(tileSize);
	tmsize_t tileRowStride=TIFFTileRowSize(tiff);
	tmsize_t pixelSize=tmsize_t(numSamples*((numBits+7)/8));
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Read image data by channels: */
		uint32_t tileIndex=0;
		for(uint16_t channel=0;channel<numSamples;++channel)
			{
			/* Read channel data by tiles: */
			for(uint32_t ty=0;ty<height;ty+=tileHeight)
				{
				/* Determine actual tile height in this tile row: */
				uint32_t th=Misc::min(height-ty,tileHeight);
				
				/* Read all tiles in this tile row: */
				for(uint32_t tx=0;tx<width;tx+=tileWidth,++tileIndex)
					{
					/* Read the next tile: */
					TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
					
					/* Determine actual tile width in this tile column: */
					uint32_t tw=Misc::min(width-tx,tileWidth);
					
					/* Copy tile data by row: */
					uint8_t* rowPtr=image+(height-1-ty)*rowStride+tx*pixelSize;
					uint8_t* tilePtr=tileBuffer;
					switch(numBits)
						{
						case 8:
							for(uint32_t y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,numSamples,channel,rowPtr,tilePtr);
							break;
						
						case 16:
							for(uint32_t y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint16_t*>(rowPtr),reinterpret_cast<uint16_t*>(tilePtr));
							break;
						
						case 32:
							for(uint32_t y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint32_t*>(rowPtr),reinterpret_cast<uint32_t*>(tilePtr));
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
		for(uint32_t ty=0;ty<height;ty+=tileHeight)
			{
			/* Determine actual tile height in this tile row: */
			uint32_t th=Misc::min(height-ty,tileHeight);
			
			/* Read all tiles in this tile row: */
			for(uint32_t tx=0;tx<width;tx+=tileWidth,++tileIndex)
				{
				/* Read the next tile: */
				TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
				
				/* Determine actual tile width in this tile column: */
				uint32_t tw=Misc::min(width-tx,tileWidth);
				tmsize_t ts=tw*pixelSize;
				
				/* Copy tile data by row: */
				uint8_t* rowPtr=image+(height-1-ty)*rowStride+tx*pixelSize;
				uint8_t* tilePtr=tileBuffer;
				for(uint32_t y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
					{
					/* Copy the tile row: */
					memcpy(rowPtr,tilePtr,ts);
					}
				}
			}
		}
	}

void TIFFReader::streamStrips(TIFFReader::PixelStreamingCallback pixelStreamingCallback,void* pixelStreamingUserData)
	{
	/* Create a buffer to hold a strip of image data: */
	Misc::SelfDestructArray<uint8_t> stripBuffer(TIFFStripSize(tiff));
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Calculate the strip stride: */
		ptrdiff_t stripStride=ptrdiff_t(width)*ptrdiff_t((numBits+7)/8);
		
		/* Read pixel data by channels: */
		for(uint16_t channel=0;channel<numSamples;++channel)
			{
			/* Read channel data by rows: */
			uint32_t rowEnd=height;
			for(uint32_t strip=0;rowEnd>0;++strip)
				{
				/* Read the next strip of channel data into the strip buffer: */
				uint32_t rowStart=rowEnd>=rowsPerStrip?rowEnd-rowsPerStrip:0;
				uint8_t* stripPtr=stripBuffer;
				TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
				
				/* Stream the channel data strip row-by-row: */
				for(uint32_t row=rowEnd;row!=rowStart;--row,stripPtr+=stripStride)
					pixelStreamingCallback(0,row-1,width,channel,stripPtr,pixelStreamingUserData);
				
				/* Prepare for the next strip: */
				rowEnd=rowStart;
				}
			}
		}
	else
		{
		/* Calculate the strip stride: */
		ptrdiff_t stripStride=ptrdiff_t(width)*ptrdiff_t(numSamples)*ptrdiff_t((numBits+7)/8);
		
		/* Read pixel data by rows: */
		uint32_t rowEnd=height;
		for(uint32_t strip=0;rowEnd>0;++strip)
			{
			/* Read the next strip of pixel data into the strip buffer: */
			uint32_t rowStart=rowEnd>=rowsPerStrip?rowEnd-rowsPerStrip:0;
			uint8_t* stripPtr=stripBuffer;
			TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
			
			/* Stream the pixel data strip row-by-row: */
			for(uint32_t row=rowEnd;row!=rowStart;--row,stripPtr+=stripStride)
				pixelStreamingCallback(0,row-1,width,uint16(-1),stripPtr,pixelStreamingUserData);
			
			/* Prepare for the next strip: */
			rowEnd=rowStart;
			}
		}
	}

void TIFFReader::streamTiles(TIFFReader::PixelStreamingCallback pixelStreamingCallback,void* pixelStreamingUserData)
	{
	/* Create a buffer to hold a tile of image data: */
	tmsize_t tileSize=TIFFTileSize(tiff);
	Misc::SelfDestructArray<uint8_t> tileBuffer(tileSize);
	tmsize_t tileRowStride=TIFFTileRowSize(tiff);
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Read pixel data by channels: */
		uint32_t tileIndex=0;
		for(uint16_t channel=0;channel<numSamples;++channel)
			{
			for(uint32_t ty=0;ty<height;ty+=tileHeight)
				{
				/* Determine actual tile height in this tile row: */
				uint32_t th=Misc::min(height-ty,tileHeight);
				
				/* Read all tiles in this tile row: */
				for(uint32_t tx=0;tx<width;tx+=tileWidth,++tileIndex)
					{
					/* Determine actual tile width in this tile column: */
					uint32_t tw=Misc::min(width-tx,tileWidth);
					
					/* Read the next tile of channel data: */
					uint8_t* tilePtr=tileBuffer;
					TIFFReadEncodedTile(tiff,tileIndex,tilePtr,tileSize);
					
					/* Stream the channel data tile row-by-row: */
					uint32_t rowEnd=height-(ty+th);
					for(uint32_t row=height-ty;row!=rowEnd;--row,tilePtr+=tileRowStride)
						pixelStreamingCallback(tx,row-1,tw,channel,tilePtr,pixelStreamingUserData);
					}
				}
			}
		}
	else
		{
		/* Read pixel data by tiles: */
		uint32_t tileIndex=0;
		for(uint32_t ty=0;ty<height;ty+=tileHeight)
			{
			/* Determine actual tile height in this tile row: */
			uint32_t th=Misc::min(height-ty,tileHeight);
			
			/* Read all tiles in this tile row: */
			for(uint32_t tx=0;tx<width;tx+=tileWidth,++tileIndex)
				{
				/* Determine actual tile width in this tile column: */
				uint32_t tw=Misc::min(width-tx,tileWidth);
				
				/* Read the next tile of pixel data: */
				uint8_t* tilePtr=tileBuffer;
				TIFFReadEncodedTile(tiff,tileIndex,tilePtr,tileSize);
				
				/* Stream the pixel data tile row-by-row: */
				uint32_t rowEnd=height-(ty+th);
				for(uint32_t row=height-ty;row!=rowEnd;--row,tilePtr+=tileRowStride)
					pixelStreamingCallback(tx,row-1,tw,uint16(-1),tilePtr,pixelStreamingUserData);
				}
			}
		}
	}

}

#endif

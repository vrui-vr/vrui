/***********************************************************************
ImageReaderGIF - Class to read images from files in GIF format.
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

#include <Images/ImageReaderGIF.h>

#include <string.h>
#include <Misc/Utility.h>
#include <Misc/StdError.h>
#include <Misc/SelfDestructArray.h>
#include <IO/LZWDecompressor.h>
#include <Images/BaseImage.h>

namespace IO {

/**************
Helper classes:
**************/

class GIFBlock:public File // Helper class to read a GIF file block, which consists of a sequence of chunks up to 255 byte in size, terminated by a zero-sized chunk
	{
	/* Elements: */
	private:
	FilePtr source; // The data source from which the block is being read
	size_t chunkSizeLeft; // Number of unread bytes in the current chunk
	Misc::UInt32 codeBuffer; // Buffer to read LZW code words from the GIF block
	unsigned int numCodeBufferBits; // Number of bits currently in the code buffer
	
	/* Protected methods from IO::File: */
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	GIFBlock(File& sSource); // Creates a block from the given data source
	~GIFBlock(void); // Closes the chunk, skipping all unread data
	
	/* Methods from class IO::File: */
	virtual size_t getReadBufferSize(void) const;
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	
	/* New methods: */
	Misc::UInt32 readCode(unsigned int numCodeBits) // Reads a code word of the given number of bits
		{
		/* Stuff bytes from the source into the code buffer on the left until it has enough bits: */
		while(numCodeBufferBits<numCodeBits)
			{
			codeBuffer|=Misc::UInt32(read<Misc::UInt8>())<<numCodeBufferBits;
			numCodeBufferBits+=8;
			}
		
		/* Return the requested number of bits from the right of the code buffer: */
		Misc::UInt32 result=codeBuffer&((1U<<numCodeBits)-1U);
		codeBuffer>>=numCodeBits;
		numCodeBufferBits-=numCodeBits;
		return result;
		}
	};

size_t GIFBlock::readData(File::Byte* buffer,size_t bufferSize)
	{
	/* Check if the current chunk has been read completely: */
	if(chunkSizeLeft==0)
		{
		/* Read the size of the next chunk: */
		chunkSizeLeft=size_t(source->read<Misc::UInt8>());
		
		/* Bail out if this was the terminating zero-sized chunk: */
		if(chunkSizeLeft==0)
			return 0;
		}
	
	/* Read as much of the remainder of the current chunk as is available: */
	void* chunkBuffer;
	size_t chunkReadSize=source->readInBuffer(chunkBuffer,chunkSizeLeft);
	chunkSizeLeft-=chunkReadSize;
	
	/* Install a fake read buffer pointing into the source's read buffer: */
	setReadBuffer(chunkReadSize,static_cast<Byte*>(chunkBuffer),false);
	
	return chunkReadSize;
	}

GIFBlock::GIFBlock(File& sSource)
	:File(),
	 source(&sSource),chunkSizeLeft(0),
	 codeBuffer(0x0U),numCodeBufferBits(0)
	{
	/* Copy the source's endianness setting: */
	setSwapOnRead(source->mustSwapOnRead());
	
	/* Disable read-through: */
	canReadThrough=false;
	}

GIFBlock::~GIFBlock(void)
	{
	/* Skip any unread chunk data: */
	while(true)
		{
		/* Check if the current chunk has been read completely: */
		if(chunkSizeLeft==0)
			{
			/* Read the size of the next chunk and bail out if it's the terminating chunk: */
			chunkSizeLeft=source->read<Misc::UInt8>();
			if(chunkSizeLeft==0)
				break;
			}
		
		/* Read as much of the remainder of the current chunk as is available: */
		void* chunkBuffer;
		size_t chunkReadSize=source->readInBuffer(chunkBuffer,chunkSizeLeft);
		chunkSizeLeft-=chunkReadSize;
		}
	
	/* Release the read buffer: */
	setReadBuffer(0,0,false);
	}

size_t GIFBlock::getReadBufferSize(void) const
	{
	/* Return the source's read buffer size, since we're sharing it: */
	return source->getReadBufferSize();
	}

size_t GIFBlock::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Ignore the request and return the source's read buffer size, since we're sharing it: */
	return source->getReadBufferSize();
	}

}

namespace Images {

void ImageReaderGIF::readNextImageBlock(void)
	{
	/* Process blocks until the next image block or end of file: */
	bool goOn=true;
	while(goOn)
		{
		/* Process the next block based on its identifier: */
		switch(file->getChar())
			{
			case 0x21: // Extension block with sub-type
				{
				switch(file->getChar())
					{
					case 0x01: // Plain text extension
						break;
					
					case 0xf9: // Graphics control extension
						break;
					
					case 0xfe: // Comment extension
						break;
					
					case 0xff: // Application extension
						break;
					}
				
				/* Skip the extension block's contents: */
				IO::GIFBlock block(*file);
				
				break;
				}
			
			case 0x2c: // Image block
				{
				/* Read the image header: */
				file->read<Misc::UInt16>(imageSpec.rect.offset.getComponents(),2);
				file->read<Misc::UInt16>(imageSpec.rect.size.getComponents(),2);
				imageSpec.colorSpace=RGB;
				imageSpec.hasAlpha=false;
				imageSpec.numChannels=3;
				imageSpec.valueType=UnsignedInt;
				imageSpec.numFieldBits=8;
				imageSpec.numFieldBytes=1;
				imageSpec.numValueBits=8;
				
				/* Stop looking: */
				goOn=false;
				
				break;
				}
			
			case 0x3b: // Trailer
				/* Stop looking and indicate that there are no more images: */
				done=true;
				goOn=false;
				
				break;
			}
		}
	}

/*******************************
Methods of class ImageReaderGIF:
*******************************/

ImageReaderGIF::ImageReaderGIF(IO::File& sFile)
	:ImageReader(sFile),
	 globalColorMap(0),
	 done(false)
	{
	file->setEndianness(Misc::LittleEndian);
	
	/* Read the GIF format signature: */
	char sig[3];
	file->readRaw(sig,3);
	if(memcmp(sig,"GIF",3)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"File is not a GIF file");
	
	/* Read the GIF format version: */
	char version[3];
	file->readRaw(version,3);
	if(memcmp(version,"87a",3)!=0&&memcmp(version,"89a",3)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"File has unsupported GIF version");
	
	/* Read the canvas size: */
	file->read<Misc::UInt16>(canvasSize.getComponents(),2);
	
	/* Read the global image flags: */
	unsigned int globalFlags=file->read<Misc::UInt8>();
	bitsPerPixel=(globalFlags&0x07U)+1U;
	
	/* Read the background color index: */
	file->read(backgroundColorIndex);
	
	/* Read and decode the pixel aspect ratio: */
	pixelAspectRatio=(float(file->read<Misc::UInt8>())+15.0f)/64.0f;
	
	/* Read the global color map if one is defined: */
	if(globalFlags&0x80U)
		{
		unsigned int numColors=1U<<bitsPerPixel;
		globalColorMap=new Color[numColors];
		file->read(globalColorMap[0].getComponents(),numColors*3);
		}
	
	/* Read blocks until the first image block: */
	readNextImageBlock();
	}

ImageReaderGIF::~ImageReaderGIF(void)
	{
	/* Release allocated resources: */
	delete[] globalColorMap;
	}

bool ImageReaderGIF::eof(void) const
	{
	return done;
	}

namespace {

/**************
Helper classes:
**************/

class RowMapper // Helper class to access rows of an image for GIF assembly
	{
	/* Elements: */
	Images::BaseImage& image;
	unsigned int row; // Index of the current image row
	unsigned int interlacePhase; // Index of the current interlace phase; 0 if image is non-interlaced
	unsigned int rowStep; // Step factor between image rows, for interlaced images
	
	/* Constructors and destructors: */
	public:
	RowMapper(Images::BaseImage& sImage,bool interlaced) // Creates a row mapper for the given image, for interlaced access if given flag is true
		:image(sImage),
		 row(image.getHeight()-1),
		 interlacePhase(interlaced?3:0),
		 rowStep(interlaced?8:1)
		{
		}
	
	/* Methods: */
	ImageReaderGIF::Color* getCurrentRow(void) const // Returns a pointer to the beginning of the current row
		{
		return static_cast<ImageReaderGIF::Color*>(image.modifyPixels())+row*image.getWidth();
		}
	ImageReaderGIF::Color* getNextRow(void) // Returns a pointer to the beginning of the next row; throws exception if image has been written completely
		{
		/* Check if the current interlace phase is continuing: */
		if(row>=rowStep)
			{
			/* Go to the next row: */
			row-=rowStep;
			}
		else
			{
			/* Check if the image is fully constructed: */
			if(interlacePhase==0)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image data overrun");
			
			/* Go to the next interlace phase: */
			--interlacePhase;
			switch(interlacePhase)
				{
				case 2: // Every eighth row, starting at 4
					row=image.getHeight()-5;
					rowStep=8;
					break;
				
				case 1: // Every fourth row, starting at 2
					row=image.getHeight()-3;
					rowStep=4;
					break;
				
				case 0: // All odd image rows
					row=image.getHeight()-2;
					rowStep=2;
					break;
				}
			}
		
		return static_cast<ImageReaderGIF::Color*>(image.modifyPixels())+row*image.getWidth();
		}
	};

}

BaseImage ImageReaderGIF::readImage(void)
	{
	/* Check if the image size and position are valid: */
	if(imageSpec.rect.offset[0]<0||(unsigned int)(imageSpec.rect.offset[0])+imageSpec.rect.size[0]>canvasSize[0]||
	   imageSpec.rect.offset[1]<0||(unsigned int)(imageSpec.rect.offset[1])+imageSpec.rect.size[1]>canvasSize[1])
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image rectangle exceeds canvas size");
	
	/* Read the image flags: */
	unsigned int imageFlags=file->read<Misc::UInt8>();
	bool imageInterlaced=(imageFlags&0x40U)!=0x00U;
	unsigned int imageBitsPerPixel=bitsPerPixel;
	Color* colorMap=globalColorMap;
	Misc::SelfDestructArray<Color> imageColorMap;
	if((imageFlags&0x80U)!=0x00U)
		{
		/* Use the number of colors from the image header: */
		imageBitsPerPixel=(imageFlags&0x7U)+1U;
		
		/* Read the image's local color map: */
		unsigned int numColors=1U<<imageBitsPerPixel;
		imageColorMap.setTarget(new Color[numColors]);
		file->read(imageColorMap[0].getComponents(),numColors*3);
		
		/* Use the image's local color map for image generation: */
		colorMap=imageColorMap.getArray();
		}
	
	/* Create the result image: */
	BaseImage result(imageSpec.rect.size,3,1,GL_RGB,GL_UNSIGNED_BYTE);
	
	/* Read the image's character code size: */
	unsigned int numCharBits=file->read<Misc::UInt8>();
	
	/* Create a block reader and an LZW decompressor to read the compressed image data stream: */
	IO::GIFBlock blockReader(*file);
	IO::LZWDecompressor decompressor(numCharBits,4096);
	
	/* Create a row mapper to access the image's pixel rows in the proper order: */
	RowMapper rowMapper(result,imageInterlaced);
	Color* rowPtr=rowMapper.getCurrentRow();
	Color* rowEnd=rowPtr+result.getWidth();
	
	/* Read strings of color table indices from the GIF block until the end-of-image marker: */
	while(true)
		{
		/* Decompress the next code: */
		const IO::LZWDecompressor::Char* stringPtr=decompressor.decompress(blockReader.readCode(decompressor.getNumCodeBits()));
		const IO::LZWDecompressor::Char* stringEnd=decompressor.getStringEnd();
		
		/* Bail out if the image data stream is finished: */
		if(stringPtr==0)
			break;
		
		/* Copy color values into the result image: */
		for(;stringPtr!=stringEnd;++stringPtr,++rowPtr)
			{
			/* Check if the current image row is complete: */
			if(rowPtr==rowEnd)
				{
				/* Go to the next image row in sequence: */
				rowPtr=rowMapper.getNextRow();
				rowEnd=rowPtr+result.getWidth();
				}
			
			/* Set the next pixel value: */
			*rowPtr=colorMap[(unsigned int)*stringPtr];
			}
		}
	
	/* Check if the image has been read completely: */
	if(rowPtr!=static_cast<const Color*>(result.getPixels())+result.getWidth())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Incomplete GIF data stream");
	
	return result;
	}

}

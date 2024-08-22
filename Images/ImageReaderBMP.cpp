/***********************************************************************
ImageReaderBMP - Class to read images from files in Windows BMP format.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <Images/ImageReaderBMP.h>

#include <stddef.h>
#include <Misc/StdError.h>
#include <Images/BaseImage.h>
#include <Images/PixelSwizzler.h>

namespace Images {

/*******************************
Methods of class ImageReaderBMP:
*******************************/

ImageReaderBMP::ImageReaderBMP(IO::File& sFile)
	:ImageReader(sFile),
	 bottomUp(true),numBitsPerPixel(0),compressionMethod(BMP_RGB),
	 resolutionUnit(BMP_PixelsPerMeter),numPaletteColors(0),
	 done(false)
	{
	/* BMP files are always little endian: */
	file->setEndianness(Misc::LittleEndian);
	
	/* Keep track of the total number of bytes read from the file: */
	size_t numReadBytes=0;
	
	/* Read the BMP file header and check for the magic number: */
	unsigned int type=file->read<Misc::UInt16>();
	numReadBytes+=sizeof(Misc::UInt16);
	if(type!=0x4d42U&&type!=0x4142U&&type!=0x4943U&&type!=0x5043U&&type!=0x4349U&&type!=0x5450U) // Valid values: BM, BA, CI, CP, IC, PT
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid BMP file header");
	
	/* Remember if this is an OS/2 bitmap: */
	bool os2=type!=0x4d42U;
	
	/* Read file size: */
	size_t fileSize=file->read<Misc::UInt32>();
	numReadBytes+=sizeof(Misc::UInt32);
	
	/* Skip reserved fields: */
	file->skip<Misc::UInt16>(2);
	numReadBytes+=2*sizeof(Misc::UInt16);
	
	/* Read pixel data offset: */
	size_t pixelDataOffset=file->read<Misc::UInt32>();
	numReadBytes+=sizeof(Misc::UInt32);
	
	/* Remember the file position where the DIB header started: */
	size_t dibHeaderStart=numReadBytes;
	
	/* Read the DIB bitmap information header: */
	unsigned int dibHeaderSize=file->read<Misc::UInt32>();
	numReadBytes+=sizeof(Misc::UInt32);
	if(dibHeaderSize<12U)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid DIB header size");
	
	/* Read the image size depending on DIB header size: */
	Misc::SInt32 dibSize[2]; // Windows uses signed sizes and indicates image row order
	if(dibHeaderSize==12U)
		{
		/* Image sizes are 16 bit: */
		if(os2)
			{
			/* OS/2 uses unsigned integers: */
			for(int i=0;i<2;++i)
				dibSize[i]=Misc::SInt32(file->read<Misc::UInt16>());
			numReadBytes+=2*sizeof(Misc::UInt16);
			}
		else
			{
			/* Windows uses signed integers: */
			for(int i=0;i<2;++i)
				dibSize[i]=Misc::SInt32(file->read<Misc::SInt16>());
			numReadBytes+=2*sizeof(Misc::SInt16);
			}
		}
	else if(dibHeaderSize>=16U)
		{
		/* Image sizes are signed 32 bit: */
		file->read(dibSize,2);
		numReadBytes+=2*sizeof(Misc::SInt32);
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid DIB header size");
	
	/* Check for row order and correctness: */
	if(dibSize[0]<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Negative image width");
	if(dibSize[1]<0)
		{
		bottomUp=false;
		dibSize[1]=-dibSize[1];
		}
	
	/* Set the image size: */
	canvasSize=Size(dibSize[0],dibSize[1]);
	imageSpec.rect=Rect(canvasSize);
	
	/* Read the number of color planes and bits per pixel: */
	unsigned int numColorPlanes=file->read<Misc::UInt16>();
	numReadBytes+=sizeof(Misc::UInt16);
	if(numColorPlanes!=1)	
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid number of color planes");
	numBitsPerPixel=file->read<Misc::UInt16>();
	numReadBytes+=sizeof(Misc::UInt16);
	
	/* Fill in the rest of the image specification: */
	setFormatSpec(RGB,false);
	setValueSpec(UnsignedInt,8);
	
	size_t rawImageDataSize=0;
	for(int i=0;i<2;++i)
		resolution[i]=0;
	unsigned int numPaletteColors=0;
	unsigned int numImportantColors=0;
	if(dibHeaderSize>=40U)
		{
		/* Read the compression method: */
		compressionMethod=file->read<Misc::UInt32>();
		numReadBytes+=sizeof(Misc::UInt32);
		
		/* Read the raw image data size: */
		rawImageDataSize=file->read<Misc::UInt32>();
		numReadBytes+=sizeof(Misc::UInt32);
		
		/* Read the image resolution: */
		for(int i=0;i<2;++i)
			resolution[i]=file->read<Misc::SInt32>();
		numReadBytes+=2*sizeof(Misc::SInt32);
		
		/* Read the palette size and number of "important" colors: */
		numPaletteColors=file->read<Misc::UInt32>();
		numReadBytes+=sizeof(Misc::UInt32);
		numImportantColors=file->read<Misc::UInt32>();
		numReadBytes+=sizeof(Misc::UInt32);
		}
	
	/* Check for correctness: */
	if(rawImageDataSize==0&&compressionMethod!=BMP_RGB&&compressionMethod!=BMP_CMYK)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No raw data size for compressed image");
	if(numPaletteColors==0)
		numPaletteColors=1U<<numBitsPerPixel;
	if(numImportantColors==0)
		numImportantColors=numPaletteColors;
	
	if(os2&&dibHeaderSize>=64U)
		{
		/* Read the resolution unit (only 0 for pixels/meter is defined): */
		resolutionUnit=file->read<Misc::UInt16>();
		numReadBytes+=sizeof(Misc::UInt16);
		if(resolutionUnit!=BMP_PixelsPerMeter)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid image resolution unit");
		
		/* Skip padding: */
		file->skip<Misc::UInt16>(1);
		numReadBytes+=sizeof(Misc::UInt16);
		
		/* Read row order: */
		unsigned int rowOrder=file->read<Misc::UInt16>();
		numReadBytes+=sizeof(Misc::UInt16);
		if(rowOrder!=0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid image row order");
		}
	
	/* Initialize the RGBA pixel component masks to the default RGBA layout: */
	for(int i=0;i<4;++i)
		rgbaMasks[i]=0xffU<<(i*8);
	
	/* Read the optional RGBA field bit mask definition: */
	if(!os2)
		{
		/* Determine the number of bit masks in the DIB header: */
		unsigned int numDibRGBAMasks=0;
		if(compressionMethod==BMP_BitFields)
			{
			if(dibHeaderSize>=52U)
				numDibRGBAMasks=3;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing RGB bit masks");
			}
		else if(compressionMethod==BMP_AlphaBitFields&&dibHeaderSize>=56U)
			{
			if(dibHeaderSize>=56U)
				numDibRGBAMasks=4;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing RGBA bit masks");
			}
		
		/* Read the pixel bit masks: */
		for(unsigned int i=0;i<numDibRGBAMasks;++i)
			rgbaMasks[i]=file->read<Misc::UInt32>();
		numReadBytes+=numDibRGBAMasks*sizeof(Misc::UInt32);
		}
	
	/* Skip ahead to the end of the DIB header: */
	file->skip<Misc::UInt8>(dibHeaderStart+dibHeaderSize-numReadBytes);
	numReadBytes=dibHeaderStart+dibHeaderSize;
	
	/* Check for the presence of a color map: */
	size_t paletteGap=pixelDataOffset-numReadBytes;
	size_t paletteEntrySize=4*sizeof(Misc::UInt8);
	if(os2&&dibHeaderSize==12U)
		paletteEntrySize=3*sizeof(Misc::UInt8);
	if(paletteGap>=numPaletteColors*paletteEntrySize)
		{
		/* Read the color map: */
		palette.setTarget(new Misc::UInt32[numPaletteColors]);
		if(paletteEntrySize==4)
			{
			/* Read the palette directly: */
			file->read(palette.getArray(),numPaletteColors);
			numReadBytes+=numPaletteColors*sizeof(Misc::UInt32);
			}
		else
			{
			/* Read and expand 24-bit RGB palette entries: */
			for(unsigned int i=0;i<numPaletteColors;++i)
				{
				/* Read the RGB entry: */
				Misc::UInt8 rgb[3];
				file->read(rgb,3);
				numReadBytes+=3*sizeof(Misc::UInt8);
				
				/* Expand the RGB entry to RGBA: */
				palette[i]=(((Misc::UInt32(rgb[2])<<8)|Misc::UInt32(rgb[1]))<<8)|Misc::UInt32(rgb[0]);
				}
			}
		}
	
	/* Skip ahead to the beginning of the pixel data: */
	file->skip<Misc::UInt8>(pixelDataOffset-numReadBytes);
	numReadBytes=pixelDataOffset;
	}

bool ImageReaderBMP::eof(void) const
	{
	return done;
	}

BaseImage ImageReaderBMP::readImage(void)
	{
	/* Create the result image: */
	Size& size=imageSpec.rect.size;
	BaseImage result=createImage();
	Misc::UInt8* image=static_cast<Misc::UInt8*>(result.replacePixels());
	ptrdiff_t rowStride=result.getRowStride();
	
	/* Check for top-down image row layout: */
	if(!bottomUp)
		{
		/* Read the result image top to bottom: */
		image+=(size[1]-1)*rowStride;
		rowStride=-rowStride;
		}
	
	/* Calculate the pixel array layout: */
	unsigned int dwordsPerRow=(size[0]*numBitsPerPixel+31)/32;
	unsigned int fullDwordsPerRow=(size[0]*numBitsPerPixel)/32;
	Misc::SelfDestructArray<Misc::UInt32> rawRowBuffer(new Misc::UInt32[dwordsPerRow]);
	
	/* Check if there is a color map: */
	if(palette!=0)
		{
		/* Read the image by rows: */
		for(unsigned int y=size[1];y>0;--y,image+=rowStride)
			{
			/* Read the image row by dwords and extract bits as they go: */
			Misc::UInt32 bitbuffer;
			unsigned int numBitsInBuffer=0;
			}
		}
	else
		{
		/* Create a pixel swizzler to convert the image file's pixels into canonical RGB(A): */
		PixelSwizzler swizzler(rgbaMasks);
		
		/* Read the image by rows: */
		for(unsigned int y=size[1];y>0;--y,image+=rowStride)
			{
			/* Read the pixel row from the file into the raw row buffer: */
			file->read(rawRowBuffer.getArray(),dwordsPerRow);
			
			/* Copy pixel values from the raw row buffer into the image: */
			if(numBitsPerPixel==32)
				{
				Misc::UInt32* rrPtr=rawRowBuffer.getArray();
				Misc::UInt8* imgPtr=image;
				for(unsigned int x=size[0];x>0;--x,++rrPtr,imgPtr+=4)
					swizzler.swizzle(4,imgPtr,*rrPtr);
				}
			else if(numBitsPerPixel==24)
				{
				}
			else if(numBitsPerPixel==16)
				{
				}
			}
		}
	
	
	
	
	
	
	
	
	
	
	
	
	
	/* There can be only one image in a BMP file: */
	done=true;
	
	return result;
	}

}

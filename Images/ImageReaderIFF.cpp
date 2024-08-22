/***********************************************************************
ImageReaderIFF - Class to read images from files in IFF (Interchange
File Format) format.
Copyright (c) 2021-2024 Oliver Kreylos

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

#include <Images/ImageReaderIFF.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <Images/BaseImage.h>

namespace Images {

void ImageReaderIFF::readImageHeaders(void)
	{
	/* Read the FORM chunk's sub-chunks until the chunk is over or the next image body chunk is encountered: */
	bool haveBmhd=false;
	while(!form->eof())
		{
		/* Open the next chunk: */
		IO::IFFChunkPtr chunk=new IO::IFFChunk(form);
		
		if(chunk->isChunk("BMHD")) // Bitmap header chunk
			{
			if(chunk->getChunkSize()<20)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid BMHD chunk");
			
			for(int i=0;i<2;++i)
				imageSpec.rect.size[i]=chunk->read<Misc::UInt16>();
			for(int i=0;i<2;++i)
				imageSpec.rect.offset[i]=chunk->read<Misc::UInt16>();
			numBitPlanes=chunk->read<Misc::UInt8>();
			masking=chunk->read<Misc::UInt8>();
			compress=chunk->read<Misc::UInt8>()!=0;
			chunk->skip<Misc::UInt8>(1);
			transparentColorIndex=chunk->read<Misc::UInt16>();
			#if 0
			unsigned int pixelWidth=chunk->read<Misc::UInt8>();
			unsigned int pixelHeight=chunk->read<Misc::UInt8>();
			unsigned int pageWidth=chunk->read<Misc::UInt16>();
			unsigned int pageHeight=chunk->read<Misc::UInt16>();
			#endif
			
			haveBmhd=true;
			}
		else if(chunk->isChunk("CMAP")) // Color map chunk
			{
			size_t colorMapSize=chunk->getChunkSize()/sizeof(ColorMapEntry);
			if(colorMapSize!=1U<<numBitPlanes)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching color map size");
			
			colorMap.setTarget(new ColorMapEntry[colorMapSize]);
			for(size_t i=0;i<colorMapSize;++i)
				chunk->read(colorMap[i].getRgba(),3);
			}
		else if(chunk->isChunk("BODY"))
			{
			/* Check that there was a bitmap header chunk: */
			if(!haveBmhd)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No BMHD chunk found");
			
			/* Check if there is an alpha channel: */
			imageSpec.hasAlpha=masking==Interleaved||masking==TransparentColor;
			
			/* Determine the image's color space: */
			if(numBitPlanes==24||colorMap!=0)
				{
				/* Image is a paletted or true-color RGB image: */
				setFormatSpec(RGB,imageSpec.hasAlpha);
				}
			else if(numBitPlanes==8)
				{
				/* Image is a grayscale image: */
				setFormatSpec(Grayscale,imageSpec.hasAlpha);
				}
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported image format");
			
			/* Initialize the image's channel value specs: */
			setValueSpec(UnsignedInt,8);
			
			/* Remember this chunk and bail out: */
			body=chunk;
			break;
			}
		}
	}

void ImageReaderIFF::readScanline(GLubyte* scanline,unsigned int scanlineSize)
	{
	if(compress)
		{
		/* Read a run-length encoded scanline: */
		GLubyte* scanlineEnd=scanline+scanlineSize;
		while(scanline!=scanlineEnd)
			{
			/* Read the next RLE packet: */
			int repeat=body->read<Misc::SInt8>();
			if(repeat>=0)
				{
				/* Read a literal run of pixels: */
				unsigned int runLength=repeat+1;
				body->read(scanline,runLength);
				scanline+=runLength;
				}
			else if(repeat>-128)
				{
				/* Read a repeat pixel run: */
				unsigned int runLength=-repeat+1;
				GLubyte byte=body->read<GLubyte>();
				for(unsigned int i=0;i<runLength;++i,++scanline)
					*scanline=byte;
				}
			}
		}
	else
		{
		/* Read an uncompressed scanline: */
		body->read(scanline,scanlineSize);
		}
	}

ImageReaderIFF::ImageReaderIFF(IO::File& file)
	:ImageReader(file),
	 numBitPlanes(0),masking(0x0U),compress(false),transparentColorIndex(~0x0U)
	{
	/* Open the file's root FORM chunk: */
	form=new IO::IFFChunk(&file);
	if(!form->isChunk("FORM"))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid FORM chunk");
	
	/* Read the FORM chunk type: */
	char formType[4];
	form->read(formType,4);
	if(memcmp(formType,"ILBM",4)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid FORM chunk type");
	
	/* Read the first set of image headers in the FORM chunk: */
	readImageHeaders();
	}

bool ImageReaderIFF::eof(void) const
	{
	/* Report end of file if there is no BODY chunk: */
	return body==0;
	}

BaseImage ImageReaderIFF::readImage(void)
	{
	/* Check that there is a BODY chunk: */
	if(body==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing BODY chunk");
	
	/* Create the result image: */
	const Size& size=imageSpec.rect.size;
	BaseImage result=createImage();
	
	/* Calculate the length of each image scanline in bytes: */
	unsigned int scanlineSize=((size[0]+15U)/8U)&~0x1U; // One bit per pixel, padded to even byte size
	
	/* Calculate the number of 8-pixel blocks per scanline: */
	unsigned int numBlocks=(size[0]+7U)>>3;
	unsigned int numFullBlocks=size[0]>>3;
	
	/* Allocate a scanline buffer: */
	Misc::SelfDestructArray<GLubyte> scanline(new GLubyte[scanlineSize]);
	
	/* Allocate a pixel buffer: */
	Misc::SelfDestructArray<unsigned int> pixelValues(new unsigned int[(size[0]+7U)&~0x7U]);
	
	/* Read the image one row at a time: */
	GLubyte* imageRowPtr=static_cast<GLubyte*>(result.replacePixels());
	ptrdiff_t imageRowStride=result.getRowStride();
	imageRowPtr+=ptrdiff_t(size[1]-1)*imageRowStride;
	for(unsigned int y=size[1];y>0;--y,imageRowPtr-=imageRowStride)
		{
		/* Initialize the pixel value buffer: */
		memset(pixelValues,0,size[0]*sizeof(unsigned int));
		
		/* Process all scanline bit planes: */
		unsigned int planeMask=0x1U;
		for(unsigned int plane=0;plane<numBitPlanes;++plane,planeMask<<=1)
			{
			/* Read the scanline: */
			readScanline(scanline,scanlineSize);
			
			/* Copy bits from the scanline into the pixel value buffer: */
			unsigned int* pixelValueBlock=pixelValues;
			for(unsigned int block=0;block<numBlocks;++block,pixelValueBlock+=8)
				{
				unsigned int byte(scanline[block]);
				for(int x=7;x>=0;--x,byte>>=1)
					if(byte&0x1U)
						pixelValueBlock[x]|=planeMask;
				}
			}
		
		/* Copy pixels from the pixel value buffer into the result image: */
		GLubyte* pPtr=imageRowPtr;
		unsigned int* pvPtr=pixelValues;
		if(colorMap!=0)
			{
			if(masking==TransparentColor)
				{
				/* Look up color values from the color map and check for the transparent color index: */
				for(unsigned int x=size[0];x>0;--x,pPtr+=imageSpec.numChannels,++pvPtr)
					{
					/* Look up the pixel's color in the color map and copy that into the image: */
					const ColorMapEntry& cme=colorMap[*pvPtr];
					for(int i=0;i<3;++i)
						pPtr[i]=cme[i];
					
					/* Check for the transparent color index: */
					pPtr[3]=*pvPtr==transparentColorIndex?GLubyte(0):GLubyte(255);
					}
				}
			else
				{
				/* Look up color values from the color map: */
				for(unsigned int x=size[0];x>0;--x,pPtr+=imageSpec.numChannels,++pvPtr)
					{
					/* Look up the pixel's color in the color map and copy that into the image: */
					const ColorMapEntry& cme=colorMap[*pvPtr];
					for(int i=0;i<3;++i)
						pPtr[i]=cme[i];
					}
				}
			}
		else if(numBitPlanes==24)
			{
			if(masking==TransparentColor)
				{
				/* Copy true color pixels and check for the transparent color index: */
				for(unsigned int x=size[0];x>0;--x,pPtr+=imageSpec.numChannels,++pvPtr)
					{
					/* Split the pixel value into red, green, and blue components: */
					unsigned int val=*pvPtr;
					pPtr[0]=GLubyte(val&0xffU);
					val>>=8;
					pPtr[1]=GLubyte(val&0xffU);
					val>>=8;
					pPtr[2]=GLubyte(val&0xffU);
					
					/* Check for the transparent color index: */
					pPtr[3]=*pvPtr==transparentColorIndex?GLubyte(0):GLubyte(255);
					}
				}
			else
				{
				/* Copy true color pixels: */
				for(unsigned int x=size[0];x>0;--x,pPtr+=imageSpec.numChannels,++pvPtr)
					{
					/* Split the pixel value into red, green, and blue components: */
					unsigned int val=*pvPtr;
					pPtr[0]=GLubyte(val&0xffU);
					val>>=8;
					pPtr[1]=GLubyte(val&0xffU);
					val>>=8;
					pPtr[2]=GLubyte(val&0xffU);
					}
				}
			}
		else if(numBitPlanes==8)
			{
			if(masking==TransparentColor)
				{
				/* Copy greyscale pixels and check for the transparent color index: */
				for(unsigned int x=size[0];x>0;--x,pPtr+=imageSpec.numChannels,++pvPtr)
					{
					pPtr[0]=GLubyte(*pvPtr&0xffU);
					
					/* Check for the transparent color index: */
					pPtr[1]=*pvPtr==transparentColorIndex?GLubyte(0):GLubyte(255);
					}
				}
			else
				{
				/* Copy greyscale pixels: */
				for(unsigned int x=size[0];x>0;--x,pPtr+=imageSpec.numChannels,++pvPtr)
					pPtr[0]=GLubyte(*pvPtr&0xffU);
				}
			}
		
		if(masking==Interleaved)
			{
			/* Read the masking scanline: */
			readScanline(scanline,scanlineSize);
			
			/* Copy bits from the scanline into the result pixel row's alpha channel: */
			pPtr=imageRowPtr+(imageSpec.numChannels-1);
			for(unsigned int block=0;block<numFullBlocks;++block)
				{
				unsigned int byte(scanline[block]);
				for(int x=7;x>=0;--x,byte>>=1,pPtr+=imageSpec.numChannels)
					pPtr[0]=((byte&0x1U)!=0x0U)?GLubyte(255):GLubyte(0);
				}
			
			/* Check if there is a partial pixel block at the end of the image row: */
			if(numFullBlocks<numBlocks)
				{
				unsigned int byte(scanline[numFullBlocks]);
				int x=int(size[0]&0x7U)-1;
				byte>>=7-x;
				for(;x>=0;--x,byte>>=1,pPtr+=imageSpec.numChannels)
					pPtr[0]=((byte&0x1U)!=0x0U)?GLubyte(255):GLubyte(0);
				}
			}
		}
	
	/* Read the next set of image headers: */
	colorMap.setTarget(0);
	body=0;
	readImageHeaders();
	
	return result;
	}

}

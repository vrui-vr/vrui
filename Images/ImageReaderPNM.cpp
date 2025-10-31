/***********************************************************************
ImageReaderPNM - Class to read images from files in Portable aNyMap
format.
Copyright (c) 2013-2025 Oliver Kreylos

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

#include <Images/ImageReaderPNM.h>

#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <IO/ValueSource.h>
#include <Images/BaseImage.h>

namespace Images {

namespace {

/****************
Helper functions:
****************/

inline void skipComments(IO::ValueSource& source)
	{
	/* Skip all consecutive comment indicators: */
	while(source.peekc()=='#')
		{
		/* Skip until the first non-whitespace character on the next line: */
		source.skipLine();
		source.skipWs();
		}
	}

#if 0

/***********************************************************************
The following functions are from a severely over-engineered version of
the ImageReader class and are disabled for the time being, to keep
things simple.
***********************************************************************/

template <class PixelChannelParam>
inline
void
readASCII(
	IO::ValueSource& source,
	const ImageReader::ImageSpec& imageSpec,
	const ImageReader::ImagePlane imagePlanes[])
	{
	/* Convert image strides to multiples of pixel channel type: */
	ptrdiff_t pixelStrides[3],rowStrides[3];
	PixelChannelParam* rowPtrs[3];
	for(unsigned int i=0;i<imageSpec.numChannels;++i)
		{
		pixelStrides[i]=imagePlanes[i].pixelStride/sizeof(PixelChannelParam);
		rowStrides[i]=imagePlanes[i].rowStride/sizeof(PixelChannelParam);
		rowPtrs[i]=static_cast<PixelChannelParam*>(imagePlanes[i].basePtr);
		
		/* Flip the image vertically: */
		rowPtrs[i]+=(imageSpec.size[1]-1)*rowStrides[i];
		}
	
	/* Read the image one row at a time: */
	for(unsigned int y=0;y<imageSpec.size[1];++y)
		{
		/* Read the row one pixel at a time: */
		PixelChannelParam* pPtrs[3];
		for(unsigned int i=0;i<imageSpec.numChannels;++i)
			pPtrs[i]=rowPtrs[i];
		for(unsigned int x=0;x<imageSpec.size[0];++x)
			{
			for(unsigned int i=0;i<imageSpec.numChannels;++i)
				{
				/* Read the next pixel channel value from the ASCII file: */
				skipComments(source);
				*(pPtrs[i])=source.readUnsignedInteger();
				
				/* Go to the next pixel in this channel: */
				pPtrs[i]+=pixelStrides[i];
				}
			}
		
		/* Go to the next pixel rows for each channel (decrement pointers to flip image vertically): */
		for(unsigned int i=0;i<imageSpec.numChannels;++i)
			rowPtrs[i]-=rowStrides[i];
		}
	}

template <class PixelChannelParam>
inline
void
readBinary(
	IO::FilePtr file,
	const ImageReader::ImageSpec& imageSpec,
	const ImageReader::ImagePlane imagePlanes[])
	{
	/* Convert image strides to multiples of pixel channel type: */
	ptrdiff_t pixelStrides[3],rowStrides[3];
	PixelChannelParam* rowPtrs[3];
	for(unsigned int i=0;i<imageSpec.numChannels;++i)
		{
		pixelStrides[i]=imagePlanes[i].pixelStride/sizeof(PixelChannelParam);
		rowStrides[i]=imagePlanes[i].rowStride/sizeof(PixelChannelParam);
		rowPtrs[i]=static_cast<PixelChannelParam*>(imagePlanes[i].basePtr);
		
		/* Flip the image vertically: */
		rowPtrs[i]+=(imageSpec.size[1]-1)*rowStrides[i];
		}
	
	/* Read the image one row at a time: */
	for(unsigned int y=0;y<imageSpec.size[1];++y)
		{
		/* Read the row one pixel at a time: */
		PixelChannelParam* pPtrs[3];
		for(unsigned int i=0;i<imageSpec.numChannels;++i)
			pPtrs[i]=rowPtrs[i];
		for(unsigned int i=imageSpec.numChannels;i<3;++i)
			pPtrs[i]=0;
		for(unsigned int x=0;x<imageSpec.size[0];++x)
			{
			for(unsigned int i=0;i<imageSpec.numChannels;++i)
				{
				/* Read the next pixel channel value from the binary file: */
				*(pPtrs[i])=file->read<PixelChannelParam>();
				
				/* Go to the next pixel in this channel: */
				pPtrs[i]+=pixelStrides[i];
				}
			}
		
		/* Go to the next pixel rows for each channel (decrement pointers to flip image vertically): */
		for(unsigned int i=0;i<imageSpec.numChannels;++i)
			rowPtrs[i]-=rowStrides[i];
		}
	}

#endif

}

/*******************************
Methods of class ImageReaderPNM:
*******************************/

ImageReaderPNM::ImageReaderPNM(IO::File& sFile)
	:ImageReader(sFile),
	 endianness(Misc::BigEndian),
	 done(false)
	{
	/* Attach a value source to the file to read the ASCII file header: */
	IO::ValueSource header(file);
	header.skipWs();
	
	/* Read the magic field including the image type indicator: */
	int magic=header.getChar();
	imageType=header.getChar();
	if(magic!='P'||((imageType<'1'||imageType>'6')&&imageType!='f'&&imageType!='F'))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid PNM header");
	header.skipWs();
	
	/* Read the image width, height, and maximal pixel component value or scale factor: */
	skipComments(header);
	imageSpec.rect.size[0]=header.readUnsignedInteger();
	if(imageType=='1'||imageType=='4') // PBM files don't have the maxValue field
		{
		skipComments(header);
		header.setWhitespace(""); // Disable all whitespace to read the last header field
		imageSpec.rect.size[1]=header.readUnsignedInteger();
		maxValue=1;
		}
	else
		{
		skipComments(header);
		imageSpec.rect.size[1]=header.readUnsignedInteger();
		
		skipComments(header);
		header.setWhitespace(""); // Disable all whitespace to read the last header field
		
		if(imageType=='F'||imageType=='f')
			{
			/* Read the pixel value scale factor for floating-point images: */
			scale=header.readNumber();
			
			/* A negative scale factor indicates little-endian byte layout: */
			if(scale<0.0f)
				{
				scale=-scale;
				endianness=Misc::LittleEndian;
				}
			}
		else
			{
			/* Read the maximum pixel value field for integer images: */
			maxValue=header.readUnsignedInteger();
			}
		}
	
	/* Read the single (whitespace) character separating the header from the image data: */
	header.getChar();
	
	/* Fill in the rest of the image specification: */
	canvasSize=imageSpec.rect.size;
	imageSpec.rect.offset=Offset(0,0);
	switch(imageType)
		{
		case '1': // ASCII bitmap
		case '4': // Binary bitmap
			setFormatSpec(Grayscale,false);
			setValueSpec(UnsignedInt,8U);
			break;
		
		case '2': // ASCII grayscale image
		case '5': // Binary grayscale image
			setFormatSpec(Grayscale,false);
			setValueSpec(UnsignedInt,maxValue<256U?8U:16U);
			break;
		
		case '3': // ASCII RGB image
		case '6': // Binary RGB image
			setFormatSpec(RGB,false);
			setValueSpec(UnsignedInt,maxValue<256U?8U:16U);
			break;
		
		case 'f': // Floating-point grayscale image
			setFormatSpec(Grayscale,false);
			setValueSpec(Float,32U);
			break;
		
		case 'F': // Floating-point RGB image
			setFormatSpec(RGB,false);
			setValueSpec(Float,32U);
			break;
		}
	}

bool ImageReaderPNM::eof(void) const
	{
	return done;
	}

#if 0

/***********************************************************************
The following functions are from a severely over-engineered version of
the ImageReader class and are disabled for the time being, to keep
things simple.
***********************************************************************/

void ImageReaderPNM::readNative(ImageReader::ImagePlane imagePlanes[])
	{
	switch(imageType)
		{
		case '1': // ASCII bitmap
			{
			/* Create a value source to parse ASCII pixel values: */
			IO::ValueSource image(file);
			image.skipWs();
			
			/* Read the image one row at a time: */
			Misc::UInt8* rowPtr=static_cast<Misc::UInt8*>(imagePlanes[0].basePtr);
			for(unsigned int y=0;y<imageSpec.size[1];++y,rowPtr+=imagePlanes[0].rowStride)
				{
				/* Read the row one pixel at a time: */
				Misc::UInt8* pPtr=rowPtr;
				for(unsigned int x=0;x<imageSpec.size[0];++x,pPtr+=imagePlanes[0].pixelStride)
					;
				}
			
			break;
			}
		
		case '2': // ASCII grayscale image
		case '3': // ASCII RGB image
			{
			/* Create a value source to parse ASCII pixel values: */
			IO::ValueSource image(file);
			image.skipWs();
			
			/* Determine the native pixel size: */
			if(maxValue<256U)
				{
				/* Read 8-bit pixels: */
				readASCII<Misc::UInt8>(image,imageSpec,imagePlanes);
				}
			else
				{
				/* Read 16-bit pixels: */
				readASCII<Misc::UInt16>(image,imageSpec,imagePlanes);
				}
			break;
			}
		
		case '4': // Binary bitmap
			{
			/* Read the image one row at a time: */
			Misc::UInt8* rowPtr=static_cast<Misc::UInt8*>(imagePlanes[0].basePtr);
			for(unsigned int y=0;y<imageSpec.size[1];++y,rowPtr+=imagePlanes[0].rowStride)
				{
				/* Read the row one pixel at a time: */
				Misc::UInt8* pPtr=rowPtr;
				for(unsigned int x=0;x<imageSpec.size[0];++x,pPtr+=imagePlanes[0].pixelStride)
					;
				}
			break;
			}
		
		case '5': // Binary grayscale image
		case '6': // Binary RGB image
			{
			/* Determine the native pixel size: */
			if(maxValue<256U)
				{
				/* Read 8-bit pixels: */
				readBinary<Misc::UInt8>(file,imageSpec,imagePlanes);
				}
			else
				{
				/* Read 16-bit pixels: */
				readBinary<Misc::UInt16>(file,imageSpec,imagePlanes);
				}
			break;
			}
		
		}
	
	/* There can be only one image in a PNM file: */
	done=true;
	}

#endif

namespace {

/****************
Helper functions:
****************/

template <class DestScalarParam>
inline
void
readAsciiImage(
	IO::File& file,
	const Size& size,
	unsigned int numChannels,
	DestScalarParam* pixels)
	{
	/* Create a value source to parse ASCII pixel values: */
	IO::ValueSource image(&file);
	image.skipWs();
	
	/* Read the image by rows in top-down order: */
	ptrdiff_t rowStride=size[0]*numChannels;
	DestScalarParam* rowPtr=pixels+(size[1]-1)*rowStride;
	for(unsigned int y=size[1];y>0;--y,rowPtr-=rowStride)
		{
		/* Read the row of pixel values from the source: */
		DestScalarParam* pPtr=rowPtr;
		for(unsigned int x=size[0]*numChannels;x>0;--x,++pPtr)
			*pPtr=DestScalarParam(image.readUnsignedInteger());
		}
	}

template <class DestScalarParam>
inline
void
readBinaryImage(
	IO::File& file,
	const Size& size,
	unsigned int numChannels,
	DestScalarParam* pixels)
	{
	/* Read the image by rows in top-down order: */
	ptrdiff_t rowStride=size[0]*numChannels;
	DestScalarParam* rowPtr=pixels+(size[1]-1)*rowStride;
	for(unsigned int y=size[1];y>0;--y,rowPtr-=rowStride)
		{
		/* Read the row of pixel values from the source: */
		file.read(rowPtr,size[0]*numChannels);
		}
	}

}

BaseImage ImageReaderPNM::readImage(void)
	{
	/* Read the image: */
	const Size& size=imageSpec.rect.size;
	BaseImage result=createImage();
	switch(imageType)
		{
		case '1': // ASCII bitmap image
			{
			/* Create a value source to parse ASCII pixel values: */
			IO::ValueSource image(file);
			image.skipWs();
			
			/* Read each row of the image file: */
			ptrdiff_t rowStride=size[0];
			GLubyte* rowPtr=static_cast<GLubyte*>(result.replacePixels())+(size[1]-1)*rowStride;
			for(unsigned int y=size[1];y>0;--y,rowPtr-=rowStride)
				{
				GLubyte* pPtr=rowPtr;
				for(unsigned int x=size[0];x>0;--x,++pPtr)
					*pPtr=GLubyte(image.readUnsignedInteger()!=0U?255U:0U);
				}
			
			break;
			}
		
		case '2': // ASCII grayscale image
		case '3': // ASCII RGB color image
			if(imageSpec.numFieldBytes==2)
				readAsciiImage(*file,size,imageSpec.numChannels,static_cast<GLushort*>(result.replacePixels()));
			else
				readAsciiImage(*file,size,imageSpec.numChannels,static_cast<GLubyte*>(result.replacePixels()));
			
			break;
		
		case '4': // Binary bitmap image
			{
			/* Allocate a row buffer: */
			unsigned int rawWidth=(size[0]+7)>>3;
			Misc::SelfDestructArray<Misc::UInt8> tempRow(rawWidth);
			
			/* Read each row of the image file: */
			ptrdiff_t rowStride=size[0];
			GLubyte* rowPtr=static_cast<GLubyte*>(result.replacePixels())+(size[1]-1)*rowStride;
			for(unsigned int y=size[1];y>0;--y,rowPtr-=rowStride)
				{
				/* Read the source image row: */
				file->read(tempRow.getArray(),rawWidth);
				
				/* Convert pixel values: */
				Misc::UInt8* tempRowPtr=tempRow;
				GLubyte* pPtr=rowPtr;
				for(unsigned int x=size[0];x>0;++tempRowPtr)
					for(Misc::UInt8 mask=0x80U;mask!=0x0U&&x>0;mask>>=1,--x,++pPtr)
						*pPtr=GLubyte(((*tempRowPtr)&mask)!=0x0U?255U:0U);
				}
			
			break;
			}
		
		case '5': // Binary grayscale image
		case '6': // Binary RGB color image
			/* Set the image's endianness: */
			file->setEndianness(endianness);
			
			if(imageSpec.numFieldBytes==2)
				readBinaryImage(*file,size,imageSpec.numChannels,static_cast<GLushort*>(result.replacePixels()));
			else
				readBinaryImage(*file,size,imageSpec.numChannels,static_cast<GLubyte*>(result.replacePixels()));
			
			break;
		
		case 'f': // Floating-point grayscale image
		case 'F': // Floating-point RGB image
			{
			/* Set the image's endianness: */
			file->setEndianness(endianness);
			
			GLfloat* pixels=static_cast<GLfloat*>(result.replacePixels());
			readBinaryImage(*file,size,imageSpec.numChannels,pixels);
			
			/* Scale each pixel by the pixel scale factor: */
			if(scale!=1.0f)
				{
				GLfloat* cEnd=pixels+size_t(size[1])*size_t(size[0])*size_t(imageSpec.numChannels);
				for(GLfloat* cPtr=pixels;cPtr!=cEnd;++cPtr)
					*cPtr*=scale;
				}
			
			break;
			}
		}
	
	/* There can be only one image in a PNM file: */
	done=true;
	
	/* Return the result image: */
	return result;
	}

}

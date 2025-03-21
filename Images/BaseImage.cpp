/***********************************************************************
BaseImage - Generic base class to represent images of arbitrary pixel
formats. The image coordinate system is such that pixel (0,0) is in the
lower-left corner.
Copyright (c) 2016-2024 Oliver Kreylos

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

#include <Images/BaseImage.h>

#include <stddef.h>
#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <IO/File.h>
#include <Math/Math.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTTextureSRGB.h>

namespace Images {

/***********************************************
Methods of class BaseImage::ImageRepresentation:
***********************************************/

BaseImage::ImageRepresentation::ImageRepresentation(const Size& sSize,unsigned int sNumChannels,unsigned int sChannelSize,GLenum sFormat,GLenum sScalarType)
	:refCount(1U),
	 size(sSize),
	 numChannels(sNumChannels),channelSize(sChannelSize),
	 image(0),
	 format(sFormat),scalarType(sScalarType)
	{
	/* Allocate the image array: */
	image=malloc(size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize));
	}

BaseImage::ImageRepresentation::ImageRepresentation(const ImageRepresentation& source)
	:refCount(1U),
	 size(source.size),
	 numChannels(source.numChannels),channelSize(source.channelSize),
	 image(0),
	 format(source.format),scalarType(source.scalarType)
	{
	/* Allocate the image array: */
	image=malloc(size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize));
	
	/* Copy the source image byte-by-byte: */
	memcpy(image,source.image,size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize));
	}

BaseImage::ImageRepresentation::~ImageRepresentation(void)
	{
	/* Destroy the allocated image array: */
	free(image);
	}

namespace {

/***************************************************************************
Generic function to convert color components between supported scalar types:
***************************************************************************/

template <class DestScalarParam,class SourceScalarParam>
DestScalarParam convertColorScalar(SourceScalarParam value);

/*********************
Conversions to GLbyte:
*********************/

template <>
inline GLbyte convertColorScalar<GLbyte,GLbyte>(GLbyte value)
	{
	return value;
	}

template <>
inline GLbyte convertColorScalar<GLbyte,GLubyte>(GLubyte value)
	{
	return GLbyte(value>>1);
	}

template <>
inline GLbyte convertColorScalar<GLbyte,GLshort>(GLshort value)
	{
	return GLbyte(value>>8);
	}

template <>
inline GLbyte convertColorScalar<GLbyte,GLushort>(GLushort value)
	{
	return GLbyte(value>>9);
	}

template <>
inline GLbyte convertColorScalar<GLbyte,GLint>(GLint value)
	{
	return GLbyte(value>>24);
	}

template <>
inline GLbyte convertColorScalar<GLbyte,GLuint>(GLuint value)
	{
	return GLbyte(value>>25);
	}

template <>
inline GLbyte convertColorScalar<GLbyte,GLfloat>(GLfloat value)
	{
	if(value<0.0f)
		return GLbyte(0);
	else if(value>=1.0f)
		return GLbyte(127);
	else
		return GLbyte(value*128.0f);
	}

template <>
inline GLbyte convertColorScalar<GLbyte,GLdouble>(GLdouble value)
	{
	if(value<0.0)
		return GLbyte(0);
	else if(value>=1.0)
		return GLbyte(127);
	else
		return GLbyte(value*128.0);
	}

/**********************
Conversions to GLubyte:
**********************/

template <>
inline GLubyte convertColorScalar<GLubyte,GLbyte>(GLbyte value)
	{
	if(value<GLbyte(0))
		return GLubyte(0);
	else
		{
		GLubyte v(value);
		return (v<<1)|(v>>6);
		}
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLubyte>(GLubyte value)
	{
	return value;
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLshort>(GLshort value)
	{
	if(value<GLshort(0))
		return GLubyte(0);
	else
		return GLubyte(value>>7);
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLushort>(GLushort value)
	{
	return GLubyte(value>>8);
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLint>(GLint value)
	{
	if(value<GLint(0))
		return GLubyte(0);
	else
		return GLubyte(value>>23);
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLuint>(GLuint value)
	{
	return GLubyte(value>>24);
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLfloat>(GLfloat value)
	{
	if(value<0.0f)
		return GLubyte(0);
	else if(value>=1.0f)
		return GLubyte(255);
	else
		return GLubyte(value*256.0f);
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLdouble>(GLdouble value)
	{
	if(value<0.0)
		return GLubyte(0);
	else if(value>=1.0)
		return GLubyte(255);
	else
		return GLubyte(value*256.0);
	}

/**********************
Conversions to GLshort:
**********************/

template <>
inline GLshort convertColorScalar<GLshort,GLbyte>(GLbyte value)
	{
	GLshort v(value);
	return (v<<8)|v;
	}

template <>
inline GLshort convertColorScalar<GLshort,GLubyte>(GLubyte value)
	{
	GLshort v(value);
	return (v<<7)|(v>>1);
	}

template <>
inline GLshort convertColorScalar<GLshort,GLshort>(GLshort value)
	{
	return value;
	}

template <>
inline GLshort convertColorScalar<GLshort,GLushort>(GLushort value)
	{
	return GLshort(value>>1);
	}

template <>
inline GLshort convertColorScalar<GLshort,GLint>(GLint value)
	{
	return GLshort(value>>16);
	}

template <>
inline GLshort convertColorScalar<GLshort,GLuint>(GLuint value)
	{
	return GLshort(value>>17);
	}

template <>
inline GLshort convertColorScalar<GLshort,GLfloat>(GLfloat value)
	{
	if(value<0.0f)
		return GLshort(0);
	else if(value>=1.0f)
		return GLshort(32767);
	else
		return GLshort(value*32768.0f);
	}

template <>
inline GLshort convertColorScalar<GLshort,GLdouble>(GLdouble value)
	{
	if(value<0.0)
		return GLshort(0);
	else if(value>=1.0)
		return GLshort(32767);
	else
		return GLshort(value*32768.0);
	}

/**********************
Conversions to GLushort:
**********************/

template <>
inline GLushort convertColorScalar<GLushort,GLbyte>(GLbyte value)
	{
	if(value<GLbyte(0))
		return GLushort(0);
	else
		{
		GLushort v(value);
		return (v<<9)|(v<<2)|(v>>5);
		}
	}

template <>
inline GLushort convertColorScalar<GLushort,GLubyte>(GLubyte value)
	{
	GLushort v(value);
	return (v<<8)|v;
	}

template <>
inline GLushort convertColorScalar<GLushort,GLshort>(GLshort value)
	{
	if(value<GLshort(0))
		return GLushort(0);
	else
		{
		GLushort v(value);
		return (v<<1)|(v>>14);
		}
	}

template <>
inline GLushort convertColorScalar<GLushort,GLushort>(GLushort value)
	{
	return value;
	}

template <>
inline GLushort convertColorScalar<GLushort,GLint>(GLint value)
	{
	if(value<GLint(0))
		return GLushort(0);
	else
		return GLushort(value>>15);
	}

template <>
inline GLushort convertColorScalar<GLushort,GLuint>(GLuint value)
	{
	return GLushort(value>>16);
	}

template <>
inline GLushort convertColorScalar<GLushort,GLfloat>(GLfloat value)
	{
	if(value<0.0f)
		return GLushort(0);
	else if(value>=1.0f)
		return GLushort(65535);
	else
		return GLushort(value*65536.0f);
	}

template <>
inline GLushort convertColorScalar<GLushort,GLdouble>(GLdouble value)
	{
	if(value<0.0)
		return GLushort(0);
	else if(value>=1.0)
		return GLushort(65535);
	else
		return GLushort(value*65536.0);
	}

/*******************************************************
Helper classes and functions for basic image operations:
*******************************************************/

template <class SourceScalarParam>
inline
void
toUInt8Typed(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Convert all samples from the source image to the destination image: */
	GLubyte* dPtr=static_cast<GLubyte*>(dest.replacePixels());
	const SourceScalarParam* sPtr=static_cast<const SourceScalarParam*>(source.getPixels());
	size_t numSamples=size_t(source.getHeight())*size_t(source.getWidth())*size_t(source.getNumChannels());
	for(size_t i=numSamples;i>0;--i,++dPtr,++sPtr)
		*dPtr=convertColorScalar<GLubyte,SourceScalarParam>(*sPtr);
	}

template <class ScalarParam>
inline
void
dropAlphaTyped(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Drop the alpha value of all pixels: */
	unsigned int numChannels=dest.getNumChannels();
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i)
		{
		/* Copy the non-alpha channels: */
		for(size_t j=0;j<numChannels;++j)
			*(dPtr++)=*(sPtr++);
		
		/* Skip the source's alpha channel: */
		++sPtr;
		}
	}

void dropAlphaImpl(const BaseImage& source,BaseImage& dest)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			dropAlphaTyped<signed char>(source,dest);
			break;
		
		case GL_UNSIGNED_BYTE:
			dropAlphaTyped<unsigned char>(source,dest);
			break;
		
		case GL_SHORT:
			dropAlphaTyped<signed short>(source,dest);
			break;
		
		case GL_UNSIGNED_SHORT:
			dropAlphaTyped<unsigned short>(source,dest);
			break;
		
		case GL_INT:
			dropAlphaTyped<signed int>(source,dest);
			break;
		
		case GL_UNSIGNED_INT:
			dropAlphaTyped<unsigned int>(source,dest);
			break;
		
		case GL_FLOAT:
			dropAlphaTyped<float>(source,dest);
			break;
		
		case GL_DOUBLE:
			dropAlphaTyped<double>(source,dest);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
		}
	}

template <class ScalarParam>
inline
void
addAlphaTyped(
	const BaseImage& source,
	BaseImage& dest,
	ScalarParam alpha)
	{
	/* Add the constant alpha value to all pixels: */
	unsigned int numChannels=source.getNumChannels();
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i)
		{
		/* Copy the non-alpha channels: */
		for(size_t j=0;j<numChannels;++j)
			*(dPtr++)=*(sPtr++);
		
		/* Add an alpha value to the destination: */
		*(dPtr++)=alpha;
		}
	}

void addAlphaImpl(const BaseImage& source,BaseImage& dest,double alpha)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			addAlphaTyped<signed char>(source,dest,(signed char)(Math::clamp(Math::floor(alpha*128.0),0.0,127.0)));
			break;
		
		case GL_UNSIGNED_BYTE:
			addAlphaTyped<unsigned char>(source,dest,(unsigned char)(Math::clamp(Math::floor(alpha*256.0),0.0,255.0)));
			break;
		
		case GL_SHORT:
			addAlphaTyped<signed short>(source,dest,(signed short)(Math::clamp(Math::floor(alpha*32768.0),0.0,32767.0)));
			break;
		
		case GL_UNSIGNED_SHORT:
			addAlphaTyped<unsigned short>(source,dest,(unsigned short)(Math::clamp(Math::floor(alpha*65536.0),0.0,65535.0)));
			break;
		
		case GL_INT:
			addAlphaTyped<signed int>(source,dest,(signed int)(Math::clamp(Math::floor(alpha*2147483648.0),0.0,2147483647.0)));
			break;
		
		case GL_UNSIGNED_INT:
			addAlphaTyped<unsigned int>(source,dest,(unsigned int)(Math::clamp(Math::floor(alpha*4294967296.0),0.0,4294967295.0)));
			break;
		
		case GL_FLOAT:
			addAlphaTyped<float>(source,dest,float(alpha));
			break;
		
		case GL_DOUBLE:
			addAlphaTyped<double>(source,dest,alpha);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
		}
	}

template <class ScalarParam,class WeightParam>
inline
void
toGreyTypedInt(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Convert all pixels to luminance and retain an existing alpha channel: */
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	if(source.getNumChannels()==4)
		{
		/* Convert RGBA to LUMINANCE_ALPHA: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=4)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=ScalarParam((WeightParam(sPtr[0])*WeightParam(77)+WeightParam(sPtr[1])*WeightParam(150)+WeightParam(sPtr[2])*WeightParam(29))>>WeightParam(8));
			
			/* Copy alpha channel: */
			*(dPtr++)=sPtr[3];
			}
		}
	else
		{
		/* Convert RGB to LUMINANCE: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=3)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=ScalarParam((WeightParam(sPtr[0])*WeightParam(77)+WeightParam(sPtr[1])*WeightParam(150)+WeightParam(sPtr[2])*WeightParam(29))>>WeightParam(8));
			}
		}
	}

template <class ScalarParam>
inline
void
toGreyTypedFloat(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Convert all pixels to luminance and retain an existing alpha channel: */
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	if(source.getNumChannels()==4)
		{
		/* Convert RGBA to LUMINANCE_ALPHA: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=4)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=sPtr[0]*ScalarParam(0.299)+sPtr[1]*ScalarParam(0.587)+sPtr[2]*ScalarParam(0.114);
			
			/* Copy alpha channel: */
			*(dPtr++)=sPtr[3];
			}
		}
	else
		{
		/* Convert RGB to LUMINANCE: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=3)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=sPtr[0]*ScalarParam(0.299)+sPtr[1]*ScalarParam(0.587)+sPtr[2]*ScalarParam(0.114);
			}
		}
	}

void toGreyImpl(const BaseImage& source,BaseImage& dest)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			toGreyTypedInt<signed char,signed short>(source,dest);
			break;
		
		case GL_UNSIGNED_BYTE:
			toGreyTypedInt<unsigned char,unsigned short>(source,dest);
			break;
		
		case GL_SHORT:
			toGreyTypedInt<signed short,signed int>(source,dest);
			break;
		
		case GL_UNSIGNED_SHORT:
			toGreyTypedInt<unsigned short,unsigned int>(source,dest);
			break;
		
		case GL_INT:
			toGreyTypedInt<signed int,signed long>(source,dest);
			break;
		
		case GL_UNSIGNED_INT:
			toGreyTypedInt<unsigned int,unsigned long>(source,dest);
			break;
		
		case GL_FLOAT:
			toGreyTypedFloat<float>(source,dest);
			break;
		
		case GL_DOUBLE:
			toGreyTypedFloat<double>(source,dest);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
		}
	}

template <class ScalarParam>
inline
void
toRgbTyped(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Convert all pixels to RGB and retain an existing alpha channel: */
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	if(source.getNumChannels()==2)
		{
		/* Convert LUMINANCE_ALPHA to RGBA: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=2)
			{
			/* Copy pixel luminance: */
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			
			/* Copy alpha channel: */
			*(dPtr++)=sPtr[1];
			}
		}
	else
		{
		/* Convert LUMINANCE to RGB: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,++sPtr)
			{
			/* Copy pixel luminance: */
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			}
		}
	}

void toRgbImpl(const BaseImage& source,BaseImage& dest)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			toRgbTyped<signed char>(source,dest);
			break;
		
		case GL_UNSIGNED_BYTE:
			toRgbTyped<unsigned char>(source,dest);
			break;
		
		case GL_SHORT:
			toRgbTyped<signed short>(source,dest);
			break;
		
		case GL_UNSIGNED_SHORT:
			toRgbTyped<unsigned short>(source,dest);
			break;
		
		case GL_INT:
			toRgbTyped<signed int>(source,dest);
			break;
		
		case GL_UNSIGNED_INT:
			toRgbTyped<unsigned int>(source,dest);
			break;
		
		case GL_FLOAT:
			toRgbTyped<float>(source,dest);
			break;
		
		case GL_DOUBLE:
			toRgbTyped<double>(source,dest);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
		}
	}

template <class ScalarParam,class AccumParam>
inline
void
shrinkTypedInt(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Average all blocks of 2x2 pixels in the source image: */
	unsigned int nc=source.getNumChannels();
	const ScalarParam* sRow0Ptr=static_cast<const ScalarParam*>(source.getPixels());
	const ptrdiff_t sStride=source.getSize(0)*nc;
	const ScalarParam* sRow1Ptr=sRow0Ptr+sStride;
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(unsigned int y=0;y<source.getSize(1);y+=2,sRow0Ptr+=sStride*2,sRow1Ptr+=sStride*2)
		{
		const ScalarParam* s0Ptr=sRow0Ptr;
		const ScalarParam* s1Ptr=sRow1Ptr;
		for(unsigned int x=0;x<source.getSize(0);x+=2,s0Ptr+=nc,s1Ptr+=nc)
			{
			for(unsigned int i=0;i<source.getNumChannels();++i,++s0Ptr,++s1Ptr,++dPtr)
				{
				/* Average the current 2x2 pixel block: */
				AccumParam sum0=AccumParam(s0Ptr[0])+AccumParam(s0Ptr[nc]);
				AccumParam sum1=AccumParam(s1Ptr[0])+AccumParam(s1Ptr[nc]);
				*dPtr=ScalarParam((sum0+sum1+2)>>2);
				}
			}
		}
	}

template <class ScalarParam>
inline
void
shrinkTypedFloat(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Average all blocks of 2x2 pixels in the source image: */
	unsigned int nc=source.getNumChannels();
	const ScalarParam* sRow0Ptr=static_cast<const ScalarParam*>(source.getPixels());
	const ptrdiff_t sStride=source.getSize(0)*nc;
	const ScalarParam* sRow1Ptr=sRow0Ptr+sStride;
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(unsigned int y=0;y<source.getSize(1);y+=2,sRow0Ptr+=sStride*2,sRow1Ptr+=sStride*2)
		{
		const ScalarParam* s0Ptr=sRow0Ptr;
		const ScalarParam* s1Ptr=sRow1Ptr;
		for(unsigned int x=0;x<source.getSize(0);x+=2,s0Ptr+=nc,s1Ptr+=nc)
			{
			for(unsigned int i=0;i<source.getNumChannels();++i,++s0Ptr,++s1Ptr,++dPtr)
				{
				/* Average the current 2x2 pixel block: */
				*dPtr=(s0Ptr[0]+s0Ptr[nc]+s1Ptr[0]+s1Ptr[nc])*ScalarParam(0.25);
				}
			}
		}
	}

}

/**********************************
Static elements of class BaseImage:
**********************************/

bool BaseImage::useGammaCorrection=false;

/**************************
Methods of class BaseImage:
**************************/

void BaseImage::setUseGammaCorrection(bool newUseGammaCorrection)
	{
	useGammaCorrection=newUseGammaCorrection;
	}

BaseImage::BaseImage(IO::File& imageFile)
	:rep(0)
	{
	/* Read the image's format: */
	Size size;
	for(int i=0;i<2;++i)
		size[i]=(unsigned int)(imageFile.read<Misc::UInt32>());
	unsigned int numChannels=(unsigned int)(imageFile.read<Misc::UInt8>());
	unsigned int channelSize=(unsigned int)(imageFile.read<Misc::UInt8>());
	GLenum format=GLenum(imageFile.read<Misc::UInt32>());
	GLenum scalarType=GLenum(imageFile.read<Misc::UInt32>());
	
	/* Create an image representation: */
	rep=new ImageRepresentation(size,numChannels,channelSize,format,scalarType);
	
	/* Read the image's pixels: */
	size_t numComponents=size_t(size[1])*size_t(size[0])*size_t(numChannels);
	switch(scalarType)
		{
		case GL_BYTE:
			imageFile.read(static_cast<signed char*>(rep->image),numComponents);
			break;
		
		case GL_UNSIGNED_BYTE:
			imageFile.read(static_cast<unsigned char*>(rep->image),numComponents);
			break;
		
		case GL_SHORT:
			imageFile.read(static_cast<signed short*>(rep->image),numComponents);
			break;
		
		case GL_UNSIGNED_SHORT:
			imageFile.read(static_cast<unsigned short*>(rep->image),numComponents);
			break;
		
		case GL_INT:
			imageFile.read(static_cast<signed int*>(rep->image),numComponents);
			break;
		
		case GL_UNSIGNED_INT:
			imageFile.read(static_cast<unsigned int*>(rep->image),numComponents);
			break;
		
		case GL_FLOAT:
			imageFile.read(static_cast<float*>(rep->image),numComponents);
			break;
		
		case GL_DOUBLE:
			imageFile.read(static_cast<double*>(rep->image),numComponents);
			break;
		
		default:
			rep->detach();
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image file has unsupported pixel format");
		}
	}

void BaseImage::write(IO::File& imageFile) const
	{
	/* Write the image's format: */
	for(int i=0;i<2;++i)
		imageFile.write(Misc::UInt32(rep->size[i]));
	imageFile.write(Misc::UInt8(rep->numChannels));
	imageFile.write(Misc::UInt8(rep->channelSize));
	imageFile.write(Misc::UInt32(rep->format));
	imageFile.write(Misc::UInt32(rep->scalarType));
	
	/* Write the image's pixels: */
	size_t numComponents=size_t(rep->size[1])*size_t(rep->size[0])*size_t(rep->numChannels);
	switch(rep->scalarType)
		{
		case GL_BYTE:
			imageFile.write(static_cast<const signed char*>(rep->image),numComponents);
			break;
		
		case GL_UNSIGNED_BYTE:
			imageFile.write(static_cast<const unsigned char*>(rep->image),numComponents);
			break;
		
		case GL_SHORT:
			imageFile.write(static_cast<const signed short*>(rep->image),numComponents);
			break;
		
		case GL_UNSIGNED_SHORT:
			imageFile.write(static_cast<const unsigned short*>(rep->image),numComponents);
			break;
		
		case GL_INT:
			imageFile.write(static_cast<const signed int*>(rep->image),numComponents);
			break;
		
		case GL_UNSIGNED_INT:
			imageFile.write(static_cast<const unsigned int*>(rep->image),numComponents);
			break;
		
		case GL_FLOAT:
			imageFile.write(static_cast<const float*>(rep->image),numComponents);
			break;
		
		case GL_DOUBLE:
			imageFile.write(static_cast<const double*>(rep->image),numComponents);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image file has unsupported pixel format");
		}
	}

BaseImage BaseImage::toUInt8(void) const
	{
	/* Return the image itself if its scalar type is already GL_UNSIGNED_BYTE: */
	if(rep->scalarType==GL_UNSIGNED_BYTE)
		return *this;
	
	/* Process the image based on its format: */
	BaseImage result(rep->size,rep->numChannels,1,rep->format,GL_UNSIGNED_BYTE);
	switch(rep->scalarType)
		{
		case GL_BYTE:
			toUInt8Typed<GLbyte>(*this,result);
			break;
		
		case GL_SHORT:
			toUInt8Typed<GLshort>(*this,result);
			break;
		
		case GL_UNSIGNED_SHORT:
			toUInt8Typed<GLushort>(*this,result);
			break;
		
		case GL_INT:
			toUInt8Typed<GLint>(*this,result);
			break;
		
		case GL_UNSIGNED_INT:
			toUInt8Typed<GLuint>(*this,result);
			break;
		
		case GL_FLOAT:
			toUInt8Typed<GLfloat>(*this,result);
			break;
		
		case GL_DOUBLE:
			toUInt8Typed<GLdouble>(*this,result);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image file has unsupported pixel format");
		}
	
	return result;
	}

BaseImage BaseImage::dropAlpha(void) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_LUMINANCE_ALPHA)
		{
		/* Drop the alpha channel to create a luminance-only image: */
		BaseImage result(rep->size,rep->numChannels-1,rep->channelSize,GL_LUMINANCE,rep->scalarType);
		dropAlphaImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_RGBA)
		{
		/* Drop the alpha channel to create an RGB image: */
		BaseImage result(rep->size,rep->numChannels-1,rep->channelSize,GL_RGB,rep->scalarType);
		dropAlphaImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE||rep->format==GL_RGB)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
	}

BaseImage BaseImage::addAlpha(double alpha) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_LUMINANCE)
		{
		/* Add an alpha channel to create a luminance-alpha image: */
		BaseImage result(rep->size,rep->numChannels+1,rep->channelSize,GL_LUMINANCE_ALPHA,rep->scalarType);
		addAlphaImpl(*this,result,alpha);
		
		return result;
		}
	else if(rep->format==GL_RGB)
		{
		/* Add an alpha channel to create an RGBA image: */
		BaseImage result(rep->size,rep->numChannels+1,rep->channelSize,GL_RGBA,rep->scalarType);
		addAlphaImpl(*this,result,alpha);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE_ALPHA||rep->format==GL_RGBA)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
	}

BaseImage BaseImage::toGrey(void) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_RGB)
		{
		/* Convert RGB to luminance: */
		BaseImage result(rep->size,1,rep->channelSize,GL_LUMINANCE,rep->scalarType);
		toGreyImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_RGBA)
		{
		/* Convert RGBA to luminance-alpha: */
		BaseImage result(rep->size,2,rep->channelSize,GL_LUMINANCE_ALPHA,rep->scalarType);
		toGreyImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE||rep->format==GL_LUMINANCE_ALPHA)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
	}

BaseImage BaseImage::toRgb(void) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_LUMINANCE)
		{
		/* Convert luminance to RGB: */
		BaseImage result(rep->size,3,rep->channelSize,GL_RGB,rep->scalarType);
		toRgbImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE_ALPHA)
		{
		/* Convert luminance-alpha to RGBA: */
		BaseImage result(rep->size,4,rep->channelSize,GL_RGBA,rep->scalarType);
		toRgbImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_RGB||rep->format==GL_RGBA)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
	}

BaseImage BaseImage::shrink(void) const
	{
	/* Check that the image's size is even: */
	if(rep->size[0]%2!=0||rep->size[1]%2!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image size is not divisible by two");
	
	/* Create a reduced-sized image with the same pixel format: */
	BaseImage result(Size(rep->size[0]/2,rep->size[1]/2),rep->numChannels,rep->channelSize,rep->format,rep->scalarType);
	
	/* Delegate to a typed version of this function: */
	switch(rep->scalarType)
		{
		case GL_BYTE:
			shrinkTypedInt<signed char,signed short>(*this,result);
			break;
		
		case GL_UNSIGNED_BYTE:
			shrinkTypedInt<unsigned char,unsigned short>(*this,result);
			break;
		
		case GL_SHORT:
			shrinkTypedInt<signed short,signed int>(*this,result);
			break;
		
		case GL_UNSIGNED_SHORT:
			shrinkTypedInt<unsigned short,unsigned int>(*this,result);
			break;
		
		case GL_INT:
			shrinkTypedInt<signed int,signed long>(*this,result);
			break;
		
		case GL_UNSIGNED_INT:
			shrinkTypedInt<unsigned int,unsigned long>(*this,result);
			break;
		
		case GL_FLOAT:
			shrinkTypedFloat<float>(*this,result);
			break;
		
		case GL_DOUBLE:
			shrinkTypedFloat<double>(*this,result);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
		}
	
	return result;
	}

GLint BaseImage::getInternalFormat(void) const
	{
	/* Guess an appropriate internal image format: */
	GLint internalFormat;
	if(useGammaCorrection)
		{
		internalFormat=GL_SRGB_ALPHA_EXT;
		if(rep->format==GL_LUMINANCE&&rep->channelSize==8)
			internalFormat=GL_SRGB8_EXT;
		else if(rep->format==GL_LUMINANCE_ALPHA&&rep->channelSize==8)
			internalFormat=GL_SRGB8_ALPHA8_EXT;
		else if(rep->format==GL_RGB&&rep->channelSize==8)
			internalFormat=GL_SRGB8_EXT;
		else if(rep->format==GL_RGBA&&rep->channelSize==8)
			internalFormat=GL_SRGB8_ALPHA8_EXT;
		}
	else
		{
		internalFormat=GL_RGBA;
		if(rep->format==GL_LUMINANCE&&rep->channelSize==8)
			internalFormat=GL_RGB8;
		else if(rep->format==GL_LUMINANCE_ALPHA&&rep->channelSize==8)
			internalFormat=GL_RGBA8;
		else if(rep->format==GL_RGB&&rep->channelSize==8)
			internalFormat=GL_RGB8;
		else if(rep->format==GL_RGBA&&rep->channelSize==8)
			internalFormat=GL_RGBA8;
		}
	
	return internalFormat;
	}

BaseImage& BaseImage::glReadPixels(const Offset& offset)
	{
	/* Un-share the image representation without retaining pixels: */
	ownRepresentation(false);
	
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_SKIP_PIXELS,0);
	glPixelStorei(GL_PACK_ROW_LENGTH,0);
	glPixelStorei(GL_PACK_SKIP_ROWS,0);
	
	/* Read image: */
	::glReadPixels(offset[0],offset[1],rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	
	return *this;
	}

void BaseImage::glDrawPixels(void) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Write image: */
	::glDrawPixels(rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	}

void BaseImage::glTexImage2D(GLenum target,GLint level,GLint internalFormat,bool padImageSize) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	if(padImageSize)
		{
		/* Calculate the texture width and height as the next power of two: */
		Size texSize;
		for(int i=0;i<2;++i)
			for(texSize[i]=1;texSize[i]<rep->size[i];texSize[i]<<=1)
				;
		
		if(texSize!=rep->size)
			{
			/* Create the padded texture image: */
			::glTexImage2D(target,level,internalFormat,texSize[0],texSize[1],0,rep->format,rep->scalarType,0);
			
			/* Upload the image: */
			::glTexSubImage2D(target,level,0,0,rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
			}
		else
			{
			/* Upload the image: */
			::glTexImage2D(target,level,internalFormat,rep->size[0],rep->size[1],0,rep->format,rep->scalarType,rep->image);
			}
		}
	else
		{
		/* Upload the image: */
		::glTexImage2D(target,level,internalFormat,rep->size[0],rep->size[1],0,rep->format,rep->scalarType,rep->image);
		}
	}

void BaseImage::glTexImage2DMipmap(GLenum target,GLint internalFormat,bool padImageSize) const
	{
	/* Set the texture's mipmap level range: */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	
	/* Check if the GL_EXT_framebuffer_object OpenGL extension is supported: */
	if(GLEXTFramebufferObject::isSupported())
		{
		/* Initialize the GL_EXT_framebuffer_object extension: */
		GLEXTFramebufferObject::initExtension();
		
		/* Upload the base level texture image: */
		glTexImage2D(target,0,internalFormat,padImageSize);
		
		/* Request automatic mipmap generation: */
		glGenerateMipmapEXT(target);
		}
	else
		{
		/* Create mipmaps manually by successively downsampling this image: */
		BaseImage level=*this;
		GLint levelIndex=0;
		while(true)
			{
			/* Upload the current level texture image: */
			level.glTexImage2D(target,levelIndex,internalFormat,padImageSize);
			++levelIndex;
			
			/* Bail out if we can't shrink further: */
			if(level.getSize(0)%2!=0||level.getSize(1)%2!=0)
				break;
			
			/* Downsample the current level image: */
			level=level.shrink();
			}
		
		/* Set the texture's mipmap level range: */
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,levelIndex-1);
		}
	}

void BaseImage::glTexSubImage2D(GLenum target,GLint level,const Offset& offset) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Upload the image: */
	::glTexSubImage2D(target,level,offset[0],offset[1],rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	}

void BaseImage::glTexSubImage3D(GLenum target,GLint level,const Offset& offset,GLint zOffset) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Upload the image: */
	::glTexSubImage3D(target,level,offset[0],offset[1],zOffset,rep->size[0],rep->size[1],1,rep->format,rep->scalarType,rep->image);
	}

}

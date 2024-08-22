/***********************************************************************
Image - Base class to represent images of arbitrary pixel formats. The
image coordinate system is such that pixel (0,0) is in the lower-left
corner.
Copyright (c) 2007-2024 Oliver Kreylos

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

#include <Images/Image.h>

#include <Misc/StdError.h>

namespace Images {

/**********************
Methods of class Image:
**********************/

template <class ScalarParam,int numComponentsParam>
inline
Image<ScalarParam,numComponentsParam>::Image(
	const BaseImage& source)
	:BaseImage(source)
	{
	if(source.isValid())
		{
		/* Check if the source image's format is compatible with this pixel type: */
		if(source.getNumChannels()!=(unsigned int)(numComponents)||source.getScalarType()!=GLScalarLimits<Scalar>::type)
			{
			/* Invalidate the image and throw an exception: */
			invalidate();
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot share image of different pixel format");
			}
		}
	}

template <class ScalarParam,int numComponentsParam>
inline
Image<ScalarParam,numComponentsParam>&
Image<ScalarParam,numComponentsParam>::operator=(
	const BaseImage& source)
	{
	if(this!=&source)
		{
		if(source.isValid())
			{
			/* Check if the source image's format is compatible with this pixel type: */
			if(source.getNumChannels()!=(unsigned int)(numComponents)||source.getScalarType()!=GLScalarLimits<Scalar>::type)
				{
				/* Throw an exception: */
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot share image of different pixel format");
				}
			}
		
		/* Assign the source image: */
		BaseImage::operator=(source);
		}
	
	return *this;
	}

template <class ScalarParam,int numComponentsParam>
inline
Image<ScalarParam,numComponentsParam>&
Image<ScalarParam,numComponentsParam>::clear(
	const typename Image<ScalarParam,numComponentsParam>::Color& c)
	{
	/* Set all pixel values: */
	size_t numPixels=size_t(getWidth())*size_t(getHeight());
	Color* imgPtr=static_cast<Color*>(BaseImage::replacePixels());
	for(unsigned int i=numPixels;i>0;--i,++imgPtr)
		*imgPtr=c;
	
	return *this;
	}

template <class ScalarParam,int numComponentsParam>
inline
Image<ScalarParam,numComponentsParam>&
Image<ScalarParam,numComponentsParam>::resize(
	const Size& newSize)
	{
	Size oldSize=getSize();
	
	/* Allocate an intermediate floating-point buffer: */
	typedef typename GLScalarLimits<Scalar>::AccumulatorScalar AScalar; // Type to use for intermediate accumulated pixel values
	typedef GLColor<AScalar,numComponents> AColor;
	AColor* buffer=new AColor[size_t(newSize[0])*size_t(oldSize[1])];
	
	/* Resample pixel rows: */
	const Color* sImage=static_cast<const Color*>(BaseImage::getPixels());
	for(unsigned int x=0;x<newSize[0];++x)
		{
		/* Note: sampleX is the x coordinate of the next pixel to the right, to get around some issues. */
		AScalar sampleX=(AScalar(x)+AScalar(0.5))*AScalar(oldSize[0])/AScalar(newSize[0])+AScalar(0.5);
		
		unsigned int sx=(unsigned int)(sampleX);
		const Color* sCol0=sx>0?sImage+(sx-1):sImage;
		const Color* sCol1=sx<oldSize[0]?sImage+sx:sImage+(oldSize[0]-1);
		AScalar w1=sampleX-AScalar(sx);
		AScalar w0=AScalar(1)-w1;
		AColor* dCol=buffer+x;
		for(unsigned int y=0;y<oldSize[1];++y,sCol0+=oldSize[0],sCol1+=oldSize[0],dCol+=newSize[0])
			{
			for(int i=0;i<numComponents;++i)
				(*dCol)[i]=AScalar((*sCol0)[i])*w0+AScalar((*sCol1)[i])*w1;
			}
		}
	
	/* Create the new image representation: */
	*this=Image(newSize,getFormat());
	
	/* Resample pixel columns: */
	Color* dImage=static_cast<Color*>(BaseImage::replacePixels());
	for(unsigned int y=0;y<newSize[1];++y)
		{
		/* Note: sampleY is the y coordinate of the next pixel to the top, to get around some issues. */
		AScalar sampleY=(AScalar(y)+AScalar(0.5f))*AScalar(oldSize[1])/AScalar(newSize[1])+AScalar(0.5);
		
		unsigned int sy=(unsigned int)(sampleY);
		const AColor* sRow0=sy>0?buffer+(size_t(sy-1)*size_t(newSize[0])):buffer;
		const AColor* sRow1=sy<oldSize[1]?buffer+(size_t(sy)*size_t(newSize[0])):buffer+(size_t(oldSize[1]-1)*size_t(newSize[0]));
		AScalar w1=sampleY-AScalar(sy);
		AScalar w0=AScalar(1)-w1;
		Color* dRow=dImage+(size_t(y)*size_t(newSize[0]));
		for(unsigned int x=0;x<newSize[0];++x,++sRow0,++sRow1,++dRow)
			{
			for(int i=0;i<numComponents;++i)
				(*dRow)[i]=GLScalarLimits<Scalar>::fromAccumulator((*sRow0)[i]*w0+(*sRow1)[i]*w1);
			}
		}
	delete[] buffer;
	
	return *this;
	}

/*************************************************
Force instantiation of all standard Image classes:
*************************************************/

template class Image<GLubyte,3>;
template class Image<GLubyte,4>;
template class Image<GLushort,3>;
template class Image<GLushort,4>;
template class Image<GLfloat,3>;
template class Image<GLfloat,4>;

}

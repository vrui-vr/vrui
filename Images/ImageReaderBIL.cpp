/***********************************************************************
ImageReaderBIL - Class to read images from files in BIL (Band
Interleaved by Line), BIP (Band Interleaved by Pixel), or BSQ (Band
Sequential) formats.
Copyright (c) 2018-2024 Oliver Kreylos

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

#include <Images/ImageReaderBIL.h>

#include <string.h>
#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Misc/FileNameExtensions.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <IO/ValueSource.h>
#include <Images/BaseImage.h>

namespace Images {

namespace {

/****************
Helper functions:
****************/

ImageReaderBIL::FileLayout readHeaderFileImpl(IO::File& headerFile,const char* ext,int extLen)
	{
	typedef ImageReaderBIL::FileLayout FileLayout;
	
	/* Create default BIL file layout: */
	FileLayout result;
	result.size=Size(-1,-1);
	result.numBands=1;
	result.numBits=8;
	result.pixelSigned=false;
	result.byteOrder=Misc::HostEndianness;
	
	/* Determine default file layout based on image file name extension: */
	result.bandLayout=FileLayout::BIL;
	if(extLen==4)
		{
		if(strncasecmp(ext+1,"BIP",3)==0)
			result.bandLayout=FileLayout::BIP;
		else if(strncasecmp(ext+1,"BSQ",3)==0)
			result.bandLayout=FileLayout::BSQ;
		}
	
	result.skipBytes=0;
	result.bandGapBytes=0;
	result.metadata.haveMap=false;
	result.metadata.map[0]=0.0;
	result.metadata.map[1]=0.0;
	result.metadata.haveDim=false;
	result.metadata.dim[0]=1.0;
	result.metadata.dim[1]=1.0;
	result.metadata.haveNoData=false;
	bool haveBandRowBytes=false;
	bool haveTotalRowBytes=false;
	int mapMask=0x0;
	bool mapIsLowerLeft=true;
	int dimMask=0x0;
	
	/* Attach a value source to the header file to parse it: */
	IO::ValueSource header(headerFile);
	header.setPunctuation("\n");
	
	/* Process all token=value pairs in the header file: */
	header.skipWs();
	while(!header.eof())
		{
		/* Read the next token: */
		std::string token=header.readString();
		
		if(strcasecmp(token.c_str(),"NROWS")==0||strcasecmp(token.c_str(),"ROWS")==0)
			result.size[1]=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"NCOLS")==0||strcasecmp(token.c_str(),"COLS")==0)
			result.size[0]=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"NBANDS")==0||strcasecmp(token.c_str(),"BANDS")==0)
			result.numBands=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"NBITS")==0)
			{
			result.numBits=header.readUnsignedInteger();
			if(result.numBits!=1&&result.numBits!=4&&result.numBits!=8&&result.numBits!=16&&result.numBits!=32)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid pixel size declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"PIXELTYPE")==0)
			{
			/* Check if the pixel type is signed: */
			if(header.isCaseLiteral("SIGNEDINT"))
				result.pixelSigned=true;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid pixel type declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"BYTEORDER")==0||strcasecmp(token.c_str(),"BYTE_ORDER")==0)
			{
			/* Read the byte order: */
			std::string byteOrder=header.readString();
			if(strcasecmp(byteOrder.c_str(),"I")==0||strcasecmp(byteOrder.c_str(),"LSBFIRST")==0)
				result.byteOrder=Misc::LittleEndian;
			else if(strcasecmp(byteOrder.c_str(),"M")==0||strcasecmp(byteOrder.c_str(),"MSBFIRST")==0)
				result.byteOrder=Misc::BigEndian;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid byte order declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"LAYOUT")==0||strcasecmp(token.c_str(),"INTERLEAVING")==0)
			{
			/* Read the file layout: */
			std::string layout=header.readString();
			if(strcasecmp(layout.c_str(),"BIP")==0)
				result.bandLayout=FileLayout::BIP;
			else if(strcasecmp(layout.c_str(),"BIL")==0)
				result.bandLayout=FileLayout::BIL;
			else if(strcasecmp(layout.c_str(),"BSQ")==0)
				result.bandLayout=FileLayout::BSQ;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid image file layout declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"SKIPBYTES")==0)
			result.skipBytes=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"BANDROWBYTES")==0)
			{
			result.bandRowBytes=header.readUnsignedInteger();
			haveBandRowBytes=true;
			}
		else if(strcasecmp(token.c_str(),"BANDGAPBYTES")==0)
			result.bandGapBytes=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"TOTALROWBYTES")==0)
			{
			result.totalRowBytes=header.readUnsignedInteger();
			haveTotalRowBytes=true;
			}
		else if(strcasecmp(token.c_str(),"ULXMAP")==0||strcasecmp(token.c_str(),"UL_X_COORDINATE")==0)
			{
			mapMask|=0x1;
			result.metadata.map[0]=header.readNumber();
			mapIsLowerLeft=false;
			}
		else if(strcasecmp(token.c_str(),"ULYMAP")==0||strcasecmp(token.c_str(),"UL_Y_COORDINATE")==0)
			{
			mapMask|=0x2;
			result.metadata.map[1]=header.readNumber();
			mapIsLowerLeft=false;
			}
		else if(strcasecmp(token.c_str(),"XLLCORNER")==0)
			{
			mapMask|=0x1;
			result.metadata.map[0]=header.readNumber();
			mapIsLowerLeft=true;
			}
		else if(strcasecmp(token.c_str(),"YLLCORNER")==0)
			{
			mapMask|=0x2;
			result.metadata.map[1]=header.readNumber();
			mapIsLowerLeft=true;
			}
		else if(strcasecmp(token.c_str(),"XDIM")==0)
			{
			dimMask|=0x1;
			result.metadata.dim[0]=header.readNumber();
			}
		else if(strcasecmp(token.c_str(),"YDIM")==0)
			{
			dimMask|=0x2;
			result.metadata.dim[1]=header.readNumber();
			}
		else if(strcasecmp(token.c_str(),"CELLSIZE")==0)
			{
			dimMask=0x3;
			result.metadata.dim[1]=result.metadata.dim[0]=header.readNumber();
			}
		else if(strcasecmp(token.c_str(),"NODATA")==0||strcasecmp(token.c_str(),"NODATA_VALUE")==0)
			{
			result.metadata.haveNoData=true;
			result.metadata.noData=header.readNumber();
			}
		
		/* Skip the rest of the line: */
		header.skipLine();
		header.skipWs();
		}
	
	/* Set layout default values: */
	if(!haveBandRowBytes)
		result.bandRowBytes=(result.size[0]*result.numBits+7)/8;
	if(!haveTotalRowBytes)
		result.totalRowBytes=result.bandLayout==FileLayout::BIL?result.numBands*result.bandRowBytes:(result.size[0]*result.numBands*result.numBits+7)/8;
	if(mapMask==0x3)
		{
		result.metadata.haveMap=true;
		if(mapIsLowerLeft&&(dimMask&0x2)!=0x0)
			result.metadata.map[1]=result.metadata.map[1]+double(result.size[1]-1)*result.metadata.dim[1];
		}
	result.metadata.haveDim=dimMask==0x3;
	
	return result;
	}


template <class ComponentParam>
void readBIPImageData(IO::File& imageFile,const ImageReaderBIL::FileLayout& layout,ComponentParam* data)
	{
	/* Read the image file line-by-line: */
	size_t rowSize=layout.size[0]*layout.numBands;
	size_t rowSkip=layout.totalRowBytes-rowSize*sizeof(ComponentParam);
	ComponentParam* rowPtr=data+rowSize*(layout.size[1]-1);
	for(size_t y=layout.size[1];y>0;--y,rowPtr-=rowSize)
		{
		/* Read a row of image data: */
		imageFile.read(rowPtr,rowSize);
		
		/* Skip the gap between rows: */
		imageFile.skip<Misc::UInt8>(rowSkip);
		}
	}

template <class ComponentParam>
void readBILImageData(IO::File& imageFile,const ImageReaderBIL::FileLayout& layout,ComponentParam* data)
	{
	/* Read the image file line-by-line and band-by-band: */
	Misc::SelfDestructArray<ComponentParam> band(layout.size[0]);
	size_t rowSize=layout.size[0]*layout.numBands;
	size_t bandSkip=layout.bandRowBytes-layout.size[0]*sizeof(ComponentParam);
	size_t rowSkip=layout.totalRowBytes-layout.numBands*layout.bandRowBytes;
	ComponentParam* rowPtr=data+rowSize*(layout.size[1]-1);
	for(size_t y=layout.size[1];y>0;--y,rowPtr-=rowSize)
		{
		for(size_t i=0;i<layout.numBands;++i)
			{
			/* Read one band of data for the current line: */
			imageFile.read<ComponentParam>(band,layout.size[0]);
			
			/* Copy the band data into the row's pixels: */
			ComponentParam* bPtr=band;
			ComponentParam* pPtr=rowPtr+i;
			for(size_t x=0;x<layout.size[0];++x,++bPtr,pPtr+=layout.numBands)
				*pPtr=*bPtr;
			
			/* Skip the gap between bands: */
			imageFile.skip<Misc::UInt8>(bandSkip);
			}
		
		/* Skip the gap between rows: */
		imageFile.skip<Misc::UInt8>(rowSkip);
		}
	}

template <class ComponentParam>
void readBSQImageData(IO::File& imageFile,const ImageReaderBIL::FileLayout& layout,ComponentParam* data)
	{
	/* Read the image file band-by-band and line-by-line: */
	Misc::SelfDestructArray<ComponentParam> band(layout.size[0]);
	size_t rowSize=layout.size[0]*layout.numBands;
	for(size_t i=0;i<layout.numBands;++i)
		{
		ComponentParam* rowPtr=data+rowSize*(layout.size[1]-1);
		for(size_t y=layout.size[1];y>0;--y,rowPtr-=rowSize)
			{
			/* Read one band of data for the current line: */
			imageFile.read<ComponentParam>(band,layout.size[0]);
			
			/* Copy the band data into the row's pixels: */
			ComponentParam* bPtr=band;
			ComponentParam* pPtr=rowPtr+i;
			for(size_t x=0;x<layout.size[0];++x,++bPtr,pPtr+=layout.numBands)
				*pPtr=*bPtr;
			}
		
		/* Skip the band gap: */
		imageFile.skip<Misc::UInt8>(layout.bandGapBytes);
		}
	}

template <class ComponentParam>
BaseImage readImageData(IO::File& imageFile,const ImageReaderBIL::FileLayout& layout,GLenum scalarType)
	{
	/* Determine a compatible texture format: */
	GLenum format;
	switch(layout.numBands)
		{
		case 1:
			format=GL_LUMINANCE;
			break;
		
		case 2:
			format=GL_LUMINANCE_ALPHA;
			break;
		
		case 3:
			format=GL_RGB;
			break;
		
		case 4:
			format=GL_RGBA;
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel format");
		}
	
	/* Create the result image: */
	BaseImage result(layout.size,layout.numBands,(layout.numBits+7)/8,format,scalarType);
	
	/* Skip the image header: */
	imageFile.skip<Misc::UInt8>(layout.skipBytes);
	
	/* Read the image file according to its interleave format: */
	switch(layout.bandLayout)
		{
		case ImageReaderBIL::FileLayout::BIP:
			readBIPImageData(imageFile,layout,static_cast<ComponentParam*>(result.modifyPixels()));
			break;
		
		case ImageReaderBIL::FileLayout::BIL:
			readBILImageData(imageFile,layout,static_cast<ComponentParam*>(result.modifyPixels()));
			break;
		
		case ImageReaderBIL::FileLayout::BSQ:
			readBSQImageData(imageFile,layout,static_cast<ComponentParam*>(result.modifyPixels()));
			break;
		}
	
	return result;
	}

}

/*******************************
Methods of class ImageReaderBIL:
*******************************/

void ImageReaderBIL::setImageSpec(void)
	{
	/* Set the image specification: */
	canvasSize=layout.size;
	imageSpec.rect=Rect(canvasSize);
	imageSpec.colorSpace=layout.numBands<=2?Grayscale:RGB;
	imageSpec.hasAlpha=layout.numBands==2||layout.numBands==4;
	if(layout.numBits==8||layout.numBits==16)
		imageSpec.valueType=layout.pixelSigned?SignedInt:UnsignedInt;
	else if(layout.numBits==32)
		imageSpec.valueType=Float;
	setValueSpec(imageSpec.valueType,layout.numBits);
	}

ImageReaderBIL::FileLayout ImageReaderBIL::readHeaderFile(const char* imageFileName)
	{
	/* Retrieve the file name extension: */
	const char* ext=Misc::getExtension(imageFileName);
	int extLen=strlen(ext);
	if(strcasecmp(ext,".gz")==0)
		{
		/* Strip the gzip extension and try again: */
		const char* gzExt=ext;
		ext=Misc::getExtension(imageFileName,gzExt);
		extLen=gzExt-ext;
		}
	
	/* Construct the header file name: */
	std::string headerFileName(imageFileName,ext);
	headerFileName.append(".hdr");
	
	/* Read the header file and return the image file's layout and optional metadata: */
	return readHeaderFileImpl(*IO::openFile(headerFileName.c_str()),ext,extLen);
	}

ImageReaderBIL::FileLayout ImageReaderBIL::readHeaderFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* Retrieve the file name extension: */
	const char* ext=Misc::getExtension(imageFileName);
	int extLen=strlen(ext);
	if(strcasecmp(ext,".gz")==0)
		{
		/* Strip the gzip extension and try again: */
		const char* gzExt=ext;
		ext=Misc::getExtension(imageFileName,gzExt);
		extLen=gzExt-ext;
		}
	
	/* Construct the header file name: */
	std::string headerFileName(imageFileName,ext);
	headerFileName.append(".hdr");
	
	/* Read the header file and return the image file's layout and optional metadata: */
	return readHeaderFileImpl(*directory.openFile(headerFileName.c_str()),ext,extLen);
	}

ImageReaderBIL::ImageReaderBIL(const ImageReaderBIL::FileLayout& sLayout,IO::File& imageFile)
	:ImageReader(imageFile),
	 layout(sLayout),
	 done(false)
	{
	/* Set up the image specification according to the file layout: */
	setImageSpec();
	}

ImageReaderBIL::ImageReaderBIL(const char* imageFileName)
	:ImageReader(*IO::openFile(imageFileName)),
	 layout(readHeaderFile(imageFileName)),
	 done(false)
	{
	/* Set up the image specification according to the file layout: */
	setImageSpec();
	}

ImageReaderBIL::ImageReaderBIL(const IO::Directory& directory,const char* imageFileName)
	:ImageReader(*directory.openFile(imageFileName)),
	 layout(readHeaderFile(directory,imageFileName)),
	 done(false)
	{
	/* Set up the image specification according to the file layout: */
	setImageSpec();
	}

bool ImageReaderBIL::eof(void) const
	{
	return done;
	}

BaseImage ImageReaderBIL::readImage(void)
	{
	BaseImage result;
	
	/* Read the image file according to its pixel type: */
	file->setEndianness(layout.byteOrder);
	switch(layout.numBits)
		{
		case 8:
			if(layout.pixelSigned)
				result=readImageData<Misc::SInt8>(*file,layout,GL_BYTE);
			else
				result=readImageData<Misc::UInt8>(*file,layout,GL_UNSIGNED_BYTE);
			break;
		
		case 16:
			if(layout.pixelSigned)
				result=readImageData<Misc::SInt16>(*file,layout,GL_SHORT);
			else
				result=readImageData<Misc::UInt16>(*file,layout,GL_UNSIGNED_SHORT);
			break;
		
		case 32:
			result=readImageData<Misc::Float32>(*file,layout,GL_FLOAT);
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image has unsupported pixel size");
		}
	
	/* There can only be one image in a BIL file: */
	done=true;
	
	return result;
	}

}

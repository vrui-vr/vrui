/***********************************************************************
ReadImageFile - Functions to read generic images from image files in a
variety of formats.
Copyright (c) 2005-2025 Oliver Kreylos

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

#include <Images/ReadImageFile.h>

#include <Misc/Utility.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <IO/OpenFile.h>
#include <IO/Directory.h>
#include <Images/Config.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>
#include <Images/ImageReader.h>

namespace Images {

bool canReadImageFileFormat(ImageFileFormat imageFileFormat)
	{
	/* Check if the format is supported: */
	if(imageFileFormat==IFF_PNM||imageFileFormat==IFF_BIL||imageFileFormat==IFF_IFF)
		return true;
	#if IMAGES_CONFIG_HAVE_PNG
	if(imageFileFormat==IFF_PNG)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_JPEG
	if(imageFileFormat==IFF_JPEG)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_TIFF
	if(imageFileFormat==IFF_TIFF)
		return true;
	#endif
	
	return false;
	}

std::vector<std::string> getSupportedImageFileExtensions(void)
	{
	std::vector<std::string> result;
	
	result.push_back(".pbm");
	result.push_back(".pgm");
	result.push_back(".pnm");
	result.push_back(".ppm");
	result.push_back(".pfm");
	
	result.push_back(".bip");
	result.push_back(".bil");
	result.push_back(".bsq");
	result.push_back(".img");
	
	#if IMAGES_CONFIG_HAVE_PNG
	result.push_back(".png");
	#endif
	
	#if IMAGES_CONFIG_HAVE_JPEG
	result.push_back(".jpg");
	result.push_back(".jpeg");
	#endif
	
	#if IMAGES_CONFIG_HAVE_TIFF
	result.push_back(".tif");
	result.push_back(".tiff");
	#endif
	
	result.push_back(".iff");
	
	return result;
	}

BaseImage readGenericImageFile(IO::File& file,ImageFileFormat imageFileFormat)
	{
	/* Create an image reader: */
	Misc::SelfDestructPointer<ImageReader> reader(ImageReader::create(imageFileFormat,file));
	
	/* Read the image: */
	return reader->readImage();
	}

BaseImage readGenericImageFile(const char* imageFileName)
	{
	/* Create an image reader: */
	Misc::SelfDestructPointer<ImageReader> reader(ImageReader::create(imageFileName));
	
	/* Read the image: */
	return reader->readImage();
	}

BaseImage readGenericImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* Create an image reader: */
	Misc::SelfDestructPointer<ImageReader> reader(ImageReader::create(directory,imageFileName));
	
	/* Read the image: */
	return reader->readImage();
	}

namespace {

/********************************************
Helper structures for the cursor file reader:
********************************************/

struct CursorFileHeader
	{
	/* Elements: */
	public:
	unsigned int magic;
	unsigned int headerSize;
	unsigned int version;
	unsigned int numTOCEntries;
	};

struct CursorTOCEntry
	{
	/* Elements: */
	public:
	unsigned int chunkType;
	unsigned int chunkSubtype;
	unsigned int chunkPosition;
	};

struct CursorCommentChunkHeader
	{
	/* Elements: */
	public:
	unsigned int headerSize;
	unsigned int chunkType; // 0xfffe0001U
	unsigned int chunkSubtype;
	unsigned int version;
	unsigned int commentLength;
	/* Comment characters follow */
	};

struct CursorImageChunkHeader
	{
	/* Elements: */
	public:
	unsigned int headerSize;
	unsigned int chunkType; // 0xfffd0002U
	unsigned int chunkSubtype;
	unsigned int version;
	Size size;
	Offset hotspot;
	unsigned int delay;
	/* ARGB pixel data follows */
	};

}

/***********************************************
Function to read cursor files in Xcursor format:
***********************************************/

RGBAImage readCursorFile(IO::File& file,unsigned int nominalSize,Offset* hotspot)
	{
	/* Read the magic value to determine file endianness: */
	size_t filePos=0;
	CursorFileHeader fh;
	fh.magic=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	if(fh.magic==0x58637572U)
		file.setSwapOnRead(true);
	else if(fh.magic!=0x72756358U)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid Xcursor file header");
	
	/* Read the rest of the file header: */
	fh.headerSize=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	fh.version=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	fh.numTOCEntries=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	
	/* Read the table of contents: */
	size_t imageChunkOffset=0;
	for(unsigned int i=0;i<fh.numTOCEntries;++i)
		{
		CursorTOCEntry te;
		te.chunkType=file.read<unsigned int>();
		filePos+=sizeof(unsigned int);
		te.chunkSubtype=file.read<unsigned int>();
		filePos+=sizeof(unsigned int);
		te.chunkPosition=file.read<unsigned int>();
		filePos+=sizeof(unsigned int);
		
		if(te.chunkType==0xfffd0002U&&te.chunkSubtype==nominalSize)
			{
			imageChunkOffset=size_t(te.chunkPosition);
			break;
			}
		}
	
	if(imageChunkOffset==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No matching image found");
	
	/* Skip ahead to the beginning of the image chunk: */
	file.skip<char>(imageChunkOffset-filePos);
	
	/* Read the image chunk: */
	CursorImageChunkHeader ich;
	ich.headerSize=file.read<unsigned int>();
	ich.chunkType=file.read<unsigned int>();
	ich.chunkSubtype=file.read<unsigned int>();
	ich.version=file.read<unsigned int>();
	file.read(ich.size.getComponents(),2);
	file.read(ich.hotspot.getComponents(),2);
	if(hotspot!=0)
		*hotspot=ich.hotspot;
	ich.delay=file.read<unsigned int>();
	if(ich.headerSize!=9*sizeof(unsigned int)||ich.chunkType!=0xfffd0002U||ich.version!=1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid image chunk header");
	
	/* Create the result image: */
	RGBAImage result(ich.size);
	
	/* Read the image row-by-row: */
	for(unsigned int row=result.getHeight();row>0;--row)
		{
		RGBAImage::Color* rowPtr=result.modifyPixelRow(row-1);
		file.read(rowPtr->getRgba(),result.getWidth()*4);
		
		/* Convert BGRA data into RGBA data: */
		for(unsigned int i=0;i<result.getWidth();++i)
			Misc::swap(rowPtr[i][0],rowPtr[i][2]);
		}
	
	/* Return the result image: */
	return result;
	}

RGBAImage readCursorFile(const char* cursorFileName,unsigned int nominalSize,Offset* hotspot)
	{
	RGBAImage result;
	
	try
		{
		/* Open the cursor file and read it: */
		result=readCursorFile(*IO::openFile(cursorFileName),nominalSize,hotspot);
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot read cursor file %s due to error %s",cursorFileName,err.what());
		}
	
	return result;
	}

RGBAImage readCursorFile(const IO::Directory& directory,const char* cursorFileName,unsigned int nominalSize,Offset* hotspot)
	{
	RGBAImage result;
	
	try
		{
		/* Open the cursor file and read it: */
		result=readCursorFile(*directory.openFile(cursorFileName),nominalSize,hotspot);
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot read cursor file %s due to error %s",directory.getPath(cursorFileName).c_str(),err.what());
		}
	
	return result;
	}

/***********************************
Deprecated functions to read images:
***********************************/

RGBImage readImageFile(IO::File& file,ImageFileFormat imageFileFormat)
	{
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(file,imageFileFormat);
	return RGBImage(result.dropAlpha().toRgb());
	}

RGBImage readImageFile(const char* imageFileName)
	{
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(imageFileName);
	return RGBImage(result.dropAlpha().toRgb());
	}

RGBImage readImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(directory,imageFileName);
	return RGBImage(result.dropAlpha().toRgb());
	}

RGBAImage readTransparentImageFile(IO::File& file,ImageFileFormat imageFileFormat)
	{
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(file,imageFileFormat);
	return RGBAImage(result.addAlpha(1.0).toRgb());
	}

RGBAImage readTransparentImageFile(const char* imageFileName)
	{
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(imageFileName);
	return RGBAImage(result.addAlpha(1.0).toRgb());
	}

RGBAImage readTransparentImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(directory,imageFileName);
	return RGBAImage(result.addAlpha(1.0).toRgb());
	}

}

/***********************************************************************
ImageTextureNode - Class for textures loaded from external image files.
Copyright (c) 2009-2024 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/ImageTextureNode.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/VarIntMarshaller.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTTextureFilterAnisotropic.h>
#include <Images/BaseImage.h>
#include <Images/ImageFileFormats.h>
#include <Images/ReadImageFile.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/GLRenderState.h>

#define PRELOAD_TEXTURES 1

namespace SceneGraph {

/*******************************************
Methods of class ImageTextureNode::DataItem:
*******************************************/

ImageTextureNode::DataItem::DataItem(void)
	:textureObjectId(0),
	 version(0)
	{
	glGenTextures(1,&textureObjectId);
	}

ImageTextureNode::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&textureObjectId);
	}

/*****************************************
Static elements of class ImageTextureNode:
*****************************************/

const char* ImageTextureNode::className="ImageTexture";

/*********************************
Methods of class ImageTextureNode:
*********************************/

void ImageTextureNode::loadImageFile(IO::Directory& baseDirectory)
	{
	/* Determine the texture image file's format: */
	imageFileFormat=Images::getImageFileFormat(url.getValue(0).c_str());
	
	if(imageFileFormat<Images::IFF_NUM_FORMATS)
		{
		/* Load the image file into memory: */
		imageFile=new IO::VariableMemoryFile;
		IO::FilePtr sourceImageFile=baseDirectory.openFile(url.getValue(0).c_str());
		
		/* Copy the source image file's contents to the image file: */
		while(true)
			{
			/* Read a chunk of the source image file into its internal read buffer: */
			void* buffer;
			size_t bufferSize=sourceImageFile->readInBuffer(buffer);
			
			/* Bail out if the file is all read: */
			if(bufferSize==0)
				break;
			
			/* Write the source image file's buffer contents to the image file: */
			imageFile->writeRaw(buffer,bufferSize);
			}
		imageFile->flush();
		}
	else
		imageFile=0;
	
	/* Invalidate the cached texture: */
	++version;
	}

void ImageTextureNode::uploadTexture(ImageTextureNode::DataItem* dataItem) const
	{
	if(imageFileFormat<Images::IFF_NUM_FORMATS)
		{
		/* Load the texture image: */
		Images::BaseImage texture=Images::readGenericImageFile(*imageFile->getReader(),imageFileFormat);
		
		/* Upload the texture image: */
		int mml=mipmapLevel.getValue();
		texture.glTexImage2D(GL_TEXTURE_2D,0,false);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,mml);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter.getValue()?(mml>0?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR):GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter.getValue()?GL_LINEAR:GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,repeatS.getValue()?GL_REPEAT:GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,repeatT.getValue()?GL_REPEAT:GL_CLAMP);
		
		/* Check if mipmapping was requested and mipmap generation is supported: */
		if(mml>0&&GLEXTFramebufferObject::isSupported())
			{
			/* Initialize the framebuffer extension: */
			GLEXTFramebufferObject::initExtension();
			
			/* Auto-generate all requested mipmap levels: */
			glGenerateMipmapEXT(GL_TEXTURE_2D);
			}
		
		/* Check if anisotropic filtering was requested and is supported: */
		if(anisotropyLevel.getValue()>1&&GLEXTTextureFilterAnisotropic::isSupported())
			{
			/* Initialize the framebuffer extension: */
			GLEXTTextureFilterAnisotropic::initExtension();
			
			/* Request anisotropic filtering: */
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,anisotropyLevel.getValue());
			}
		}
	
	/* Mark the texture object as up-to-date: */
	dataItem->version=version;
	}

ImageTextureNode::ImageTextureNode(void)
	:repeatS(true),repeatT(true),filter(true),mipmapLevel(0),anisotropyLevel(1),
	 imageFileFormat(Images::IFF_NUM_FORMATS),
	 version(0)
	{
	}

const char* ImageTextureNode::getClassName(void) const
	{
	return className;
	}

void ImageTextureNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		if(!url.getValues().empty())
			{
			/* Immediately load the image file referenced by the url field: */
			loadImageFile(vrmlFile.getBaseDirectory());
			}
		else
			{
			/* Reset the image file: */
			imageFileFormat=Images::IFF_NUM_FORMATS;
			imageFile=0;
			++version;
			}
		}
	else if(strcmp(fieldName,"repeatS")==0)
		{
		vrmlFile.parseField(repeatS);
		}
	else if(strcmp(fieldName,"repeatT")==0)
		{
		vrmlFile.parseField(repeatT);
		}
	else if(strcmp(fieldName,"filter")==0)
		{
		vrmlFile.parseField(filter);
		}
	else if(strcmp(fieldName,"mipmapLevel")==0)
		{
		vrmlFile.parseField(mipmapLevel);
		}
	else if(strcmp(fieldName,"anisotropyLevel")==0)
		{
		vrmlFile.parseField(anisotropyLevel);
		}
	else
		TextureNode::parseField(fieldName,vrmlFile);
	}

void ImageTextureNode::update(void)
	{
	/* Clamp the mipmap level: */
	if(mipmapLevel.getValue()<0)
		mipmapLevel.setValue(0);
	
	/* Clamp the anisotropy level: */
	if(anisotropyLevel.getValue()<1)
		anisotropyLevel.setValue(1);
	}

void ImageTextureNode::read(SceneGraphReader& reader)
	{
	/* Read all fields: */
	url.clearValues();
	reader.readField(repeatS);
	reader.readField(repeatT);
	reader.readField(filter);
	reader.readField(mipmapLevel);
	reader.readField(anisotropyLevel);
	
	/*********************************************************************
	Don't read the texture image's URL, but read its image file format and
	file contents instead.
	*********************************************************************/
	
	/* Read the image file's format from the source file: */
	imageFileFormat=static_cast<Images::ImageFileFormat>(reader.getFile().read<Misc::UInt8>());
	
	/* Read the image file's content if the format is valid: */
	if(imageFileFormat<Images::IFF_NUM_FORMATS)
		{
		/* Read the size of the image file from the source file: */
		size_t imageFileSize=Misc::readVarInt32(reader.getFile());
		
		/* Read the image file: */
		imageFile=new IO::VariableMemoryFile;
		while(imageFileSize>0)
			{
			/* Read a chunk of the source file into its internal read buffer: */
			void* buffer;
			size_t bufferSize=reader.getFile().readInBuffer(buffer,imageFileSize);
			imageFileSize-=bufferSize;
			
			/* Bail out if the file is all read: */
			if(bufferSize==0)
				break;
			
			/* Write the source file's buffer contents to the image file: */
			imageFile->writeRaw(buffer,bufferSize);
			}
		imageFile->flush();
		}
	else
		imageFile=0;
	
	/* Invalidate the cached texture: */
	++version;
	}

void ImageTextureNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeField(repeatS);
	writer.writeField(repeatT);
	writer.writeField(filter);
	writer.writeField(mipmapLevel);
	writer.writeField(anisotropyLevel);
	
	/*********************************************************************
	Don't write the texture image's URL, but write its image file format
	and file contents instead.
	*********************************************************************/
	
	/* Write the image file's format to the destination file: */
	writer.getFile().write(Misc::UInt8(imageFileFormat));
	
	/* Write the image file's contents if the image file format is valid: */
	if(imageFileFormat<Images::IFF_NUM_FORMATS)
		{
		/* Write the size of the image file to the destination file: */
		Misc::writeVarInt32(imageFile->getDataSize(),writer.getFile());
		
		/* Copy the image file's contents to the destination file: */
		IO::FilePtr reader=imageFile->getReader();
		while(true)
			{
			/* Read a chunk of the image file into its internal read buffer: */
			void* buffer;
			size_t bufferSize=reader->readInBuffer(buffer);
			
			/* Bail out if the file is all read: */
			if(bufferSize==0)
				break;
			
			/* Write the image file's buffer contents to the destination file: */
			writer.getFile().writeRaw(buffer,bufferSize);
			}
		}
	}

void ImageTextureNode::setGLState(GLRenderState& renderState) const
	{
	/* Check if the image file format is valid: */
	if(imageFileFormat<Images::IFF_NUM_FORMATS)
		{
		/* Enable 2D textures: */
		renderState.enableTexture2D();
		
		/* Get the data item: */
		DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
		
		/* Bind the texture object: */
		renderState.bindTexture2D(dataItem->textureObjectId);
		
		/* Upload the current texture image if the texture object is outdated: */
		if(dataItem->version!=version)
			uploadTexture(dataItem);
		
		#if 0
		
		/* Enable alpha testing for now: */
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL,0.5f);
		
		#endif
		}
	else
		{
		/* Disable texture mapping: */
		renderState.disableTextures();
		}
	}

void ImageTextureNode::resetGLState(GLRenderState& renderState) const
	{
	#if 0
	
	/* Disable alpha testing for now: */
	glDisable(GL_ALPHA_TEST);
	
	#endif
	
	/* Don't do anything; next guy cleans up */
	}

void ImageTextureNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the GL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	#if PRELOAD_TEXTURES
	
	/* Upload the initial texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	uploadTexture(dataItem);
	glBindTexture(GL_TEXTURE_2D,0);
	
	#endif
	}

void ImageTextureNode::setUrl(const std::string& newUrl,IO::Directory& baseDirectory)
	{
	/* Store the URL and load the referenced image file: */
	url.setValue(newUrl);
	loadImageFile(baseDirectory);
	}

void ImageTextureNode::setUrl(const std::string& newUrl)
	{
	/* Store the URL and load the referenced image file: */
	url.setValue(newUrl);
	loadImageFile(*IO::Directory::getCurrent());
	}

void ImageTextureNode::setImageFile(Images::ImageFileFormat newImageFileFormat,IO::FilePtr newImageFile)
	{
	/* Store the new image file format: */
	imageFileFormat=newImageFileFormat;
	
	/* Copy the given image file into memory: */
	imageFile=new IO::VariableMemoryFile;
	
	/* Copy the source image file's contents to the image file: */
	while(true)
		{
		/* Read a chunk of the source image file into its internal read buffer: */
		void* buffer;
		size_t bufferSize=newImageFile->readInBuffer(buffer);
		
		/* Bail out if the file is all read: */
		if(bufferSize==0)
			break;
		
		/* Write the source image file's buffer contents to the image file: */
		imageFile->writeRaw(buffer,bufferSize);
		}
	imageFile->flush();
	
	/* Invalidate the cached texture: */
	++version;
	}

}

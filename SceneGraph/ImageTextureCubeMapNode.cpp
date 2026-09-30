/***********************************************************************
ImageTextureCubeMapNode - Class for cubemap textures loaded from six-
tuples of external image files.
Copyright (c) 2026 Oliver Kreylos

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

#include <SceneGraph/ImageTextureCubeMapNode.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/VarIntMarshaller.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBSeamlessCubeMap.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTTextureCubeMap.h>
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

/**************************************************
Methods of class ImageTextureCubeMapNode::DataItem:
**************************************************/

ImageTextureCubeMapNode::DataItem::DataItem(void)
	:haveCubeMaps(GLEXTTextureCubeMap::isSupported()),
	 haveSeamlessCubeMaps(GLARBSeamlessCubeMap::isSupported()),
	 haveMipmaps(GLEXTFramebufferObject::isSupported()),
	 haveAnisotropicFiltering(GLEXTTextureFilterAnisotropic::isSupported()),
	 textureObjectId(0),
	 version(0)
	{
	/* Initialize needed OpenGL extensions: */
	if(haveCubeMaps)
		GLEXTTextureCubeMap::initExtension();
	if(haveSeamlessCubeMaps)
		GLARBSeamlessCubeMap::initExtension();
	if(haveMipmaps)
		GLEXTFramebufferObject::initExtension();
	if(haveAnisotropicFiltering)
		GLEXTTextureFilterAnisotropic::initExtension();
	
	glGenTextures(1,&textureObjectId);
	}

ImageTextureCubeMapNode::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&textureObjectId);
	}

/************************************************
Static elements of class ImageTextureCubeMapNode:
************************************************/

const char* ImageTextureCubeMapNode::className="ImageTextureCubeMap";

/****************************************
Methods of class ImageTextureCubeMapNode:
****************************************/

bool ImageTextureCubeMapNode::loadFaceImageFile(int faceIndex,IO::Directory& baseDirectory)
	{
	bool result=false;
	
	/* Determine the texture image file's format: */
	imageFileFormats[faceIndex]=Images::getImageFileFormat(faceUrls.getValue(faceIndex).c_str());
	
	if(imageFileFormats[faceIndex]<Images::IFF_NUM_FORMATS)
		{
		/* Load the image file into memory: */
		imageFiles[faceIndex]=new IO::VariableMemoryFile;
		IO::FilePtr sourceImageFile=baseDirectory.openFile(faceUrls.getValue(faceIndex).c_str());
		
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
			imageFiles[faceIndex]->writeRaw(buffer,bufferSize);
			}
		imageFiles[faceIndex]->flush();
		
		result=true;
		}
	else
		imageFiles[faceIndex]=0;
	
	return result;
	}

void ImageTextureCubeMapNode::checkCubeMap(void)
	{
	/* Check if all face images are present: */
	cubeMapValid=true;
	for(int faceIndex=0;faceIndex<6;++faceIndex)
		cubeMapValid=cubeMapValid&&imageFileFormats[faceIndex]<Images::IFF_NUM_FORMATS;
	
	/* Do some additional consistency checks prescribed by OpenGL here... */
	// IMPLEMENT ME!
	}

void ImageTextureCubeMapNode::uploadTexture(ImageTextureCubeMapNode::DataItem* dataItem) const
	{
	/* Set the texture's parameters: */
	int mml=dataItem->haveMipmaps?mipmapLevel.getValue():0;
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT,GL_TEXTURE_MAX_LEVEL,mml);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT,GL_TEXTURE_MIN_FILTER,filter.getValue()?(mml>0?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR):GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT,GL_TEXTURE_MAG_FILTER,filter.getValue()?GL_LINEAR:GL_NEAREST);
	if(anisotropyLevel.getValue()>1&&dataItem->haveAnisotropicFiltering)
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,anisotropyLevel.getValue());
	
	/* Upload the texture's face images: */
	for(int faceIndex=0;faceIndex<6;++faceIndex)
		{
		/* Load the face texture image: */
		Images::BaseImage faceTexture=Images::readGenericImageFile(*imageFiles[faceIndex]->getReader(),imageFileFormats[faceIndex]);
		
		/* Upload the face texture image: */
		faceTexture.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+faceIndex,0,false);
		
		/* Auto-generate all requested mipmap levels if mipmapping was requested and is supported: */
		if(mml>0)
			glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+faceIndex);
		}
	
	/* Mark the texture object as up-to-date: */
	dataItem->version=version;
	}

ImageTextureCubeMapNode::ImageTextureCubeMapNode(void)
	:filter(true),mipmapLevel(0),anisotropyLevel(1),
	 cubeMapValid(false),version(0)
	{
	/* Invalidate all face images: */
	for(int faceIndex=0;faceIndex<6;++faceIndex)
		imageFileFormats[faceIndex]=Images::IFF_NUM_FORMATS;
	}

const char* ImageTextureCubeMapNode::getClassName(void) const
	{
	return className;
	}

void ImageTextureCubeMapNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"faceUrls")==0)
		{
		vrmlFile.parseField(faceUrls);
		
		if(!faceUrls.getValues().empty())
			{
			if(faceUrls.getNumValues()!=6)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"faceUrls field needs exactly six elements");
			
			/* Immediately load the image files referenced by the faceUrls field: */
			cubeMapValid=true;
			for(int faceIndex=0;faceIndex<6;++faceIndex)
				cubeMapValid=loadFaceImageFile(faceIndex,vrmlFile.getBaseDirectory())&&cubeMapValid;
			}
		else
			{
			/* Invalidate all face images: */
			for(int faceIndex=0;faceIndex<6;++faceIndex)
				{
				imageFileFormats[faceIndex]=Images::IFF_NUM_FORMATS;
				imageFiles[faceIndex]=0;
				}
			cubeMapValid=false;
			}
		
		/* Invalidate the cached texture: */
		++version;
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

void ImageTextureCubeMapNode::update(void)
	{
	/* Clamp the mipmap level: */
	if(mipmapLevel.getValue()<0)
		mipmapLevel.setValue(0);
	
	/* Clamp the anisotropy level: */
	if(anisotropyLevel.getValue()<1)
		anisotropyLevel.setValue(1);
	}

void ImageTextureCubeMapNode::read(SceneGraphReader& reader)
	{
	/* Read all fields: */
	faceUrls.clearValues();
	reader.readField(filter);
	reader.readField(mipmapLevel);
	reader.readField(anisotropyLevel);
	
	/*********************************************************************
	Don't read the face texture images' URLs, but read their image file
	formats and file contents instead.
	*********************************************************************/
	
	cubeMapValid=true;
	for(int faceIndex=0;faceIndex<6;++faceIndex)
		{
		/* Read the face image file's format from the source file: */
		imageFileFormats[faceIndex]=static_cast<Images::ImageFileFormat>(reader.getFile().read<Misc::UInt8>());
		
		/* Read the face image file's content if the format is valid: */
		if(imageFileFormats[faceIndex]<Images::IFF_NUM_FORMATS)
			{
			/* Read the size of the face image file from the source file: */
			size_t imageFileSize=Misc::readVarInt32(reader.getFile());
			
			/* Read the face image file: */
			imageFiles[faceIndex]=new IO::VariableMemoryFile;
			while(imageFileSize>0)
				{
				/* Read a chunk of the source file into its internal read buffer: */
				void* buffer;
				size_t bufferSize=reader.getFile().readInBuffer(buffer,imageFileSize);
				imageFileSize-=bufferSize;
				
				/* Bail out if the file is all read: */
				if(bufferSize==0)
					break;
				
				/* Write the source file's buffer contents to the face image file: */
				imageFiles[faceIndex]->writeRaw(buffer,bufferSize);
				}
			imageFiles[faceIndex]->flush();
			}
		else
			{
			imageFiles[faceIndex]=0;
			cubeMapValid=false;
			}
		}
	
	/* Invalidate the cached texture: */
	++version;
	}

void ImageTextureCubeMapNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeField(filter);
	writer.writeField(mipmapLevel);
	writer.writeField(anisotropyLevel);
	
	/*********************************************************************
	Don't write the face texture images' URLs, but write their image file
	formats and file contents instead.
	*********************************************************************/
	
	for(int faceIndex=0;faceIndex<6;++faceIndex)
		{
		/* Write the face image file's format to the destination file: */
		writer.getFile().write(Misc::UInt8(imageFileFormats[faceIndex]));
		
		/* Write the face image file's contents if the image file format is valid: */
		if(imageFileFormats[faceIndex]<Images::IFF_NUM_FORMATS)
			{
			/* Write the size of the face image file to the destination file: */
			Misc::writeVarInt32(imageFiles[faceIndex]->getDataSize(),writer.getFile());
			
			/* Copy the image file's contents to the destination file: */
			IO::FilePtr reader=imageFiles[faceIndex]->getReader();
			while(true)
				{
				/* Read a chunk of the image file into its internal read buffer: */
				void* buffer;
				size_t bufferSize=reader->readInBuffer(buffer);
				
				/* Bail out if the file is all read: */
				if(bufferSize==0)
					break;
				
				/* Write the face image file's buffer contents to the destination file: */
				writer.getFile().writeRaw(buffer,bufferSize);
				}
			}
		}
	}

void ImageTextureCubeMapNode::setGLState(GLRenderState& renderState) const
	{
	/* Get the data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Check if the cube map is valid and cube mapping is supported: */
	if(cubeMapValid&&dataItem->haveCubeMaps)
		{
		/* Enable cube map textures: */
		renderState.enableTextureCubeMap();
		
		/* Bind the texture object: */
		renderState.bindTextureCubeMap(dataItem->textureObjectId);
		
		/* Upload the current texture image if the texture object is outdated: */
		if(dataItem->version!=version)
			uploadTexture(dataItem);
		
		/* Enable seamless cube mapping if supported: */
		if(dataItem->haveSeamlessCubeMaps)
			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		
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

void ImageTextureCubeMapNode::resetGLState(GLRenderState& renderState) const
	{
	#if 0
	
	/* Disable alpha testing for now: */
	glDisable(GL_ALPHA_TEST);
	
	#endif
	
	/* Don't do anything; next guy cleans up */
	}

void ImageTextureCubeMapNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the GL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	#if PRELOAD_TEXTURES
	
	/* Upload the initial texture object: */
	glBindTexture(GL_TEXTURE_CUBE_MAP_EXT,dataItem->textureObjectId);
	uploadTexture(dataItem);
	glBindTexture(GL_TEXTURE_CUBE_MAP_EXT,0);
	
	#endif
	}

void ImageTextureCubeMapNode::setFaceUrl(int faceIndex,const std::string& newUrl,IO::Directory& baseDirectory)
	{
	/* Store the face URL and load the referenced image file: */
	while(faceUrls.getNumValues()<=faceIndex)
		faceUrls.appendValue("");
	faceUrls.setValue(faceIndex,newUrl);
	loadFaceImageFile(faceIndex,baseDirectory);
	
	/* Check if the cube map is valid and invalidate the cached texture: */
	checkCubeMap();
	++version;
	}

void ImageTextureCubeMapNode::setFaceUrl(int faceIndex,const std::string& newUrl)
	{
	/* Store the face URL and load the referenced image file: */
	while(faceUrls.getNumValues()<=faceIndex)
		faceUrls.appendValue("");
	faceUrls.setValue(faceIndex,newUrl);
	loadFaceImageFile(faceIndex,*IO::Directory::getCurrent());
	
	/* Check if the cube map is valid and invalidate the cached texture: */
	checkCubeMap();
	++version;
	}

void ImageTextureCubeMapNode::setFaceImageFile(int faceIndex,Images::ImageFileFormat newImageFileFormat,IO::FilePtr newImageFile)
	{
	/* Store the new image file format: */
	imageFileFormats[faceIndex]=newImageFileFormat;
	
	/* Copy the given image file into memory: */
	imageFiles[faceIndex]=new IO::VariableMemoryFile;
	
	/* Copy the source image file's contents to the image file: */
	while(true)
		{
		/* Read a chunk of the source image file into its internal read buffer: */
		void* buffer;
		size_t bufferSize=newImageFile->readInBuffer(buffer);
		
		/* Bail out if the file is all read: */
		if(bufferSize==0)
			break;
		
		/* Write the source image file's buffer contents to the face image file: */
		imageFiles[faceIndex]->writeRaw(buffer,bufferSize);
		}
	imageFiles[faceIndex]->flush();
	
	/* Check if the cube map is valid and invalidate the cached texture: */
	checkCubeMap();
	++version;
	}

}

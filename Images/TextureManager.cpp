/***********************************************************************
TextureManager - Class to simplify texture management by encapsulating
loading textures from files, decoding image file formats, and uploading
decoded images to OpenGL texture objects for rendering.
Copyright (c) 2021 Oliver Kreylos

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

#include <Images/TextureManager.h>

#include <Misc/MessageLogger.h>
#include <IO/OpenFile.h>
 
namespace Images {

/****************************************
Methods of class TextureManager::Texture:
****************************************/

TextureManager::Texture::Texture(ImageFileFormat sImageFileFormat,GLenum sTarget,GLenum sInternalFormat)
	:imageFileFormat(sImageFileFormat),
	 target(sTarget),internalFormat(sInternalFormat),
	 settingsVersion(1)
	{
	/* Initialize wrapping and filtering modes: */
	wrapModes[1]=wrapModes[0]=GL_REPEAT;
	filterModes[1]=filterModes[0]=GL_LINEAR;
	}

void TextureManager::Texture::setWrapModes(GLenum wrapS,GLenum wrapT)
	{
	/* Update the wrapping modes and invalidate cached state: */
	wrapModes[0]=wrapS;
	wrapModes[1]=wrapT;
	++settingsVersion;
	}

void TextureManager::Texture::setFilterModes(GLenum minFilter,GLenum magFilter)
	{
	/* Update the filtering modes and invalidate cached state: */
	filterModes[0]=minFilter;
	filterModes[1]=magFilter;
	++settingsVersion;
	}

/*******************************
Methods of class TextureManager:
*******************************/

void* TextureManager::loaderThreadMethod(void)
	{
	/* Process image data loading requests until shut down: */
	while(true)
		{
		/* Wait for the next image data loading request: */
		Texture* texture=0;
		IO::FilePtr file;
		std::string fileName;
		{
		Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
		while(runLoaderThreads&&loadRequests.empty())
			loadRequestCond.wait(loadRequestLock);
		if(!runLoaderThreads)
			break;
		texture=loadRequests.front().texture;
		file=loadRequests.front().file;
		if(file==0)
			fileName=loadRequests.front().fileName;
		loadRequests.pop_front();
		}
		
		IO::VariableMemoryFilePtr imageFile;
		try
			{
			/* Open the image file if a file name was requested: */
			if(file==0)
				file=IO::openFile(fileName.c_str());
			
			/* Load the image file into memory: */
			imageFile=new IO::VariableMemoryFile;
			imageFile->readFile(*file);
			}
		catch(const std::runtime_error& err)
			{
			/* Print an error message or something: */
			Misc::formattedUserError("Images::TextureManager: Caught exception %s while loading image data",err.what());
			
			/* Destroy the in-memory image file: */
			imageFile=0;
			}
		
		/* Update the texture state structure: */
		{
		Threads::MutexCond::Lock textureMapLock(textureMapCond);
		texture->imageFile=imageFile;
		++numFilesLoaded;
		textureMapCond.broadcast();
		}
		}
	
	return 0;
	}

TextureManager::TextureManager(unsigned int sNumLoaderThreads)
	:lastHandle(0),textureMap(17),numFilesLoaded(0),
	 numLoaderThreads(sNumLoaderThreads),loaderThreads(new Threads::Thread[numLoaderThreads]),
	 runLoaderThreads(false),
	 numLoadRequests(0)
	{
	/* Start the image data loader threads: */
	runLoaderThreads=true;
	for(unsigned int i=0;i<numLoaderThreads;++i)
		loaderThreads[i].start(this,&TextureManager::loaderThreadMethod);
	}

TextureManager::~TextureManager(void)
	{
	/* Shut down all image loader threads: */
	{
	Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
	runLoaderThreads=false;
	loadRequestCond.broadcast();
	}
	
	/* Wake up anyone still waiting on loading to complete (which isn't going to happen now): */
	textureMapCond.broadcast();
	
	/* Destroy all image loader threads: */
	for(unsigned int i=0;i<numLoaderThreads;++i)
		loaderThreads[i].join();
	delete[] loaderThreads;
	}

TextureManager::Handle TextureManager::loadTexture(const char* fileName,GLenum target,GLenum internalFormat,bool locked)
	{
	/* Determine the given image file's image file format: */
	ImageFileFormat format=getImageFileFormat(fileName);
	
	/* Create a new texture structure and store it in the map: */
	Texture* texture=0;
	if(locked)
		{
		for(++lastHandle;lastHandle==0||textureMap.isEntry(lastHandle);++lastHandle)
			;
		TextureMap::Iterator tIt=textureMap.setAndFindEntry(TextureMap::Entry(lastHandle,Texture(format,target,internalFormat)));
		texture=&tIt->getDest();
		}
	else
		{
		Threads::MutexCond::Lock textureMapLock(textureMapCond);
		for(++lastHandle;lastHandle==0||textureMap.isEntry(lastHandle);++lastHandle)
			;
		TextureMap::Iterator tIt=textureMap.setAndFindEntry(TextureMap::Entry(lastHandle,Texture(format,target,internalFormat)));
		texture=&tIt->getDest();
		}
	
	/* Ask the loader threads to load the image file: */
	{
	Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
	++numLoadRequests;
	loadRequests.push_back(LoadRequest(texture,fileName));
	loadRequestCond.signal();
	}
	
	return lastHandle;
	}

TextureManager::Handle TextureManager::loadTexture(IO::File& file,ImageFileFormat format,GLenum target,GLenum internalFormat,bool locked)
	{
	/* Create a new texture structure and store it in the map: */
	Texture* texture=0;
	if(locked)
		{
		for(++lastHandle;lastHandle==0||textureMap.isEntry(lastHandle);++lastHandle)
			;
		TextureMap::Iterator tIt=textureMap.setAndFindEntry(TextureMap::Entry(lastHandle,Texture(format,target,internalFormat)));
		texture=&tIt->getDest();
		}
	else
		{
		Threads::MutexCond::Lock textureMapLock(textureMapCond);
		for(++lastHandle;lastHandle==0||textureMap.isEntry(lastHandle);++lastHandle)
			;
		TextureMap::Iterator tIt=textureMap.setAndFindEntry(TextureMap::Entry(lastHandle,Texture(format,target,internalFormat)));
		texture=&tIt->getDest();
		}
	
	/* Ask the loader threads to load the image file: */
	{
	Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
	++numLoadRequests;
	loadRequests.push_back(LoadRequest(texture,file));
	loadRequestCond.signal();
	}
	
	return lastHandle;
	}

void TextureManager::waitForImageData(void)
	{
	/* Block until the number of loaded files matches the number of load requests: */
	Threads::MutexCond::Lock textureMapLock(textureMapCond);
	while(runLoaderThreads&&numFilesLoaded!=numLoadRequests)
		textureMapCond.wait(textureMapLock);
	}

}

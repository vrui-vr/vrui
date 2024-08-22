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

#ifndef IMAGES_TEXTUREMANAGER_INCLUDED
#define IMAGES_TEXTUREMANAGER_INCLUDED

#include <deque>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/MutexCond.h>
#include <Threads/Thread.h>
#include <IO/File.h>
#include <IO/VariableMemoryFile.h>
#include <GL/gl.h>
#include <Images/ImageFileFormats.h>

namespace Images {

class TextureManager
	{
	/* Embedded classes: */
	public:
	typedef unsigned int Handle; // Type for handles identifying textures; 0 is an invalid handle
	
	class Texture // Class to represent textures on the CPU side of things
		{
		friend class TextureManager;
		
		/* Elements: */
		private:
		IO::VariableMemoryFilePtr imageFile; // Pointer to an in-memory file containing the texture's image data
		ImageFileFormat imageFileFormat; // Format of the image data contained in the image file
		GLenum target; // Texture target to which the texture image will be bound
		GLenum internalFormat; // Internal texture format for the texture image
		GLenum wrapModes[2]; // Horizontal and vertical texture wrapping modes
		GLenum filterModes[2]; // Minification and magnification texture filtering modes
		unsigned int settingsVersion; // Version number of texture settings to invalidate cached settings
		
		/* Constructors and destructors: */
		public:
		Texture(ImageFileFormat sImageFileFormat,GLenum sTarget,GLenum sInternalFormat); // Creates and empty texture of the given image file foramt for the given texture target
		
		/* Methods: */
		void setWrapModes(GLenum wrapS,GLenum wrapT); // Sets the texture's coordinate wrapping modes
		void setFilterModes(GLenum minFilter,GLenum magFilter); // Sets the texture's minification and magnification filtering modes
		};
	
	private:
	typedef Misc::HashTable<Handle,Texture> TextureMap; // Type for hash tables mapping texture handles to CPU-side texture states
	
	struct LoadRequest // Structure defining a request to load image data into a texture state
		{
		/* Elements: */
		public:
		Texture* texture; // Pointer to the texture state structure to be updated
		IO::FilePtr file; // Pointer to the file from which to load image data
		std::string fileName; // Name of the file from which to load image data if file pointer is invalid
		
		/* Constructors and destructors: */
		LoadRequest(Texture* sTexture,IO::File& sFile) // Creates a load request for the given texture state and already-open image file
			:texture(sTexture),file(&sFile)
			{
			}
		LoadRequest(Texture* sTexture,const char* sFileName) // Creates a load request for the given texture state and an image file of the given name
			:texture(sTexture),fileName(sFileName)
			{
			}
		};
	
	/* Elements: */
	Handle lastHandle; // Handle that was assigned to the last created texture
	Threads::MutexCond textureMapCond; // Mutex protecting the CPU-side texture state map and condition variable to signal completion of an image data load request
	TextureMap textureMap; // Hash table mapping texture handles to CPU-side texture states
	size_t numFilesLoaded; // Total number of image data files that have been loaded; can be read without locking
	unsigned int numLoaderThreads; // Number of threads to load encoded image data from image files
	Threads::Thread* loaderThreads; // Array of encoded image data loader threads
	volatile bool runLoaderThreads; // Flag to keep the image data loader threads running
	Threads::MutexCond loadRequestCond; // Condition variable to signal a new image data loading request
	size_t numLoadRequests; // Total number of image data load requests that have been issued; can be read without locking
	std::deque<LoadRequest> loadRequests; // List of pending image data loading requests
	
	/* Private methods: */
	void* loaderThreadMethod(void); // Method running an image data loader thread
	
	/* Constructors and destructors: */
	public:
	TextureManager(unsigned int sNumLoaderThreads); // Creates an empty texture manager with the given number of image data loader threads
	~TextureManager(void); // Releases all resources and destroys the texture manager
	
	/* Methods: */
	Threads::MutexCond& getTextureMapCond(void) // Returns a reference to the mutex serializing access to the texture map, to create locks
		{
		return textureMapCond;
		}
	Handle loadTexture(const char* fileName,GLenum target,GLenum internalFormat,bool locked =false); // Loads an image from the given file name / URL for the given texture target and internal format and returns a texture handle; if locked flag is true, caller already holds lock on texture map
	Handle loadTexture(IO::File& file,ImageFileFormat format,GLenum target,GLenum internalFormat,bool locked =false); // Loads an image of the given format from the given file for the given texture target and internal format and returns a texture handle; if locked flag is true, caller already holds lock on texture map
	void waitForImageData(void); // Blocks until all texture images have been loaded into CPU-side memory; keeps blocking if additional textures are requested while blocked
	Texture* getTexture(Handle handle,bool locked) // Returns a pointer to the CPU-side texture of the given handle; if locked flag is true, caller already holds a lock on the texture map
		{
		Texture* result;
		if(locked)
			result=&textureMap.getEntry(handle).getDest();
		else
			{
			Threads::MutexCond::Lock textureMapLock(textureMapCond);
			result=&textureMap.getEntry(handle).getDest();
			}
		return result;
		}
	};

}

#endif

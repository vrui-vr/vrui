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

#ifndef SCENEGRAPH_IMAGETEXTURECUBEMAPNODE_INCLUDED
#define SCENEGRAPH_IMAGETEXTURECUBEMAPNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <IO/VariableMemoryFile.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Images/ImageFileFormats.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/TextureNode.h>

/* Forward declarations: */
namespace IO {
class Directory;
}

namespace SceneGraph {

class ImageTextureCubeMapNode:public TextureNode,public GLObject
	{
	/* Embedded classes: */
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		bool haveCubeMaps; // Flag if the GL_EXT_texture_cube_map extension is supported
		bool haveSeamlessCubeMaps; // Flag if the GL_ARB_seamless_cube_map extension is supported
		bool haveMipmaps; // Flag if the GL_EXT_framebuffer_object extension is supported and allows mipmap creation
		bool haveAnisotropicFiltering; // Flag if the GL_EXT_texture_filter_anisotropic extension is supported
		GLuint textureObjectId; // ID of texture object
		unsigned int version; // Version of texture in texture object
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	MFString faceUrls; // Field containing the URLs of the six cube map face images in the order +X, -X, +Y, -Y, +Z, -Z
	SFBool filter; // Enables bilinear (or trilinear if mipmapLevel>0) filtering
	SFInt mipmapLevel; // Maximum mipmap level that should be generated from the texture image; 0 disables mipmapping
	SFInt anisotropyLevel; // Maximum anisotropy sampling level that should be used for the texture; 1 disables anisotropic filtering
	
	/* Derived state: */
	protected:
	Images::ImageFileFormat imageFileFormats[6]; // Format of the six image files containing the texture's pixels
	IO::VariableMemoryFilePtr imageFiles[6]; // In-memory copies of the six image files containing the texture's pixels
	bool cubeMapValid; // Flag if the cube map texture represented by the six face image files is valid
	unsigned int version; // Version number of texture
	
	/* Private methods: */
	private:
	bool loadFaceImageFile(int faceIndex,IO::Directory& baseDirectory); // Loads the cube map face image file referenced by the current value of the url field, relative to the given base directory; returns true if a valid image file was loaded
	void checkCubeMap(void); // Checks if the cube map is valid after any face images have been loaded
	void uploadTexture(DataItem* dataItem) const; // Uploads the current texture image into the given data item's texture object
	
	/* Constructors and destructors: */
	public:
	ImageTextureCubeMapNode(void); // Creates a default cube map image texture node with no texture image
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from class AttributeNode: */
	virtual void setGLState(GLRenderState& renderState) const;
	virtual void resetGLState(GLRenderState& renderState) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	void setFaceUrl(int faceIndex,const std::string& newUrl,IO::Directory& baseDirectory); // Loads an image texture from the given URL relative to the given base directory into the given cube map face
	void setFaceUrl(int faceIndex,const std::string& newUrl); // Ditto, with URL relative to the current directory into the given cube map face
	void setFaceImageFile(int faceIndex,Images::ImageFileFormat newImageFileFormat,IO::FilePtr newImageFile); // Directly loads an image texture from the given file into the given cube map face, bypassing the url field
	};

typedef Misc::Autopointer<ImageTextureCubeMapNode> ImageTextureCubeMapNodePointer;

}

#endif

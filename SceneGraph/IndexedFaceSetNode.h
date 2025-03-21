/***********************************************************************
IndexedFaceSetNode - Class for sets of polygonal faces as renderable
geometry.
Copyright (c) 2009-2023 Oliver Kreylos

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

#ifndef SCENEGRAPH_INDEXEDFACESETNODE_INCLUDED
#define SCENEGRAPH_INDEXEDFACESETNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/TextureCoordinateNode.h>

namespace SceneGraph {

class IndexedFaceSetNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef SF<ColorNodePointer> SFColorNode;
	typedef SF<CoordinateNodePointer> SFCoordinateNode;
	typedef SF<NormalNodePointer> SFNormalNode;
	typedef SF<TextureCoordinateNodePointer> SFTextureCoordinateNode;
	
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object containing the face set's vertices, if supported
		GLuint indexBufferObjectId; // ID of index buffer object containing the face set's triangle vertex indices, if supported
		ptrdiff_t texCoordOffset; // Offset of texture coordinate in interleaved vertex buffer
		ptrdiff_t colorOffset; // Offset of color in interleaved vertex buffer
		ptrdiff_t normalOffset; // Offset of normal vector in interleaved vertex buffer
		ptrdiff_t positionOffset; // Offset of vertex position in interleaved vertex buffer
		size_t vertexSize; // Total vertex size in interleaved vertex buffer
		int vertexArrayPartsMask; // Bit mask of used vertex properties in vertex buffer
		unsigned int version; // Version of face set stored in the buffer objects
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFTextureCoordinateNode texCoord;
	SFColorNode color;
	SFNormalNode normal;
	SFCoordinateNode coord;
	MFInt texCoordIndex;
	MFInt colorIndex;
	SFBool colorPerVertex;
	MFInt normalIndex;
	SFBool normalPerVertex;
	MFInt coordIndex;
	SFBool ccw;
	SFBool convex;
	SFBool solid;
	SFFloat creaseAngle;
	
	/* Derived state: */
	protected:
	bool haveColors; // Flag if the face set's vertices have per-vertex color values
	Box bbox; // Bounding box containing all vertices referenced by the face set
	size_t numValidFaces; // Number of valid (>=3 vertices) faces in the indexed face set
	int vertexIndexMin,vertexIndexMax; // Range of vertex indices used by the indexed face set's valid faces
	size_t maxNumFaceVertices; // Maximum number of vertices in any face in the indexed face set
	size_t totalNumFaceVertices; // Total number of face vertices in the indexed face set
	size_t totalNumTriangles; // Total number of triangles defined by the indexed face set, assuming trivial triangulation
	unsigned int version; // Version number of face set
	
	/* Private methods: */
	private:
	void testCollisionSolidCcw(SphereCollisionQuery& collisionQuery) const; // Collision test for solid face set with counter-clockwise face winding order
	void testCollisionSolidCw(SphereCollisionQuery& collisionQuery) const; // Collision test for solid face set with clockwise face winding order
	void testCollisionNonSolid(SphereCollisionQuery& collisionQuery) const; // Collision test for non-solid face set
	
	/* Protected methods: */
	protected:
	void uploadConvexFaceSet(DataItem* dataItem,GLubyte* bufferPtr) const; // Uploads new face set into OpenGL buffers, assuming that all faces are convex
	void uploadNonConvexFaceSet(DataItem* dataItem,GLubyte* bufferPtr) const; // Uploads new face set into OpenGL buffers, assuming that all faces are convex
	
	/* Constructors and destructors: */
	public:
	IndexedFaceSetNode(void); // Creates a default face set
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from class GeometryNode: */
	virtual bool canCollide(void) const;
	virtual int getGeometryRequirementMask(void) const;
	virtual Box calcBoundingBox(void) const;
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(int appearanceRequirementMask,GLRenderState& renderState) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

typedef Misc::Autopointer<IndexedFaceSetNode> IndexedFaceSetNodePointer;

}

#endif

/***********************************************************************
ElevationGridNode - Class for quad-based height fields as renderable
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

#ifndef SCENEGRAPH_ELEVATIONGRIDNODE_INCLUDED
#define SCENEGRAPH_ELEVATIONGRIDNODE_INCLUDED

#include <stddef.h>
#include <IO/Directory.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/TextureCoordinateNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/ColorMapNode.h>
#include <SceneGraph/ImageProjectionNode.h>

namespace SceneGraph {

class ElevationGridNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef SF<TextureCoordinateNodePointer> SFTextureCoordinateNode;
	typedef SF<ColorNodePointer> SFColorNode;
	typedef SF<NormalNodePointer> SFNormalNode;
	typedef SF<ColorMapNodePointer> SFColorMapNode;
	typedef SF<ImageProjectionNodePointer> SFImageProjectionNode;
	
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		bool havePrimitiveRestart; // Flag whether the local OpenGL supports the GL_NV_primitive_restart extension
		GLuint vertexBufferObjectId; // ID of vertex buffer object containing the vertices, if supported
		ptrdiff_t vertexSize,texCoordOffset,colorOffset,normalOffset,positionOffset; // Layout of vertex data in the vertex buffer
		int vertexArrayPartsMask; // Bit mask of used vertex components
		GLuint indexBufferObjectId; // ID of index buffer object containing the vertex indices, if supported
		GLuint numQuads; // Number of quads in a non-indexed quad set
		GLuint numTriangles; // Number of triangles in a non-indexed quad/triangle set
		unsigned int version; // Version of point set stored in vertex buffer object
		
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
	SFColorMapNode colorMap;
	SFImageProjectionNode imageProjection;
	SFBool colorPerVertex;
	SFNormalNode normal;
	SFBool normalPerVertex;
	SFFloat creaseAngle;
	SFPoint origin;
	SFInt xDimension;
	SFFloat xSpacing;
	SFInt zDimension;
	SFFloat zSpacing;
	MFFloat height;
	MFString heightUrl; // URL of external file(s) containing the height array in several file formats
	MFString heightUrlFormat; // Format of external height file: "BIL", "ARC/INFO ASCII GRID", "RAW (UINT8 SINT8 UINT16 SINT16 UINT32 SINT32 FLOAT32 FLOAT64) (LE BE)"
	SFFloat heightScale; // Scale factor applied to all height values
	SFBool heightIsY;
	SFBool removeInvalids; // Flag to remove "invalid" elevation samples from the height array by leaving holes in the triangulation
	SFFloat invalidHeight; // Value to indicate "invalid" elevations
	SFBool ccw;
	SFBool solid;
	
	/* Derived state: */
	public:
	IO::DirectoryPtr baseDirectory; // Base directory for relative URLs
	unsigned int propMask; // Mask of elevation grid properties that were explicitly specified in the VRML file
	protected:
	bool valid; // Flag whether the elevation grid has a valid representation
	bool haveInvalids; // Flag whether there are some invalid elevation samples that need to be removed
	bool canRender; // Flag whether the elevation grid supports rendering (at least one row/column of quads)
	bool haveColors; // Flag whether the elevation grid defines per-vertex or per-face colors
	bool indexed; // Flag whether the elevation grid is represented as a set of indexed quad strips or a set of quads
	Box bbox; // Bounding box of the elevation grid
	unsigned int version; // Version number of elevation grid
	
	/* Private methods: */
	Point* calcVertices(void) const; // Returns a new-allocated array of vertex positions, untransformed by the point transformation
	Vector* calcQuadNormals(void) const; // Returns a new-allocated array of non-normalized per-quad normal vectors
	int* calcHoleyQuadCases(GLuint& numQuads,GLuint& numTriangles) const; // Returns a new-allocated array of quad triangulation cases
	Vector* calcHoleyQuadNormals(const int* quadCases) const; // Returns a new-allocated array of non-normalized per-quad normal vectors with removal of invalid samples
	void uploadIndexedQuadStripSet(DataItem* dataItem) const; // Uploads the elevation grid as a set of indexed quad strips when there is no point transform
	void uploadQuadSet(void) const; // Uploads the elevation grid as a set of quads
	void uploadHoleyQuadTriangleSet(GLuint& numQuads,GLuint& numTriangles) const; // Uploads the elevation grid as a set of quads and triangles with removal of invalid samples; updates passed number of quads and triangles
	
	/* Constructors and destructors: */
	public:
	ElevationGridNode(void); // Creates a default elevation grid
	
	/* Methods from Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from GeometryNode: */
	virtual bool canCollide(void) const;
	virtual int getGeometryRequirementMask(void) const;
	virtual Box calcBoundingBox(void) const;
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(int appearanceRequirementMask,GLRenderState& renderState) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

#endif

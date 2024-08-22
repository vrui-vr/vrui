/***********************************************************************
SphereNode - Class for spheres as renderable geometry.
Copyright (c) 2013-2023 Oliver Kreylos

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

#ifndef SCENEGRAPH_SPHERENODE_INCLUDED
#define SCENEGRAPH_SPHERENODE_INCLUDED

#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>

namespace SceneGraph {

class SphereNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object containing the sphere's vertices, if supported
		GLuint indexBufferObjectId; // ID of index buffer object containing the sphere's triangle vertex indices, if supported
		size_t numVertices; // Number of vertices stored in the vertex buffer object
		ptrdiff_t texCoordOffset; // Offset of texture coordinate in interleaved vertex buffer
		ptrdiff_t normalOffset; // Offset of normal vector in interleaved vertex buffer
		ptrdiff_t positionOffset; // Offset of vertex position in interleaved vertex buffer
		size_t vertexSize; // Total vertex size in interleaved vertex buffer
		int vertexArrayPartsMask; // Bit mask of used vertex properties in vertex buffer
		unsigned int version; // Version of sphere stored in the buffer objects
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFPoint center;
	SFFloat radius;
	SFInt numSegments;
	SFBool latLong;
	SFBool ccw; // Flag whether to show the outside or inside of the sphere
	
	/* Derived state: */
	private:
	unsigned int version; // Version number of sphere
	
	/* Private methods: */
	void updateArrays(DataItem* dataItem) const; // Updates the vertex and index arrays to render the sphere
	
	/* Constructors and destructors: */
	public:
	SphereNode(void); // Creates a default sphere (centered at origin, radius 1)
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual EventOut* getEventOut(const char* fieldName) const;
	virtual EventIn* getEventIn(const char* fieldName);
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

typedef Misc::Autopointer<SphereNode> SphereNodePointer;

}

#endif

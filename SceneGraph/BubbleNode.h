/***********************************************************************
BubbleNode - Class for speech bubbles as renderable geometry.
Copyright (c) 2023 Oliver Kreylos

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

#ifndef SCENEGRAPH_BUBBLENODE_INCLUDED
#define SCENEGRAPH_BUBBLENODE_INCLUDED

#include <Misc/Autopointer.h>
#include <Geometry/Point.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>

namespace SceneGraph {

class BubbleNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object containing the face set's vertices, if supported
		unsigned int version; // Version of bubble geometry stored in the buffer object
		size_t numVertices; // Number of vertices currently stored in the buffer object
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFPoint origin; // Origin point of the bubble's interior
	SFFloat width,height; // Width and height of the bubble's interior
	SFFloat marginWidth; // Width of the margin between the bubble's interior and its border
	SFColor interiorColor; // Color for the bubble's interior and margin
	SFFloat borderDepth; // Depth of border raised above the bubble's interior
	SFFloat borderWidth; // Width of border around the bubble
	SFFloat backsideDepth; // Depth of bubble's backside behind its interior
	SFFloat pointHeight; // Height of the bubble's point
	SFString pointAlignment; // Alignment of the bubble's point: LEFTOUT, LEFTIN, CENTER, RIGHTIN, RIGHTOUT
	SFColor borderColor; // Color for the bubble's border and backside
	SFInt numSegments; // Number of segments to represent the bubble's rounded parts
	
	/* Derived state: */
	protected:
	Scalar x0,x1,y0,y1,z0,z1,z2,px0,px1,px2; // Bubble layout parameters
	GLsizei numComponentVertices[6]; // Number of vertices for each of the bubble components
	size_t numVertices; // Total number of vertices
	unsigned int version; // Version of bubble geometry defined by its fields
	
	/* Private methods: */
	void updateVertexBuffer(DataItem* dataItem) const; // Method to upload geometry into a bound vertex buffer object
	
	/* Constructors and destructors: */
	public:
	BubbleNode(void); // Creates a default bubble
	
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
	virtual void glRenderAction(int appearanceRequirementsMask,GLRenderState& renderState) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	Point calcBubblePoint(void) const; // Returns the point of the bubble's point
	};

typedef Misc::Autopointer<BubbleNode> BubbleNodePointer;

}

#endif

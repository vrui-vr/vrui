/***********************************************************************
LineSetNode - Class for sets of lines as renderable geometry, with a
creation interface mimicking OpenGL immediate mode rendering.
Copyright (c) 2025 Oliver Kreylos

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

#ifndef SCENEGRAPH_LINESETNODE_INCLUDED
#define SCENEGRAPH_LINESETNODE_INCLUDED

#include <stddef.h>
#include <vector>
#include <Misc/Autopointer.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>

namespace SceneGraph {

class LineSetNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef unsigned short VertexIndex; // Type for vertex indices; short because objects are assumed to be small
	typedef GLColor<GLubyte,4> VertexColor; // Type for vertex colors
	private:
	struct Vertex; // Structure to store line vertices
	typedef std::vector<Vertex> VertexList; // Type for lists of vertices
	struct Line; // Forward declaration of structure for lines
	typedef std::vector<Line> LineList; // Type for lists of lines
	struct DataItem; // Structure to store OpenGL state
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFFloat lineWidth;
	
	/* Internal state: */
	private:
	VertexIndex numVertices; // Number of vertices in the line set
	VertexList vertices; // List containing the line vertices
	size_t numLines; // Number of lines in the line set
	LineList lines; // List containing the lines
	unsigned int arraysVersion; // Version number of the vertex and line lists
	VertexColor color; // Color to use for all subsequent vertices
	
	/* Constructors and destructors: */
	public:
	LineSetNode(void); // Creates an empty line set
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void update(void);
	
	/* Methods from class GeometryNode: */
	virtual bool canCollide(void) const;
	virtual int getGeometryRequirementMask(void) const;
	virtual Box calcBoundingBox(void) const;
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(int appearanceRequirementsMask,GLRenderState& renderState) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	VertexIndex getNextVertexIndex(void) const // Returns the index of the next vertex that will be added
		{
		/* Return the number of vertices currently in the line set: */
		return numVertices;
		}
	VertexIndex addVertex(const Color& color,const Point& position); // Adds a new vertex with the given color and position; returns vertex's index
	VertexIndex addVertex(const VertexColor& color,const Point& position); // Ditto, using 8-bit vertex color
	void setColor(const Color& newColor); // Sets the color to be used for all subsequent vertices
	void setColor(const VertexColor& newColor); // Ditto, using 8-bit color
	VertexIndex addVertex(const Point& position); // Adds a new vertex with the current color and the given position; returns vertex's index
	void addLine(VertexIndex v0,VertexIndex v1); // Adds a new line using the vertices of the given indices
	void addLine(const Point& p0,const Point& p1); // Adds a new line using the given vertices and the current color
	void addCircle(const Point& center,const Rotation& frame,Scalar radius,Scalar tolerance); // Adds a circle
	void addCircleArc(const Point& center,const Rotation& frame,Scalar radius,Scalar angle0,Scalar angle1,Scalar tolerance); // Adds a circular arc between the given two angles in radians, angle0<angle1
	void addNumber(const Point& anchor,const Rotation& frame,Scalar size,int hAlign,int vAlign,const char* number);
	void clear(void); // Deletes all vertices and lines from the line set
	};

typedef Misc::Autopointer<LineSetNode> LineSetNodePointer;

}

#endif

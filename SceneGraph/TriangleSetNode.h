/***********************************************************************
TriangleSetNode - Class for sets of triangles as renderable geometry,
with a creation interface mimicking OpenGL immediate mode rendering.
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

#ifndef SCENEGRAPH_TRIANGLESETNODE_INCLUDED
#define SCENEGRAPH_TRIANGLESETNODE_INCLUDED

#include <stddef.h>
#include <vector>
#include <Misc/Autopointer.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>

namespace SceneGraph {

class TriangleSetNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef unsigned short VertexIndex; // Type for vertex indices; short because objects are assumed to be small
	typedef GLColor<GLubyte,4> VertexColor; // Type for vertex colors
	private:
	struct Vertex; // Structure to store line vertices
	typedef std::vector<Vertex> VertexList; // Type for lists of vertices
	struct Triangle; // Forward declaration of structure for triangles
	typedef std::vector<Triangle> TriangleList; // Type for lists of triangles
	struct DataItem; // Structure to store OpenGL state
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFBool twoSided;
	
	/* Internal state: */
	private:
	VertexIndex numVertices; // Number of vertices in the triangle set
	VertexList vertices; // List containing the triangle vertices
	size_t numTriangles; // Number of triangles in the triangle set
	TriangleList triangles; // List containing the triangles
	unsigned int arraysVersion; // Version number of the vertex and triangle lists
	VertexColor color; // Color to use for all subsequent vertices
	Vector normal; // Normal vector to use for all subsequent vertices
	
	/* Constructors and destructors: */
	public:
	TriangleSetNode(void); // Creates an empty triangle set
	
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
		/* Return the number of vertices currently in the triangle set: */
		return numVertices;
		}
	VertexIndex addVertex(const Color& color,const Vector& normal,const Point& position); // Adds a new vertex with the given color, normal vector, and position; the normal vector is assumed to be unit length; returns vertex's index
	VertexIndex addVertex(const VertexColor& color,const Vector& normal,const Point& position); // Ditto, using 8-bit vertex color
	void setColor(const Color& newColor); // Sets the color to be used for all subsequent vertices
	void setColor(const VertexColor& newColor); // Ditto, using 8-bit color
	void setNormal(const Vector& newNormal); // Sets the normal vector to be used for all subsequent vertices; the normal vector is assumed to be unit length
	VertexIndex addVertex(const Point& position); // Adds a new vertex with the current color and normal vector and the given position; returns vertex's index
	void addTriangle(VertexIndex v0,VertexIndex v1,VertexIndex v2); // Adds a new triangle using the vertices of the given indices
	void addTriangle(const Point& p0,const Point& p1,const Point& p2); // Adds a new triangle using the given vertices and the current color and normal vector
	void addQuad(VertexIndex v0,VertexIndex v1,VertexIndex v2,VertexIndex v3); // Adds a new quad defined by a loop of four vertices in counter-clockwise order
	void addPolygon(VertexIndex begin,VertexIndex end); // Adds a new polygon using all vertices in the half-open index range [begin, end)
	void addTriangleFan(VertexIndex begin,VertexIndex end); // Adds a new triangle fan using all vertices in the half-open index range [begin, end)
	void addTriangleStrip(VertexIndex begin,VertexIndex end); // Adds a new triangle strip using all vertices in the half-open index range [begin, end)
	static unsigned int calcCircleTessellation(Scalar radius,Scalar tolerance); // Returns the minimum number of vertices required to render a disk of the given radius within the given tolerance
	static double calcCircleAdjustedRadius(Scalar radius,unsigned int tessellation); // Returns an adjusted radius to render a disk of the given radius and number of vertices with optimal approximation
	VertexIndex addTessellatedCircle(const Point& center,const Rotation& frame,double adjustedRadius,unsigned int tessellation); // Adds a circle of vertices with the given tessellation and adjusted radius, but does not add triangles; returns the index of the circle's first vertex
	void addTessellatedDisk(VertexIndex circleBase,unsigned int tessellation); // Adds a disk for a previously added circle of vertices with the given tessellation and the given base index
	void addDisk(const Point& center,const Rotation& frame,Scalar radius,Scalar tolerance) // Adds a disk with the given radius and approximation tolerance
		{
		/* Calculate a tessellation and adjusted radius to render the disk: */
		unsigned int tessellation=calcCircleTessellation(radius,tolerance);
		double adjustedRadius=calcCircleAdjustedRadius(radius,tessellation);
		
		/* Add the disk's vertices and triangles: */
		VertexIndex circleBase=addTessellatedCircle(center,frame,adjustedRadius,tessellation);
		addTessellatedDisk(circleBase,tessellation);
		}
	void addTessellatedRing(VertexIndex innerCircleBase,unsigned int innerTessellation,VertexIndex outerCircleBase,unsigned int outerTessellation); // Adds a ring between two previously added circles of vertices with the given tessellations and base indices
	void addRing(const Point& center,const Rotation& frame,Scalar innerRadius,Scalar outerRadius,Scalar tolerance) // Adds a ring with the given inner and outer radii and approximation tolerance
		{
		/* Calculate inner and outer tessellations and adjusted radii to render the ring: */
		unsigned int innerTessellation=calcCircleTessellation(innerRadius,tolerance);
		double innerAdjustedRadius=calcCircleAdjustedRadius(innerRadius,innerTessellation);
		unsigned int outerTessellation=calcCircleTessellation(outerRadius,tolerance);
		double outerAdjustedRadius=calcCircleAdjustedRadius(outerRadius,outerTessellation);
		
		/* Add the ring's inner and outer vertices and triangles: */
		VertexIndex innerCircleBase=addTessellatedCircle(center,frame,innerAdjustedRadius,innerTessellation);
		VertexIndex outerCircleBase=addTessellatedCircle(center,frame,outerAdjustedRadius,outerTessellation);
		addTessellatedRing(innerCircleBase,innerTessellation,outerCircleBase,outerTessellation);
		}
	void clear(void); // Deletes all vertices and triangles from the triangle set
	};

typedef Misc::Autopointer<TriangleSetNode> TriangleSetNodePointer;

}

#endif

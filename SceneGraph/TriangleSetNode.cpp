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

#include <SceneGraph/TriangleSetNode.h>

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryVertex.icpp>
#include <SceneGraph/BaseAppearanceNode.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*********************************************
Declaration of struct TriangleSetNode::Vertex:
*********************************************/

struct TriangleSetNode::Vertex
	{
	/* Elements: */
	public:
	VertexColor color; // Vertex's color
	Vector normal; // Vertex's normal vector
	Point position; // Vertex's position
	
	/* Constructors and destructors: */
	Vertex(const VertexColor& sColor,const Vector& sNormal,const Point& sPosition)
		:color(sColor),normal(sNormal),position(sPosition)
		{
		}
	};

/***********************************************
Declaration of struct TriangleSetNode::Triangle:
***********************************************/

struct TriangleSetNode::Triangle
	{
	/* Elements: */
	public:
	VertexIndex v[3]; // Array of the indices of the triangle's three vertices
	
	/* Constructors and destructors: */
	Triangle(VertexIndex v0,VertexIndex v1,VertexIndex v2) // Creates triangle from triplet of vertex indices
		{
		v[0]=v0;
		v[1]=v1;
		v[2]=v2;
		}
	};

/***********************************************
Declaration of struct TriangleSetNode::DataItem:
***********************************************/

struct TriangleSetNode::DataItem:public GLObject::DataItem
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<void,0,GLubyte,4,GLfloat,GLfloat,3> BufferVertex; // Type for vertices stored in buffers
	typedef GLushort BufferIndex; // Type for line vertex indices stored in buffers
	
	/* Elements: */
	GLuint vertexBufferId; // ID of vertex buffer holding the list of vertices
	GLuint triangleBufferId; // ID of index buffer holding the list of triangle vertex indices
	unsigned int arraysVersion; // Version number of the vertex and triangle arrays held in the buffers
	
	/* Constructors and destructors: */
	DataItem(void);
	virtual ~DataItem(void);
	};

/******************************************
Methods of class TriangleSetNode::DataItem:
******************************************/

TriangleSetNode::DataItem::DataItem(void)
	:vertexBufferId(0),triangleBufferId(0),
	 arraysVersion(0)
	{
	/* Initialize the vertex buffer object extension: */
	GLARBVertexBufferObject::initExtension();
	
	/* Create the vertex and index buffer objects: */
	glGenBuffersARB(1,&vertexBufferId);
	glGenBuffersARB(1,&triangleBufferId);
	}

TriangleSetNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex and index buffer objects: */
	glDeleteBuffersARB(1,&vertexBufferId);
	glDeleteBuffersARB(1,&triangleBufferId);
	}

/****************************************
Static elements of class TriangleSetNode:
****************************************/

const char* TriangleSetNode::className="TriangleSet";

/********************************
Methods of class TriangleSetNode:
********************************/

TriangleSetNode::TriangleSetNode(void)
	:twoSided(false),
	 numVertices(0),numTriangles(0),
	 arraysVersion(0),
	 color(255U,255U,255U),normal(0,0,1)
	{
	}

const char* TriangleSetNode::getClassName(void) const
	{
	return className;
	}

void TriangleSetNode::update(void)
	{
	/* Bump up the arrays version number: */
	++arraysVersion;
	}

bool TriangleSetNode::canCollide(void) const
	{
	return true;
	}

int TriangleSetNode::getGeometryRequirementMask(void) const
	{
	int result=BaseAppearanceNode::HasSurfaces|BaseAppearanceNode::HasColors;
	if(twoSided.getValue())
		result|=BaseAppearanceNode::HasTwoSidedSurfaces;
	return result;
	}

Box TriangleSetNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	/* Add all vertices to the bounding box, regardless whether they are used or not: */
	for(VertexList::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		result.addPoint(vIt->position);
	
	return result;
	}

void TriangleSetNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Bail out if there are no triangles: */
	if(numTriangles==0)
		return;
	
	#if 0
	
	/* Test the sphere against the first line segment: */
	LineList::const_iterator lIt=lines.begin();
	
	/* Test the sphere against the line segment's first vertex: */
	const Point& p0=vertices[lIt->v[0]].position;
	collisionQuery.testVertexAndUpdate(p0);
	
	/* Test the sphere against the line segment's second vertex: */
	VertexIndex i1=lIt->v[1];
	const Point& p1=vertices[i1].position;
	collisionQuery.testVertexAndUpdate(p1);
	
	/* Test the sphere against the line segment: */
	collisionQuery.testEdgeAndUpdate(p0,p1);
	
	/* Test the sphere against all remaining line segments: */
	for(++lIt;lIt!=lines.end();++lIt)
		{
		/* Test the sphere against the line segment's first vertex, unless it is the same as the previous segment's second vertex: */
		VertexIndex i0=lIt->v[0];
		const Point& p0=vertices[i0].position;
		if(i0!=i1)
			collisionQuery.testVertexAndUpdate(p0);
		
		/* Test the sphere against the line segment's second vertex: */
		i1=lIt->v[1];
		const Point& p1=vertices[i1].position;
		collisionQuery.testVertexAndUpdate(p1);
		
		/* Test the sphere against the line segment: */
		collisionQuery.testEdgeAndUpdate(p0,p1);
		}
	
	#endif
	}

void TriangleSetNode::glRenderAction(int appearanceRequirementsMask,GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Bind the triangle set's vertex and index buffer objects: */
	renderState.bindVertexBuffer(dataItem->vertexBufferId);
	renderState.bindIndexBuffer(dataItem->triangleBufferId);
	
	/* Check if the buffer data are outdated: */
	if(dataItem->arraysVersion!=arraysVersion)
		{
		/* Upload the new vertex array: */
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,size_t(numVertices)*sizeof(DataItem::BufferVertex),0,GL_STATIC_DRAW_ARB);
		DataItem::BufferVertex* bvPtr=static_cast<DataItem::BufferVertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		for(VertexList::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt,++bvPtr)
			{
			bvPtr->color=DataItem::BufferVertex::Color(vIt->color);
			bvPtr->normal=DataItem::BufferVertex::Normal(vIt->normal);
			bvPtr->position=DataItem::BufferVertex::Position(vIt->position);
			}
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Upload the new triangle array: */
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(DataItem::BufferIndex),0,GL_STATIC_DRAW_ARB);
		DataItem::BufferIndex* biPtr=static_cast<DataItem::BufferIndex*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		for(TriangleList::const_iterator tIt=triangles.begin();tIt!=triangles.end();++tIt,biPtr+=3)
			for(int i=0;i<3;++i)
				biPtr[i]=DataItem::BufferIndex(tIt->v[i]);
		glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
		
		/* Mark the vertex and index buffer objects as up-to-date: */
		dataItem->arraysVersion=arraysVersion;
		}
	
	/* Set up the vertex array: */
	renderState.enableVertexArrays(DataItem::BufferVertex::getPartsMask());
	glVertexPointer(static_cast<DataItem::BufferVertex*>(0));
	
	/* Draw the triangle set: */
	glDrawElements(GL_TRIANGLES,GLsizei(numTriangles*3),GL_UNSIGNED_SHORT,static_cast<const DataItem::BufferIndex*>(0));
	}

void TriangleSetNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

TriangleSetNode::VertexIndex TriangleSetNode::addVertex(const Color& color,const Vector& normal,const Point& position)
	{
	/* Retrieve the index of the next vertex: */
	VertexIndex newVertexIndex=numVertices;
	
	/* Add another vertex to the object: */
	vertices.push_back(Vertex(VertexColor(color),normal,position));
	++numVertices;
	
	return newVertexIndex;
	}

TriangleSetNode::VertexIndex TriangleSetNode::addVertex(const TriangleSetNode::VertexColor& color,const Vector& normal,const Point& position)
	{
	/* Retrieve the index of the next vertex: */
	VertexIndex newVertexIndex=numVertices;
	
	/* Add another vertex to the object: */
	vertices.push_back(Vertex(color,normal,position));
	++numVertices;
	
	return newVertexIndex;
	}

void TriangleSetNode::setColor(const Color& newColor)
	{
	/* Set the current color: */
	color=VertexColor(newColor);
	}

void TriangleSetNode::setColor(const TriangleSetNode::VertexColor& newColor)
	{
	/* Set the current color: */
	color=newColor;
	}

void TriangleSetNode::setNormal(const Vector& newNormal)
	{
	/* Set the current normal vector: */
	normal=newNormal;
	}

TriangleSetNode::VertexIndex TriangleSetNode::addVertex(const Point& position)
	{
	/* Retrieve the index of the next vertex: */
	VertexIndex newVertexIndex=numVertices;
	
	/* Add another vertex to the object: */
	vertices.push_back(Vertex(color,normal,position));
	++numVertices;
	
	return newVertexIndex;
	}

void TriangleSetNode::addTriangle(TriangleSetNode::VertexIndex v0,TriangleSetNode::VertexIndex v1,TriangleSetNode::VertexIndex v2)
	{
	/* Add another triangle: */
	triangles.push_back(Triangle(v0,v1,v2));
	++numTriangles;
	}

void TriangleSetNode::addTriangle(const Point& p0,const Point& p1,const Point& p2)
	{
	/* Add three new vertices to the vertex list: */
	VertexIndex i0=numVertices;
	vertices.push_back(Vertex(color,normal,p0));
	vertices.push_back(Vertex(color,normal,p1));
	vertices.push_back(Vertex(color,normal,p2));
	numVertices+=3;
	
	/* Add another triangle: */
	triangles.push_back(Triangle(i0+0,i0+1,i0+2));
	++numTriangles;
	}

void TriangleSetNode::addQuad(TriangleSetNode::VertexIndex v0,TriangleSetNode::VertexIndex v1,TriangleSetNode::VertexIndex v2,TriangleSetNode::VertexIndex v3)
	{
	/* Add the quad as two triangles sharing a diagonal: */
	triangles.push_back(Triangle(v0,v1,v2));
	triangles.push_back(Triangle(v2,v1,v3));
	numTriangles+=2;
	}

void TriangleSet::addPolygon(TriangleSetNode::VertexIndex begin,TriangleSetNode::VertexIndex end)
	{
	unsigned int numVertices=end-begin;
	if(numVertices>=3)
		{
		/* Add the polygon's triangles: */
		VertexIndex up=begin+1;
		VertexIndex down=begin+numVertices-1;
		triangles.push_back(Triangle(begin,up,down));
		unsigned int trianglesToAdd=numVertices-3;
		while(trianglesToAdd>=2)
			{
			VertexIndex lastUp=up++;
			triangles.push_back(Triangle(down,lastUp,up));
			VertexIndex lastDown=down--;
			triangles.push_back(Triangle(lastDown,up,down));
			trianglesToAdd-=2;
			}
		if(trianglesToAdd!=0)
			{
			VertexIndex lastUp=up++;
			triangles.push_back(Triangle(down,lastUp,up));
			}
		numTriangles+=numVertices-2;
		}
	}

void TriangleSet::addTriangleFan(TriangleSetNode::VertexIndex begin,TriangleSetNode::VertexIndex end)
	{
	unsigned int numVertices=end-begin;
	if(numVertices>=3)
		{
		/* Add the triangle fan's triangles: */
		VertexIndex center=begin;
		for(++begin;begin+1<end;++begin)
			triangles.push_back(Triangle(center,begin,begin+1));
		numTriangles+=numVertices-2;
		}
	}

void TriangleSet::addTriangleStrip(TriangleSetNode::VertexIndex begin,TriangleSetNode::VertexIndex end)
	{
	unsigned int numVertices=end-begin;
	if(numVertices>=3)
		{
		/* Add the triangle strip's triangles: */
		while(begin+3<end)
			{
			triangles.push_back(Triangle(begin,begin+1,begin+2));
			triangles.push_back(Triangle(begin+2,begin+1,begin+3));
			begin+=2;
			}
		if(begin+2<end)
			triangles.push_back(Triangle(begin,begin+1,begin+2));
		numTriangles+=numVertices-2;
		}
	}

unsigned int TriangleSetNode::calcCircleTessellation(Scalar radius,Scalar tolerance)
	{
	/* Do all calculations in double precision: */
	double r(radius);
	double eps(tolerance);
	
	/* Calculate and return an appropriate tessellation for the given radius and tolerance: */
	return (unsigned int)(Math::ceil(Math::clamp(Math::Constants<double>::pi/Math::acos((r-eps)/(r+eps)),3.0,8192.0)));
	}

double TriangleSetNode::calcCircleAdjustedRadius(Scalar radius,unsigned int tessellation)
	{
	/* Adjust the radius for minimal deviation for the given tesselation: */
	return 2.0*double(radius)/(1.0+Math::cos(Math::Constants<double>::pi/double(tessellation)));
	}

TriangleSetNode::VertexIndex TriangleSetNode::addTessellatedCircle(const Point& center,const Rotation& frame,double adjustedRadius,unsigned int tessellation)
	{
	/* Add the circle's vertices: */
	Vector diskNormal=frame.getDirection(2);
	VertexIndex result=numVertices;
	for(unsigned int i=0;i<tessellation;++i)
		{
		double angle=(2.0*Math::Constants<double>::pi*double(i))/double(tessellation);
		vertices.push_back(Vertex(color,diskNormal,center+frame.transform(Vector(Scalar(Math::cos(angle)*adjustedRadius),Scalar(Math::sin(angle)*adjustedRadius),0))));
		}
	numVertices+=tessellation;
	
	return result;
	}

void TriangleSetNode::addTessellatedDisk(TriangleSetNode::VertexIndex circleBase,unsigned int tessellation)
	{
	/* Add the disk's triangles: */
	VertexIndex up=circleBase+1;
	VertexIndex down=circleBase+tessellation-1;
	triangles.push_back(Triangle(circleBase,up,down));
	unsigned int trianglesToAdd=tessellation-3;
	while(trianglesToAdd>=2)
		{
		VertexIndex lastUp=up++;
		triangles.push_back(Triangle(down,lastUp,up));
		VertexIndex lastDown=down--;
		triangles.push_back(Triangle(lastDown,up,down));
		trianglesToAdd-=2;
		}
	if(trianglesToAdd!=0)
		{
		VertexIndex lastUp=up++;
		triangles.push_back(Triangle(down,lastUp,up));
		}
	numTriangles+=tessellation-2;
	}

void TriangleSetNode::addTessellatedRing(TriangleSetNode::VertexIndex innerCircleBase,unsigned int innerTessellation,TriangleSetNode::VertexIndex outerCircleBase,unsigned int outerTessellation)
	{
	/* Add the ring's triangles: */
	VertexIndex innerIndex=innerCircleBase;
	unsigned int innerPriority=outerTessellation;
	VertexIndex outerIndex=outerCircleBase;
	unsigned int outerPriority=innerTessellation;
	unsigned int endPriority=innerTessellation*outerTessellation;
	while(innerPriority<endPriority||outerPriority<endPriority)
		{
		if(innerPriority<=outerPriority)
			{
			VertexIndex lastInnerIndex=innerIndex++;
			triangles.push_back(Triangle(lastInnerIndex,outerIndex,innerIndex));
			innerPriority+=outerTessellation;
			}
		else
			{
			VertexIndex lastOuterIndex=outerIndex++;
			triangles.push_back(Triangle(innerIndex,lastOuterIndex,outerIndex));
			outerPriority+=innerTessellation;
			}
		}
	triangles.push_back(Triangle(innerIndex,outerIndex,innerCircleBase));
	triangles.push_back(Triangle(innerCircleBase,outerIndex,outerCircleBase));
	numTriangles+=innerTessellation+outerTessellation;
	}

void TriangleSetNode::clear(void)
	{
	/* Clear the triangle set: */
	numVertices=0;
	vertices.clear();
	numTriangles=0;
	triangles.clear();
	}

}

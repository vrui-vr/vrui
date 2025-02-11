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

#include <SceneGraph/LineSetNode.h>

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryVertex.h>
#include <SceneGraph/BaseAppearanceNode.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*****************************************
Declaration of struct LineSetNode::Vertex:
*****************************************/

struct LineSetNode::Vertex
	{
	/* Elements: */
	public:
	VertexColor color; // Vertex's color
	Point position; // Vertex's position
	
	/* Constructors and destructors: */
	Vertex(const VertexColor& sColor,const Point& sPosition)
		:color(sColor),position(sPosition)
		{
		}
	};

/***************************************
Declaration of struct LineSetNode::Line:
***************************************/

struct LineSetNode::Line
	{
	/* Elements: */
	public:
	VertexIndex v[2]; // Array of the indices of the line segment's two vertices
	
	/* Constructors and destructors: */
	Line(VertexIndex v0,VertexIndex v1) // Creates line segment from pair of vertex indices
		{
		v[0]=v0;
		v[1]=v1;
		}
	};

/*******************************************
Declaration of struct LineSetNode::DataItem:
*******************************************/

struct LineSetNode::DataItem:public GLObject::DataItem
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,3> BufferVertex; // Type for vertices stored in buffers
	typedef GLushort BufferIndex; // Type for line vertex indices stored in buffers
	
	/* Elements: */
	GLuint vertexBufferId; // ID of vertex buffer holding the list of vertices
	GLuint lineBufferId; // ID of index buffer holding the list of line vertex indices
	unsigned int arraysVersion; // Version number of the vertex and line arrays held in the buffers
	
	/* Constructors and destructors: */
	DataItem(void);
	virtual ~DataItem(void);
	};

/**************************************
Methods of class LineSetNode::DataItem:
**************************************/

LineSetNode::DataItem::DataItem(void)
	:vertexBufferId(0),lineBufferId(0),
	 arraysVersion(0)
	{
	/* Initialize the vertex buffer object extension: */
	GLARBVertexBufferObject::initExtension();
	
	/* Create the vertex and index buffer objects: */
	glGenBuffersARB(1,&vertexBufferId);
	glGenBuffersARB(1,&lineBufferId);
	}

LineSetNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex and index buffer objects: */
	glDeleteBuffersARB(1,&vertexBufferId);
	glDeleteBuffersARB(1,&lineBufferId);
	}

/************************************
Static elements of class LineSetNode:
************************************/

const char* LineSetNode::className="LineSet";

/****************************
Methods of class LineSetNode:
****************************/

LineSetNode::LineSetNode(void)
	:lineWidth(1.0f),
	 numVertices(0),numLines(0),
	 arraysVersion(0),
	 color(255U,255U,255U)
	{
	}

const char* LineSetNode::getClassName(void) const
	{
	return className;
	}

void LineSetNode::update(void)
	{
	/* Bump up the arrays version number: */
	++arraysVersion;
	}

bool LineSetNode::canCollide(void) const
	{
	return true;
	}

int LineSetNode::getGeometryRequirementMask(void) const
	{
	return BaseAppearanceNode::HasLines|BaseAppearanceNode::HasColors;
	}

Box LineSetNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	/* Add all vertices to the bounding box, regardless whether they are used or not: */
	for(VertexList::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		result.addPoint(vIt->position);
	
	return result;
	}

void LineSetNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Bail out if there are no lines: */
	if(numLines==0)
		return;
	
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
	}

void LineSetNode::glRenderAction(int appearanceRequirementsMask,GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	glLineWidth(lineWidth.getValue());
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Bind the line set's vertex and index buffer objects: */
	renderState.bindVertexBuffer(dataItem->vertexBufferId);
	renderState.bindIndexBuffer(dataItem->lineBufferId);
	
	/* Check if the buffer data are outdated: */
	if(dataItem->arraysVersion!=arraysVersion)
		{
		/* Upload the new vertex array: */
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,size_t(numVertices)*sizeof(DataItem::BufferVertex),0,GL_STATIC_DRAW_ARB);
		DataItem::BufferVertex* bvPtr=static_cast<DataItem::BufferVertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		for(VertexList::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt,++bvPtr)
			{
			bvPtr->color=DataItem::BufferVertex::Color(vIt->color);
			bvPtr->position=DataItem::BufferVertex::Position(vIt->position);
			}
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Upload the new line array: */
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numLines*2*sizeof(DataItem::BufferIndex),0,GL_STATIC_DRAW_ARB);
		DataItem::BufferIndex* biPtr=static_cast<DataItem::BufferIndex*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
		for(LineList::const_iterator lIt=lines.begin();lIt!=lines.end();++lIt,biPtr+=2)
			for(int i=0;i<2;++i)
				biPtr[i]=DataItem::BufferIndex(lIt->v[i]);
		glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
		
		/* Mark the vertex and index buffer objects as up-to-date: */
		dataItem->arraysVersion=arraysVersion;
		}
	
	/* Set up the vertex array: */
	renderState.enableVertexArrays(DataItem::BufferVertex::getPartsMask());
	glVertexPointer(static_cast<DataItem::BufferVertex*>(0));
	
	/* Draw the line set: */
	glDrawElements(GL_LINES,GLsizei(numLines*2),GL_UNSIGNED_SHORT,static_cast<const DataItem::BufferIndex*>(0));
	}

void LineSetNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

LineSetNode::VertexIndex LineSetNode::addVertex(const Color& color,const Point& position)
	{
	/* Retrieve the index of the next vertex: */
	VertexIndex newVertexIndex=numVertices;
	
	/* Add another vertex to the object: */
	vertices.push_back(Vertex(VertexColor(color),position));
	++numVertices;
	
	return newVertexIndex;
	}

LineSetNode::VertexIndex LineSetNode::addVertex(const LineSetNode::VertexColor& color,const Point& position)
	{
	/* Retrieve the index of the next vertex: */
	VertexIndex newVertexIndex=numVertices;
	
	/* Add another vertex to the object: */
	vertices.push_back(Vertex(color,position));
	++numVertices;
	
	return newVertexIndex;
	}

void LineSetNode::setColor(const Color& newColor)
	{
	/* Set the current color: */
	color=VertexColor(newColor);
	}

void LineSetNode::setColor(const LineSetNode::VertexColor& newColor)
	{
	/* Set the current color: */
	color=newColor;
	}

LineSetNode::VertexIndex LineSetNode::addVertex(const Point& position)
	{
	/* Retrieve the index of the next vertex: */
	VertexIndex newVertexIndex=numVertices;
	
	/* Add another vertex to the object: */
	vertices.push_back(Vertex(color,position));
	++numVertices;
	
	return newVertexIndex;
	}

void LineSetNode::addLine(LineSetNode::VertexIndex v0,LineSetNode::VertexIndex v1)
	{
	/* Add another line: */
	lines.push_back(Line(v0,v1));
	++numLines;
	}

void LineSetNode::addLine(const Point& p0,const Point& p1)
	{
	/* Add two new vertices to the vertex list: */
	VertexIndex i0=numVertices;
	vertices.push_back(Vertex(color,p0));
	vertices.push_back(Vertex(color,p1));
	numVertices+=2;
	
	/* Add another line: */
	lines.push_back(Line(i0+0,i0+1));
	++numLines;
	}

void LineSetNode::addCircle(const Point& center,const Rotation& frame,Scalar radius,Scalar tolerance)
	{
	/* Do all calculations in double precision: */
	double r(radius);
	double eps(tolerance);
	
	/* Calculate an appropriate tesselation for the given tolerance: */
	int tesselation=int(Math::ceil(Math::clamp(Math::Constants<double>::pi/Math::acos((r-eps)/(r+eps)),3.0,8192.0)));
	
	/* Adjust the radius for minimal deviation for the given tesselation: */
	double rp=2.0*r/(1.0+Math::cos(Math::Constants<double>::pi/double(tesselation)));
	
	/* Add the circle's vertices and line segments: */
	VertexIndex base=numVertices;
	for(int i=0;i<tesselation;++i)
		{
		double angle=(2.0*Math::Constants<double>::pi*double(i))/double(tesselation);
		vertices.push_back(Vertex(color,center+frame.transform(Vector(Scalar(Math::cos(angle)*rp),Scalar(Math::sin(angle)*rp),0))));
		lines.push_back(Line(base+VertexIndex(i),base+VertexIndex((i+1)%tesselation)));
		}
	numVertices+=tesselation;
	numLines+=tesselation;
	}

void LineSetNode::addCircleArc(const Point& center,const Rotation& frame,Scalar radius,Scalar angle0,Scalar angle1,Scalar tolerance)
	{
	/* Do all calculations in double precision: */
	double r(radius);
	double a0(angle0);
	double a1(angle1);
	double eps(tolerance);
	
	/* Calculate an appropriate tesselation for the given tolerance: */
	double alpha=2.0*Math::acos((r-eps)/(r+eps));
	double beta=Math::acos((r-eps)/r);
	int n=int(Math::ceil(Math::clamp(((a1-a0)-2.0*beta)/alpha,0.0,8190.0)));
	
	/* Create at least one intermediate vertex if the arc spans more than a semicircle: */
	if(n==0&&a1-a0>Math::Constants<double>::pi)
		n=1;
	
	/* Add the first arc vertex exactly on the circle: */
	vertices.push_back(Vertex(color,center+frame.transform(Vector(Scalar(Math::cos(a0)*r),Scalar(Math::sin(a0)*r),0))));
	++numVertices;
	
	/* Check whether there need to be intermediate vertices: */
	if(n>0)
		{
		/* Adjust the spacing angles to properly distribute the intermediate vertices around the arc: */
		double alpha0=(a1-a0)/double(n+2);
		double alpha1=(a1-a0)/double(n);
		double alpha,beta;
		for(int i=0;i<20;++i)
			{
			/* Pick a test alpha in the middle: */
			alpha=Math::mid(alpha0,alpha1);
			
			/* Calculate the total spanned angle: */
			double ca=Math::cos(Math::div2(alpha));
			double beta=Math::acos(2.0*ca/(1.0+ca));
			if(2.0*beta+double(n)*alpha<a1-a0)
				alpha0=alpha;
			else
				alpha1=alpha;
			}
		
		/* Calculate the adjusted radius: */
		double rp=2.0*r/(1.0+Math::cos(Math::div2(alpha)));
		
		/* Generate the intermediate vertices: */
		for(int i=0;i<n;++i)
			{
			/* Add the intermediate vertex: */
			double angle=a0+beta+Math::div2(alpha)+double(i)*alpha;
			vertices.push_back(Vertex(color,center+frame.transform(Vector(Scalar(Math::cos(angle)*rp),Scalar(Math::sin(angle)*rp),0))));
			++numVertices;
			
			/* Add the intermediate line segment: */
			lines.push_back(Line(numVertices-2,numVertices-1));
			++numLines;
			}
		}
	
	/* Add the final arc vertex exactly on the circle: */
	vertices.push_back(Vertex(color,center+frame.transform(Vector(Scalar(Math::cos(a1)*r),Scalar(Math::sin(a1)*r),0))));
	++numVertices;
	
	/* Add the final line segment: */
	lines.push_back(Line(numVertices-2,numVertices-1));
	++numLines;
	}

void LineSetNode::addNumber(const Point& anchor,const Rotation& frame,Scalar size,int hAlign,int vAlign,const char* number)
	{
	/* Calculate the width of the given number string: */
	Scalar u=size/Scalar(12);
	int len=0;
	for(const char* nPtr=number;*nPtr!='\0';++nPtr)
		++len;
	Scalar width=Scalar(len)*Scalar(8)*u-Scalar(2)*u;
	Vector pen(Math::div2(width)*Scalar(-1-hAlign),Math::div2(size)*Scalar(-1-vAlign),0);
	
	/* Character shapes: */
	static const int shapeVertices[][1+6*2]=
		{
		{4,3,3,0,6,6,6,3,9}, // +
		{2,0,6,6,6}, // -
		{4,2,0,4,0,2,2,4,2}, // .
		{6,0,0,6,0,0,6,6,6,0,12,6,12}, // E
		{4,0,0,6,0,0,12,6,12}, // 0
		{2,3,0,3,12}, // 1
		{6,0,0,6,0,0,6,6,6,0,12,6,12}, // 2
		{6,0,0,6,0,0,6,6,6,0,12,6,12}, // 3
		{5,6,0,0,6,6,6,0,12,6,12}, // 4
		{6,0,0,6,0,0,6,6,6,0,12,6,12}, // 5
		{5,0,0,6,0,0,6,6,6,0,12}, // 6
		{3,6,0,0,12,6,12}, // 7
		{6,0,0,6,0,0,6,6,6,0,12,6,12}, // 8
		{5,6,0,0,6,6,6,0,12,6,12} // 9
		};
	static const int shapeLines[][1+7*2]=
		{
		{2,0,3,1,2}, // +
		{1,0,1}, // -
		{4,0,1,1,3,3,2,2,0}, // .
		{5,1,0,0,2,2,4,4,5,2,3}, // E
		{4,0,1,1,3,3,2,2,0}, // 0
		{1,0,1}, // 1
		{5,1,0,0,2,2,3,3,5,5,4}, // 2
		{5,0,1,1,3,3,5,5,4,2,3}, // 3
		{4,0,2,2,4,2,1,1,3}, // 4
		{5,0,1,1,3,3,2,2,4,4,5}, // 5
		{5,2,3,3,1,1,0,0,2,2,4}, // 6
		{2,0,2,2,1}, // 7
		{7,0,1,1,3,3,5,5,4,4,2,2,0,2,3}, // 8
		{5,0,2,2,4,4,3,3,1,1,2} // 9
		};
	
	/* Generate the characters: */
	for(const char* nPtr=number;*nPtr!='\0';++nPtr)
		{
		int shapeIndex=-1;
		switch(*nPtr)
			{
			case '+':
				shapeIndex=0;
				break;
			
			case '-':
				shapeIndex=1;
				break;
			
			case '.':
				shapeIndex=2;
				break;
			
			case 'E':
			case 'e':
				shapeIndex=3;
				break;
			
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				shapeIndex=4+(*nPtr-'0');
				break;
			
			default:
				shapeIndex=-1;
			}
		
		if(shapeIndex>=0)
			{
			/* Add the shape's vertices: */
			VertexIndex base=numVertices;
			const int* sPtr=shapeVertices[shapeIndex];
			int snv=*sPtr;
			++sPtr;
			for(int i=0;i<snv;++i,sPtr+=2)
				vertices.push_back(Vertex(color,anchor+frame.transform(pen+Vector(Scalar(sPtr[0]),Scalar(sPtr[1]),0)*u)));
			numVertices+=snv;
			
			/* Add the shape's lines: */
			const int* lPtr=shapeLines[shapeIndex];
			int snl=*lPtr;
			++lPtr;
			for(int i=0;i<snl;++i,lPtr+=2)
				lines.push_back(Line(base+VertexIndex(lPtr[0]),base+VertexIndex(lPtr[1])));
			numLines+=snl;
			}
		
		/* Advance the pen position: */
		pen[0]+=Scalar(8)*u;
		}
	}

void LineSetNode::clear(void)
	{
	/* Clear the line set: */
	numVertices=0;
	vertices.clear();
	numLines=0;
	lines.clear();
	}

}

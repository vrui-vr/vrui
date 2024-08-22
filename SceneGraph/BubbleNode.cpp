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

#include <SceneGraph/BubbleNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryVertex.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/BaseAppearanceNode.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*************************************
Methods of class BubbleNode::DataItem:
*************************************/

BubbleNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),
	 version(0),numVertices(0)
	{
	/* Initialize the vertex buffer object extension: */
	GLARBVertexBufferObject::initExtension();
	
	/* Create the vertex buffer object: */
	glGenBuffersARB(1,&vertexBufferObjectId);
	}

BubbleNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex buffer object: */
	glDeleteBuffersARB(1,&vertexBufferObjectId);
	}

/***********************************
Static elements of class BubbleNode:
***********************************/

const char* BubbleNode::className="Bubble";

/***************************
Methods of class BubbleNode:
***************************/

namespace {

/****************
Helper functions:
****************/

typedef GLGeometry::Vertex<void,0,GLubyte,4,GLfloat,GLfloat,3> Vertex;

void uploadTopVertex(Vertex* vPtr,const GLColor<GLubyte,4>& color,Scalar x,Scalar y,Scalar dx,Scalar dy,Scalar radius,Scalar z)
	{
	vPtr->color=color;
	vPtr->normal[0]=Scalar(0);
	vPtr->normal[1]=Scalar(0);
	vPtr->normal[2]=Scalar(1);
	vPtr->position[0]=x+dx*radius;
	vPtr->position[1]=y+dy*radius;
	vPtr->position[2]=z;
	}

void uploadInnerVertex(Vertex* vPtr,const GLColor<GLubyte,4>& color,Scalar x,Scalar y,Scalar dx,Scalar dy,Scalar radius,Scalar z)
	{
	vPtr->color=color;
	vPtr->normal[0]=-dx;
	vPtr->normal[1]=-dy;
	vPtr->normal[2]=Scalar(0);
	vPtr->position[0]=x+dx*radius;
	vPtr->position[1]=y+dy*radius;
	vPtr->position[2]=z;
	}

void uploadOuterVertex(Vertex* vPtr,const GLColor<GLubyte,4>& color,Scalar x,Scalar y,Scalar dx,Scalar dy,Scalar radius,Scalar z)
	{
	vPtr->color=color;
	vPtr->normal[0]=dx;
	vPtr->normal[1]=dy;
	vPtr->normal[2]=Scalar(0);
	vPtr->position[0]=x+dx*radius;
	vPtr->position[1]=y+dy*radius;
	vPtr->position[2]=z;
	}

void uploadBottomVertex(Vertex* vPtr,const GLColor<GLubyte,4>& color,Scalar x,Scalar y,Scalar dx,Scalar dy,Scalar radius,Scalar z)
	{
	vPtr->color=color;
	vPtr->normal[0]=Scalar(0);
	vPtr->normal[1]=Scalar(0);
	vPtr->normal[2]=Scalar(-1);
	vPtr->position[0]=x+dx*radius;
	vPtr->position[1]=y+dy*radius;
	vPtr->position[2]=z;
	}

}

void BubbleNode::updateVertexBuffer(DataItem* dataItem) const
	{
	/* Access bubble layout parameters: */
	Scalar mw(marginWidth.getValue());
	Scalar bd(borderDepth.getValue());
	Scalar bw(borderWidth.getValue());
	Scalar ph=pointHeight.getValue();
	GLColor<GLubyte,4> ic(interiorColor.getValue());
	GLColor<GLubyte,4> bc(borderColor.getValue());
	
	/* Precompute sine/cosine tables: */
	int ns=numSegments.getValue();
	Scalar* css=new Scalar[(ns+1)*2];
	for(int i=0;i<=ns;++i)
		{
		Scalar angle=Math::Constants<Scalar>::pi*Scalar(i)/Scalar(2*ns);
		css[i*2+0]=Math::cos(angle);
		css[i*2+1]=Math::sin(angle);
		}
	
	/* Reallocate the vertex buffer if the number of vertices changed: */
	if(dataItem->numVertices!=numVertices)
		{
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,numVertices*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
		dataItem->numVertices=numVertices;
		}
	
	/* Upload vertices into the vertex buffer: */
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	
	/* Upload the inside and margin quad strip vertices: */
	for(int i=0;i<=ns;++i)
		{
		uploadTopVertex(vPtr++,ic,x0,y1,-css[i*2+0],css[i*2+1],mw,z1);
		uploadTopVertex(vPtr++,ic,x0,y0,-css[i*2+0],-css[i*2+1],mw,z1);
		}
	for(int i=0;i<=ns;++i)
		{
		uploadTopVertex(vPtr++,ic,x1,y1,css[i*2+1],css[i*2+0],mw,z1);
		uploadTopVertex(vPtr++,ic,x1,y0,css[i*2+1],-css[i*2+0],mw,z1);
		}
	
	if(bd>Scalar(0))
		{
		/* Upload the inner border quad strip vertices: */
		for(int i=0;i<=ns;++i)
			{
			uploadInnerVertex(vPtr++,bc,x1,y0,css[i*2+1],-css[i*2+0],mw,z1);
			uploadInnerVertex(vPtr++,bc,x1,y0,css[i*2+1],-css[i*2+0],mw,z2);
			}
		for(int i=0;i<=ns;++i)
			{
			uploadInnerVertex(vPtr++,bc,x1,y1,css[i*2+0],css[i*2+1],mw,z1);
			uploadInnerVertex(vPtr++,bc,x1,y1,css[i*2+0],css[i*2+1],mw,z2);
			}
		for(int i=0;i<=ns;++i)
			{
			uploadInnerVertex(vPtr++,bc,x0,y1,-css[i*2+1],css[i*2+0],mw,z1);
			uploadInnerVertex(vPtr++,bc,x0,y1,-css[i*2+1],css[i*2+0],mw,z2);
			}
		for(int i=0;i<=ns;++i)
			{
			uploadInnerVertex(vPtr++,bc,x0,y0,-css[i*2+0],-css[i*2+1],mw,z1);
			uploadInnerVertex(vPtr++,bc,x0,y0,-css[i*2+0],-css[i*2+1],mw,z2);
			}
		
		uploadInnerVertex(vPtr++,bc,x1,y0,css[0*2+1],-css[0*2+0],mw,z1);
		uploadInnerVertex(vPtr++,bc,x1,y0,css[0*2+1],-css[0*2+0],mw,z2);
		}
	
	/* Upload the top border quad strip vertices: */
	for(int i=0;i<=ns;++i)
		{
		uploadTopVertex(vPtr++,bc,x1,y0,css[i*2+1],-css[i*2+0],mw,z2);
		uploadTopVertex(vPtr++,bc,x1,y0,css[i*2+1],-css[i*2+0],mw+bw,z2);
		}
	for(int i=0;i<=ns;++i)
		{
		uploadTopVertex(vPtr++,bc,x1,y1,css[i*2+0],css[i*2+1],mw,z2);
		uploadTopVertex(vPtr++,bc,x1,y1,css[i*2+0],css[i*2+1],mw+bw,z2);
		}
	for(int i=0;i<=ns;++i)
		{
		uploadTopVertex(vPtr++,bc,x0,y1,-css[i*2+1],css[i*2+0],mw,z2);
		uploadTopVertex(vPtr++,bc,x0,y1,-css[i*2+1],css[i*2+0],mw+bw,z2);
		}
	for(int i=0;i<=ns;++i)
		{
		uploadTopVertex(vPtr++,bc,x0,y0,-css[i*2+0],-css[i*2+1],mw,z2);
		uploadTopVertex(vPtr++,bc,x0,y0,-css[i*2+0],-css[i*2+1],mw+bw,z2);
		}
	
	uploadTopVertex(vPtr++,bc,x0,y0,-css[ns*2+0],-css[ns*2+1],mw,z2);
	uploadTopVertex(vPtr++,bc,px0,y0,0,-1,mw+bw,z2);
	
	uploadTopVertex(vPtr++,bc,x1,y0,css[0*2+1],-css[0*2+0],mw,z2);
	uploadTopVertex(vPtr++,bc,px2,y0,0,-1,mw+bw,z2);
	
	uploadTopVertex(vPtr++,bc,x1,y0,css[0*2+1],-css[0*2+0],mw,z2);
	uploadTopVertex(vPtr++,bc,x1,y0,css[0*2+1],-css[0*2+0],mw+bw,z2);
	
	/* Upload the outer border quad strip vertices: */
	uploadOuterVertex(vPtr++,bc,px2,y0,0,-1,mw+bw,z2);
	uploadOuterVertex(vPtr++,bc,px2,y0,0,-1,mw+bw,z0);
	
	for(int i=0;i<=ns;++i)
		{
		uploadOuterVertex(vPtr++,bc,x1,y0,css[i*2+1],-css[i*2+0],mw+bw,z2);
		uploadOuterVertex(vPtr++,bc,x1,y0,css[i*2+1],-css[i*2+0],mw+bw,z0);
		}
	for(int i=0;i<=ns;++i)
		{
		uploadOuterVertex(vPtr++,bc,x1,y1,css[i*2+0],css[i*2+1],mw+bw,z2);
		uploadOuterVertex(vPtr++,bc,x1,y1,css[i*2+0],css[i*2+1],mw+bw,z0);
		}
	for(int i=0;i<=ns;++i)
		{
		uploadOuterVertex(vPtr++,bc,x0,y1,-css[i*2+1],css[i*2+0],mw+bw,z2);
		uploadOuterVertex(vPtr++,bc,x0,y1,-css[i*2+1],css[i*2+0],mw+bw,z0);
		}
	for(int i=0;i<=ns;++i)
		{
		uploadOuterVertex(vPtr++,bc,x0,y0,-css[i*2+0],-css[i*2+1],mw+bw,z2);
		uploadOuterVertex(vPtr++,bc,x0,y0,-css[i*2+0],-css[i*2+1],mw+bw,z0);
		}
	
	uploadOuterVertex(vPtr++,bc,px0,y0,0,-1,mw+bw,z2);
	uploadOuterVertex(vPtr++,bc,px0,y0,0,-1,mw+bw,z0);
	
	/* Upload the backside quad strip vertices: */
	for(int i=0;i<=ns;++i)
		{
		uploadBottomVertex(vPtr++,bc,x0,y0,-css[i*2+0],-css[i*2+1],mw+bw,z0);
		uploadBottomVertex(vPtr++,bc,x0,y1,-css[i*2+0],css[i*2+1],mw+bw,z0);
		}
	
	uploadBottomVertex(vPtr++,bc,px0,y0,0,-1,mw+bw,z0);
	uploadBottomVertex(vPtr++,bc,x0,y1,-css[ns*2+0],css[ns*2+1],mw+bw,z0);
	
	uploadBottomVertex(vPtr++,bc,px2,y0,0,-1,mw+bw,z0);
	uploadBottomVertex(vPtr++,bc,x1,y1,css[0*2+1],css[0*2+0],mw+bw,z0);
	
	for(int i=0;i<=ns;++i)
		{
		uploadBottomVertex(vPtr++,bc,x1,y0,css[i*2+1],-css[i*2+0],mw+bw,z0);
		uploadBottomVertex(vPtr++,bc,x1,y1,css[i*2+1],css[i*2+0],mw+bw,z0);
		}
	
	/* Upload the bubble point triangle vertices: */
	uploadTopVertex(vPtr++,bc,px2,y0,0,-1,mw+bw,z2);
	uploadTopVertex(vPtr++,bc,px0,y0,0,-1,mw+bw,z2);
	uploadTopVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,0,0,0,z2);
	
	Scalar dx=-ph;
	Scalar dy=px0-px1;
	Scalar dl=Math::sqrt(dx*dx+dy*dy);
	dx/=dl;
	dy/=dl;
	uploadOuterVertex(vPtr++,bc,px0,y0-(mw+bw),dx,dy,0,z2);
	uploadOuterVertex(vPtr++,bc,px0,y0-(mw+bw),dx,dy,0,z0);
	uploadOuterVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,dx,dy,0,z0);
	
	uploadOuterVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,dx,dy,0,z0);
	uploadOuterVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,dx,dy,0,z2);
	uploadOuterVertex(vPtr++,bc,px0,y0-(mw+bw),dx,dy,0,z2);
	
	dx=ph;
	dy=px1-px2;
	dl=Math::sqrt(dx*dx+dy*dy);
	dx/=dl;
	dy/=dl;
	uploadOuterVertex(vPtr++,bc,px2,y0-(mw+bw),dx,dy,0,z0);
	uploadOuterVertex(vPtr++,bc,px2,y0-(mw+bw),dx,dy,0,z2);
	uploadOuterVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,dx,dy,0,z2);
	
	uploadOuterVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,dx,dy,0,z2);
	uploadOuterVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,dx,dy,0,z0);
	uploadOuterVertex(vPtr++,bc,px2,y0-(mw+bw),dx,dy,0,z0);
	
	uploadBottomVertex(vPtr++,bc,px0,y0,0,-1,mw+bw,z0);
	uploadBottomVertex(vPtr++,bc,px2,y0,0,-1,mw+bw,z0);
	uploadBottomVertex(vPtr++,bc,px1,y0-(mw+bw)-ph,0,0,0,z0);
	
	/* Finish uploading vertices into the vertex buffer: */
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Clean up: */
	delete[] css;
	}

BubbleNode::BubbleNode(void)
	:origin(Point::origin),width(1.5),height(1),
	 marginWidth(0.1),
	 interiorColor(Color(0.5f,0.5f,0.5f)),
	 borderDepth(0.05),borderWidth(0.1),backsideDepth(0.05),
	 pointHeight(0.5),pointAlignment("LEFTIN"),
	 borderColor(Color(0.0f,0.3f,0.8f)),
	 numSegments(8),
	 version(0)
	{
	/* Calculate the initial bubble layout: */
	update();
	}

const char* BubbleNode::getClassName(void) const
	{
	return className;
	}

EventOut* BubbleNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"origin")==0)
		return makeEventOut(this,origin);
	else if(strcmp(fieldName,"width")==0)
		return makeEventOut(this,width);
	else if(strcmp(fieldName,"height")==0)
		return makeEventOut(this,height);
	else if(strcmp(fieldName,"marginWidth")==0)
		return makeEventOut(this,marginWidth);
	else if(strcmp(fieldName,"interiorColor")==0)
		return makeEventOut(this,interiorColor);
	else if(strcmp(fieldName,"borderDepth")==0)
		return makeEventOut(this,borderDepth);
	else if(strcmp(fieldName,"borderWidth")==0)
		return makeEventOut(this,borderWidth);
	else if(strcmp(fieldName,"backsideDepth")==0)
		return makeEventOut(this,backsideDepth);
	else if(strcmp(fieldName,"pointHeight")==0)
		return makeEventOut(this,pointHeight);
	else if(strcmp(fieldName,"pointAlignment")==0)
		return makeEventOut(this,pointAlignment);
	else if(strcmp(fieldName,"borderColor")==0)
		return makeEventOut(this,borderColor);
	else if(strcmp(fieldName,"numSegments")==0)
		return makeEventOut(this,numSegments);
	else
		return GeometryNode::getEventOut(fieldName);
	}

EventIn* BubbleNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"origin")==0)
		return makeEventIn(this,origin);
	else if(strcmp(fieldName,"width")==0)
		return makeEventIn(this,width);
	else if(strcmp(fieldName,"height")==0)
		return makeEventIn(this,height);
	else if(strcmp(fieldName,"marginWidth")==0)
		return makeEventIn(this,marginWidth);
	else if(strcmp(fieldName,"interiorColor")==0)
		return makeEventIn(this,interiorColor);
	else if(strcmp(fieldName,"borderDepth")==0)
		return makeEventIn(this,borderDepth);
	else if(strcmp(fieldName,"borderWidth")==0)
		return makeEventIn(this,borderWidth);
	else if(strcmp(fieldName,"backsideDepth")==0)
		return makeEventIn(this,backsideDepth);
	else if(strcmp(fieldName,"pointHeight")==0)
		return makeEventIn(this,pointHeight);
	else if(strcmp(fieldName,"pointAlignment")==0)
		return makeEventIn(this,pointAlignment);
	else if(strcmp(fieldName,"borderColor")==0)
		return makeEventIn(this,borderColor);
	else if(strcmp(fieldName,"numSegments")==0)
		return makeEventIn(this,numSegments);
	else
		return GeometryNode::getEventIn(fieldName);
	}

void BubbleNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"origin")==0)
		vrmlFile.parseField(origin);
	else if(strcmp(fieldName,"width")==0)
		vrmlFile.parseField(width);
	else if(strcmp(fieldName,"height")==0)
		vrmlFile.parseField(height);
	else if(strcmp(fieldName,"marginWidth")==0)
		vrmlFile.parseField(marginWidth);
	else if(strcmp(fieldName,"interiorColor")==0)
		vrmlFile.parseField(interiorColor);
	else if(strcmp(fieldName,"borderDepth")==0)
		vrmlFile.parseField(borderDepth);
	else if(strcmp(fieldName,"borderWidth")==0)
		vrmlFile.parseField(borderWidth);
	else if(strcmp(fieldName,"backsideDepth")==0)
		vrmlFile.parseField(backsideDepth);
	else if(strcmp(fieldName,"pointHeight")==0)
		vrmlFile.parseField(pointHeight);
	else if(strcmp(fieldName,"pointAlignment")==0)
		vrmlFile.parseField(pointAlignment);
	else if(strcmp(fieldName,"borderColor")==0)
		vrmlFile.parseField(borderColor);
	else if(strcmp(fieldName,"numSegments")==0)
		vrmlFile.parseField(numSegments);
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void BubbleNode::update(void)
	{
	/* Calculate the bubble layout: */
	x0=origin.getValue()[0];
	x1=x0+width.getValue();
	y0=origin.getValue()[1];
	y1=y0+height.getValue();
	z1=origin.getValue()[2];
	z0=z1-backsideDepth.getValue();
	z2=z1+borderDepth.getValue();
	Scalar ph=pointHeight.getValue();
	
	/* Align the bubble point: */
	if(pointAlignment.getValue()=="LEFTOUT")
		{
		px1=x0-ph;
		px0=x0;
		px2=Math::min(px0+ph,x1);
		}
	else if(pointAlignment.getValue()=="LEFTIN")
		{
		px1=Math::mid(x0,x1);
		px0=Math::max(px1-ph*Scalar(1.5),x0);
		px2=Math::min(px0+ph,x1);
		}
	else if(pointAlignment.getValue()=="CENTER")
		{
		px1=Math::mid(x0,x1);
		px0=Math::max(px1-ph*Scalar(0.5),x0);
		px2=Math::min(px1+ph*Scalar(0.5),x1);
		}
	else if(pointAlignment.getValue()=="RIGHTIN")
		{
		px1=Math::mid(x0,x1);
		px2=Math::min(px1+ph*Scalar(1.5),x1);
		px0=Math::max(px2-ph,x0);
		}
	else
		{
		px1=x1+ph;
		px2=x1;
		px0=Math::max(px2-ph,x0);
		}
	
	/* Calculate the number of vertices needed to render the bubble geometry: */
	int ns=numSegments.getValue();
	numComponentVertices[0]=(ns+1)*4; // Inside and margin, rendered as quad strip
	numComponentVertices[1]=((ns+1)*4+1)*2; // Inner border, rendered as quad strip
	numComponentVertices[2]=((ns+1)*4+3)*2; // Top border, rendered as quad strip
	numComponentVertices[3]=((ns+1)*4+2)*2; // Outer border, rendered as quad strip
	numComponentVertices[4]=(ns+1)*4+4; // Backside, rendered as quad strip
	numComponentVertices[5]=6*3; // Bubble point, rendered as triangles
	numVertices=0;
	for(int i=0;i<6;++i)
		if(i!=1||borderDepth.getValue()>Scalar(0))
			numVertices+=numComponentVertices[i];
	
	/* Invalidate the vertex buffer object: */
	++version;
	}

void BubbleNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GeometryNode::read(reader);
	
	/* Read all fields: */
	reader.readField(origin);
	reader.readField(width);
	reader.readField(height);
	reader.readField(marginWidth);
	reader.readField(interiorColor);
	reader.readField(borderDepth);
	reader.readField(borderWidth);
	reader.readField(backsideDepth);
	reader.readField(borderColor);
	reader.readField(numSegments);
	}

void BubbleNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GeometryNode::write(writer);
	
	/* Write all fields: */
	writer.writeField(origin);
	writer.writeField(width);
	writer.writeField(height);
	writer.writeField(marginWidth);
	writer.writeField(interiorColor);
	writer.writeField(borderDepth);
	writer.writeField(borderWidth);
	writer.writeField(backsideDepth);
	writer.writeField(borderColor);
	writer.writeField(numSegments);
	}

bool BubbleNode::canCollide(void) const
	{
	return true;
	}

int BubbleNode::getGeometryRequirementMask(void) const
	{
	return BaseAppearanceNode::HasSurfaces|BaseAppearanceNode::HasColors;
	}

Box BubbleNode::calcBoundingBox(void) const
	{
	/* Construct the bounding box: */
	Box result;
	Scalar mbw=marginWidth.getValue()+borderWidth.getValue();
	result.min=Point(Math::min(x0-mbw,px1),y0-mbw-pointHeight.getValue(),z0);
	result.max=Point(Math::max(x1+mbw,px1),y1+mbw,z2);
	
	return result;
	}

void BubbleNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Don't do nothing: */
	}

void BubbleNode::glRenderAction(int appearanceRequirementMask,GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	renderState.setFrontFace(GL_CCW);
	renderState.enableCulling(GL_BACK);
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Bind the bubble's vertex buffer object: */
	renderState.bindVertexBuffer(dataItem->vertexBufferObjectId);
	if(dataItem->version!=version)
		{
		/* Upload the bubble's vertices into the vertex buffer: */
		updateVertexBuffer(dataItem);
		
		/* Mark the vertex buffer as up-to-date: */
		dataItem->version=version;
		}
	
	/* Enable vertex buffer rendering: */
	const Vertex* vPtr(0);
	int vertexArrayPartsMask=GLVertexArrayParts::Position;
	vertexArrayPartsMask|=GLVertexArrayParts::Color;
	glColorPointer(4,GL_UNSIGNED_BYTE,sizeof(Vertex),&vPtr->color);
	if((appearanceRequirementMask&GeometryNode::NeedsNormals)!=0x0)
		{
		vertexArrayPartsMask|=GLVertexArrayParts::Normal;
		glNormalPointer(GL_FLOAT,sizeof(Vertex),&vPtr->normal);
		}
	glVertexPointer(3,GL_FLOAT,sizeof(Vertex),&vPtr->position);
	renderState.enableVertexArrays(vertexArrayPartsMask);
	
	/* Draw the bubble's quad strips and triangles: */
	GLint firstVertex=0;
	
	/* Draw the inside and margin: */
	glDrawArrays(GL_QUAD_STRIP,firstVertex,numComponentVertices[0]);
	firstVertex+=numComponentVertices[0];
	
	if(borderDepth.getValue()>Scalar(0))
		{
		/* Draw the inside border: */
		glDrawArrays(GL_QUAD_STRIP,firstVertex,numComponentVertices[1]);
		firstVertex+=numComponentVertices[1];
		}
	
	/* Draw the top border: */
	glDrawArrays(GL_QUAD_STRIP,firstVertex,numComponentVertices[2]);
	firstVertex+=numComponentVertices[2];
	
	/* Draw the outer border: */
	glDrawArrays(GL_QUAD_STRIP,firstVertex,numComponentVertices[3]);
	firstVertex+=numComponentVertices[3];
	
	/* Draw the backside: */
	glDrawArrays(GL_QUAD_STRIP,firstVertex,numComponentVertices[4]);
	firstVertex+=numComponentVertices[4];
	
	/* Draw the bubble point: */
	glDrawArrays(GL_TRIANGLES,firstVertex,numComponentVertices[5]);
	}

void BubbleNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

Point BubbleNode::calcBubblePoint(void) const
	{
	return Point(px1,y0-(marginWidth.getValue()+borderWidth.getValue())-pointHeight.getValue(),Math::mid(z0,z2));
	}

}

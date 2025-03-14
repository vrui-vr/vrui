/***********************************************************************
PointSetNode - Class for sets of points as renderable geometry.
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

#include <SceneGraph/PointSetNode.h>

#include <string.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLSphereRenderer.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLGeometryVertex.h>
#include <SceneGraph/BaseAppearanceNode.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***************************************
Methods of class PointSetNode::DataItem:
***************************************/

PointSetNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),
	 version(0)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create the vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		}
	}

PointSetNode::DataItem::~DataItem(void)
	{
	if(vertexBufferObjectId!=0)
		{
		/* Destroy the buffer object: */
		glDeleteBuffersARB(1,&vertexBufferObjectId);
		}
	}

/*************************************
Static elements of class PointSetNode:
*************************************/

const char* PointSetNode::className="PointSet";

/*****************************
Methods of class PointSetNode:
*****************************/

PointSetNode::PointSetNode(void)
	:drawSpheres(false),pointSize(Scalar(1)),
	 sphereRenderer(0),version(0)
	{
	}

PointSetNode::~PointSetNode(void)
	{
	delete sphereRenderer;
	}

const char* PointSetNode::getClassName(void) const
	{
	return className;
	}

void PointSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"color")==0)
		{
		vrmlFile.parseSFNode(color);
		}
	else if(strcmp(fieldName,"coord")==0)
		{
		vrmlFile.parseSFNode(coord);
		}
	else if(strcmp(fieldName,"drawSpheres")==0)
		{
		vrmlFile.parseField(drawSpheres);
		}
	else if(strcmp(fieldName,"pointSize")==0)
		{
		vrmlFile.parseField(pointSize);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void PointSetNode::update(void)
	{
	/* Bump up the point set's version number: */
	++version;
	
	/* Check if sphere rendering was requested: */
	if(drawSpheres.getValue())
		{
		/* Create a sphere renderer: */
		if(sphereRenderer==0)
			sphereRenderer=new GLSphereRenderer();
		
		/* Set the sphere renderer's parameters: */
		sphereRenderer->setFixedRadius(pointSize.getValue());
		sphereRenderer->setColorMaterial(color.getValue()!=0);
		}
	else if(sphereRenderer!=0)
		{
		/* Destroy the sphere renderer: */
		delete sphereRenderer;
		sphereRenderer=0;
		}
	}

void PointSetNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GeometryNode::read(reader);
	
	/* Read all fields: */
	reader.readSFNode(color);
	reader.readSFNode(coord);
	reader.readField(drawSpheres);
	reader.readField(pointSize);
	}

void PointSetNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GeometryNode::write(writer);
	
	/* Write all fields: */
	writer.writeSFNode(color);
	writer.writeSFNode(coord);
	writer.writeField(drawSpheres);
	writer.writeField(pointSize);
	}

bool PointSetNode::canCollide(void) const
	{
	return true;
	}

int PointSetNode::getGeometryRequirementMask(void) const
	{
	int result=0x0;
	if(drawSpheres.getValue())
		result|=BaseAppearanceNode::HasSurfaces;
	else
		result|=BaseAppearanceNode::HasPoints;
	
	return result;
	}

Box PointSetNode::calcBoundingBox(void) const
	{
	Box bbox=Box::empty;
	
	if(coord.getValue()!=0)
		{
		/* Return the bounding box of the point coordinates: */
		Box box;
		if(pointTransform.getValue()!=0)
			{
			/* Return the bounding box of the transformed point coordinates: */
			bbox=pointTransform.getValue()->calcBoundingBox(coord.getValue()->point.getValues());
			}
		else
			{
			/* Return the bounding box of the untransformed point coordinates: */
			bbox=coord.getValue()->calcBoundingBox();
			}
		
		/* Expand the bounding box if sphere rendering is requested: */
		if(drawSpheres.getValue())
			bbox.extrude(pointSize.getValue());
		}
	
	return bbox;
	}

void PointSetNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	if(coord.getValue()!=0)
		{
		const std::vector<Point>& points=coord.getValue()->point.getValues();
		if(pointTransform.getValue()!=0)
			{
			/* Test the sphere against all transformed points: */
			for(std::vector<Point>::const_iterator pIt=points.begin();pIt!=points.end();++pIt)
				collisionQuery.testVertexAndUpdate(pointTransform.getValue()->transformPoint(*pIt));
			}
		else
			{
			/* Test the sphere against all points: */
			for(std::vector<Point>::const_iterator pIt=points.begin();pIt!=points.end();++pIt)
				collisionQuery.testVertexAndUpdate(*pIt);
			}
		}
	}

void PointSetNode::glRenderAction(int appearanceRequirementsMask,GLRenderState& renderState) const
	{
	if(coord.getValue()!=0&&coord.getValue()->point.getNumValues()>0)
		{
		/* Get the context data item: */
		DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
		
		/* Set up OpenGL state: */
		renderState.uploadModelview();
		if(drawSpheres.getValue())
			sphereRenderer->enable(GLfloat(renderState.getTransform().getScaling()),renderState.contextData);
		else
			glPointSize(pointSize.getValue());
		
		if(dataItem->vertexBufferObjectId!=0)
			{
			typedef GLGeometry::Vertex<void,0,GLubyte,4,void,Scalar,3> ColorVertex;
			typedef GLGeometry::Vertex<void,0,void,0,void,Scalar,3> Vertex;
			
			/* Bind the point set's vertex buffer object: */
			renderState.bindVertexBuffer(dataItem->vertexBufferObjectId);
			
			/* Check if the vertex buffer object is outdated: */
			if(dataItem->version!=version)
				{
				const std::vector<Point>& points=coord.getValue()->point.getValues();
				size_t numPoints=points.size();
				
				/* Prepare a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numPoints*(color.getValue()!=0?sizeof(ColorVertex):sizeof(Vertex)),0,GL_STATIC_DRAW_ARB);
				if(color.getValue()!=0)
					{
					const std::vector<Color>& colors=color.getValue()->color.getValues();
					ColorVertex* vPtr=static_cast<ColorVertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
					if(pointTransform.getValue()!=0)
						{
						for(size_t i=0;i<numPoints;++i,++vPtr)
							{
							vPtr->color=colors[i];
							vPtr->position=pointTransform.getValue()->transformPoint(points[i]);
							}
						}
					else
						{
						for(size_t i=0;i<numPoints;++i,++vPtr)
							{
							vPtr->color=colors[i];
							vPtr->position=points[i];
							}
						}
					}
				else
					{
					Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
					if(pointTransform.getValue()!=0)
						{
						for(size_t i=0;i<numPoints;++i,++vPtr)
							vPtr->position=pointTransform.getValue()->transformPoint(points[i]);
						}
					else
						{
						for(size_t i=0;i<numPoints;++i,++vPtr)
							vPtr->position=points[i];
						}
					}
				
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				
				/* Mark the vertex buffer object as up-to-date: */
				dataItem->version=version;
				}
			
			/* Set up the vertex arrays: */
			if(color.getValue()!=0)
				{
				renderState.enableVertexArrays(ColorVertex::getPartsMask());
				glVertexPointer(static_cast<ColorVertex*>(0));
				}
			else
				{
				renderState.enableVertexArrays(Vertex::getPartsMask());
				glVertexPointer(static_cast<Vertex*>(0));
				}
			
			/* Draw the point set: */
			glDrawArrays(GL_POINTS,0,coord.getValue()->point.getNumValues());
			}
		else
			{
			/* Render the point set in immediate mode: */
			glBegin(GL_POINTS);
			const std::vector<Point>& points=coord.getValue()->point.getValues();
			size_t numPoints=points.size();
			if(color.getValue()!=0)
				{
				/* Color each point: */
				const std::vector<Color>& colors=color.getValue()->color.getValues();
				size_t numColors=colors.size();
				for(size_t i=0;i<numPoints;++i)
					{
					if(i<numColors)
						glColor(colors[i]);
					if(pointTransform.getValue()!=0)
						glVertex(pointTransform.getValue()->transformPoint(points[i]));
					else
						glVertex(points[i]);
					}
				}
			else
				{
				for(size_t i=0;i<numPoints;++i)
					{
					if(pointTransform.getValue()!=0)
						glVertex(pointTransform.getValue()->transformPoint(points[i]));
					else
						glVertex(points[i]);
					}
				}
			glEnd();
			}
		
		/* Restore OpenGL state: */
		if(drawSpheres.getValue())
			sphereRenderer->disable(renderState.contextData);
		}
	}

void PointSetNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}

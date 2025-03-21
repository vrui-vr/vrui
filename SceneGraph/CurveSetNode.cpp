/***********************************************************************
CurveSetNode - Class for sets of curves written by curve tracing
application.
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

#include <SceneGraph/CurveSetNode.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/VarIntMarshaller.h>
#include <IO/File.h>
#include <IO/ValueSource.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
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
Methods of class CurveSetNode::DataItem:
***************************************/

CurveSetNode::DataItem::DataItem(GLContextData& contextData)
	:vertexBufferObjectId(0),indexBufferObjectId(0),
	 version(0),
	 lineLightingShader(contextData)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create the vertex and index buffer objects: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		glGenBuffersARB(1,&indexBufferObjectId);
		}
	}

CurveSetNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex buffer object: */
	if(vertexBufferObjectId!=0)
		{
		glDeleteBuffersARB(1,&vertexBufferObjectId);
		glDeleteBuffersARB(1,&indexBufferObjectId);
		}
	}

/*************************************
Static elements of class CurveSetNode:
*************************************/

const char* CurveSetNode::className="CurveSet";

/*****************************
Methods of class CurveSetNode:
*****************************/

CurveSetNode::CurveSetNode(void)
	:fromBinary(false),numLineSegments(0),
	 version(0)
	{
	}

const char* CurveSetNode::getClassName(void) const
	{
	return className;
	}

void CurveSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		/* Remember the VRML file's base directory: */
		baseDirectory=&vrmlFile.getBaseDirectory();
		}
	else if(strcmp(fieldName,"color")==0)
		vrmlFile.parseField(color);
	else if(strcmp(fieldName,"lineWidth")==0)
		vrmlFile.parseField(lineWidth);
	else if(strcmp(fieldName,"pointSize")==0)
		vrmlFile.parseField(pointSize);
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	
	fromBinary=false;
	}

void CurveSetNode::update(void)
	{
	if(fromBinary)
		{
		/* Recalculate the total number of line segments: */
		numLineSegments=0;
		for(std::vector<GLsizei>::iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
			numLineSegments+=*nvIt-1;
		}
	else
		{
		/* Re-read the curve vertex list: */
		numVertices.clear();
		numLineSegments=0;
		vertices.clear();
		for(size_t fileIndex=0;fileIndex<url.getNumValues();++fileIndex)
			{
			/* Open the curve file: */
			IO::ValueSource source(baseDirectory->openFile(url.getValue(fileIndex).c_str()));
			source.skipWs();
			
			/* Read the number of curves: */
			size_t numCurves=source.readUnsignedInteger();
			
			/* Read all curves: */
			for(size_t i=0;i<numCurves;++i)
				{
				/* Read the number of vertices in the curve: */
				GLsizei nv=GLsizei(source.readUnsignedInteger());
				numVertices.push_back(nv);
				numLineSegments+=nv-1;
				
				/* Read all vertices: */
				for(GLsizei j=0;j<nv;++j)
					{
					/* Read the next vertex: */
					Point v;
					for(int k=0;k<3;++k)
						v[k]=Scalar(source.readNumber());
					
					/* Store the vertex: */
					vertices.push_back(v);
					}
				}
			}
		}
	
	if(pointTransform.getValue()!=0)
		{
		/* Transform all curve vertices: */
		for(std::vector<Point>::iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
			*vIt=pointTransform.getValue()->transformPoint(*vIt);
		}
	
	/* Bump up the indexed line set's version number: */
	++version;
	}

void CurveSetNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GeometryNode::read(reader);
	
	/* Read all fields: */
	reader.readField(color);
	reader.readField(lineWidth);
	reader.readField(pointSize);
	
	/*********************************************************************
	Don't read the URL field, but read the curves that would have been
	parsed from it instead.
	*********************************************************************/
	
	url.clearValues();
	
	/* Read the number of vertices for each curve: */
	numVertices.clear();
	unsigned int nnvs=Misc::readVarInt32(reader.getFile());
	numVertices.reserve(nnvs);
	for(unsigned int i=0;i<nnvs;++i)
		numVertices.push_back(reader.getFile().read<Misc::UInt16>());
	
	/* Read the list of vertices for all curves: */
	vertices.clear();
	unsigned int nvs=Misc::readVarInt32(reader.getFile());
	vertices.reserve(nvs);
	for(unsigned int i=0;i<nvs;++i)
		{
		Point p;
		reader.getFile().read(p.getComponents(),3);
		vertices.push_back(p);
		}
	
	fromBinary=true;
	}

void CurveSetNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GeometryNode::write(writer);
	
	/* Write all fields: */
	writer.writeField(color);
	writer.writeField(lineWidth);
	writer.writeField(pointSize);
	
	/*********************************************************************
	Don't write the URL field, but write the curves parsed from it
	instead.
	*********************************************************************/
	
	/* Write the number of vertices for each curve: */
	Misc::writeVarInt32(numVertices.size(),writer.getFile());
	for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
		writer.getFile().write(Misc::UInt16(*nvIt));
	
	/* Write the list of vertices for all curves: */
	Misc::writeVarInt32(vertices.size(),writer.getFile());
	for(std::vector<Point>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		writer.getFile().write(vIt->getComponents(),3);
	}

bool CurveSetNode::canCollide(void) const
	{
	return true;
	}

int CurveSetNode::getGeometryRequirementMask(void) const
	{
	int result=BaseAppearanceNode::HasLines;
	if(pointSize.getValue()>Scalar(0))
		result|=BaseAppearanceNode::HasPoints;
	
	return result;
	}

Box CurveSetNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	/* Add all vertices to the box: */
	for(std::vector<Point>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		result.addPoint(*vIt);
	
	return result;
	}

void CurveSetNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Test the sphere against all curves: */
	std::vector<Point>::const_iterator vIt=vertices.begin();
	for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
		if(*nvIt>0)
			{
			/* Test the sphere against the curve's first vertex: */
			const Point* v0=&*vIt;
			collisionQuery.testVertexAndUpdate(*v0);
			++vIt;
			
			/* Test the sphere against the curve's segments: */
			for(GLsizei i=1;i<*nvIt;++i,++vIt)
				{
				/* Test the sphere against the line segment: */
				const Point* v1=&*vIt;
				collisionQuery.testEdgeAndUpdate(*v0,*v1);
				
				/* Test the sphere against the line segment's end vertex: */
				collisionQuery.testVertexAndUpdate(*v1);
				
				/* Go to the next line segment: */
				v0=v1;
				++vIt;
				}
			}
	}

void CurveSetNode::glRenderAction(int appearanceRequirementsMask,GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	glLineWidth(lineWidth.getValue());
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	if(dataItem->vertexBufferObjectId!=0&&dataItem->indexBufferObjectId!=0)
		{
		/*******************************************************************
		Render the curve set from the vertex buffer:
		*******************************************************************/
		
		typedef GLGeometry::Vertex<void,0,void,0,Scalar,Scalar,3> Vertex;
		
		/* Bind the curve set's vertex and index buffer objects: */
		renderState.bindVertexBuffer(dataItem->vertexBufferObjectId);
		renderState.bindIndexBuffer(dataItem->indexBufferObjectId);
		
		if(dataItem->version!=version)
			{
			/* Allocate the vertex buffer: */
			glBufferDataARB(GL_ARRAY_BUFFER_ARB,(vertices.size())*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
			Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
			
			/* Copy curve vertices into the vertex buffer by curve: */
			std::vector<Point>::const_iterator vIt=vertices.begin();
			for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
				{
				/* Copy vertices for this curve: */
				for(GLsizei i=0;i<*nvIt;++i,++vIt,++vPtr)
					{
					if(i==0)
						vPtr->normal=Geometry::normalize(Vertex::Normal(vIt[1]-vIt[0]));
					else if(i==*nvIt-1)
						vPtr->normal=Geometry::normalize(Vertex::Normal(vIt[0]-vIt[-1]));
					else
						vPtr->normal=Geometry::normalize(Vertex::Normal(vIt[1]-vIt[-1]));
					vPtr->position=Vertex::Position(*vIt);
					}
				}
			
			/* Finish uploading into the vertex buffer: */
			glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			
			/* Allocate the index buffer: */
			glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,(numLineSegments*2+numVertices.size()*2)*sizeof(GLuint),0,GL_STATIC_DRAW_ARB);
			GLuint* iPtr=static_cast<GLuint*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
			
			/* Create line segment indices for all curves: */
			GLuint baseIndex=0;
			for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
				{
				/* Create line segment indices for this curve: */
				for(GLsizei i=1;i<*nvIt;++i,iPtr+=2)
					{
					iPtr[0]=baseIndex+i-1;
					iPtr[1]=baseIndex+i;
					}
				
				/* Go to the next curve: */
				baseIndex+=*nvIt;
				}
			
			/* Create point indices for all curve endpoints: */
			baseIndex=0;
			for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt,iPtr+=2)
				{
				/* Create line segment indices for this curve: */
				iPtr[0]=baseIndex;
				iPtr[1]=baseIndex+*nvIt-1;
				
				/* Go to the next curve: */
				baseIndex+=*nvIt;
				}
			
			/* Finish uploading into the index buffer: */
			glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
			
			/* Mark the vertex and index buffer objects as up-to-date: */
			dataItem->version=version;
			}
		
		/* Set up the vertex array: */
		renderState.enableVertexArrays(Vertex::getPartsMask());
		glVertexPointer(static_cast<Vertex*>(0));
		
		/* Draw all curves: */
		if(renderState.currentState.lightingEnabled)
			dataItem->lineLightingShader.activate();
		else
			glColor(color.getValue());
		const GLuint* indexBase=0;
		glDrawElements(GL_LINES,numLineSegments*2,GL_UNSIGNED_INT,indexBase);
		if(renderState.currentState.lightingEnabled)
			dataItem->lineLightingShader.deactivate();
		
		if(pointSize.getValue()>Scalar(0))
			{
			/* Set up OpenGL state: */
			renderState.disableMaterials();
			renderState.disableTextures();
			glPointSize(pointSize.getValue());
			glColor(color.getValue());
			
			/* Draw the endpoints of all curves: */
			glDrawElements(GL_POINTS,numVertices.size()*2,GL_UNSIGNED_INT,indexBase+numLineSegments*2);
			}
		}
	else
		{
		/*******************************************************************
		Render the curve set directly:
		*******************************************************************/
		
		/* Draw all curves: */
		std::vector<Point>::const_iterator vIt=vertices.begin();
		for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
			{
			/* Render the curve: */
			glBegin(GL_LINE_STRIP);
			for(GLsizei i=0;i<*nvIt;++i,++vIt)
				glVertex(*vIt);
			glEnd();
			}
		
		/* Draw the endpoints of all curves: */
		glBegin(GL_POINTS);
		GLint baseVertexIndex=0;
		for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
			{
			glVertex(vertices[baseVertexIndex]);
			glVertex(vertices[baseVertexIndex+*nvIt-1]);
			
			/* Go to the next curve: */
			baseVertexIndex+=*nvIt;
			}
		glEnd();
		}
	}

void CurveSetNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem(contextData);
	contextData.addDataItem(this,dataItem);
	}

}

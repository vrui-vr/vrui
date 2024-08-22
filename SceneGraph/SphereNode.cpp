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

#include <SceneGraph/SphereNode.h>

#include <string.h>
#include <Misc/StandardMarshallers.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/BaseAppearanceNode.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*************************************
Methods of class SphereNode::DataItem:
*************************************/

SphereNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),indexBufferObjectId(0),
	 numVertices(0),
	 version(0)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create the vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		
		/* Create the index buffer object: */
		glGenBuffersARB(1,&indexBufferObjectId);
		}
	}

SphereNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex and index buffer objects: */
	if(vertexBufferObjectId!=0)
		glDeleteBuffersARB(1,&vertexBufferObjectId);
	if(indexBufferObjectId!=0)
		glDeleteBuffersARB(1,&indexBufferObjectId);
	}

/***********************************
Static elements of class SphereNode:
***********************************/

const char* SphereNode::className="Sphere";

/***************************
Methods of class SphereNode:
***************************/

namespace {

template <class IndexParam>
inline
void
uploadLatLongIndices(
	unsigned int numSegments)
	{
	/* Calculate the number of indices to be uploaded: */
	unsigned int numQuads=numSegments*2;
	size_t numIndices=size_t(numSegments)*size_t(numQuads+1)*2;
	
	/* Prepare the index buffer: */
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numIndices*sizeof(IndexParam),0,GL_STATIC_DRAW_ARB);
	IndexParam* indexPtr=static_cast<IndexParam*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	
	for(unsigned int parallel=0;parallel<numSegments;++parallel)
		{
		unsigned int baseIndex=parallel*(numQuads+1);
		for(unsigned int meridian=0;meridian<=numQuads;++meridian,++baseIndex,indexPtr+=2)
			{
			indexPtr[0]=baseIndex+(numQuads+1);
			indexPtr[1]=baseIndex;
			}
		}
	
	/* Finalize the index buffer: */
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	}

void uploadIcoQuadRow(const Point& center,Scalar radius,unsigned int numSegments,bool closed,Scalar dy,const Vector& c00,const Vector& c10,const Vector& c01,const Vector& c11,bool uploadNormals,GLubyte* normalPtr,GLubyte* positionPtr,size_t vertexStride)
	{
	/* Upload vertices for the first half of the double-quad: */
	for(unsigned int x=closed?0:1;x<=numSegments;++x,normalPtr+=vertexStride,positionPtr+=vertexStride)
		{
		Scalar dx=Scalar(x)/Scalar(numSegments);
		Vector v;
		if(dy>dx)
			{
			/* Calculate barycentric interpolation weights for the top-left triangle: */
			Scalar w00=Scalar(1)-dy;
			Scalar w01=dy-dx;
			Scalar w11=dx;
			
			/* Interpolate the top-left triangle: */
			for(int i=0;i<3;++i)
				v[i]=c00[i]*w00+c01[i]*w01+c11[i]*w11;
			}
		else
			{
			/* Calculate barycentric interpolation weights for the bottom-right triangle: */
			Scalar w00=Scalar(1)-dx;
			Scalar w10=dx-dy;
			Scalar w11=dy;
			
			/* Interpolate the bottom-right triangle: */
			for(int i=0;i<3;++i)
				v[i]=c00[i]*w00+c10[i]*w10+c11[i]*w11;
			}
	
		/* Project the interpolated point out to the sphere: */
		v.normalize();
		
		/* Upload the vertex normal if needed: */
		if(uploadNormals!=0)
			*reinterpret_cast<Vector*>(normalPtr)=v;
		
		/* Upload the vertex position: */
		*reinterpret_cast<Point*>(positionPtr)=Geometry::addScaled(center,v,radius);
		}
	}

template <class IndexParam>
inline
void
uploadIcoIndices(
	unsigned int numSegments)
	{
	/* Calculate the number of indices to be uploaded: */
	unsigned int numQuads=numSegments*2;
	size_t numIndices=5*size_t(numSegments)*size_t(numQuads+1)*2;
	
	/* Prepare the index buffer: */
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numIndices*sizeof(IndexParam),0,GL_STATIC_DRAW_ARB);
	IndexParam* indexPtr=static_cast<IndexParam*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	
	for(unsigned int dquad=0;dquad<5;++dquad)
		{
		/* Calculate the base index of vertices for this double-quad: */
		IndexParam dquadBaseIndex=dquad*(numSegments+1)*(numQuads+1);
		
		/* Upload indices for all triangle strips: */
		for(unsigned int y=0;y<numSegments;++y)
			{
			unsigned int baseIndex=dquadBaseIndex+y*(numQuads+1);
			for(unsigned int x=0;x<=numQuads;++x,++baseIndex,indexPtr+=2)
				{
				indexPtr[0]=baseIndex+numQuads+1;
				indexPtr[1]=baseIndex;
				}
			}
		}
	
	/* Finalize the index buffer: */
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	}

}

void SphereNode::updateArrays(SphereNode::DataItem* dataItem) const
	{
	/* Calculate the required layout of vertices in the interleaved vertex buffer: */
	dataItem->vertexArrayPartsMask=0x0;
	dataItem->vertexSize=0;
	dataItem->texCoordOffset=dataItem->vertexSize;
	if(numNeedsTexCoords!=0)
		{
		dataItem->vertexSize+=sizeof(TexCoord);
		dataItem->vertexArrayPartsMask|=GLVertexArrayParts::TexCoord;
		}
	dataItem->normalOffset=dataItem->vertexSize;
	if(numNeedsNormals!=0)
		{
		dataItem->vertexSize+=sizeof(Vector);
		dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Normal;
		}
	dataItem->positionOffset=dataItem->vertexSize;
	dataItem->vertexSize+=sizeof(Point);
	dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Position;
	
	int numSegs=numSegments.getValue();
	const Point& c=center.getValue();
	Scalar r=radius.getValue();
	
	/* Determine whether to create a lat/long sphere for equirectangular texture mapping or a subdivided icosahedron: */
	if(numNeedsTexCoords!=0||latLong.getValue())
		{
		/* Calculate the number of quadrilaterals per segment: */
		int numQuads=numSegs*2;
		
		/* Calculate the number of vertices: */
		dataItem->numVertices=size_t(numSegs+1)*size_t(numQuads+1);
		
		/* Create the vertex buffer and access it for vertex data upload: */
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,dataItem->numVertices*dataItem->vertexSize,0,GL_STATIC_DRAW_ARB);
		GLubyte* bufferPtr=static_cast<GLubyte*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		
		/* Upload the sphere vertices: */
		GLubyte* texCoordPtr=bufferPtr+dataItem->texCoordOffset;
		GLubyte* normalPtr=bufferPtr+dataItem->normalOffset;
		GLubyte* positionPtr=bufferPtr+dataItem->positionOffset;
		const Scalar pi=Math::Constants<Scalar>::pi;
		for(int parallel=0;parallel<=numSegs;++parallel)
			{
			Scalar texY=Scalar(parallel)/Scalar(numSegs);
			
			/* Calculate the latitude of this ring of vertices: */
			Scalar lat=(Scalar(parallel)/Scalar(numSegs)-Scalar(0.5))*pi;
			Scalar cLat=Math::cos(lat);
			Scalar sLat=Math::sin(lat);
			for(int meridian=0;meridian<=numQuads;++meridian)
				{
				/* Calculate the longitude of this vertex: */
				Scalar lng=meridian<numQuads?Scalar(meridian)/Scalar(numSegs)*pi:Scalar(0);
				Scalar cLng=Math::cos(lng);
				Scalar sLng=Math::sin(lng);
				
				/* Unit vector from center of sphere towards this vertex: */
				Vector dir(-sLng*cLat,sLat,-cLng*cLat);
				
				/* Upload required vertex components: */
				if(numNeedsTexCoords!=0)
					{
					TexCoord& tc=*reinterpret_cast<TexCoord*>(texCoordPtr);
					tc[0]=Scalar(meridian)/Scalar(numQuads);
					tc[1]=texY;
					texCoordPtr+=dataItem->vertexSize;
					}
				if(numNeedsNormals!=0)
					{
					Vector& n=*reinterpret_cast<Vector*>(normalPtr);
					for(int i=0;i<3;++i)
						n[i]=dir[i];
					normalPtr+=dataItem->vertexSize;
					}
				Point& p=*reinterpret_cast<Point*>(positionPtr);
				for(int i=0;i<3;++i)
					p[i]=c[i]+dir[i]*r;
				positionPtr+=dataItem->vertexSize;
				}
			}
		
		/* Finalize the vertex buffer: */
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Upload vertex indices into the index buffer depending on the required index type: */
		if(dataItem->numVertices<=size_t(65536))
			uploadLatLongIndices<GLushort>(numSegs);
		else
			uploadLatLongIndices<GLuint>(numSegs);
		}
	else
		{
		/* Construct the static icosahedron model: */
		const Scalar b0=0.525731112119133606f; // b0=sqrt((5.0-sqrt(5.0))/10);
		const Scalar b1=0.850650808352039932f; // b1=sqrt((5.0+sqrt(5.0))/10);
		static const Vector vertices[12]=
			{
			Vector(-b0,  0, b1),Vector( b0,  0, b1),Vector(-b0,  0,-b1),Vector( b0,  0,-b1),
			Vector(  0, b1, b0),Vector(  0, b1,-b0),Vector(  0,-b1, b0),Vector(  0,-b1,-b0),
			Vector( b1, b0,  0),Vector(-b1, b0,  0),Vector( b1,-b0,  0),Vector(-b1,-b0,  0)
			};
		static const int dquadIndices[5][6]=
			{
			{1,8,3,0,4,5},
			{4,5,3,0,9,2},
			{9,2,3,0,11,7},
			{11,7,3,0,6,10},
			{6,10,3,0,1,8}
			};
		
		/* Calculate the number of vertices: */
		dataItem->numVertices=5*size_t(numSegs+1)*size_t(numSegs*2+1);
		
		/* Create the vertex buffer and access it for vertex data upload: */
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,dataItem->numVertices*dataItem->vertexSize,0,GL_STATIC_DRAW_ARB);
		GLubyte* bufferPtr=static_cast<GLubyte*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		
		/* Upload the sphere vertices: */
		GLubyte* normalPtr=bufferPtr+dataItem->normalOffset;
		GLubyte* positionPtr=bufferPtr+dataItem->positionOffset;
		
		for(int dquad=0;dquad<5;++dquad)
			{
			/* Access the double-quad's six corner vertices: */
			const Vector& c00(vertices[dquadIndices[dquad][0]]);
			const Vector& c10(vertices[dquadIndices[dquad][1]]);
			const Vector& c20(vertices[dquadIndices[dquad][2]]);
			const Vector& c01(vertices[dquadIndices[dquad][3]]);
			const Vector& c11(vertices[dquadIndices[dquad][4]]);
			const Vector& c21(vertices[dquadIndices[dquad][5]]);
			
			/* Create rows of vertices for the double-quad: */
			for(int y=0;y<=numSegs;++y)
				{
				Scalar dy=Scalar(y)/Scalar(numSegs);
				
				/* Upload vertices for the first half of the double-quad: */
				uploadIcoQuadRow(c,r,numSegs,true,dy,c00,c10,c01,c11,numNeedsNormals!=0,normalPtr,positionPtr,dataItem->vertexSize);
				normalPtr+=size_t(numSegs+1)*dataItem->vertexSize;
				positionPtr+=size_t(numSegs+1)*dataItem->vertexSize;
				
				/* Upload vertices for the second half of the double-quad: */
				uploadIcoQuadRow(c,r,numSegs,false,dy,c10,c20,c11,c21,numNeedsNormals!=0,normalPtr,positionPtr,dataItem->vertexSize);
				normalPtr+=size_t(numSegs)*dataItem->vertexSize;
				positionPtr+=size_t(numSegs)*dataItem->vertexSize;
				}
			}
		
		/* Finish the vertex buffer: */
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Upload vertex indices into the index buffer depending on the required index type: */
		if(dataItem->numVertices<=size_t(65536))
			uploadIcoIndices<GLushort>(numSegs);
		else
			uploadIcoIndices<GLuint>(numSegs);
		}
	}

SphereNode::SphereNode(void)
	:center(Point::origin),
	 radius(1.0f),
	 numSegments(12),
	 latLong(true),
	 ccw(true),
	 version(1)
	{
	}

const char* SphereNode::getClassName(void) const
	{
	return className;
	}

EventOut* SphereNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventOut(this,center);
	else if(strcmp(fieldName,"radius")==0)
		return makeEventOut(this,radius);
	else if(strcmp(fieldName,"numSegments")==0)
		return makeEventOut(this,numSegments);
	else if(strcmp(fieldName,"latLong")==0)
		return makeEventOut(this,latLong);
	else if(strcmp(fieldName,"ccw")==0)
		return makeEventOut(this,ccw);
	else
		return GeometryNode::getEventOut(fieldName);
	}

EventIn* SphereNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventIn(this,center);
	else if(strcmp(fieldName,"radius")==0)
		return makeEventIn(this,radius);
	else if(strcmp(fieldName,"numSegments")==0)
		return makeEventIn(this,numSegments);
	else if(strcmp(fieldName,"latLong")==0)
		return makeEventIn(this,latLong);
	else if(strcmp(fieldName,"ccw")==0)
		return makeEventIn(this,ccw);
	else
		return GeometryNode::getEventIn(fieldName);
	}

void SphereNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"center")==0)
		vrmlFile.parseField(center);
	else if(strcmp(fieldName,"radius")==0)
		vrmlFile.parseField(radius);
	else if(strcmp(fieldName,"numSegments")==0)
		vrmlFile.parseField(numSegments);
	else if(strcmp(fieldName,"latLong")==0)
		vrmlFile.parseField(latLong);
	else if(strcmp(fieldName,"ccw")==0)
		vrmlFile.parseField(ccw);
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void SphereNode::update(void)
	{
	/* Invalidate the sphere arrays: */
	++version;
	}

void SphereNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GeometryNode::read(reader);
	
	/* Read all fields: */
	reader.readField(center);
	reader.readField(radius);
	reader.readField(numSegments);
	reader.readField(latLong);
	if(reader.getMinorVersion()<1U)
		{
		/* Ignore the texCoords field: */
		Misc::Marshaller<bool>::read(reader.getFile());
		}
	reader.readField(ccw);
	}

void SphereNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GeometryNode::write(writer);
	
	/* Write all fields: */
	writer.writeField(center);
	writer.writeField(radius);
	writer.writeField(numSegments);
	writer.writeField(latLong);
	writer.writeField(ccw);
	}

bool SphereNode::canCollide(void) const
	{
	return true;
	}

int SphereNode::getGeometryRequirementMask(void) const
	{
	return BaseAppearanceNode::HasSurfaces;
	}

Box SphereNode::calcBoundingBox(void) const
	{
	/* Return a box around the sphere: */
	Point pmin=center.getValue();
	Point pmax=center.getValue();
	for(int i=0;i<3;++i)
		{
		pmin[i]-=radius.getValue();
		pmax[i]+=radius.getValue();
		}
	return Box(pmin,pmax);
	}

void SphereNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Check whether the sphere collides outside-in or inside-out: */
	if(ccw.getValue())
		{
		Vector sc0=collisionQuery.getC0()-center.getValue(); // Vector from this sphere's center to sliding sphere's starting point
		
		Scalar a=collisionQuery.getC0c1Sqr(); // Quadratic coefficient from quadratic formula
		Scalar bh=sc0*collisionQuery.getC0c1(); // Halved linear coefficient from quadratic formula
		Scalar c=sc0.sqr()-Math::sqr(radius.getValue()+collisionQuery.getRadius()); // Constant coefficient from quadratic formula
		
		Scalar discq=bh*bh-a*c; // Quarter discriminant from quadratic formula
		if(discq>=Scalar(0))
			{
			/* Find the quadratic equation's smaller root: */
			// Scalar lambda=bh>=Scalar(0)?(-bh-Math::sqrt(discq))/a:c/(-bh+Math::sqrt(discq)); // Slightly more stable formula
			Scalar lambda=c/(-bh+Math::sqrt(discq)); // Stable formulation for negative bh, which is the only bh that counts
			
			/* Check whether this sphere will affect the collision: */
			if(lambda>=Scalar(0))
				{
				if(lambda<collisionQuery.getHitLambda())
					{
					/* Update the hit result: */
					collisionQuery.update(lambda,Geometry::addScaled(sc0,collisionQuery.getC0c1(),lambda));
					}
				}
			else if(c<Scalar(0)&&bh<Scalar(0)&&collisionQuery.getHitLambda()>Scalar(0))
				{
				/* Sphere already penetrates the sphere; prevent it from getting worse: */
				collisionQuery.update(Scalar(0),sc0);
				}
			}
		}
	else
		{
		Vector c0s=center.getValue()-collisionQuery.getC0(); // Vector from sliding sphere's starting point to this sphere's center
		
		Scalar a=collisionQuery.getC0c1Sqr(); // Quadratic coefficient from quadratic formula
		Scalar bh=c0s*collisionQuery.getC0c1(); // Halved linear coefficient from quadratic formula
		Scalar c=c0s.sqr()-Math::sqr(radius.getValue()-collisionQuery.getRadius()); // Constant coefficient from quadratic formula
		
		Scalar discq=bh*bh-a*c; // Quarter discriminant from quadratic formula
		if(discq>=Scalar(0))
			{
			/* Find the quadratic equation's larger root: */
			Scalar lambda=bh>=Scalar(0)?(bh+Math::sqrt(discq))/a:c/(bh-Math::sqrt(discq)); // Slightly more stable formula
			
			/* Check whether this sphere will affect the collision: */
			if(lambda>=Scalar(0))
				{
				if(lambda<collisionQuery.getHitLambda())
					{
					/* Update the hit result: */
					collisionQuery.update(lambda,Geometry::addScaled(c0s,collisionQuery.getC0c1(),-lambda));
					}
				}
			else if(collisionQuery.getHitLambda()>Scalar(0))
				{
				/* Sphere already outside the sphere; prevent it from getting worse: */
				collisionQuery.update(Scalar(0),c0s);
				}
			}
		else if(collisionQuery.getHitLambda()>Scalar(0))
			{
			/* Sphere already outside the sphere; prevent it from getting worse: */
			collisionQuery.update(Scalar(0),c0s);
			}
		}
	}

void SphereNode::glRenderAction(int appearanceRequirementMask,GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	renderState.setFrontFace(ccw.getValue()?GL_CCW:GL_CW);
	renderState.enableCulling(GL_BACK);
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	if(dataItem->vertexBufferObjectId!=0&&dataItem->indexBufferObjectId!=0)
		{
		/*******************************************************************
		Render the sphere from the vertex and index buffers:
		*******************************************************************/
		
		/* Bind the sphere's vertex and index buffer objects: */
		renderState.bindVertexBuffer(dataItem->vertexBufferObjectId);
		renderState.bindIndexBuffer(dataItem->indexBufferObjectId);
		
		/* Check if the buffer objects are up-to-date: */
		if(dataItem->version!=version)
			{
			/* Update the buffers and mark them as up-to-date: */
			updateArrays(dataItem);
			dataItem->version=version;
			}
		
		/* Enable vertex buffer rendering: */
		int vertexArrayPartsMask=GLVertexArrayParts::Position;
		if(appearanceRequirementMask&NeedsTexCoords)
			{
			vertexArrayPartsMask|=GLVertexArrayParts::TexCoord;
			glTexCoordPointer(2,GL_FLOAT,dataItem->vertexSize,static_cast<const GLubyte*>(0)+dataItem->texCoordOffset);
			}
		if(appearanceRequirementMask&NeedsNormals)
			{
			vertexArrayPartsMask|=GLVertexArrayParts::Normal;
			glNormalPointer(GL_FLOAT,dataItem->vertexSize,static_cast<const GLubyte*>(0)+dataItem->normalOffset);
			}
		glVertexPointer(3,GL_FLOAT,dataItem->vertexSize,static_cast<const GLubyte*>(0)+dataItem->positionOffset);
		renderState.enableVertexArrays(vertexArrayPartsMask);
		
		/* Draw the vertex array: */
		bool drawLatLon=numNeedsTexCoords!=0||latLong.getValue();
		unsigned int numSegs=numSegments.getValue();
		unsigned int numStrips=drawLatLon?numSegs:5*numSegs;
		unsigned int stripLength=(numSegs*2+1)*2;
		GLenum stripType=drawLatLon?GL_QUAD_STRIP:GL_TRIANGLE_STRIP;
		
		/* Draw the sphere as numSegs quad strips: */
		if(dataItem->numVertices<=size_t(65536U))
			{
			GLushort* indexPtr(0);
			for(unsigned int strip=0;strip<numStrips;++strip,indexPtr+=stripLength)
				glDrawElements(stripType,stripLength,GL_UNSIGNED_SHORT,indexPtr);
			}
		else
			{
			GLuint* indexPtr(0);
			for(unsigned int strip=0;strip<numStrips;++strip,indexPtr+=stripLength)
				glDrawElements(stripType,stripLength,GL_UNSIGNED_INT,indexPtr);
			}
		}
	}

void SphereNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}

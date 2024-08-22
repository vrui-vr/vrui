/***********************************************************************
ElevationGridNode - Class for quad-based height fields as renderable
geometry.
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

#define GLGEOMETRY_NONSTANDARD_TEMPLATES

#include <SceneGraph/ElevationGridNode.h>

#include <string.h>
#include <Misc/SelfDestructArray.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLNVPrimitiveRestart.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLGeometryVertex.h>
#include <SceneGraph/BaseAppearanceNode.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

#include <SceneGraph/Internal/LoadElevationGrid.h>

#define BENCHMARKING 0

#if BENCHMARKING
#include <iostream>
#include <Realtime/Time.h>
#endif

namespace SceneGraph {

/********************************************
Methods of class ElevationGridNode::DataItem:
********************************************/

ElevationGridNode::DataItem::DataItem(void)
	:havePrimitiveRestart(GLNVPrimitiveRestart::isSupported()),
	 vertexBufferObjectId(0),
	 vertexSize(0),texCoordOffset(0),colorOffset(0),normalOffset(0),positionOffset(0),vertexArrayPartsMask(0x0),
	 indexBufferObjectId(0),
	 numQuads(0),numTriangles(0),
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
	
	if(havePrimitiveRestart)
		GLNVPrimitiveRestart::initExtension();
	}

ElevationGridNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex buffer object: */
	if(vertexBufferObjectId!=0)
		glDeleteBuffersARB(1,&vertexBufferObjectId);
	
	/* Destroy the index buffer object: */
	if(indexBufferObjectId!=0)
		glDeleteBuffersARB(1,&indexBufferObjectId);
	}

/******************************************
Static elements of class ElevationGridNode:
******************************************/

const char* ElevationGridNode::className="ElevationGrid";

/**********************************
Methods of class ElevationGridNode:
**********************************/

Point* ElevationGridNode::calcVertices(void) const
	{
	/* Allocate the result array: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Point* vertices=new Point[zDim*xDim];
	
	/* Calculate all vertex positions: */
	Scalar xSp=xSpacing.getValue();
	Scalar zSp=zSpacing.getValue();
	Point* vPtr=vertices;
	const Scalar* hPtr=&height.getValue(0);
	if(heightIsY.getValue())
		{
		Point p;
		p[2]=origin.getValue()[2];
		for(int z=0;z<zDim;++z,p[2]+=zSp)
			{
			p[0]=origin.getValue()[0];
			for(int x=0;x<xDim;++x,++vPtr,++hPtr,p[0]+=xSp)
				{
				p[1]=origin.getValue()[1]+*hPtr*heightScale.getValue();
				*vPtr=p;
				}
			}
		}
	else
		{
		Point p;
		p[1]=origin.getValue()[1];
		for(int z=0;z<zDim;++z,p[1]+=zSp)
			{
			p[0]=origin.getValue()[0];
			for(int x=0;x<xDim;++x,++vPtr,++hPtr,p[0]+=xSp)
				{
				p[2]=origin.getValue()[2]+*hPtr*heightScale.getValue();
				*vPtr=p;
				}
			}
		}
	
	return vertices;
	}

Vector* ElevationGridNode::calcQuadNormals(void) const
	{
	/* Allocate the result array: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Vector* normals=new Vector[(zDim-1)*(xDim-1)];
	
	/* Calculate the normal scaling factors: */
	Scalar nx=zSpacing.getValue()*heightScale.getValue();
	Scalar ny=xSpacing.getValue()*zSpacing.getValue();
	Scalar nz=xSpacing.getValue()*heightScale.getValue();
	if(!ccw.getValue())
		{
		/* Flip normal vectors if quads are oriented clockwise: */
		nx=-nx;
		ny=-ny;
		nz=-nz;
		}
	
	if(heightIsY.getValue())
		{
		/* Calculate all quad normal vectors: */
		Vector* nPtr=normals;
		for(int z=0;z<zDim-1;++z)
			for(int x=0;x<xDim-1;++x,++nPtr)
				{
				/* Calculate the quad normal as the average of the normals of the quad's two triangles: */
				const Scalar* h=&(height.getValue(z*xDim+x));
				(*nPtr)[0]=(h[0]-h[1]+h[xDim]-h[xDim+1])*nx;
				(*nPtr)[1]=ny*Scalar(2); // To average over sum of two triangle normals
				(*nPtr)[2]=(h[0]+h[1]-h[xDim]-h[xDim+1])*nz;
				}
		}
	else
		{
		/* Flip normal vectors to account for y,z-swap: */
		nx=-nx;
		ny=-ny;
		nz=-nz;
		
		/* Calculate all quad normal vectors: */
		Vector* nPtr=normals;
		for(int z=0;z<zDim-1;++z)
			for(int x=0;x<xDim-1;++x,++nPtr)
				{
				/* Calculate the quad normal as the average of the normals of the quad's two triangles: */
				const Scalar* h=&(height.getValue(z*xDim+x));
				(*nPtr)[0]=(h[0]-h[1]+h[xDim]-h[xDim+1])*nx;
				(*nPtr)[1]=(h[0]+h[1]-h[xDim]-h[xDim+1])*nz;
				(*nPtr)[2]=ny*Scalar(2); // To average over sum of two triangle normals
				}
		}
	
	return normals;
	}

int* ElevationGridNode::calcHoleyQuadCases(GLuint& numQuads,GLuint& numTriangles) const
	{
	/* Allocate the result array: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	int* quadCases=new int[(zDim-1)*(xDim-1)];
	
	/* Calculate the triangulation cases for all grid cells: */
	numQuads=0;
	numTriangles=0;
	int* qcPtr=quadCases;
	for(int z=0;z<zDim-1;++z)
		for(int x=0;x<xDim-1;++x,++qcPtr)
			{
			/* Compare the grid cell's four corner elevations against the invalid value: */
			const Scalar* h=&(height.getValue(z*xDim+x));
			int c=0x0;
			if(h[0]!=invalidHeight.getValue())
				c+=0x1;
			if(h[1]!=invalidHeight.getValue())
				c+=0x2;
			if(h[xDim]!=invalidHeight.getValue())
				c+=0x4;
			if(h[xDim+1]!=invalidHeight.getValue())
				c+=0x8;
			
			/* Accumulate the number of quads or triangles for this grid cell: */
			if(c==0x7||c==0xb||c==0xd||c==0xe)
				++numTriangles;
			if(c==0xf)
				++numQuads;
			
			/* Store the quad case: */
			*qcPtr=c;
			}
	
	return quadCases;
	}

Vector* ElevationGridNode::calcHoleyQuadNormals(const int* quadCases) const
	{
	/* Allocate the result array: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Vector* normals=new Vector[(zDim-1)*(xDim-1)];
	
	/* Calculate the normal scaling factors: */
	Scalar nx=zSpacing.getValue()*heightScale.getValue();
	Scalar ny=xSpacing.getValue()*zSpacing.getValue();
	Scalar nz=xSpacing.getValue()*heightScale.getValue();
	if(!ccw.getValue())
		{
		/* Flip normal vectors if quads are oriented clockwise: */
		nx=-nx;
		ny=-ny;
		nz=-nz;
		}
	
	if(heightIsY.getValue())
		{
		/* Calculate all quad normal vectors: */
		Vector* nPtr=normals;
		const int* qcPtr=quadCases;
		for(int z=0;z<zDim-1;++z)
			for(int x=0;x<xDim-1;++x,++nPtr,++qcPtr)
				{
				/* Calculate the quad normal depending on the quad's triangulation case: */
				const Scalar* h=&(height.getValue(z*xDim+x));
				switch(*qcPtr)
					{
					case 0x7: // Lower-left triangle
						(*nPtr)[0]=(h[1]-h[0])*nx;
						(*nPtr)[1]=ny;
						(*nPtr)[2]=(h[0]-h[xDim])*nz;
						break;
					
					case 0xb: // Lower-right triangle
						(*nPtr)[0]=(h[0]-h[1])*nx;
						(*nPtr)[1]=ny;
						(*nPtr)[2]=(h[1]-h[xDim+1])*nz;
						break;
					
					case 0xd: // Upper-left triangle
						(*nPtr)[0]=(h[xDim]-h[xDim+1])*nx;
						(*nPtr)[1]=ny;
						(*nPtr)[2]=(h[0]-h[xDim])*nz;
						break;
					
					case 0xe: // Upper-right triangle
						(*nPtr)[0]=(h[xDim]-h[xDim+1])*nx;
						(*nPtr)[1]=ny;
						(*nPtr)[2]=(h[1]-h[xDim+1])*nz;
						break;
					
					case 0xf: // Full quad
						(*nPtr)[0]=(h[0]-h[1]+h[xDim]-h[xDim+1])*nx;
						(*nPtr)[1]=ny*Scalar(2); // To average over sum of two triangle normals;
						(*nPtr)[2]=(h[0]+h[1]-h[xDim]-h[xDim+1])*nz;
						break;
					
					default:
						*nPtr=Vector::zero;
					}
				}
		}
	else
		{
		/* Flip normal vectors to account for y,z-swap: */
		nx=-nx;
		ny=-ny;
		nz=-nz;
		
		/* Calculate all quad normal vectors: */
		Vector* nPtr=normals;
		const int* qcPtr=quadCases;
		for(int z=0;z<zDim-1;++z)
			for(int x=0;x<xDim-1;++x,++nPtr,++qcPtr)
				{
				/* Calculate the quad normal depending on the quad's triangulation case: */
				const Scalar* h=&(height.getValue(z*xDim+x));
				switch(*qcPtr)
					{
					case 0x7: // Lower-left triangle
						(*nPtr)[0]=(h[1]-h[0])*nx;
						(*nPtr)[1]=(h[0]-h[xDim])*nz;
						(*nPtr)[2]=ny;
						break;
					
					case 0xb: // Lower-right triangle
						(*nPtr)[0]=(h[0]-h[1])*nx;
						(*nPtr)[1]=(h[1]-h[xDim+1])*nz;
						(*nPtr)[2]=ny;
						break;
					
					case 0xd: // Upper-left triangle
						(*nPtr)[0]=(h[xDim]-h[xDim+1])*nx;
						(*nPtr)[1]=(h[0]-h[xDim])*nz;
						(*nPtr)[2]=ny;
						break;
					
					case 0xe: // Upper-right triangle
						(*nPtr)[0]=(h[xDim]-h[xDim+1])*nx;
						(*nPtr)[1]=(h[1]-h[xDim+1])*nz;
						(*nPtr)[2]=ny;
						break;
					
					case 0xf: // Full quad
						(*nPtr)[0]=(h[0]-h[1]+h[xDim]-h[xDim+1])*nx;
						(*nPtr)[1]=(h[0]+h[1]-h[xDim]-h[xDim+1])*nz;
						(*nPtr)[2]=ny*Scalar(2); // To average over sum of two triangle normals;
						break;
					
					default:
						*nPtr=Vector::zero;
					}
				}
		}
	
	return normals;
	}

namespace {

/****************************
Helper classes and functions:
****************************/

class UploadNormalHeightIsYFunctor
	{
	/* Elements: */
	private:
	char* normalPtr; // Pointer to the next normal vector to be uploaded in GL buffer memory
	ptrdiff_t stride; // Memory increment from one normal vector to the next in bytes
	
	/* Constructors and destructors: */
	public:
	UploadNormalHeightIsYFunctor(char* sNormalPtr,ptrdiff_t sStride)
		:normalPtr(sNormalPtr),stride(sStride)
		{
		}
	
	/* Methods: */
	void operator()(Scalar nx,Scalar ny,Scalar nz)
		{
		/* Calculate the length of the given normal vector: */
		Scalar nLen=Math::sqrt(nx*nx+ny*ny+nz*nz);
		
		/* Upload the normalized normal vector to the destination: */
		Scalar* n=reinterpret_cast<Scalar*>(normalPtr);
		n[0]=nx/nLen;
		n[1]=ny/nLen;
		n[2]=nz/nLen;
		
		/* Advance to the next normal vector: */
		normalPtr+=stride;
		}
	};

class UploadNormalHeightIsZFunctor
	{
	/* Elements: */
	private:
	char* normalPtr; // Pointer to the next normal vector to be uploaded in GL buffer memory
	ptrdiff_t stride; // Memory increment from one normal vector to the next in bytes
	
	/* Constructors and destructors: */
	public:
	UploadNormalHeightIsZFunctor(char* sNormalPtr,ptrdiff_t sStride)
		:normalPtr(sNormalPtr),stride(sStride)
		{
		}
	
	/* Methods: */
	void operator()(Scalar nx,Scalar ny,Scalar nz)
		{
		/* Calculate the length of the given normal vector: */
		Scalar nLen=Math::sqrt(nx*nx+ny*ny+nz*nz);
		
		/* Upload the normalized normal vector to the destination: */
		Scalar* n=reinterpret_cast<Scalar*>(normalPtr);
		n[0]=nx/nLen;
		n[1]=nz/nLen;
		n[2]=ny/nLen;
		
		/* Advance to the next normal vector: */
		normalPtr+=stride;
		}
	};

class UploadNormalTransformedHeightIsYFunctor
	{
	/* Elements: */
	private:
	const PointTransformNode& pointTransform; // The point transformation node
	const Geometry::Point<Scalar,3>* positionPtr; // Pointer to the next untransformed vertex position
	char* normalPtr; // Pointer to the next normal vector to be uploaded in GL buffer memory
	ptrdiff_t stride; // Memory increment from one normal vector to the next in bytes
	
	/* Constructors and destructors: */
	public:
	UploadNormalTransformedHeightIsYFunctor(const PointTransformNode& sPointTransform,const Geometry::Point<Scalar,3>* sPositionPtr,char* sNormalPtr,ptrdiff_t sStride)
		:pointTransform(sPointTransform),positionPtr(sPositionPtr),
		 normalPtr(sNormalPtr),stride(sStride)
		{
		}
	
	/* Methods: */
	void operator()(Scalar nx,Scalar ny,Scalar nz)
		{
		/* Transform and normalize the given normal vector: */
		PointTransformNode::TVector tNormal=pointTransform.transformNormal(*positionPtr,PointTransformNode::TVector(nx,ny,nz)).normalize();
		
		/* Upload the normalized normal vector to the destination: */
		Scalar* n=reinterpret_cast<Scalar*>(normalPtr);
		n[0]=tNormal[0];
		n[1]=tNormal[1];
		n[2]=tNormal[2];
		
		/* Advance to the next position and normal vector: */
		++positionPtr;
		normalPtr+=stride;
		}
	};

class UploadNormalTransformedHeightIsZFunctor
	{
	/* Elements: */
	private:
	const PointTransformNode& pointTransform; // The point transformation node
	const Geometry::Point<Scalar,3>* positionPtr; // Pointer to the next untransformed vertex position
	char* normalPtr; // Pointer to the next normal vector to be uploaded in GL buffer memory
	ptrdiff_t stride; // Memory increment from one normal vector to the next in bytes
	
	/* Constructors and destructors: */
	public:
	UploadNormalTransformedHeightIsZFunctor(const PointTransformNode& sPointTransform,const Geometry::Point<Scalar,3>* sPositionPtr,char* sNormalPtr,ptrdiff_t sStride)
		:pointTransform(sPointTransform),positionPtr(sPositionPtr),
		 normalPtr(sNormalPtr),stride(sStride)
		{
		}
	
	/* Methods: */
	void operator()(Scalar nx,Scalar ny,Scalar nz)
		{
		/* Transform and normalize the given normal vector: */
		PointTransformNode::TVector tNormal=pointTransform.transformNormal(*positionPtr,PointTransformNode::TVector(nx,nz,ny)).normalize();
		
		/* Upload the normalized normal vector to the destination: */
		Scalar* n=reinterpret_cast<Scalar*>(normalPtr);
		n[0]=tNormal[0];
		n[1]=tNormal[1];
		n[2]=tNormal[2];
		
		/* Advance to the next position and normal vector: */
		++positionPtr;
		normalPtr+=stride;
		}
	};

template <class UploadNormalFunctorParam>
inline
void
uploadNormals(
	int xDim,
	int zDim,
	Scalar xSp,
	Scalar zSp,
	bool flip,
	Scalar heightScale,
	const Scalar* heights,
	UploadNormalFunctorParam& uploadNormal)
	{
	/* Calculate normal vector scaling factors: */
	Scalar nx=heightScale/xSp;
	Scalar ny(2);
	Scalar nz=heightScale/zSp;
	
	/* Invert normal vectors if requested: */
	if(flip)
		{
		nx=-nx;
		ny=-ny;
		nz=-nz;
		}
	
	/* Calculate all normal vectors: */
	const Scalar* hPtr=heights;
	if(zDim>=3&&xDim>=3)
		{
		/* Calculate the first row of normals: */
		uploadNormal((Scalar(3)*hPtr[0]-Scalar(4)*hPtr[1]+hPtr[2])*nx,
		             ny,
		             (Scalar(3)*hPtr[0]-Scalar(4)*hPtr[xDim]+hPtr[2*xDim])*nz);
		++hPtr;
		for(int x=1;x<xDim-1;++x,++hPtr)
			{
			uploadNormal((hPtr[-1]-hPtr[1])*nx,
			             ny,
			             (Scalar(3)*hPtr[0]-Scalar(4)*hPtr[xDim]+hPtr[2*xDim])*nz);
			}
		uploadNormal((-hPtr[-2]+Scalar(4)*hPtr[-1]-Scalar(3)*hPtr[0])*nx,
		             ny,
		             (Scalar(3)*hPtr[0]-Scalar(4)*hPtr[xDim]+hPtr[2*xDim])*nz);
		++hPtr;
		
		/* Calculate the intermediate rows of normals: */
		for(int z=1;z<zDim-1;++z)
			{
			uploadNormal((Scalar(3)*hPtr[0]-Scalar(4)*hPtr[1]+hPtr[2])*nx,
			             ny,
			             (hPtr[-xDim]-hPtr[xDim])*nz);
			++hPtr;
			for(int x=1;x<xDim-1;++x,++hPtr)
				{
				uploadNormal((hPtr[-1]-hPtr[1])*nx,
				             ny,
				             (hPtr[-xDim]-hPtr[xDim])*nz);
				}
			uploadNormal((-hPtr[-2]+Scalar(4)*hPtr[-1]-Scalar(3)*hPtr[0])*nx,
			             ny,
			             (hPtr[-xDim]-hPtr[xDim])*nz);
			++hPtr;
			}
		
		/* Calculate the last row of normals: */
		uploadNormal((Scalar(3)*hPtr[0]-Scalar(4)*hPtr[1]+hPtr[2])*nx,
		             ny,
		             (-hPtr[-2*xDim]+Scalar(4)*hPtr[-xDim]-Scalar(3)*hPtr[0])*nz);
		++hPtr;
		for(int x=1;x<xDim-1;++x,++hPtr)
			{
			uploadNormal((hPtr[-1]-hPtr[1])*nx,
			             ny,
			             (-hPtr[-2*xDim]+Scalar(4)*hPtr[-xDim]-Scalar(3)*hPtr[0])*nz);
			}
		uploadNormal((-hPtr[-2]+Scalar(4)*hPtr[-1]-Scalar(3)*hPtr[0])*nx,
		             ny,
		             (-hPtr[-2*xDim]+Scalar(4)*hPtr[-xDim]-Scalar(3)*hPtr[0])*nz);
		++hPtr;
		}
	else if(zDim>=3)
		{
		/* Calculate the first row of normals: */
		{
		Scalar n01x=Scalar(2)*(hPtr[0]-hPtr[1])*nx;
		uploadNormal(n01x,ny,(Scalar(3)*hPtr[0]-Scalar(4)*hPtr[xDim]+hPtr[2*xDim])*nz);
		++hPtr;
		uploadNormal(n01x,ny,(Scalar(3)*hPtr[0]-Scalar(4)*hPtr[xDim]+hPtr[2*xDim])*nz);
		++hPtr;
		}
		
		/* Calculate the intermediate rows of normals: */
		for(int z=1;z<zDim-1;++z)
			{
			Scalar n01x=Scalar(2)*(hPtr[0]-hPtr[1])*nx;
			uploadNormal(n01x,ny,(hPtr[-xDim]-hPtr[xDim])*nz);
			++hPtr;
			uploadNormal(n01x,ny,(hPtr[-xDim]-hPtr[xDim])*nz);
			++hPtr;
			}
		
		/* Calculate the last row of normals: */
		{
		Scalar n01x=Scalar(2)*(hPtr[0]-hPtr[1])*nx;
		uploadNormal(n01x,ny,(-hPtr[-2*xDim]+Scalar(4)*hPtr[-xDim]-Scalar(3)*hPtr[0])*nz);
		++hPtr;
		uploadNormal(n01x,ny,(-hPtr[-2*xDim]+Scalar(4)*hPtr[-xDim]-Scalar(3)*hPtr[0])*nz);
		++hPtr;
		}
		}
	else if(xDim>=3)
		{
		/* Calculate the first row of normals: */
		uploadNormal((Scalar(3)*hPtr[0]-Scalar(4)*hPtr[1]+hPtr[2])*nx,ny,Scalar(2)*(hPtr[0]-hPtr[xDim])*nz);
		++hPtr;
		for(int x=1;x<xDim-1;++x,++hPtr)
			uploadNormal((hPtr[-1]-hPtr[1])*nx,ny,Scalar(2)*(hPtr[0]-hPtr[xDim])*nz);
		uploadNormal((-hPtr[-2]+Scalar(4)*hPtr[-1]-Scalar(3)*hPtr[0])*nx,ny,Scalar(2)*(hPtr[0]-hPtr[xDim])*nz);
		++hPtr;
		
		/* Calculate the last row of normals: */
		uploadNormal((Scalar(3)*hPtr[0]-Scalar(4)*hPtr[1]+hPtr[2])*nx,ny,Scalar(2)*(hPtr[-xDim]-hPtr[0])*nz);
		++hPtr;
		for(int x=1;x<xDim-1;++x,++hPtr)
			uploadNormal((hPtr[-1]-hPtr[1])*nx,ny,Scalar(2)*(hPtr[-xDim]-hPtr[0])*nz);
		uploadNormal((-hPtr[-2]+Scalar(4)*hPtr[-1]-Scalar(3)*hPtr[0])*nx,ny,Scalar(2)*(hPtr[-xDim]-hPtr[0])*nz);
		++hPtr;
		}
	else
		{
		/* Calculate the four corner normals: */
		Scalar n0x=Scalar(2)*(hPtr[0]-hPtr[1])*nx;
		Scalar n1x=Scalar(2)*(hPtr[2]-hPtr[3])*nx;
		Scalar n0z=Scalar(2)*(hPtr[0]-hPtr[2])*nx;
		Scalar n1z=Scalar(2)*(hPtr[1]-hPtr[3])*nx;
		uploadNormal(n0x,ny,n0z);
		uploadNormal(n1x,ny,n0z);
		uploadNormal(n0x,ny,n1z);
		uploadNormal(n1x,ny,n1z);
		}
	}

template <class IndexParam>
inline
void
uploadIndices(
	int xDim,
	int zDim,
	bool ccw,
	bool havePrimitiveRestart)
	{
	/* Store all vertex indices: */
	if(havePrimitiveRestart)
		{
		/* Create an index array to render the elevation grid as a single quad strip with restart: */
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,((zDim-1)*(xDim*2+1)-1)*sizeof(IndexParam),0,GL_STATIC_DRAW_ARB);
		IndexParam* iPtr=static_cast<IndexParam*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		if(ccw)
			{
			for(int z=1;z<zDim;++z)
				{
				/* Upload the partial quad strip indices: */
				for(int x=0;x<xDim;++x,iPtr+=2)
					{
					iPtr[0]=IndexParam((z-1)*xDim+x);
					iPtr[1]=IndexParam(z*xDim+x);
					}
				
				if(z<zDim-1)
					{
					/* Insert the restart index: */
					*(iPtr++)=IndexParam(-1);
					}
				}
			}
		else
			{
			for(int z=1;z<zDim;++z)
				{
				/* Upload the partial quad strip indices: */
				for(int x=0;x<xDim;++x,iPtr+=2)
					{
					iPtr[0]=IndexParam(z*xDim+x);
					iPtr[1]=IndexParam((z-1)*xDim+x);
					}
				
				if(z<zDim-1)
					{
					/* Insert the restart index: */
					*(iPtr++)=IndexParam(-1);
					}
				}
			}
		}
	else
		{
		/* Create an index array to render the elevation grid as a series of quad strips: */
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,(zDim-1)*xDim*2*sizeof(IndexParam),0,GL_STATIC_DRAW_ARB);
		IndexParam* iPtr=static_cast<IndexParam*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		if(ccw)
			{
			for(int z=1;z<zDim;++z)
				for(int x=0;x<xDim;++x,iPtr+=2)
					{
					iPtr[0]=IndexParam((z-1)*xDim+x);
					iPtr[1]=IndexParam(z*xDim+x);
					}
			}
		else
			{
			for(int z=1;z<zDim;++z)
				for(int x=0;x<xDim;++x,iPtr+=2)
					{
					iPtr[0]=IndexParam(z*xDim+x);
					iPtr[1]=IndexParam((z-1)*xDim+x);
					}
			}
		}
	
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	}

}

void ElevationGridNode::uploadIndexedQuadStripSet(ElevationGridNode::DataItem* dataItem) const
	{
	/* Retrieve the elevation grid layout: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Scalar xSp=xSpacing.getValue();
	Scalar zSp=zSpacing.getValue();
	
	/* Initialize the vertex buffer object: */
	size_t numVertices=size_t(zDim)*size_t(xDim);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,numVertices*dataItem->vertexSize,0,GL_STATIC_DRAW_ARB);
	char* vertices=static_cast<char*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	ptrdiff_t vertexStride=dataItem->vertexSize;
	
	/* Retrieve additional parameters: */
	Scalar hs=heightScale.getValue();
	const Point& o=origin.getValue();
	Scalar ho=o[heightIsY.getValue()?1:2];
	
	/* Pre-compute untransformed vertex positions into a temporary array: */
	typedef Geometry::Point<Scalar,3> Position;
	Misc::SelfDestructArray<Position> positions(new Position[size_t(zDim)*size_t(xDim)]);
	Position* pPtr=positions;
	const MFFloat::ValueList& heights=height.getValues();
	MFFloat::ValueList::const_iterator hIt=heights.begin();
	if(heightIsY.getValue())
		{
		for(int z=0;z<zDim;++z)
			{
			Scalar pz=o[2]+Scalar(z)*zSp;
			for(int x=0;x<xDim;++x,++pPtr,++hIt)
				{
				(*pPtr)[0]=o[0]+Scalar(x)*xSp;
				(*pPtr)[1]=o[1]+*hIt*hs;
				(*pPtr)[2]=pz;
				}
			}
		}
	else
		{
		for(int z=0;z<zDim;++z)
			{
			Scalar py=o[1]+Scalar(z)*zSp;
			for(int x=0;x<xDim;++x,++pPtr,++hIt)
				{
				(*pPtr)[0]=o[0]+Scalar(x)*xSp;
				(*pPtr)[1]=py;
				(*pPtr)[2]=o[2]+*hIt*hs;
				}
			}
		}
	
	/* Check if texture coordinates are required: */
	if(numNeedsTexCoords!=0)
		{
		typedef Geometry::Point<Scalar,2> TexCoord;
		
		/* Upload all vertices' texture coordinates: */
		char* tcPtr=vertices+dataItem->texCoordOffset;
		if(imageProjection.getValue()!=0)
			{
			/* Calculate texture coordinates based on vertex positions: */
			const ImageProjectionNode& ip=*imageProjection.getValue();
			Position* pPtr=positions;
			char* tcEnd=tcPtr+numVertices*vertexStride;
			for(;tcPtr!=tcEnd;tcPtr+=vertexStride,++pPtr)
				*reinterpret_cast<TexCoord*>(tcPtr)=ip.calcTexCoord(*pPtr);
			}
		else if(texCoord.getValue()!=0)
			{
			/* Copy texture coordinates from the texture coordinate node: */
			const MFTexCoord::ValueList& texCoords=texCoord.getValue()->point.getValues();
			MFTexCoord::ValueList::const_iterator tcIt=texCoords.begin();
			char* tcEnd=tcPtr+numVertices*vertexStride;
			for(;tcPtr!=tcEnd;tcPtr+=vertexStride,++tcIt)
				*reinterpret_cast<TexCoord*>(tcPtr)=*tcIt;
			}
		else
			{
			/* Generate standard texture coordinates: */
			for(int z=0;z<zDim;++z)
				{
				Scalar tz(Scalar(z)/Scalar(zDim-1));
				for(int x=0;x<xDim;++x,tcPtr+=vertexStride)
					{
					TexCoord& tc=*reinterpret_cast<TexCoord*>(tcPtr);
					tc[0]=Scalar(x)/Scalar(xDim-1);
					tc[1]=tz;
					}
				}
			}
		}
	
	/* Check if colors are defined or required: */
	if(numNeedsColors!=0||haveColors)
		{
		typedef GLColor<GLubyte,4> Color;
		
		/* Upload all vertices' colors: */
		char* cPtr=vertices+dataItem->colorOffset;
		char* cEnd=cPtr+numVertices*vertexStride;
		if(color.getValue()!=0)
			{
			/* Copy colors from the color node: */
			const MFColor::ValueList& colors=color.getValue()->color.getValues();
			MFColor::ValueList::const_iterator cIt=colors.begin();
			for(;cPtr!=cEnd;cPtr+=vertexStride,++cIt)
				*reinterpret_cast<Color*>(cPtr)=*cIt;
			}
		else if(colorMap.getValue()!=0)
			{
			/* Map colors based on height: */
			const ColorMapNode& cm=*colorMap.getValue();
			MFFloat::ValueList::const_iterator hIt=heights.begin();
			for(;cPtr!=cEnd;cPtr+=vertexStride,++hIt)
				*reinterpret_cast<Color*>(cPtr)=cm.mapColor(ho+*hIt*hs);
			}
		else
			{
			/* Upload all-white colors: */
			for(;cPtr!=cEnd;cPtr+=vertexStride)
				{
				Color& c=*reinterpret_cast<Color*>(cPtr);
				c[3]=c[2]=c[1]=c[0]=GLubyte(255U);
				}
			}
		}
	
	/* Check if normal vectors are required: */
	if(numNeedsNormals)
		{
		typedef Geometry::Vector<Scalar,3> Normal;
		
		/* Upload all vertices' normal vectors: */
		char* nPtr=vertices+dataItem->normalOffset;
		char* nEnd=nPtr+numVertices*vertexStride;
		if(normal.getValue()!=0)
			{
			/* Copy normal vectors from the normal node: */
			if(pointTransform.getValue()!=0)
				{
				const PointTransformNode& pt=*pointTransform.getValue();
				const Position* positionPtr=positions;
				const MFVector::ValueList& normals=normal.getValue()->vector.getValues();
				MFVector::ValueList::const_iterator nIt=normals.begin();
				for(;nPtr!=nEnd;nPtr+=vertexStride,++positionPtr,++nIt)
					{
					PointTransformNode::TVector tNormal=pt.transformNormal(*positionPtr,*nIt).normalize();
					Scalar* n=reinterpret_cast<Scalar*>(nPtr);
					n[0]=Scalar(tNormal[0]);
					n[1]=Scalar(tNormal[1]);
					n[2]=Scalar(tNormal[2]);
					}
				}
			else
				{
				const MFVector::ValueList& normals=normal.getValue()->vector.getValues();
				MFVector::ValueList::const_iterator nIt=normals.begin();
				for(;nPtr!=nEnd;nPtr+=vertexStride,++nIt)
					*reinterpret_cast<Normal*>(nPtr)=*nIt;
				}
			}
		else
			{
			/***************************************************************
			Use central differencing to calculate normal vectors and extend
			to the edges of the grid using the underlying parabolic model:
			***************************************************************/
			
			if(pointTransform.getValue()!=0)
				{
				if(heightIsY.getValue())
					{
					/* Upload all normal vectors using the appropriate upload functor: */
					UploadNormalTransformedHeightIsYFunctor uploadNormal(*pointTransform.getValue(),positions,nPtr,vertexStride);
					uploadNormals(xDim,zDim,xSp,zSp,!ccw.getValue(),hs,height.getValues().data(),uploadNormal);
					}
				else
					{
					/* Upload all normal vectors using the appropriate upload functor: */
					UploadNormalTransformedHeightIsZFunctor uploadNormal(*pointTransform.getValue(),positions,nPtr,vertexStride);
					uploadNormals(xDim,zDim,xSp,zSp,ccw.getValue(),hs,height.getValues().data(),uploadNormal);
					}
				}
			else
				{
				if(heightIsY.getValue())
					{
					/* Upload all normal vectors using the appropriate upload functor: */
					UploadNormalHeightIsYFunctor uploadNormal(nPtr,vertexStride);
					uploadNormals(xDim,zDim,xSp,zSp,!ccw.getValue(),hs,height.getValues().data(),uploadNormal);
					}
				else
					{
					/* Upload all normal vectors using the appropriate upload functor: */
					UploadNormalHeightIsZFunctor uploadNormal(nPtr,vertexStride);
					uploadNormals(xDim,zDim,xSp,zSp,ccw.getValue(),hs,height.getValues().data(),uploadNormal);
					}
				}
			}
		}
	
	/* Upload all vertices' positions: */
	Position* pEnd=positions+(size_t(zDim)*size_t(xDim));
	char* dpPtr=vertices+dataItem->positionOffset;
	if(pointTransform.getValue()!=0)
		{
		/* Transform and upload the pre-computed untransformed vertex positions: */
		const PointTransformNode& pt=*pointTransform.getValue();
		for(Position* pPtr=positions;pPtr!=pEnd;++pPtr,dpPtr+=vertexStride)
			*reinterpret_cast<Position*>(dpPtr)=Position(pt.transformPoint(*pPtr));
		}
	else
		{
		/* Upload the pre-computed untransformed vertex positions: */
		for(Position* pPtr=positions;pPtr!=pEnd;++pPtr,dpPtr+=vertexStride)
			*reinterpret_cast<Position*>(dpPtr)=*pPtr;
		}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Upload all vertex indices as 16- or 32-bit integers, depending on the number of vertices: */
	if(size_t(zDim)*size_t(xDim)<(dataItem->havePrimitiveRestart?65535U:65536U))
		uploadIndices<GLushort>(xDim,zDim,ccw.getValue(),dataItem->havePrimitiveRestart);
	else
		uploadIndices<GLuint>(xDim,zDim,ccw.getValue(),dataItem->havePrimitiveRestart);
	}

void ElevationGridNode::uploadQuadSet(void) const
	{
	/* Define the vertex type used in the vertex array: */
	typedef GLGeometry::Vertex<Scalar,2,GLubyte,4,Scalar,Scalar,3> Vertex;
	
	/* Retrieve the elevation grid layout: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	
	/* Calculate all untransformed vertex positions: */
	Point* vertices=calcVertices();
	
	/* Calculate all per-quad or per-vertex normal vectors if there are no explicit normals: */
	Vector* quadNormals=0;
	Vector* vertexNormals=0;
	if(normalPerVertex.getValue())
		{
		if(normal.getValue()!=0)
			{
			/* Copy the provided normals: */
			vertexNormals=new Vector[zDim*xDim];
			MF<Vector>::ValueList::const_iterator nIt=normal.getValue()->vector.getValues().begin();
			Vector* vnPtr=vertexNormals;
			if(heightIsY.getValue())
				{
				for(int i=zDim*xDim;i>0;--i,++nIt,++vnPtr)
					*vnPtr=*nIt;
				}
			else
				{
				for(int i=zDim*xDim;i>0;--i,++nIt,++vnPtr)
					{
					(*vnPtr)[0]=-(*nIt)[0];
					(*vnPtr)[1]=-(*nIt)[2];
					(*vnPtr)[2]=-(*nIt)[1];
					}
				}
			}
		else
			{
			/* Calculate the per-quad normals: */
			quadNormals=calcQuadNormals();
			
			/* Convert the per-quad normals to non-normalized per-vertex normals: */
			vertexNormals=new Vector[zDim*xDim];
			Vector* vnPtr=vertexNormals;
			for(int z=0;z<zDim;++z)
				for(int x=0;x<xDim;++x,++vnPtr)
					{
					*vnPtr=Vector::zero;
					Vector* qn=quadNormals+(z*(xDim-1)+x);
					if(x>0)
						{
						if(z>0)
							*vnPtr+=qn[-(xDim-1)-1];
						if(z<zDim-1)
							*vnPtr+=qn[-1];
						}
					if(x<xDim-1)
						{
						if(z>0)
							*vnPtr+=qn[-(xDim-1)];
						if(z<zDim-1)
							*vnPtr+=qn[0];
						}
					}
			
			/* Delete the per-quad normals: */
			delete[] quadNormals;
			quadNormals=0;
			}
		
		if(pointTransform.getValue()!=0)
			{
			/* Transform the per-vertex normals: */
			Point* vPtr=vertices;
			Vector* vnPtr=vertexNormals;
			for(int i=zDim*xDim;i>0;--i,++vPtr,++vnPtr)
				*vnPtr=pointTransform.getValue()->transformNormal(*vPtr,*vnPtr);
			}
		else
			{
			/* Normalize the per-vertex normals: */
			Vector* vnPtr=vertexNormals;
			for(int i=zDim*xDim;i>0;--i,++vnPtr)
				vnPtr->normalize();
			}
		}
	else
		{
		if(normal.getValue()!=0)
			{
			/* Copy the provided normals: */
			quadNormals=new Vector[(zDim-1)*(xDim-1)];
			MF<Vector>::ValueList::const_iterator nIt=normal.getValue()->vector.getValues().begin();
			Vector* qnPtr=quadNormals;
			if(heightIsY.getValue())
				{
				for(int i=(zDim-1)*(xDim-1);i>0;--i,++nIt,++qnPtr)
					*qnPtr=*nIt;
				}
			else
				{
				for(int i=(zDim-1)*(xDim-1);i>0;--i,++nIt,++qnPtr)
					{
					(*qnPtr)[0]=-(*nIt)[0];
					(*qnPtr)[1]=-(*nIt)[2];
					(*qnPtr)[2]=-(*nIt)[1];
					}
				}
			}
		else
			{
			/* Calculate the per-quad normals: */
			quadNormals=calcQuadNormals();
			}
		
		if(pointTransform.getValue()!=0)
			{
			/* Transform the per-quad normals: */
			Vector* qnPtr=quadNormals;
			for(int z=0;z<zDim-1;++z)
				for(int x=0;x<xDim-1;++x,++qnPtr)
					{
					/* Transform the per-quad normal around the quad's midpoint: */
					Point* vPtr=vertices+(z*xDim+x);
					Point mp;
					for(int i=0;i<3;++i)
						mp[i]=(vPtr[0][i]+vPtr[1][i]+vPtr[xDim][i]+vPtr[xDim+1][i])*Scalar(0.25);
					*qnPtr=pointTransform.getValue()->transformNormal(mp,*qnPtr);
					}
			}
		else
			{
			/* Normalize the per-quad normals: */
			Vector* qnPtr=quadNormals;
			for(int i=(zDim-1)*(xDim-1);i>0;--i,++qnPtr)
				qnPtr->normalize();
			}
		}
	
	TexCoord* vertexTexCoords=0;
	if(imageProjection.getValue()!=0)
		{
		/* Calculate per-vertex texture coordinates: */
		vertexTexCoords=new TexCoord[xDim*zDim];
		Point* vPtr=vertices;
		TexCoord* vtcPtr=vertexTexCoords;
		for(int z=0;z<zDim;++z)
			for(int x=0;x<xDim;++x,++vPtr,++vtcPtr)
				*vtcPtr=imageProjection.getValue()->calcTexCoord(*vPtr);
		}
	
	if(pointTransform.getValue()!=0)
		{
		/* Transform all vertex positions: */
		Point* vPtr=vertices;
		for(int i=zDim*xDim;i>0;--i,++vPtr)
			*vPtr=pointTransform.getValue()->transformPoint(*vPtr);
		}
	
	/* Initialize the vertex buffer object: */
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,(xDim-1)*(zDim-1)*4*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	
	/* Store all vertices: */
	int hComp=heightIsY.getValue()?1:2;
	Scalar hOffset=origin.getValue()[hComp];
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	size_t qInd=0;
	for(int z=0;z<zDim-1;++z)
		for(int x=0;x<xDim-1;++x,vPtr+=4,++qInd)
			{
			size_t vInd=z*xDim+x;
			Vertex v[4];
			
			/* Calculate the corner texture coordinates of the current quad: */
			if(imageProjection.getValue()!=0)
				{
				/* Store the per-vertex texture coordinates: */
				v[0].texCoord=vertexTexCoords[vInd];
				v[1].texCoord=vertexTexCoords[vInd+1];
				v[2].texCoord=vertexTexCoords[vInd+xDim+1];
				v[3].texCoord=vertexTexCoords[vInd+xDim];
				}
			else if(texCoord.getValue()!=0)
				{
				v[0].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd));
				v[1].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+1));
				v[2].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+xDim+1));
				v[3].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+xDim));
				}
			else
				{
				v[0].texCoord=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				v[1].texCoord=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				v[2].texCoord=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				v[3].texCoord=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				}
			
			/* Get the corner colors of the current quad: */
			if(color.getValue()!=0)
				{
				if(colorPerVertex.getValue())
					{
					v[0].color=Vertex::Color(color.getValue()->color.getValue(vInd));
					v[1].color=Vertex::Color(color.getValue()->color.getValue(vInd+1));
					v[2].color=Vertex::Color(color.getValue()->color.getValue(vInd+xDim+1));
					v[3].color=Vertex::Color(color.getValue()->color.getValue(vInd+xDim));
					}
				else
					{
					Vertex::Color c=Vertex::Color(color.getValue()->color.getValue(qInd));
					for(int i=0;i<4;++i)
						v[i].color=c;
					}
				}
			else if(colorMap.getValue()!=0)
				{
				if(colorPerVertex.getValue())
					{
					v[0].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd)*heightScale.getValue()));
					v[1].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd+1)*heightScale.getValue()));
					v[2].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd+xDim+1)*heightScale.getValue()));
					v[3].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd+xDim)*heightScale.getValue()));
					}
				else
					{
					Scalar h=(height.getValue(vInd)+height.getValue(vInd+1)+height.getValue(vInd+xDim)+height.getValue(vInd+xDim+1))*heightScale.getValue();
					Vertex::Color c=Vertex::Color(colorMap.getValue()->mapColor(hOffset+h*Scalar(0.25)));
					for(int i=0;i<4;++i)
						v[i].color=c;
					}
				}
			else
				{
				for(int i=0;i<4;++i)
					v[i].color=Vertex::Color(255,255,255);
				}
			
			/* Set the corner normal vectors and vertex positions of the current quad: */
			if(normalPerVertex.getValue())
				{
				v[0].normal=Vertex::Normal(vertexNormals[vInd]);
				v[1].normal=Vertex::Normal(vertexNormals[vInd+1]);
				v[2].normal=Vertex::Normal(vertexNormals[vInd+xDim+1]);
				v[3].normal=Vertex::Normal(vertexNormals[vInd+xDim]);
				}
			else
				{
				Vertex::Normal n=Vertex::Normal(quadNormals[qInd]);
				for(int i=0;i<4;++i)
					v[i].normal=n;
				}
			v[0].position=Vertex::Position(vertices[vInd]);
			v[1].position=Vertex::Position(vertices[vInd+1]);
			v[2].position=Vertex::Position(vertices[vInd+xDim+1]);
			v[3].position=Vertex::Position(vertices[vInd+xDim]);
			
			/* Store the corner vertices of the current quad: */
			if(ccw.getValue())
				{
				/* Store the corner vertices in counter-clockwise order: */
				for(int i=0;i<4;++i)
					vPtr[i]=v[3-i];
				}
			else
				{
				/* Store the corner vertices in clockwise order: */
				for(int i=0;i<4;++i)
					vPtr[i]=v[i];
				}
			}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Delete the per-quad and per-vertex normals, texture coordinates, and vertex positions: */
	delete[] quadNormals;
	delete[] vertexNormals;
	delete[] vertexTexCoords;
	delete[] vertices;
	}

void ElevationGridNode::uploadHoleyQuadTriangleSet(GLuint& numQuads,GLuint& numTriangles) const
	{
	/* Define the vertex type used in the vertex array: */
	typedef GLGeometry::Vertex<Scalar,2,GLubyte,4,Scalar,Scalar,3> Vertex;
	
	/* Retrieve the elevation grid layout: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	
	/* Calculate all untransformed vertex positions: */
	Point* vertices=calcVertices();
	
	/* Calculate the triangulation cases for all grid quads: */
	int* quadCases=calcHoleyQuadCases(numQuads,numTriangles);
	
	/* Calculate all per-quad or per-vertex normal vectors if there are no explicit normals: */
	Vector* quadNormals=0;
	Vector* vertexNormals=0;
	if(normalPerVertex.getValue())
		{
		if(normal.getValue()!=0)
			{
			/* Copy the provided normals: */
			vertexNormals=new Vector[zDim*xDim];
			MF<Vector>::ValueList::const_iterator nIt=normal.getValue()->vector.getValues().begin();
			Vector* vnPtr=vertexNormals;
			if(heightIsY.getValue())
				{
				for(int i=zDim*xDim;i>0;--i,++nIt,++vnPtr)
					*vnPtr=*nIt;
				}
			else
				{
				for(int i=zDim*xDim;i>0;--i,++nIt,++vnPtr)
					{
					(*vnPtr)[0]=-(*nIt)[0];
					(*vnPtr)[1]=-(*nIt)[2];
					(*vnPtr)[2]=-(*nIt)[1];
					}
				}
			}
		else
			{
			/* Calculate the per-quad normals: */
			quadNormals=calcHoleyQuadNormals(quadCases);
			
			/* Convert the per-quad normals to non-normalized per-vertex normals: */
			vertexNormals=new Vector[zDim*xDim];
			MF<Scalar>::ValueList::const_iterator hIt=height.getValues().begin();
			Vector* vnPtr=vertexNormals;
			for(int z=0;z<zDim;++z)
				for(int x=0;x<xDim;++x,++hIt,++vnPtr)
					if(*hIt!=invalidHeight.getValue())
						{
						*vnPtr=Vector::zero;
						int* qc=quadCases+(z*(xDim-1)+x);
						Vector* qn=quadNormals+(z*(xDim-1)+x);
						
						/* Add each surrounding quad's normal 0, 1, or 2 times depending on the respective quad's triangulation case: */
						if(x>0)
							{
							if(z>0)
								{
								if((qc[-(xDim-1)-1]&0xa)==0xa)
									*vnPtr+=qn[-(xDim-1)-1];
								if((qc[-(xDim-1)-1]&0xc)==0xc)
									*vnPtr+=qn[-(xDim-1)-1];
								}
							if(z<zDim-1)
								{
								if((qc[-1]&0x3)==0x3)
									*vnPtr+=qn[-1];
								if((qc[-1]&0xa)==0xa)
									*vnPtr+=qn[-1];
								}
							}
						if(x<xDim-1)
							{
							if(z>0)
								{
								if((qc[-(xDim-1)]&0x5)==0x5)
									*vnPtr+=qn[-(xDim-1)];
								if((qc[-(xDim-1)]&0xc)==0xc)
									*vnPtr+=qn[-(xDim-1)];
								}
							if(z<zDim-1)
								{
								if((qc[0]&0x3)==0x3)
									*vnPtr+=qn[0];
								if((qc[0]&0x5)==0x5)
									*vnPtr+=qn[0];
								}
							}
						}
			
			/* Delete the per-quad normals: */
			delete[] quadNormals;
			quadNormals=0;
			}
		
		if(pointTransform.getValue()!=0)
			{
			/* Transform the per-vertex normals: */
			MF<Scalar>::ValueList::const_iterator hIt=height.getValues().begin();
			Point* vPtr=vertices;
			Vector* vnPtr=vertexNormals;
			for(int i=zDim*xDim;i>0;--i,++hIt,++vPtr,++vnPtr)
				if(*hIt!=invalidHeight.getValue())
					*vnPtr=pointTransform.getValue()->transformNormal(*vPtr,*vnPtr);
			}
		else
			{
			/* Normalize the per-vertex normals: */
			MF<Scalar>::ValueList::const_iterator hIt=height.getValues().begin();
			Vector* vnPtr=vertexNormals;
			for(int i=zDim*xDim;i>0;--i,++hIt,++vnPtr)
				if(*hIt!=invalidHeight.getValue())
					vnPtr->normalize();
			}
		}
	else
		{
		if(normal.getValue()!=0)
			{
			/* Copy the provided normals: */
			quadNormals=new Vector[(zDim-1)*(xDim-1)];
			MF<Vector>::ValueList::const_iterator nIt=normal.getValue()->vector.getValues().begin();
			Vector* qnPtr=quadNormals;
			if(heightIsY.getValue())
				{
				for(int i=(zDim-1)*(xDim-1);i>0;--i,++nIt,++qnPtr)
					*qnPtr=*nIt;
				}
			else
				{
				for(int i=(zDim-1)*(xDim-1);i>0;--i,++nIt,++qnPtr)
					{
					(*qnPtr)[0]=-(*nIt)[0];
					(*qnPtr)[1]=-(*nIt)[2];
					(*qnPtr)[2]=-(*nIt)[1];
					}
				}
			}
		else
			{
			/* Calculate the per-quad normals: */
			quadNormals=calcHoleyQuadNormals(quadCases);
			}
		
		if(pointTransform.getValue()!=0)
			{
			/* Transform the per-quad normals: */
			int* qcPtr=quadCases;
			Vector* qnPtr=quadNormals;
			for(int z=0;z<zDim-1;++z)
				for(int x=0;x<xDim-1;++x,++qcPtr,++qnPtr)
					{
					/* Transform the per-quad normal around the quad's midpoint: */
					Point* vPtr=vertices+(z*xDim+x);
					Point mp;
					switch(*qcPtr)
						{
						case 0x7:
							for(int i=0;i<3;++i)
								mp[i]=(vPtr[0][i]+vPtr[1][i]+vPtr[xDim][i])/Scalar(3);
							*qnPtr=pointTransform.getValue()->transformNormal(mp,*qnPtr);
							break;
						
						case 0xb:
							for(int i=0;i<3;++i)
								mp[i]=(vPtr[0][i]+vPtr[1][i]+vPtr[xDim+1][i])/Scalar(3);
							*qnPtr=pointTransform.getValue()->transformNormal(mp,*qnPtr);
							break;
						
						case 0xd:
							for(int i=0;i<3;++i)
								mp[i]=(vPtr[0][i]+vPtr[xDim][i]+vPtr[xDim+1][i])/Scalar(3);
							*qnPtr=pointTransform.getValue()->transformNormal(mp,*qnPtr);
							break;
						
						case 0xe:
							for(int i=0;i<3;++i)
								mp[i]=(vPtr[1][i]+vPtr[xDim][i]+vPtr[xDim+1][i])/Scalar(3);
							*qnPtr=pointTransform.getValue()->transformNormal(mp,*qnPtr);
							break;
						
						case 0xf:
							for(int i=0;i<3;++i)
								mp[i]=(vPtr[0][i]+vPtr[1][i]+vPtr[xDim][i]+vPtr[xDim+1][i])*Scalar(0.25);
							*qnPtr=pointTransform.getValue()->transformNormal(mp,*qnPtr);
							break;
						}
					}
			}
		else
			{
			/* Normalize the per-quad normals: */
			Vector* qnPtr=quadNormals;
			for(int i=(zDim-1)*(xDim-1);i>0;--i,++qnPtr)
				qnPtr->normalize();
			}
		}
	
	TexCoord* vertexTexCoords=0;
	if(imageProjection.getValue()!=0)
		{
		/* Calculate per-vertex texture coordinates: */
		vertexTexCoords=new TexCoord[xDim*zDim];
		Point* vPtr=vertices;
		TexCoord* vtcPtr=vertexTexCoords;
		for(int z=0;z<zDim;++z)
			for(int x=0;x<xDim;++x,++vPtr,++vtcPtr)
				*vtcPtr=imageProjection.getValue()->calcTexCoord(*vPtr);
		}
	
	if(pointTransform.getValue()!=0)
		{
		/* Transform all vertex positions: */
		MF<Scalar>::ValueList::const_iterator hIt=height.getValues().begin();
		Point* vPtr=vertices;
		for(int i=zDim*xDim;i>0;--i,++hIt,++vPtr)
			if(*hIt!=invalidHeight.getValue())
				*vPtr=pointTransform.getValue()->transformPoint(*vPtr);
		}
	
	/* Initialize the vertex buffer object: */
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,(numQuads*4+numTriangles*3)*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	
	/* Store all vertices: */
	int hComp=heightIsY.getValue()?1:2;
	Scalar hOffset=origin.getValue()[hComp];
	Vertex* qvPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	Vertex* tvPtr=qvPtr+(numQuads*4);
	size_t qInd=0;
	for(int z=0;z<zDim-1;++z)
		for(int x=0;x<xDim-1;++x,++qInd)
			{
			size_t vInd=z*xDim+x;
			Vertex v[4];
			
			/* Calculate the corner texture coordinates of the current quad: */
			if(imageProjection.getValue()!=0)
				{
				/* Store the per-vertex texture coordinates: */
				v[0].texCoord=vertexTexCoords[vInd];
				v[1].texCoord=vertexTexCoords[vInd+1];
				v[2].texCoord=vertexTexCoords[vInd+xDim+1];
				v[3].texCoord=vertexTexCoords[vInd+xDim];
				}
			else if(texCoord.getValue()!=0)
				{
				v[0].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd));
				v[1].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+1));
				v[2].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+xDim+1));
				v[3].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+xDim));
				}
			else
				{
				v[0].texCoord=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				v[1].texCoord=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				v[2].texCoord=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				v[3].texCoord=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				}
			
			/* Get the corner colors of the current quad: */
			if(color.getValue()!=0)
				{
				if(colorPerVertex.getValue())
					{
					v[0].color=Vertex::Color(color.getValue()->color.getValue(vInd));
					v[1].color=Vertex::Color(color.getValue()->color.getValue(vInd+1));
					v[2].color=Vertex::Color(color.getValue()->color.getValue(vInd+xDim+1));
					v[3].color=Vertex::Color(color.getValue()->color.getValue(vInd+xDim));
					}
				else
					{
					Vertex::Color c=Vertex::Color(color.getValue()->color.getValue(qInd));
					for(int i=0;i<4;++i)
						v[i].color=c;
					}
				}
			else if(colorMap.getValue()!=0)
				{
				if(colorPerVertex.getValue())
					{
					v[0].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd)*heightScale.getValue()));
					v[1].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd+1)*heightScale.getValue()));
					v[2].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd+xDim+1)*heightScale.getValue()));
					v[3].color=Vertex::Color(colorMap.getValue()->mapColor(hOffset+height.getValue(vInd+xDim)*heightScale.getValue()));
					}
				else
					{
					Scalar h=Scalar(0);
					Scalar w=Scalar(0);
					if(quadCases[qInd]&0x1)
						{
						h+=height.getValue(vInd);
						w+=Scalar(1);
						}
					if(quadCases[qInd]&0x2)
						{
						h+=height.getValue(vInd+1);
						w+=Scalar(1);
						}
					if(quadCases[qInd]&0x4)
						{
						h+=height.getValue(vInd+xDim);
						w+=Scalar(1);
						}
					if(quadCases[qInd]&0x8)
						{
						h+=height.getValue(vInd+xDim+1);
						w+=Scalar(1);
						}
					Vertex::Color c=Vertex::Color(colorMap.getValue()->mapColor(hOffset+h*heightScale.getValue()/w));
					for(int i=0;i<4;++i)
						v[i].color=c;
					}
				}
			else
				{
				for(int i=0;i<4;++i)
					v[i].color=Vertex::Color(255,255,255);
				}
			
			/* Set the corner normal vectors and vertex positions of the current quad: */
			if(normalPerVertex.getValue())
				{
				v[0].normal=Vertex::Normal(vertexNormals[vInd]);
				v[1].normal=Vertex::Normal(vertexNormals[vInd+1]);
				v[2].normal=Vertex::Normal(vertexNormals[vInd+xDim+1]);
				v[3].normal=Vertex::Normal(vertexNormals[vInd+xDim]);
				}
			else
				{
				Vertex::Normal n=Vertex::Normal(quadNormals[qInd]);
				for(int i=0;i<4;++i)
					v[i].normal=n;
				}
			v[0].position=Vertex::Position(vertices[vInd]);
			v[1].position=Vertex::Position(vertices[vInd+1]);
			v[2].position=Vertex::Position(vertices[vInd+xDim+1]);
			v[3].position=Vertex::Position(vertices[vInd+xDim]);
			
			/* Store the corner vertices of the current grid cell depending on the cell's triangulation case: */
			if(ccw.getValue())
				{
				/* Store the corner vertices in counter-clockwise order: */
				switch(quadCases[qInd])
					{
					case 0x7:
						tvPtr[0]=v[3];
						tvPtr[1]=v[1];
						tvPtr[2]=v[0];
						tvPtr+=3;
						break;
					
					case 0xb:
						tvPtr[0]=v[2];
						tvPtr[1]=v[1];
						tvPtr[2]=v[0];
						tvPtr+=3;
						break;
					
					case 0xd:
						tvPtr[0]=v[3];
						tvPtr[1]=v[2];
						tvPtr[2]=v[0];
						tvPtr+=3;
						break;
					
					case 0xe:
						tvPtr[0]=v[3];
						tvPtr[1]=v[2];
						tvPtr[2]=v[1];
						tvPtr+=3;
						break;
					
					case 0xf:
						for(int i=0;i<4;++i)
							qvPtr[i]=v[3-i];
						qvPtr+=4;
					}
				}
			else
				{
				/* Store the corner vertices in clockwise order: */
				switch(quadCases[qInd])
					{
					case 0x7:
						tvPtr[0]=v[0];
						tvPtr[1]=v[1];
						tvPtr[2]=v[3];
						tvPtr+=3;
						break;
					
					case 0xb:
						tvPtr[0]=v[0];
						tvPtr[1]=v[1];
						tvPtr[2]=v[2];
						tvPtr+=3;
						break;
					
					case 0xd:
						tvPtr[0]=v[0];
						tvPtr[1]=v[2];
						tvPtr[2]=v[3];
						tvPtr+=3;
						break;
					
					case 0xe:
						tvPtr[0]=v[1];
						tvPtr[1]=v[2];
						tvPtr[2]=v[3];
						tvPtr+=3;
						break;
					
					case 0xf:
						for(int i=0;i<4;++i)
							qvPtr[i]=v[i];
						qvPtr+=4;
					}
				}
			}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Delete the per-quad and per-vertex normals, the triangulation cases, texture coordinates, and the vertex positions: */
	delete[] quadNormals;
	delete[] vertexNormals;
	delete[] quadCases;
	delete[] vertexTexCoords;
	delete[] vertices;
	}

ElevationGridNode::ElevationGridNode(void)
	:colorPerVertex(true),normalPerVertex(true),
	 creaseAngle(0),
	 origin(Point::origin),
	 xDimension(0),xSpacing(1),
	 zDimension(0),zSpacing(1),
	 heightScale(1),
	 heightIsY(true),
	 removeInvalids(false),invalidHeight(0),
	 ccw(true),solid(true),
	 propMask(0U),
	 valid(true),haveInvalids(false),canRender(false),haveColors(false),indexed(false),
	 version(0)
	{
	}

const char* ElevationGridNode::getClassName(void) const
	{
	return className;
	}

void ElevationGridNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"texCoord")==0)
		vrmlFile.parseSFNode(texCoord);
	else if(strcmp(fieldName,"color")==0)
		vrmlFile.parseSFNode(color);
	else if(strcmp(fieldName,"colorMap")==0)
		vrmlFile.parseSFNode(colorMap);
	else if(strcmp(fieldName,"imageProjection")==0)
		vrmlFile.parseSFNode(imageProjection);
	else if(strcmp(fieldName,"colorPerVertex")==0)
		vrmlFile.parseField(colorPerVertex);
	else if(strcmp(fieldName,"normal")==0)
		vrmlFile.parseSFNode(normal);
	else if(strcmp(fieldName,"normalPerVertex")==0)
		vrmlFile.parseField(normalPerVertex);
	else if(strcmp(fieldName,"creaseAngle")==0)
		vrmlFile.parseField(creaseAngle);
	else if(strcmp(fieldName,"origin")==0)
		{
		vrmlFile.parseField(origin);
		propMask|=0x1U;
		}
	else if(strcmp(fieldName,"xDimension")==0)
		vrmlFile.parseField(xDimension);
	else if(strcmp(fieldName,"xSpacing")==0)
		{
		vrmlFile.parseField(xSpacing);
		propMask|=0x2U;
		}
	else if(strcmp(fieldName,"zDimension")==0)
		vrmlFile.parseField(zDimension);
	else if(strcmp(fieldName,"zSpacing")==0)
		{
		vrmlFile.parseField(zSpacing);
		propMask|=0x4U;
		}
	else if(strcmp(fieldName,"height")==0)
		vrmlFile.parseField(height);
	else if(strcmp(fieldName,"heightUrl")==0)
		{
		vrmlFile.parseField(heightUrl);
		
		/* Remember the VRML file's base directory: */
		baseDirectory=&vrmlFile.getBaseDirectory();
		}
	else if(strcmp(fieldName,"heightUrlFormat")==0)
		vrmlFile.parseField(heightUrlFormat);
	else if(strcmp(fieldName,"heightScale")==0)
		vrmlFile.parseField(heightScale);
	else if(strcmp(fieldName,"heightIsY")==0)
		vrmlFile.parseField(heightIsY);
	else if(strcmp(fieldName,"removeInvalids")==0)
		vrmlFile.parseField(removeInvalids);
	else if(strcmp(fieldName,"invalidHeight")==0)
		{
		vrmlFile.parseField(invalidHeight);
		propMask|=0x8U;
		}
	else if(strcmp(fieldName,"ccw")==0)
		vrmlFile.parseField(ccw);
	else if(strcmp(fieldName,"solid")==0)
		vrmlFile.parseField(solid);
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void ElevationGridNode::update(void)
	{
	/* Check whether the height field should be loaded from a file: */
	if(heightUrl.getNumValues()>0)
		{
		/* Load the elevation grid's height values: */
		loadElevationGrid(*this);
		}
	
	/* Check whether the elevation grid is internally consistent: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	valid=xDim>0&&zDim>0;
	size_t numVertices=size_t(zDim)*size_t(xDim);
	size_t numQuads=size_t(zDim-1)*size_t(xDim-1);
	valid=valid&&height.getNumValues()>=numVertices;
	valid=valid&&(texCoord.getValue()==0||texCoord.getValue()->point.getNumValues()>=numVertices);
	valid=valid&&(color.getValue()==0||color.getValue()->color.getNumValues()>=(colorPerVertex.getValue()?numVertices:numQuads));
	valid=valid&&(normal.getValue()==0||normal.getValue()->vector.getNumValues()>=(normalPerVertex.getValue()?numVertices:numQuads));
	
	/* Check if there are invalid heights to be removed: */
	haveInvalids=false;
	if(valid&&removeInvalids.getValue())
		{
		/* Test all heights against the invalid value: */
		for(MFFloat::ValueList::iterator hIt=height.getValues().begin();hIt!=height.getValues().end();++hIt)
			if(*hIt==invalidHeight.getValue())
				{
				haveInvalids=true;
				break;
				}
		}
	
	/* Check whether the elevation grid supports rendering: */
	canRender=valid&&xDim>1&&zDim>1&&xSpacing.getValue()>Scalar(0)&&zSpacing.getValue()>Scalar(0);
	
	/* Check whether the elevation grid defines per-vertex or per-face colors: */
	haveColors=color.getValue()!=0||colorMap.getValue()!=0;
	
	/* Check whether the elevation grid can be represented by a set of indexed triangle strips: */
	indexed=!haveInvalids&&(colorPerVertex.getValue()||(color.getValue()==0&&colorMap.getValue()==0))&&normalPerVertex.getValue();
	
	/* Calculate the elevation grid's bounding box: */
	bbox=Box::empty;
	if(valid)
		{
		if(pointTransform.getValue()!=0)
			{
			/* Return the bounding box of the transformed point coordinates: */
			if(heightIsY.getValue())
				{
				if(haveInvalids)
					{
					MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
					Point p;
					p[2]=origin.getValue()[2];
					for(int z=0;z<zDimension.getValue();++z,p[2]+=zSpacing.getValue())
						{
						p[0]=origin.getValue()[0];
						for(int x=0;x<xDimension.getValue();++x,p[0]+=xSpacing.getValue(),++hIt)
							if(*hIt!=invalidHeight.getValue())
								{
								p[1]=origin.getValue()[1]+*hIt*heightScale.getValue();
								bbox.addPoint(pointTransform.getValue()->transformPoint(p));
								}
						}
					}
				else
					{
					MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
					Point p;
					p[2]=origin.getValue()[2];
					for(int z=0;z<zDimension.getValue();++z,p[2]+=zSpacing.getValue())
						{
						p[0]=origin.getValue()[0];
						for(int x=0;x<xDimension.getValue();++x,p[0]+=xSpacing.getValue(),++hIt)
							{
							p[1]=origin.getValue()[1]+*hIt*heightScale.getValue();
							bbox.addPoint(pointTransform.getValue()->transformPoint(p));
							}
						}
					}
				}
			else
				{
				if(haveInvalids)
					{
					MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
					Point p;
					p[1]=origin.getValue()[1];
					for(int z=0;z<zDimension.getValue();++z,p[1]+=zSpacing.getValue())
						{
						p[0]=origin.getValue()[0];
						for(int x=0;x<xDimension.getValue();++x,p[0]+=xSpacing.getValue(),++hIt)
							if(*hIt!=invalidHeight.getValue())
								{
								p[2]=origin.getValue()[2]+*hIt*heightScale.getValue();
								bbox.addPoint(pointTransform.getValue()->transformPoint(p));
								}
						}
					}
				else
					{
					MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
					Point p;
					p[1]=origin.getValue()[1];
					for(int z=0;z<zDimension.getValue();++z,p[1]+=zSpacing.getValue())
						{
						p[0]=origin.getValue()[0];
						for(int x=0;x<xDimension.getValue();++x,p[0]+=xSpacing.getValue(),++hIt)
							{
							p[2]=origin.getValue()[2]+*hIt*heightScale.getValue();
							bbox.addPoint(pointTransform.getValue()->transformPoint(p));
							}
						}
					}
				}
			}
		else
			{
			/* Return the bounding box of the untransformed point coordinates: */
			if(haveInvalids)
				{
				Scalar yMin=Math::Constants<Scalar>::max;
				Scalar yMax=Math::Constants<Scalar>::min;
				bool empty=true;
				for(MFFloat::ValueList::const_iterator hIt=height.getValues().begin();hIt!=height.getValues().end();++hIt)
					if(*hIt!=invalidHeight.getValue())
						{
						Scalar h=*hIt*heightScale.getValue();
						if(yMin>h)
							yMin=h;
						if(yMax<h)
							yMax=h;
						empty=false;
						}
				if(empty)
					bbox=Box::empty;
				else if(heightIsY.getValue())
					bbox=Box(origin.getValue()+Vector(0,yMin,0),origin.getValue()+Vector(Scalar(xDimension.getValue()-1)*xSpacing.getValue(),yMax,Scalar(zDimension.getValue()-1)*zSpacing.getValue()));
				else
					bbox=Box(origin.getValue()+Vector(0,0,yMin),origin.getValue()+Vector(Scalar(xDimension.getValue()-1)*xSpacing.getValue(),Scalar(zDimension.getValue()-1)*zSpacing.getValue(),yMax));
				}
			else
				{
				MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
				Scalar yMin,yMax;
				yMin=yMax=*hIt*heightScale.getValue();
				for(++hIt;hIt!=height.getValues().end();++hIt)
					{
					Scalar h=*hIt*heightScale.getValue();
					if(yMin>h)
						yMin=h;
					if(yMax<h)
						yMax=h;
					}
				if(heightIsY.getValue())
					bbox=Box(origin.getValue()+Vector(0,yMin,0),origin.getValue()+Vector(Scalar(xDimension.getValue()-1)*xSpacing.getValue(),yMax,Scalar(zDimension.getValue()-1)*zSpacing.getValue()));
				else
					bbox=Box(origin.getValue()+Vector(0,0,yMin),origin.getValue()+Vector(Scalar(xDimension.getValue()-1)*xSpacing.getValue(),Scalar(zDimension.getValue()-1)*zSpacing.getValue(),yMax));
				}
			}
		}
	
	/* Bump up the elevation grid's version number: */
	++version;
	}

void ElevationGridNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GeometryNode::read(reader);
	
	/* Read all fields: */
	reader.readSFNode(texCoord);
	reader.readSFNode(color);
	reader.readSFNode(colorMap);
	reader.readSFNode(imageProjection);
	reader.readField(colorPerVertex);
	reader.readSFNode(normal);
	reader.readField(normalPerVertex);
	reader.readField(creaseAngle);
	reader.readField(origin);
	reader.readField(xDimension);
	reader.readField(xSpacing);
	reader.readField(zDimension);
	reader.readField(zSpacing);
	reader.readField(height);
	reader.readField(heightScale);
	reader.readField(heightIsY);
	reader.readField(removeInvalids);
	reader.readField(invalidHeight);
	reader.readField(ccw);
	reader.readField(solid);
	
	/* Clear the height URL field to avoid reading a file that doesn't exist: */
	heightUrl.getValues().clear();
	}

void ElevationGridNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GeometryNode::write(writer);
	
	/* Write all fields: */
	writer.writeSFNode(texCoord);
	writer.writeSFNode(color);
	writer.writeSFNode(colorMap);
	writer.writeSFNode(imageProjection);
	writer.writeField(colorPerVertex);
	writer.writeSFNode(normal);
	writer.writeField(normalPerVertex);
	writer.writeField(creaseAngle);
	writer.writeField(origin);
	writer.writeField(xDimension);
	writer.writeField(xSpacing);
	writer.writeField(zDimension);
	writer.writeField(zSpacing);
	writer.writeField(height);
	writer.writeField(heightScale);
	writer.writeField(heightIsY);
	writer.writeField(removeInvalids);
	writer.writeField(invalidHeight);
	writer.writeField(ccw);
	writer.writeField(solid);
	}

bool ElevationGridNode::canCollide(void) const
	{
	/* Check whether the elevation grid supports collision: */
	return valid&&xDimension.getValue()>1&&zDimension.getValue()>1&&xSpacing.getValue()>Scalar(0)&&zSpacing.getValue()>Scalar(0)&&pointTransform.getValue()==0&&!haveInvalids;
	}

int ElevationGridNode::getGeometryRequirementMask(void) const
	{
	int result=BaseAppearanceNode::HasSurfaces;
	if(!solid.getValue())
		result|=BaseAppearanceNode::HasTwoSidedSurfaces;
	if(haveColors)
		result|=BaseAppearanceNode::HasColors;
	
	return result;
	}

Box ElevationGridNode::calcBoundingBox(void) const
	{
	/* Return the precomputed bounding box: */
	return bbox;
	}

namespace {

inline Vector triangleNormal(const Point& p0,const Point& p1,const Point& p2)
	{
	Scalar x1=(p1[0]-p0[0]);
	Scalar y1=(p1[1]-p0[1]);
	Scalar z1=(p1[2]-p0[2]);
	Scalar x2=(p2[0]-p0[0]);
	Scalar y2=(p2[1]-p0[1]);
	Scalar z2=(p2[2]-p0[2]);
	
	return Vector(y1*z2-z1*y2,z1*x2-x1*z2,x1*y2-y1*x2);
	}

}

void ElevationGridNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Find the overlap of the sphere's path and the elevation grid's bounding box and bail out if there is no overlap: */
	Math::Interval<Scalar> interval=collisionQuery.calcBoxInterval(bbox);
	if(interval.getMin()>=interval.getMax())
		return;
	
	/* Retrieve collision test parameters: */
	Point c0=collisionQuery.getC0();
	const Vector& c0c1=collisionQuery.getC0c1();
	Scalar r=collisionQuery.getRadius();
	
	/* Access elevation grid components: */
	const Scalar* h=&height.getValues().front();
	int hStride=xDimension.getValue();
	const Point& o=origin.getValue();
	Scalar xs=xSpacing.getValue();
	Scalar zs=zSpacing.getValue();
	
	/* Create a box containing the sphere's potential intersections with the elevation grid: */
	Box overlap=Box::empty;
	overlap.addPoint(Geometry::addScaled(c0,c0c1,interval.getMin()));
	overlap.addPoint(Geometry::addScaled(c0,c0c1,interval.getMax()));
	overlap.extrude(r);
	
	/* Find the grid extent of the sphere's path: */
	int xMin=Math::max(int(Math::floor((overlap.min[0]-o[0])/xs)),0);
	int xMax=Math::min(int(Math::ceil((overlap.max[0]-o[0])/xs)),xDimension.getValue()-1);
	if(heightIsY.getValue())
		{
		}
	else
		{
		int yMin=Math::max(int(Math::floor((overlap.min[1]-o[1])/zs)),0);
		int yMax=Math::min(int(Math::ceil((overlap.max[1]-o[1])/zs)),zDimension.getValue()-1);
		if(ccw.getValue())
			{
			const Scalar* hRow=h+yMin*hStride;
			Scalar yp=o[1]+zs*Scalar(yMin);
			for(int y=yMin;y<yMax;++y,hRow+=hStride,yp+=zs)
				{
				const Scalar* hPtr=hRow+xMin;
				Scalar xp=o[0]+xs*Scalar(xMin);
				for(int x=xMin;x<xMax;++x,++hPtr,xp+=xs)
					{
					/* Get the quad's vertices: */
					Point q00(xp,yp,o[2]+hPtr[0]);
					Point q10(xp+xs,yp,o[2]+hPtr[1]);
					Point q01(xp,yp+zs,o[2]+hPtr[hStride]);
					Point q11(xp+xs,yp+zs,o[2]+hPtr[hStride+1]);
					
					/* Test the quad's vertex and edges: */
					collisionQuery.testVertexAndUpdate(q00);
					collisionQuery.testEdgeAndUpdate(q00,q10);
					collisionQuery.testEdgeAndUpdate(q00,q01);
					collisionQuery.testEdgeAndUpdate(q00,q11);
					
					/* Test the quad's two triangles: */
					Vector oq=c0-q00;
					
					Vector normal0=triangleNormal(q11,q00,q01);
					Scalar n0Mag=normal0.mag();
					Scalar denom0=c0c1*normal0;
					if(denom0<Scalar(0))
						{
						Scalar lambda0=(r*n0Mag-oq*normal0)/denom0;
						Point hp0=Geometry::addScaled(c0,c0c1,lambda0);
						hp0.subtractScaled(normal0,r/n0Mag);
						hp0[0]-=xp;
						hp0[1]-=yp;
						if(lambda0<Scalar(0))
							lambda0=Scalar(0);
						if(lambda0<collisionQuery.getHitLambda()&&hp0[0]>=Scalar(0)&&hp0[1]<=zs&&hp0[0]*zs<=hp0[1]*xs)
							collisionQuery.update(lambda0,normal0);
						}
					
					Vector normal1=triangleNormal(q00,q11,q10);
					Scalar n1Mag=normal1.mag();
					Scalar denom1=c0c1*normal1;
					if(denom1<Scalar(0))
						{
						Scalar lambda1=(r*n1Mag-oq*normal1)/denom1;
						Point hp1=Geometry::addScaled(c0,c0c1,lambda1);
						hp1.subtractScaled(normal1,r/n1Mag);
						hp1[0]-=xp;
						hp1[1]-=yp;
						if(lambda1<Scalar(0))
							lambda1=Scalar(0);
						if(lambda1<collisionQuery.getHitLambda()&&hp1[0]<=xs&&hp1[1]>=Scalar(0)&&hp1[0]*zs>=hp1[1]*xs)
							collisionQuery.update(lambda1,normal1);
						}
					}
				
				/* Test the row's final vertex and edge: */
				Point q00(xp,yp,o[2]+hPtr[0]);
				Point q01(xp,yp+zs,o[2]+hPtr[hStride]);
				collisionQuery.testVertexAndUpdate(q00);
				collisionQuery.testEdgeAndUpdate(q00,q01);
				}
			
			/* Test the final row's vertices and edges: */
			const Scalar* hPtr=hRow+xMin;
			Scalar xp=o[0]+xs*Scalar(xMin);
			for(int x=xMin;x<xMax;++x,++hPtr,xp+=xs)
				{
				Point q00(xp,yp,o[2]+hPtr[0]);
				Point q10(xp+xs,yp,o[2]+hPtr[1]);
				collisionQuery.testVertexAndUpdate(q00);
				collisionQuery.testEdgeAndUpdate(q00,q10);
				}
			
			/* Test the final row's final vertex: */
			collisionQuery.testVertexAndUpdate(Point(xp,yp,o[2]+hPtr[0]));
			}
		else
			{
			const Scalar* hRow=h+yMin*hStride;
			Scalar yp=o[1]+zs*Scalar(yMin);
			for(int y=yMin;y<yMax;++y,hRow+=hStride,yp+=zs)
				{
				const Scalar* hPtr=hRow+xMin;
				Scalar xp=o[0]+xs*Scalar(xMin);
				for(int x=xMin;x<xMax;++x,++hPtr,xp+=xs)
					{
					/* Get the quad's vertices: */
					Point q00(xp,yp,o[2]+hPtr[0]);
					Point q10(xp+xs,yp,o[2]+hPtr[1]);
					Point q01(xp,yp+zs,o[2]+hPtr[hStride]);
					Point q11(xp+xs,yp+zs,o[2]+hPtr[hStride+1]);
					
					/* Test the quad's vertex and edges: */
					collisionQuery.testVertexAndUpdate(q00);
					collisionQuery.testEdgeAndUpdate(q00,q10);
					collisionQuery.testEdgeAndUpdate(q00,q01);
					collisionQuery.testEdgeAndUpdate(q10,q01);
					
					/* Test the quad's two triangles: */
					Vector oq=c0-q10;
					
					Vector normal0=triangleNormal(q10,q01,q00);
					Scalar n0Mag=normal0.mag();
					Scalar denom0=c0c1*normal0;
					if(denom0<Scalar(0))
						{
						Scalar lambda0=(r*n0Mag-oq*normal0)/denom0;
						Point hp0=Geometry::addScaled(c0,c0c1,lambda0);
						hp0.subtractScaled(normal0,r/n0Mag);
						hp0[0]-=xp;
						hp0[1]-=yp;
						if(lambda0<Scalar(0))
							lambda0=Scalar(0);
						if(lambda0<collisionQuery.getHitLambda()&&hp0[0]>=Scalar(0)&&hp0[1]>=Scalar(0)&&hp0[0]*zs<=(zs-hp0[1])*xs)
							collisionQuery.update(lambda0,normal0);
						}
					
					Vector normal1=triangleNormal(q01,q10,q11);
					Scalar n1Mag=normal1.mag();
					Scalar denom1=c0c1*normal1;
					if(denom1<Scalar(0))
						{
						Scalar lambda1=(r*n1Mag-oq*normal1)/denom1;
						Point hp1=Geometry::addScaled(c0,c0c1,lambda1);
						hp1.subtractScaled(normal1,r/n1Mag);
						hp1[0]-=xp;
						hp1[1]-=yp;
						if(lambda1<Scalar(0))
							lambda1=Scalar(0);
						if(lambda1<collisionQuery.getHitLambda()&&hp1[0]<=xs&&hp1[1]<=zs&&hp1[0]*zs>=(zs-hp1[1])*xs)
							collisionQuery.update(lambda1,normal1);
						}
					}
				
				/* Test the row's final vertex and edge: */
				Point q00(xp,yp,o[2]+hPtr[0]);
				Point q01(xp,yp+zs,o[2]+hPtr[hStride]);
				collisionQuery.testVertexAndUpdate(q00);
				collisionQuery.testEdgeAndUpdate(q00,q01);
				}
			
			/* Test the final row's vertices and edges: */
			const Scalar* hPtr=hRow+xMin;
			Scalar xp=o[0]+xs*Scalar(xMin);
			for(int x=xMin;x<xMax;++x,++hPtr,xp+=xs)
				{
				Point q00(xp,yp,o[2]+hPtr[0]);
				Point q10(xp+xs,yp,o[2]+hPtr[1]);
				collisionQuery.testVertexAndUpdate(q00);
				collisionQuery.testEdgeAndUpdate(q00,q10);
				}
			
			/* Test the final row's final vertex: */
			collisionQuery.testVertexAndUpdate(Point(xp,yp,o[2]+hPtr[0]));
			}
		}
	}

void ElevationGridNode::glRenderAction(int appearanceRequirementMask,GLRenderState& renderState) const
	{
	/* Bail out if the elevation grid does not support rendering: */
	if(!canRender)
		return;
	
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	renderState.setFrontFace(GL_CCW);
	if(solid.getValue())
		renderState.enableCulling(GL_BACK);
	else
		renderState.disableCulling();
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Check if the buffers are outdated: */
	if(dataItem->version!=version)
		{
		/* Recalculate vertex layout: */
		dataItem->vertexSize=0;
		dataItem->vertexArrayPartsMask=0x0;
		if(numNeedsTexCoords!=0)
			{
			dataItem->texCoordOffset=dataItem->vertexSize;
			dataItem->vertexSize+=2*sizeof(Scalar);
			dataItem->vertexArrayPartsMask|=GLVertexArrayParts::TexCoord;
			}
		if(numNeedsColors!=0||haveColors)
			{
			dataItem->colorOffset=dataItem->vertexSize;
			dataItem->vertexSize+=4*sizeof(GLubyte);
			dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Color;
			}
		if(numNeedsNormals!=0)
			{
			dataItem->normalOffset=dataItem->vertexSize;
			dataItem->vertexSize+=3*sizeof(Scalar);
			dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Normal;
			}
		dataItem->positionOffset=dataItem->vertexSize;
		dataItem->vertexSize+=3*sizeof(Scalar);
		dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Position;
		}
	
	/* Bind the vertex buffer object: */
	renderState.bindVertexBuffer(dataItem->vertexBufferObjectId);
	
	/* Set up the vertex arrays for rendering: */
	int vertexArrayPartsMask=GLVertexArrayParts::Position;
	if((appearanceRequirementMask&GeometryNode::NeedsTexCoords)!=0x0)
		{
		vertexArrayPartsMask|=GLVertexArrayParts::TexCoord;
		glTexCoordPointer(2,GL_FLOAT,dataItem->vertexSize,static_cast<char*>(0)+dataItem->texCoordOffset);
		}
	if(haveColors||(appearanceRequirementMask&GeometryNode::NeedsColors)!=0x0)
		{
		vertexArrayPartsMask|=GLVertexArrayParts::Color;
		glColorPointer(4,GL_UNSIGNED_BYTE,dataItem->vertexSize,static_cast<char*>(0)+dataItem->colorOffset);
		}
	if((appearanceRequirementMask&GeometryNode::NeedsNormals)!=0x0)
		{
		vertexArrayPartsMask|=GLVertexArrayParts::Normal;
		glNormalPointer(GL_FLOAT,dataItem->vertexSize,static_cast<char*>(0)+dataItem->normalOffset);
		}
	glVertexPointer(3,GL_FLOAT,dataItem->vertexSize,static_cast<char*>(0)+dataItem->positionOffset);
	renderState.enableVertexArrays(vertexArrayPartsMask);
	
	if(indexed)
		{
		/* Bind the index buffer object: */
		renderState.bindIndexBuffer(dataItem->indexBufferObjectId);
		
		/* Check if the buffers are current: */
		if(dataItem->version!=version)
			{
			#if BENCHMARKING
			Realtime::TimePointMonotonic timer;
			#endif
			
			/* Upload the set of indexed quad strips: */
			uploadIndexedQuadStripSet(dataItem);
			
			#if BENCHMARKING
			double elapsed(timer.setAndDiff());
			std::cout<<"SceneGraph::ElevationGridNode: Upload time "<<elapsed*1000.0<<" ms"<<std::endl;
			#endif
			
			/* Mark the buffers as up-to-date: */
			dataItem->version=version;
			}
		
		if(dataItem->havePrimitiveRestart)
			{
			/* Draw the elevation grid as a single indexed quad strip with restart: */
			glEnableClientState(GL_PRIMITIVE_RESTART_NV);
			if(size_t(zDimension.getValue())*size_t(xDimension.getValue())<65535U)
				{
				glPrimitiveRestartIndexNV(GLushort(-1));
				glDrawElements(GL_QUAD_STRIP,(zDimension.getValue()-1)*(xDimension.getValue()*2+1)-1,GL_UNSIGNED_SHORT,0);
				glDisableClientState(GL_PRIMITIVE_RESTART_NV);
				}
			else
				{
				glPrimitiveRestartIndexNV(GLuint(-1));
				glDrawElements(GL_QUAD_STRIP,(zDimension.getValue()-1)*(xDimension.getValue()*2+1)-1,GL_UNSIGNED_INT,0);
				glDisableClientState(GL_PRIMITIVE_RESTART_NV);
				}
			}
		else
			{
			/* Draw the elevation grid as a set of indexed quad strips: */
			if(size_t(zDimension.getValue())*size_t(xDimension.getValue())<65536U)
				{
				const GLushort* iPtr=0;
				for(int z=0;z<zDimension.getValue()-1;++z,iPtr+=xDimension.getValue()*2)
					glDrawElements(GL_QUAD_STRIP,xDimension.getValue()*2,GL_UNSIGNED_SHORT,iPtr);
				}
			else
				{
				const GLuint* iPtr=0;
				for(int z=0;z<zDimension.getValue()-1;++z,iPtr+=xDimension.getValue()*2)
					glDrawElements(GL_QUAD_STRIP,xDimension.getValue()*2,GL_UNSIGNED_INT,iPtr);
				}
			}
		}
	else
		{
		/* Check if the buffer is current: */
		if(dataItem->version!=version)
			{
			if(haveInvalids)
				{
				/* Upload the set of quads and triangles: */
				uploadHoleyQuadTriangleSet(dataItem->numQuads,dataItem->numTriangles);
				}
			else
				{
				/* Upload the set of quads: */
				uploadQuadSet();
				
				/* Store the number of uploaded quads: */
				dataItem->numQuads=(xDimension.getValue()-1)*(zDimension.getValue()-1);
				}
			
			/* Mark the buffers as up-to-date: */
			dataItem->version=version;
			}
		
		/* Draw the elevation grid as a set of quads and/or triangles: */
		if(dataItem->numQuads!=0)
			glDrawArrays(GL_QUADS,0,dataItem->numQuads*4);
		if(dataItem->numTriangles!=0)
			glDrawArrays(GL_TRIANGLES,dataItem->numQuads*4,dataItem->numTriangles*3);
		}
	}

void ElevationGridNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}

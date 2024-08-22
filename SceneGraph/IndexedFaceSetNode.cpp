/***********************************************************************
IndexedFaceSetNode - Class for sets of polygonal faces as renderable
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

#include <SceneGraph/IndexedFaceSetNode.h>

#include <string.h>
#include <utility>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/PCACalculator.h>
#include <Geometry/PrimaryPlaneProjector.h>
#include <Geometry/PolygonTriangulator.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <SceneGraph/BaseAppearanceNode.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

// DEBUGGING
#include <iostream>

namespace SceneGraph {

/*********************************************
Methods of class IndexedFaceSetNode::DataItem:
*********************************************/

IndexedFaceSetNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),indexBufferObjectId(0),
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

IndexedFaceSetNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex and index buffer objects: */
	if(vertexBufferObjectId!=0)
		glDeleteBuffersARB(1,&vertexBufferObjectId);
	if(indexBufferObjectId!=0)
		glDeleteBuffersARB(1,&indexBufferObjectId);
	}

/*******************************************
Static elements of class IndexedFaceSetNode:
*******************************************/

const char* IndexedFaceSetNode::className="IndexedFaceSet";

/***********************************
Methods of class IndexedFaceSetNode:
***********************************/

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

bool pointInFace(const Geometry::PrimaryPlaneProjector<Scalar>& ppp,const Geometry::PrimaryPlaneProjector<Scalar>::Point2& p,const MFPoint::ValueList& coords,MFInt::ValueList::const_iterator faceBegin,MFInt::ValueList::const_iterator faceEnd)
	{
	/* Walk around the face and count edge crossings: */
	bool inside=false;
	MFInt::ValueList::const_iterator it0=faceEnd-1;
	Geometry::PrimaryPlaneProjector<Scalar>::Point2 v0=ppp.project(coords[*it0]);
	for(MFInt::ValueList::const_iterator it1=faceBegin;it1!=faceEnd;it0=it1,++it1)
		{
		Geometry::PrimaryPlaneProjector<Scalar>::Point2 v1=ppp.project(coords[*it1]);
		
		/* Check for an edge crossing: */
		bool cross;
		if(v0[1]<=v1[1])
			cross=v0[1]<=p[1]&&p[1]<v1[1];
		else
			cross=v1[1]<=p[1]&&p[1]<v0[1];
		if(cross)
			{
			/* Calculate the edge intersection point: */
			Scalar w=(p[1]-v0[1])/(v1[1]-v0[1]);
			if(v0[0]+(v1[0]-v0[0])*w>=p[0])
				inside=!inside;
			}
		
		/* Go to the next edge: */
		v0=v1;
		}
	
	return inside;
	}

}

void IndexedFaceSetNode::testCollisionSolidCcw(SphereCollisionQuery& collisionQuery) const
	{
	/* Retrieve query parameters: */
	const Point& c0=collisionQuery.getC0();
	const Vector& c0c1=collisionQuery.getC0c1();
	Scalar radius=collisionQuery.getRadius();
	
	/* Get a handle to the face set's vertex coordinates and vertex indices: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Test the sphere against all faces: */
	for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Find the end of the current face's vertex list: */
		MFInt::ValueList::const_iterator faceEnd;
		for(faceEnd=ciIt;faceEnd!=coordIndices.end()&&*faceEnd>=0;++faceEnd)
			;
		
		/* Check if the face has at least three vertices: */
		if(faceEnd-ciIt>=3)
			{
			/* Calculate the plane equation defined by the first three vertices: */
			Point center=coords[*ciIt];
			Vector normal=triangleNormal(center,coords[*(ciIt+1)],coords[*(ciIt+2)]);
			
			/* Test the sphere against the face's plane: */
			Scalar denominator=c0c1*normal;
			Scalar offset=(c0-center)*normal;
			if(denominator<Scalar(0)&&offset>=Scalar(0))
				{
				/* Calculate the intersection of the sphere's path with the face's plane: */
				Scalar normalSqr=normal.sqr();
				Scalar normalMag=Math::sqrt(normalSqr);
				Scalar counter=radius*normalMag-offset;
				Scalar lambda=counter<Scalar(0)?counter/denominator:Scalar(0); // Take care of the case where the sphere is already penetrating the face
				if(lambda<collisionQuery.getHitLambda())
					{
					/* Calculate the point where the sphere hits the face's plane and project it to 2D: */
					Point hit3=c0;
					if(lambda>Scalar(0))
						hit3.addScaled(c0c1,lambda).subtractScaled(normal,radius/normalMag);
					else
						hit3.subtractScaled(normal,offset/normalSqr);
					Geometry::PrimaryPlaneProjector<Scalar> ppp(normal);
					Geometry::PrimaryPlaneProjector<Scalar>::Point2 hit=ppp.project(hit3);
					
					/* Check if the intersection point is inside the face: */
					bool inside=pointInFace(ppp,hit,coords,ciIt,faceEnd);
					if(inside)
						{
						/* This is the actual collision: */
						collisionQuery.update(lambda,normal);
						}
					else
						{
						/* Test the face's vertices and edges: */
						MFInt::ValueList::const_iterator it0=faceEnd-2;
						Geometry::PrimaryPlaneProjector<Scalar>::Point2 v0=ppp.project(coords[*it0]);
						MFInt::ValueList::const_iterator it1=faceEnd-1;
						Geometry::PrimaryPlaneProjector<Scalar>::Point2 v1=ppp.project(coords[*it1]);
						bool testE0=(hit[0]-v0[0])*(v1[1]-v0[1])>(hit[1]-v0[1])*(v1[0]-v0[0]);
						for(MFInt::ValueList::const_iterator it2=ciIt;it2!=faceEnd;it0=it1,it1=it2,++it2)
							{
							Geometry::PrimaryPlaneProjector<Scalar>::Point2 v2=ppp.project(coords[*it2]);
							bool testE1=(hit[0]-v1[0])*(v2[1]-v1[1])>(hit[1]-v1[1])*(v2[0]-v1[0]);
							
							/* Test the edge and the vertex if the hit point is outside of it: */
							if(testE1)
								{
								collisionQuery.testEdgeAndUpdate(coords[*it1],coords[*it2]);
								collisionQuery.testVertexAndUpdate(coords[*it1]);
								}
							else if(testE0)
								collisionQuery.testVertexAndUpdate(coords[*it1]);
							
							v1=v2;
							testE0=testE1;
							}
						}
					}
				}
			}
		
		/* Go to the next face: */
		if(faceEnd!=coordIndices.end())
			++faceEnd;
		ciIt=faceEnd;
		}
	}

void IndexedFaceSetNode::testCollisionSolidCw(SphereCollisionQuery& collisionQuery) const
	{
	/* Retrieve query parameters: */
	const Point& c0=collisionQuery.getC0();
	const Vector& c0c1=collisionQuery.getC0c1();
	Scalar radius=collisionQuery.getRadius();
	
	/* Get a handle to the face set's vertex coordinates and vertex indices: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Test the sphere against all faces: */
	for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Find the end of the current face's vertex list: */
		MFInt::ValueList::const_iterator faceEnd;
		for(faceEnd=ciIt;faceEnd!=coordIndices.end()&&*faceEnd>=0;++faceEnd)
			;
		
		/* Check if the face has at least three vertices: */
		if(faceEnd-ciIt>=3)
			{
			/* Calculate the plane equation defined by the first three vertices: */
			Point center=coords[*ciIt];
			Vector normal=triangleNormal(center,coords[*(ciIt+2)],coords[*(ciIt+1)]);
			
			/* Test the sphere against the face's plane: */
			Scalar denominator=c0c1*normal;
			Scalar offset=(c0-center)*normal;
			if(denominator<Scalar(0)&&offset>=Scalar(0))
				{
				/* Calculate the intersection of the sphere's path with the face's plane: */
				Scalar normalSqr=normal.sqr();
				Scalar normalMag=Math::sqrt(normalSqr);
				Scalar counter=radius*normalMag-offset;
				Scalar lambda=counter<Scalar(0)?counter/denominator:Scalar(0); // Take care of the case where the sphere is already penetrating the face
				if(lambda<collisionQuery.getHitLambda())
					{
					/* Calculate the point where the sphere hits the face's plane and project it to 2D: */
					Point hit3=c0;
					if(lambda>Scalar(0))
						hit3.addScaled(c0c1,lambda).subtractScaled(normal,radius/normalMag);
					else
						hit3.subtractScaled(normal,offset/normalSqr);
					Geometry::PrimaryPlaneProjector<Scalar> ppp(normal);
					Geometry::PrimaryPlaneProjector<Scalar>::Point2 hit=ppp.project(hit3);
					
					/* Check if the intersection point is inside the face: */
					bool inside=pointInFace(ppp,hit,coords,ciIt,faceEnd);
					if(inside)
						{
						/* This is the actual collision: */
						collisionQuery.update(lambda,normal);
						}
					else
						{
						/* Test the face's vertices and edges: */
						MFInt::ValueList::const_iterator it0=faceEnd-2;
						Geometry::PrimaryPlaneProjector<Scalar>::Point2 v0=ppp.project(coords[*it0]);
						MFInt::ValueList::const_iterator it1=faceEnd-1;
						Geometry::PrimaryPlaneProjector<Scalar>::Point2 v1=ppp.project(coords[*it1]);
						bool testE0=(hit[0]-v0[0])*(v1[1]-v0[1])<(hit[1]-v0[1])*(v1[0]-v0[0]);
						for(MFInt::ValueList::const_iterator it2=ciIt;it2!=faceEnd;it0=it1,it1=it2,++it2)
							{
							Geometry::PrimaryPlaneProjector<Scalar>::Point2 v2=ppp.project(coords[*it2]);
							bool testE1=(hit[0]-v1[0])*(v2[1]-v1[1])<(hit[1]-v1[1])*(v2[0]-v1[0]);
							
							/* Test the edge and the vertex if the hit point is outside of it: */
							if(testE1)
								{
								collisionQuery.testEdgeAndUpdate(coords[*it1],coords[*it2]);
								collisionQuery.testVertexAndUpdate(coords[*it1]);
								}
							else if(testE0)
								collisionQuery.testVertexAndUpdate(coords[*it1]);
							
							v1=v2;
							testE0=testE1;
							}
						}
					}
				}
			}
		
		/* Go to the next face: */
		if(faceEnd!=coordIndices.end())
			++faceEnd;
		ciIt=faceEnd;
		}
	}

void IndexedFaceSetNode::testCollisionNonSolid(SphereCollisionQuery& collisionQuery) const
	{
	/* Retrieve query parameters: */
	const Point& c0=collisionQuery.getC0();
	const Vector& c0c1=collisionQuery.getC0c1();
	Scalar radius=collisionQuery.getRadius();
	
	/* Get a handle to the face set's vertex coordinates and vertex indices: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Test the sphere against all faces: */
	for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Find the end of the current face's vertex list: */
		MFInt::ValueList::const_iterator faceEnd;
		for(faceEnd=ciIt;faceEnd!=coordIndices.end()&&*faceEnd>=0;++faceEnd)
			;
		
		/* Check if the face has at least three vertices: */
		if(faceEnd-ciIt>=3)
			{
			/* Calculate the plane equation defined by the first three vertices: */
			Point center=coords[*ciIt];
			Vector normal=triangleNormal(center,coords[*(ciIt+1)],coords[*(ciIt+2)]);
			
			/* Test the sphere against the slab containing the face's plane: */
			Scalar normalSqr=normal.sqr();
			Scalar normalMag=Math::sqrt(normalSqr);
			Scalar offset=(c0-center)*normal;
			Scalar radiusNormal=radius*normalMag;
			if(Math::abs(offset)>radiusNormal) // Sphere's starting point is outside the slab
				{
				Scalar denominator=c0c1*normal;
				Scalar slabOffset=Math::copysign(radiusNormal,offset);
				Scalar lambda=(slabOffset-offset)/denominator;
				if(lambda>=Scalar(0)&&lambda<collisionQuery.getHitLambda())
					{
					/* Calculate the point where the sphere hits the face's plane and project it to 2D: */
					Point hit3=Geometry::addScaled(c0,c0c1,lambda).subtractScaled(normal,Math::copysign(radius,offset)/normalMag);
					Geometry::PrimaryPlaneProjector<Scalar> ppp(normal);
					Geometry::PrimaryPlaneProjector<Scalar>::Point2 hit=ppp.project(hit3);
					
					/* Check if the intersection point is inside the face: */
					bool inside=pointInFace(ppp,hit,coords,ciIt,faceEnd);
					if(inside)
						{
						/* This is the actual collision: */
						collisionQuery.update(lambda,offset>Scalar(0)?normal:-normal);
						}
					else
						{
						/* Test the face's vertices and edges: */
						MFInt::ValueList::const_iterator it0=faceEnd-2;
						Geometry::PrimaryPlaneProjector<Scalar>::Point2 v0=ppp.project(coords[*it0]);
						MFInt::ValueList::const_iterator it1=faceEnd-1;
						Geometry::PrimaryPlaneProjector<Scalar>::Point2 v1=ppp.project(coords[*it1]);
						bool testE0=(hit[0]-v0[0])*(v1[1]-v0[1])>(hit[1]-v0[1])*(v1[0]-v0[0]);
						for(MFInt::ValueList::const_iterator it2=ciIt;it2!=faceEnd;it0=it1,it1=it2,++it2)
							{
							Geometry::PrimaryPlaneProjector<Scalar>::Point2 v2=ppp.project(coords[*it2]);
							bool testE1=(hit[0]-v1[0])*(v2[1]-v1[1])>(hit[1]-v1[1])*(v2[0]-v1[0]);
							
							/* Test the edge and the vertex if the hit point is outside of it: */
							if(testE1)
								{
								collisionQuery.testEdgeAndUpdate(coords[*it1],coords[*it2]);
								collisionQuery.testVertexAndUpdate(coords[*it1]);
								}
							else if(testE0)
								collisionQuery.testVertexAndUpdate(coords[*it1]);
							
							v1=v2;
							testE0=testE1;
							}
						}
					}
				}
			else // Sphere's starting point is inside the slab
				{
				/* Check if the sphere's starting point is inside the face: */
				Point hit3=Geometry::subtractScaled(c0,normal,offset/normalSqr);
				Geometry::PrimaryPlaneProjector<Scalar> ppp(normal);
				Geometry::PrimaryPlaneProjector<Scalar>::Point2 hit=ppp.project(hit3);
				bool inside=pointInFace(ppp,hit,coords,ciIt,faceEnd);
				if(inside)
					{
					/* Check if the sphere is attempting to penetrate deeper into the face: */
					if(collisionQuery.getHitLambda()>Scalar(0)&&(c0c1*normal)*offset<Scalar(0))
						{
						/* Prevent further movement: */
						collisionQuery.update(Scalar(0),offset>Scalar(0)?normal:-normal);
						}
					}
				else
					{
					/* Test the face's vertices and edges: */
					MFInt::ValueList::const_iterator it0=faceEnd-1;
					for(MFInt::ValueList::const_iterator it1=ciIt;it1!=faceEnd;it0=it1,++it1)
						{
						collisionQuery.testVertexAndUpdate(coords[*it1]);
						collisionQuery.testEdgeAndUpdate(coords[*it0],coords[*it1]);
						}
					}
				}
			}
		
		/* Go to the next face: */
		if(faceEnd!=coordIndices.end())
			++faceEnd;
		ciIt=faceEnd;
		}
	}

namespace {

/**************
Helper classes:
**************/

struct Face // Structure describing valid faces
	{
	/* Elements: */
	public:
	size_t firstVertex; // Index of the face's first vertex in the coordIndex array
	size_t numVertices; // Number of vertices
	};

struct NCFace // Structure describing valid faces in the non-convex case
	{
	/* Elements: */
	public:
	size_t firstVertex; // Index of the face's first vertex in the coordIndex array
	size_t numVertices; // Number of vertices
	Vector faceNormal; // Face's normal vector
	size_t firstTriangleVertex; // Index of the face's first triangle's first vertex in the triangulation list
	};

struct VertexFaces // Structure to associate faces sharing a vertex with that vertex
	{
	/* Elements: */
	public:
	size_t begin; // Index of first vertex face entry
	size_t end; // Index of first vertex face entry of next vertex
	};

struct FaceCorner // Structure storing a corner of a face
	{
	/* Elements: */
	size_t faceIndex; // Index of the face to which this corner belongs
	Scalar cornerAngle; // Interior angle of the corner in radians
	};

}

void IndexedFaceSetNode::uploadConvexFaceSet(DataItem* dataItem,GLubyte* bufferPtr) const
	{
	/* Access the face set's vertex coordinates and face vertex indices: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Create a list of valid faces: */
	std::vector<Face> faces;
	faces.reserve(numValidFaces);
	size_t firstVertex=0;
	for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Find the end of the current face: */
		MFInt::ValueList::const_iterator feIt;
		for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
			;
		
		/* Store the face if it has at least three vertices: */
		Face f;
		f.firstVertex=firstVertex;
		f.numVertices=size_t(feIt-ciIt);
		if(f.numVertices>=3)
			faces.push_back(f);
		
		/* Go to the next face: */
		ciIt=feIt;
		if(ciIt!=coordIndices.end())
			++ciIt;
		firstVertex+=f.numVertices+1;
		}
	
	/* Check if texture coordinates are needed: */
	if(numNeedsTexCoords!=0)
		{
		/* Access the interleaved buffer's texture coordinates: */
		GLubyte* tcPtr=bufferPtr+dataItem->texCoordOffset;
		
		/* Check if the face set has texture coordinates: */
		if(texCoord.getValue()!=0)
			{
			/* Access the face set's texture coordinates and texture coordinate indices: */
			const MFTexCoord::ValueList& texCoords=texCoord.getValue()->point.getValues();
			const MFInt::ValueList& texCoordIndices=texCoordIndex.getValues();
			
			/* Use texture coordinate indices to upload if they exist; otherwise, use coordinate indices: */
			const MFInt::ValueList tcis=texCoordIndices.empty()?coordIndices:texCoordIndices;
			
			/* Upload per-vertex texture coordinates using the selected indices: */
			for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Trivially triangulate the face: */
				int vis[3];
				vis[0]=tcis[fIt->firstVertex];
				vis[2]=tcis[fIt->firstVertex+1];
				for(size_t t=2;t<fIt->numVertices;++t)
					{
					vis[1]=vis[2];
					vis[2]=tcis[fIt->firstVertex+t];
					for(int i=0;i<3;++i,tcPtr+=dataItem->vertexSize)
						*reinterpret_cast<TexCoord*>(tcPtr)=texCoords[vis[i]];
					}
				}
			}
		else
			{
			/* Calculate the face set's untransformed bounding box: */
			Box ubbox;
			if(pointTransform.getValue()==0)
				ubbox=bbox;
			else
				{
				/* Add the face set's untransformed vertices to the bounding box: */
				ubbox=Box::empty;
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++ciIt)
					if(*ciIt>=0)
						ubbox.addPoint(coords[*ciIt]);
				}
			
			/* Calculate texture coordinates by mapping the largest face of the face set's untransformed bounding box to the [0, 1]^2 interval: */
			int sDim=0;
			Scalar sSize=ubbox.getSize(sDim);
			for(int i=1;i<3;++i)
				if(sSize<ubbox.getSize(i))
					{
					sDim=i;
					sSize=ubbox.getSize(i);
					}
			int tDim=sDim==0?1:0;
			Scalar tSize=ubbox.getSize(tDim);
			for(int i=1;i<3;++i)
				if(i!=sDim&&tSize<ubbox.getSize(i))
					{
					tDim=i;
					tSize=ubbox.getSize(i);
					}
			
			/* Calculate texture coordinates for all vertices: */
			for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Trivially triangulate the face: */
				TexCoord tcs[3];
				tcs[0][0]=(coords[coordIndices[fIt->firstVertex]][sDim]-bbox.min[sDim])/sSize;
				tcs[0][1]=(coords[coordIndices[fIt->firstVertex]][tDim]-bbox.min[tDim])/tSize;
				tcs[2][0]=(coords[coordIndices[fIt->firstVertex+1]][sDim]-bbox.min[sDim])/sSize;
				tcs[2][1]=(coords[coordIndices[fIt->firstVertex+1]][tDim]-bbox.min[tDim])/tSize;
				for(size_t t=2;t<fIt->numVertices;++t)
					{
					tcs[1]=tcs[2];
					tcs[2][0]=(coords[coordIndices[fIt->firstVertex+t]][sDim]-bbox.min[sDim])/sSize;
					tcs[2][1]=(coords[coordIndices[fIt->firstVertex+t]][tDim]-bbox.min[tDim])/tSize;
					for(int i=0;i<3;++i,tcPtr+=dataItem->vertexSize)
						*reinterpret_cast<TexCoord*>(tcPtr)=tcs[i];
					}
				}
			}
		}
	
	/* Check if the face set defines per-vertex or per-face colors: */
	if(haveColors)
		{
		typedef GLColor<GLubyte,4> BColor; // Type for colors uploaded to vertex buffers
		
		/* Access the interleaved buffer's colors: */
		GLubyte* cPtr=bufferPtr+dataItem->colorOffset;
		
		/* Access the face set's colors and color indices: */
		const MFColor::ValueList& colors=color.getValue()->color.getValues();
		const MFInt::ValueList& colorIndices=colorIndex.getValues();
		
		/* Check if colors are per-vertex or per-face: */
		if(colorPerVertex.getValue())
			{
			/* Use color indices to upload if they exist; otherwise, use coordinate indices: */
			const MFInt::ValueList cis=colorIndices.empty()?coordIndices:colorIndices;
			
			/* Upload per-vertex colors using the selected indices: */
			for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Trivially triangulate the face: */
				BColor cs[3];
				cs[0]=BColor(colors[cis[fIt->firstVertex]]);
				cs[2]=BColor(colors[cis[fIt->firstVertex+1]]);
				for(size_t t=2;t<fIt->numVertices;++t)
					{
					cs[1]=cs[2];
					cs[2]=BColor(colors[cis[fIt->firstVertex+t]]);
					for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
						*reinterpret_cast<BColor*>(cPtr)=cs[i];
					}
				}
			}
		else
			{
			/* Check if there are color indices: */
			if(colorIndices.empty())
				{
				/* Upload per-face colors in the order they are provided: */
				MFColor::ValueList::const_iterator cIt=colors.begin();
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++cIt)
					{
					/* Find the end of the current face: */
					MFInt::ValueList::const_iterator feIt;
					for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
						;
					
					/* Check if the face has at least three vertices: */
					size_t numVertices(feIt-ciIt);
					if(numVertices>=3)
						{
						/* Upload the face color into all vertices generated by the face: */
						BColor faceColor(*cIt);
						for(size_t i=(numVertices-2)*3;i>0;--i,cPtr+=dataItem->vertexSize)
							*reinterpret_cast<BColor*>(cPtr)=faceColor;
						}
					
					/* Go to the next face: */
					ciIt=feIt;
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			else
				{
				/* Upload per-face colors using the provided color indices: */
				MFInt::ValueList::const_iterator coliIt=colorIndices.begin();
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++coliIt)
					{
					/* Find the end of the current face: */
					MFInt::ValueList::const_iterator feIt;
					for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
						;
					
					/* Check if the face has at least three vertices: */
					size_t numVertices(feIt-ciIt);
					if(numVertices>=3)
						{
						/* Upload the face color into all vertices generated by the face: */
						BColor faceColor(colors[*coliIt]);
						for(size_t i=(numVertices-2)*3;i>0;--i,cPtr+=dataItem->vertexSize)
							*reinterpret_cast<BColor*>(cPtr)=faceColor;
						}
					
					/* Go to the next face: */
					ciIt=feIt;
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			}
		}
	
	/* Check if normal vectors are needed: */
	if(numNeedsNormals!=0)
		{
		/* Access the interleaved buffer's normal vectors: */
		GLubyte* nPtr=bufferPtr+dataItem->normalOffset;
		
		/* Check if the face set has normal vectors: */
		if(normal.getValue()!=0)
			{
			/* Access the face set's normals and normal indices: */
			const MFVector::ValueList& normals=normal.getValue()->vector.getValues();
			const MFInt::ValueList& normalIndices=normalIndex.getValues();
			
			/* Check if normals are per-vertex or per-face: */
			if(normalPerVertex.getValue())
				{
				/* Use normal indices to upload if they exist; otherwise, use coordinate indices: */
				const MFInt::ValueList nis=normalIndices.empty()?coordIndices:normalIndices;
				
				/* Upload per-vertex normal vectors using the selected indices: */
				for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
					{
					/* Trivially triangulate the face: */
					int vis[3];
					vis[0]=nis[fIt->firstVertex];
					vis[2]=nis[fIt->firstVertex+1];
					for(size_t t=2;t<fIt->numVertices;++t)
						{
						vis[1]=vis[2];
						vis[2]=nis[fIt->firstVertex+t];
						for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
							*reinterpret_cast<Vector*>(nPtr)=normals[vis[i]];
						}
					}
				}
			else
				{
				/* Check if there are normal indices: */
				if(normalIndex.getValues().empty())
					{
					/* Upload per-face normal vectors in the order they are provided: */
					MFVector::ValueList::const_iterator nIt=normals.begin();
					for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++nIt)
						{
						/* Find the end of the current face: */
						MFInt::ValueList::const_iterator feIt;
						for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
							;
						
						/* Check if the face has at least three vertices: */
						size_t numVertices(feIt-ciIt);
						if(numVertices>=3)
							{
							/* Upload the face normal into all vertices generated by the face: */
							for(size_t i=(numVertices-2)*3;i>0;--i,nPtr+=dataItem->vertexSize)
								*reinterpret_cast<Vector*>(nPtr)=*nIt;
							}
						
						/* Go to the next face: */
						ciIt=feIt;
						if(ciIt!=coordIndices.end())
							++ciIt;
						}
					}
				else
					{
					/* Upload per-face normal vectors using the provided normal vector indices: */
					MFInt::ValueList::const_iterator niIt=normalIndices.begin();
					for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++niIt)
						{
						/* Find the end of the current face: */
						MFInt::ValueList::const_iterator feIt;
						for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
							;
						
						/* Check if the face has at least three vertices: */
						size_t numVertices(feIt-ciIt);
						if(numVertices>=3)
							{
							/* Upload the face normal into all vertices generated by the face: */
							for(size_t i=(numVertices-2)*3;i>0;--i,nPtr+=dataItem->vertexSize)
								*reinterpret_cast<Vector*>(nPtr)=normals[*niIt];
							}
						
						/* Go to the next face: */
						ciIt=feIt;
						if(ciIt!=coordIndices.end())
							++ciIt;
						}
					}
				}
			}
		else if(normalPerVertex.getValue()&&creaseAngle.getValue()>Scalar(0))
			{
			/* Calculate per-face normal vectors and count the number of faces sharing each vertex: */
			std::vector<Vector> faceNormals;
			faceNormals.reserve(numValidFaces);
			std::vector<VertexFaces> vertexFaces;
			vertexFaces.reserve(vertexIndexMax-vertexIndexMin+1);
			for(int vi=vertexIndexMin;vi<=vertexIndexMax;++vi)
				{
				VertexFaces vf;
				vf.end=vf.begin=0;
				vertexFaces.push_back(vf);
				}
			for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Calculate the normal vector defined by the first three vertices: */
				const Point& v0=coords[coordIndices[fIt->firstVertex]];
				const Point& v1=coords[coordIndices[fIt->firstVertex+1]];
				const Point& v2=coords[coordIndices[fIt->firstVertex+2]];
				Vector faceNormal=(v1-v0)^(v2-v1);
				faceNormals.push_back(faceNormal.normalize());
				
				/* Count the face for each of the face's vertices: */
				MFInt::ValueList::const_iterator ciIt=coordIndices.begin()+fIt->firstVertex;
				for(size_t i=0;i<fIt->numVertices;++i,++ciIt)
					++vertexFaces[*ciIt-vertexIndexMin].end;
				}
			
			/* Calculate the face corner array's layout: */
			size_t numFaceCorners=0;
			for(std::vector<VertexFaces>::iterator vfIt=vertexFaces.begin();vfIt!=vertexFaces.end();++vfIt)
				{
				vfIt->begin=numFaceCorners;
				numFaceCorners+=vfIt->end;
				vfIt->end=vfIt->begin;
				}
			
			/* Calculate the face corner array: */
			std::vector<FaceCorner> faceCorners;
			faceCorners.reserve(numFaceCorners);
			for(size_t i=0;i<numFaceCorners;++i)
				faceCorners.push_back(FaceCorner());
			size_t faceIndex=0;
			for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt,++faceIndex)
				{
				/* Go around the face and calculate corner interior angles: */
				MFInt::ValueList::const_iterator fvIt=coordIndices.begin()+fIt->firstVertex;
				MFInt::ValueList::const_iterator fvEnd=fvIt+fIt->numVertices;
				int v0=fvEnd[-1];
				Vector e0=coords[v0]-coords[fvEnd[-2]];
				Scalar e0Sqr=e0.sqr();
				for(;fvIt!=fvEnd;++fvIt)
					{
					/* Calculate the corner's interior angle: */
					Vector e1=coords[*fvIt]-coords[v0];
					Scalar e1Sqr=e1.sqr();
					Scalar angle=Math::acos(Math::clamp(-(e0*e1)/Math::sqrt(e0Sqr*e1Sqr),Scalar(-1),Scalar(1)));
					
					/* Store the face corner with the corner's central vertex: */
					VertexFaces& vf=vertexFaces[v0-vertexIndexMin];
					faceCorners[vf.end].faceIndex=faceIndex;
					faceCorners[vf.end].cornerAngle=angle;
					++vf.end;
					
					/* Go to the next corner: */
					v0=*fvIt;
					e0=e1;
					e0Sqr=e1Sqr;
					}
				}
			
			/* Accumulate and upload vertex normal vectors: */
			Scalar creaseAngleCos=Math::cos(creaseAngle.getValue());
			std::vector<Vector> faceVertexNormals;
			faceVertexNormals.reserve(maxNumFaceVertices);
			faceIndex=0;
			for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt,++faceIndex)
				{
				/* Accumulate normal vectors for the face's vertices: */
				faceVertexNormals.clear();
				MFInt::ValueList::const_iterator fvIt=coordIndices.begin()+fIt->firstVertex;
				MFInt::ValueList::const_iterator fvEnd=fvIt+fIt->numVertices;
				for(;fvIt!=fvEnd;++fvIt)
					{
					VertexFaces& vf=vertexFaces[*fvIt-vertexIndexMin];
					Vector normal=Vector::zero;
					for(size_t fci=vf.begin;fci!=vf.end;++fci)
						{
						/* Check if the angle between the current face and the current shared face is less than the crease angle: */
						FaceCorner& fc=faceCorners[fci];
						if(faceNormals[faceIndex]*faceNormals[fc.faceIndex]>=creaseAngleCos)
							normal.addScaled(faceNormals[fc.faceIndex],fc.cornerAngle);
						}
					faceVertexNormals.push_back(normal.normalize());
					}
				
				/* Trivially triangulate the face: */
				for(size_t vi=2;vi<fIt->numVertices;++vi)
					{
					*reinterpret_cast<Vector*>(nPtr)=faceVertexNormals[0];
					nPtr+=dataItem->vertexSize;
					*reinterpret_cast<Vector*>(nPtr)=faceVertexNormals[vi-1];
					nPtr+=dataItem->vertexSize;
					*reinterpret_cast<Vector*>(nPtr)=faceVertexNormals[vi];
					nPtr+=dataItem->vertexSize;
					}
				}
			}
		else
			{
			/* Calculate and upload per-face normal vectors: */
			for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Calculate the normal vector defined by the first three vertices: */
				const Point& v0=coords[coordIndices[fIt->firstVertex]];
				const Point& v1=coords[coordIndices[fIt->firstVertex+1]];
				const Point& v2=coords[coordIndices[fIt->firstVertex+2]];
				Vector faceNormal=(v1-v0)^(v2-v1);
				faceNormal.normalize();
				
				/* Upload the face normal into all vertices generated by the face: */
				for(size_t i=(fIt->numVertices-2)*3;i>0;--i,nPtr+=dataItem->vertexSize)
					*reinterpret_cast<Vector*>(nPtr)=faceNormal;
				}
			}
		}
	
	/* Access the interleaved buffer's vertex positions: */
	GLubyte* cPtr=bufferPtr+dataItem->positionOffset;
	
	/* Check if there is a point transformation: */
	if(pointTransform.getValue()!=0)
		{
		/* Upload transformed vertex positions: */
		for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
			{
			/* Trivially triangulate the face: */
			Point cs[3];
			cs[0]=Point(pointTransform.getValue()->transformPoint(PointTransformNode::TPoint(coords[coordIndices[fIt->firstVertex]])));
			cs[2]=Point(pointTransform.getValue()->transformPoint(PointTransformNode::TPoint(coords[coordIndices[fIt->firstVertex+1]])));
			for(size_t t=2;t<fIt->numVertices;++t)
				{
				cs[1]=cs[2];
				cs[2]=Point(pointTransform.getValue()->transformPoint(PointTransformNode::TPoint(coords[coordIndices[fIt->firstVertex+t]])));
				for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
					*reinterpret_cast<Point*>(cPtr)=cs[i];
				}
			}
		}
	else
		{
		/* Upload untransformed vertex positions: */
		for(std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
			{
			/* Trivially triangulate the face: */
			int vis[3];
			vis[0]=coordIndices[fIt->firstVertex];
			vis[2]=coordIndices[fIt->firstVertex+1];
			for(size_t t=2;t<fIt->numVertices;++t)
				{
				vis[1]=vis[2];
				vis[2]=coordIndices[fIt->firstVertex+t];
				for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
					*reinterpret_cast<Point*>(cPtr)=coords[vis[i]];
				}
			}
		}
	}

void IndexedFaceSetNode::uploadNonConvexFaceSet(DataItem* dataItem,GLubyte* bufferPtr) const
	{
	typedef Geometry::PolygonTriangulator<Scalar>::IndexList IndexList;
	
	/* Access the face set's vertex coordinates and face vertex indices: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Create a list of valid faces, calculate their normal vectors and projection axes, and triangulate them: */
	std::vector<NCFace> faces;
	faces.reserve(numValidFaces);
	IndexList triangleVertexIndices;
	triangleVertexIndices.reserve(totalNumTriangles*3);
	size_t firstVertex=0;
	size_t firstTriangleVertex=0;
	for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Find the end of the current face: */
		MFInt::ValueList::const_iterator feIt;
		for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
			;
		
		/* Store the face if it has at least three vertices: */
		size_t numVertices(feIt-ciIt);
		if(numVertices>=3)
			{
			NCFace f;
			f.firstVertex=firstVertex;
			f.numVertices=numVertices;
			
			/* Calculate the face's normal vector as the smallest eigenvector of its vertices' covariance matrix: */
			Geometry::PCACalculator<3> pca;
			for(MFInt::ValueList::const_iterator fIt=ciIt;fIt!=feIt;++fIt)
				pca.accumulatePoint(coords[*fIt]);
			pca.calcCovariance();
			double evs[3];
			pca.calcEigenvalues(evs);
			f.faceNormal=Vector(pca.calcEigenvector(evs[2]));
			f.faceNormal.normalize();
			
			/* Find the primary plane best aligned with the polygon's plane: */
			int pAxis=Geometry::findParallelAxis(f.faceNormal);
			int a0,a1;
			if(f.faceNormal[pAxis]>=Scalar(0))
				{
				a0=(pAxis+1)%3;
				a1=(pAxis+2)%3;
				}
			else
				{
				a0=(pAxis+2)%3;
				a1=(pAxis+1)%3;
				}
			
			/* Calculate the face's total winding angle to check for correct orientation: */
			Scalar windingAngle(0);
			int v0=ciIt[f.numVertices-1];
			Geometry::Vector<Scalar,2> e0(coords[v0][a0]-coords[ciIt[f.numVertices-2]][a0],coords[v0][a1]-coords[ciIt[f.numVertices-2]][a1]);
			Scalar e0Sqr=e0.sqr();
			for(MFInt::ValueList::const_iterator fIt=ciIt;fIt!=feIt;++fIt)
				{
				/* Calculate the exterior corner angle: */
				Geometry::Vector<Scalar,2> e1(coords[*fIt][a0]-coords[v0][a0],coords[*fIt][a1]-coords[v0][a1]);
				Scalar e1Sqr=e1.sqr();
				Scalar alpha=Math::acos(Math::clamp((e0*e1)/Math::sqrt(e0Sqr*e1Sqr),Scalar(-1),Scalar(1)));
				if(e0[0]*e1[1]<e0[1]*e1[0])
					alpha=-alpha;
				windingAngle+=alpha;
				
				/* Go to the next corner: */
				v0=*fIt;
				e0=e1;
				e0Sqr=e1Sqr;
				}
			
			/* Flip the face's orientation if the winding angle is inconsistent with the prescribed winding order: */
			if(ccw.getValue())
				windingAngle=-windingAngle;
			if(windingAngle>Scalar(0))
				{
				f.faceNormal=-f.faceNormal;
				std::swap(a0,a1);
				}
			
			/* Triangulate the face: */
			f.firstTriangleVertex=firstTriangleVertex;
			if(f.numVertices>3)
				{
				try
					{
					Geometry::PrimaryPlaneProjector<Scalar> pp(a0,a1);
					Geometry::PolygonTriangulator<Scalar> triangulator;
					unsigned int i0(f.numVertices-1);
					for(unsigned int i1=0;i1<f.numVertices;i0=i1,++i1)
						triangulator.addEdge(pp.project(coords[ciIt[i0]]),i0,pp.project(coords[ciIt[i1]]),i1);
					triangulator.triangulate(triangleVertexIndices);
					}
				catch(Geometry::PolygonTriangulator<Scalar>::Error err)
					{
					// DEBUGGING
					std::cout<<"Can't triangulate face with "<<f.numVertices<<" vertices"<<std::endl;
					std::cout<<"set object 1 polygon from ";
					for(size_t i=0;i<f.numVertices;++i)
						std::cout<<coords[ciIt[i]][a0]<<','<<coords[ciIt[i]][a1]<<" to ";
					std::cout<<coords[ciIt[0]][a0]<<','<<coords[ciIt[0]][a1]<<std::endl;
					Geometry::Box<Scalar,2> fbox=Geometry::Box<Scalar,2>::empty;
					for(size_t i=0;i<f.numVertices;++i)
					 fbox.addPoint(Geometry::Point<Scalar,2>(coords[ciIt[i]][a0],coords[ciIt[i]][a1]));
					std::cout<<"set xrange ["<<fbox.min[0]<<':'<<fbox.max[0]<<']'<<std::endl;
					std::cout<<"set yrange ["<<fbox.min[1]<<':'<<fbox.max[1]<<']'<<std::endl;
					
					/* Remove a partial triangulation: */
					while(triangleVertexIndices.size()>firstTriangleVertex)
						triangleVertexIndices.pop_back();
					
					/* Triangulate the face trivially: */
					for(unsigned int i=2;i<f.numVertices;++i)
						{
						triangleVertexIndices.push_back(0);
						triangleVertexIndices.push_back(i-1);
						triangleVertexIndices.push_back(i);
						}
					}
				}
			else
				{
				/* Triangulate the triangle trivially: */
				for(unsigned int i=0;i<3;++i)
					triangleVertexIndices.push_back(i);
				}
			
			/* Store the face: */
			firstTriangleVertex+=(f.numVertices-2)*3;
			faces.push_back(f);
			}
		
		/* Go to the next face: */
		ciIt=feIt;
		if(ciIt!=coordIndices.end())
			++ciIt;
		firstVertex+=numVertices+1;
		}
	
	/* Check if texture coordinates are needed: */
	if(numNeedsTexCoords!=0)
		{
		/* Access the interleaved buffer's texture coordinates: */
		GLubyte* tcPtr=bufferPtr+dataItem->texCoordOffset;
		
		/* Check if the face set has texture coordinates: */
		if(texCoord.getValue()!=0)
			{
			/* Access the face set's texture coordinates and texture coordinate indices: */
			const MFTexCoord::ValueList& texCoords=texCoord.getValue()->point.getValues();
			const MFInt::ValueList& texCoordIndices=texCoordIndex.getValues();
			
			/* Use texture coordinate indices to upload if they exist; otherwise, use coordinate indices: */
			const MFInt::ValueList tcis=texCoordIndices.empty()?coordIndices:texCoordIndices;
			
			/* Upload per-vertex texture coordinates using the selected indices: */
			for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Use the computed triangulation: */
				MFInt::ValueList::const_iterator ciIt=tcis.begin()+fIt->firstVertex;
				IndexList::iterator tvIt=triangleVertexIndices.begin()+fIt->firstTriangleVertex;
				for(size_t i=(fIt->numVertices-2)*3;i>0;--i,++tvIt,tcPtr+=dataItem->vertexSize)
					*reinterpret_cast<TexCoord*>(tcPtr)=texCoords[ciIt[*tvIt]];
				}
			}
		else
			{
			/* Calculate the face set's untransformed bounding box: */
			Box ubbox;
			if(pointTransform.getValue()==0)
				ubbox=bbox;
			else
				{
				/* Add the face set's untransformed vertices to the bounding box: */
				ubbox=Box::empty;
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++ciIt)
					if(*ciIt>=0)
						ubbox.addPoint(coords[*ciIt]);
				}
			
			/* Calculate texture coordinates by mapping the largest face of the face set's untransformed bounding box to the [0, 1]^2 interval: */
			int sDim=0;
			Scalar sSize=ubbox.getSize(sDim);
			for(int i=1;i<3;++i)
				if(sSize<ubbox.getSize(i))
					{
					sDim=i;
					sSize=ubbox.getSize(i);
					}
			int tDim=sDim==0?1:0;
			Scalar tSize=ubbox.getSize(tDim);
			for(int i=1;i<3;++i)
				if(i!=sDim&&tSize<ubbox.getSize(i))
					{
					tDim=i;
					tSize=ubbox.getSize(i);
					}
			
			/* Calculate texture coordinates for all vertices: */
			for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Use the computed triangulation: */
				MFInt::ValueList::const_iterator ciIt=coordIndices.begin()+fIt->firstVertex;
				IndexList::iterator tvIt=triangleVertexIndices.begin()+fIt->firstTriangleVertex;
				for(size_t i=(fIt->numVertices-2)*3;i>0;--i,++tvIt,tcPtr+=dataItem->vertexSize)
					{
					TexCoord tc;
					tc[0]=(coords[ciIt[*tvIt]][sDim]-bbox.min[sDim])/sSize;
					tc[1]=(coords[ciIt[*tvIt]][tDim]-bbox.min[tDim])/tSize;
					*reinterpret_cast<TexCoord*>(tcPtr)=tc;
					}
				}
			}
		}
	
	/* Check if the face set defines per-vertex or per-face colors: */
	if(haveColors)
		{
		typedef GLColor<GLubyte,4> BColor; // Type for colors uploaded to vertex buffers
		
		/* Access the interleaved buffer's colors: */
		GLubyte* cPtr=bufferPtr+dataItem->colorOffset;
		
		/* Access the face set's colors and color indices: */
		const MFColor::ValueList& colors=color.getValue()->color.getValues();
		const MFInt::ValueList& colorIndices=colorIndex.getValues();
		
		/* Check if colors are per-vertex or per-face: */
		if(colorPerVertex.getValue())
			{
			/* Use color indices to upload if they exist; otherwise, use coordinate indices: */
			const MFInt::ValueList cis=colorIndices.empty()?coordIndices:colorIndices;
			
			/* Upload per-vertex colors using the selected indices: */
			for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Use the computed triangulation: */
				MFInt::ValueList::const_iterator ciIt=cis.begin()+fIt->firstVertex;
				IndexList::iterator tvIt=triangleVertexIndices.begin()+fIt->firstTriangleVertex;
				for(size_t i=(fIt->numVertices-2)*3;i>0;--i,++tvIt,cPtr+=dataItem->vertexSize)
					*reinterpret_cast<BColor*>(cPtr)=BColor(colors[ciIt[*tvIt]]);
				}
			}
		else
			{
			/* Check if there are color indices: */
			if(colorIndices.empty())
				{
				/* Upload per-face colors in the order they are provided: */
				MFColor::ValueList::const_iterator cIt=colors.begin();
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++cIt)
					{
					/* Find the end of the current face: */
					MFInt::ValueList::const_iterator feIt;
					for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
						;
					
					/* Check if the face has at least three vertices: */
					size_t numVertices(feIt-ciIt);
					if(numVertices>=3)
						{
						/* Upload the face color into all vertices generated by the face: */
						BColor faceColor(*cIt);
						for(size_t i=(numVertices-2)*3;i>0;--i,cPtr+=dataItem->vertexSize)
							*reinterpret_cast<BColor*>(cPtr)=faceColor;
						}
					
					/* Go to the next face: */
					ciIt=feIt;
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			else
				{
				/* Upload per-face colors using the provided color indices: */
				MFInt::ValueList::const_iterator coliIt=colorIndices.begin();
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++coliIt)
					{
					/* Find the end of the current face: */
					MFInt::ValueList::const_iterator feIt;
					for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
						;
					
					/* Check if the face has at least three vertices: */
					size_t numVertices(feIt-ciIt);
					if(numVertices>=3)
						{
						/* Upload the face color into all vertices generated by the face: */
						BColor faceColor(colors[*coliIt]);
						for(size_t i=(numVertices-2)*3;i>0;--i,cPtr+=dataItem->vertexSize)
							*reinterpret_cast<BColor*>(cPtr)=faceColor;
						}
					
					/* Go to the next face: */
					ciIt=feIt;
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			}
		}
	
	/* Check if normal vectors are needed: */
	if(numNeedsNormals!=0)
		{
		/* Access the interleaved buffer's normal vectors: */
		GLubyte* nPtr=bufferPtr+dataItem->normalOffset;
		
		/* Check if the face set has normal vectors: */
		if(normal.getValue()!=0)
			{
			/* Access the face set's normals and normal indices: */
			const MFVector::ValueList& normals=normal.getValue()->vector.getValues();
			const MFInt::ValueList& normalIndices=normalIndex.getValues();
			
			/* Check if normals are per-vertex or per-face: */
			if(normalPerVertex.getValue())
				{
				/* Use normal indices to upload if they exist; otherwise, use coordinate indices: */
				const MFInt::ValueList nis=normalIndices.empty()?coordIndices:normalIndices;
				
				/* Upload per-vertex normal vectors using the selected indices: */
				for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
					{
					/* Use the computed triangulation: */
					MFInt::ValueList::const_iterator ciIt=nis.begin()+fIt->firstVertex;
					IndexList::iterator tvIt=triangleVertexIndices.begin()+fIt->firstTriangleVertex;
					for(size_t i=(fIt->numVertices-2)*3;i>0;--i,++tvIt,nPtr+=dataItem->vertexSize)
						*reinterpret_cast<Vector*>(nPtr)=normals[ciIt[*tvIt]];
					}
				}
			else
				{
				/* Check if there are normal indices: */
				if(normalIndex.getValues().empty())
					{
					/* Upload per-face normal vectors in the order they are provided: */
					MFVector::ValueList::const_iterator nIt=normals.begin();
					for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++nIt)
						{
						/* Find the end of the current face: */
						MFInt::ValueList::const_iterator feIt;
						for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
							;
						
						/* Check if the face has at least three vertices: */
						size_t numVertices(feIt-ciIt);
						if(numVertices>=3)
							{
							/* Upload the face normal into all vertices generated by the face: */
							for(size_t i=(numVertices-2)*3;i>0;--i,nPtr+=dataItem->vertexSize)
								*reinterpret_cast<Vector*>(nPtr)=*nIt;
							}
						
						/* Go to the next face: */
						ciIt=feIt;
						if(ciIt!=coordIndices.end())
							++ciIt;
						}
					}
				else
					{
					/* Upload per-face normal vectors using the provided normal vector indices: */
					MFInt::ValueList::const_iterator niIt=normalIndices.begin();
					for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++niIt)
						{
						/* Find the end of the current face: */
						MFInt::ValueList::const_iterator feIt;
						for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
							;
						
						/* Check if the face has at least three vertices: */
						size_t numVertices(feIt-ciIt);
						if(numVertices>=3)
							{
							/* Upload the face normal into all vertices generated by the face: */
							for(size_t i=(numVertices-2)*3;i>0;--i,nPtr+=dataItem->vertexSize)
								*reinterpret_cast<Vector*>(nPtr)=normals[*niIt];
							}
						
						/* Go to the next face: */
						ciIt=feIt;
						if(ciIt!=coordIndices.end())
							++ciIt;
						}
					}
				}
			}
		else if(normalPerVertex.getValue()&&creaseAngle.getValue()>Scalar(0))
			{
			/* Calculate per-face normal vectors and count the number of faces sharing each vertex: */
			std::vector<Vector> faceNormals;
			faceNormals.reserve(numValidFaces);
			std::vector<VertexFaces> vertexFaces;
			vertexFaces.reserve(vertexIndexMax-vertexIndexMin+1);
			for(int vi=vertexIndexMin;vi<=vertexIndexMax;++vi)
				{
				VertexFaces vf;
				vf.end=vf.begin=0;
				vertexFaces.push_back(vf);
				}
			for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Calculate the normal vector defined by the first three vertices: */
				const Point& v0=coords[coordIndices[fIt->firstVertex]];
				const Point& v1=coords[coordIndices[fIt->firstVertex+1]];
				const Point& v2=coords[coordIndices[fIt->firstVertex+2]];
				Vector faceNormal=(v1-v0)^(v2-v1);
				faceNormals.push_back(faceNormal.normalize());
				
				/* Count the face for each of the face's vertices: */
				MFInt::ValueList::const_iterator ciIt=coordIndices.begin()+fIt->firstVertex;
				for(size_t i=0;i<fIt->numVertices;++i,++ciIt)
					++vertexFaces[*ciIt-vertexIndexMin].end;
				}
			
			/* Calculate the face corner array's layout: */
			size_t numFaceCorners=0;
			for(std::vector<VertexFaces>::iterator vfIt=vertexFaces.begin();vfIt!=vertexFaces.end();++vfIt)
				{
				vfIt->begin=numFaceCorners;
				numFaceCorners+=vfIt->end;
				vfIt->end=vfIt->begin;
				}
			
			/* Calculate the face corner array: */
			std::vector<FaceCorner> faceCorners;
			faceCorners.reserve(numFaceCorners);
			for(size_t i=0;i<numFaceCorners;++i)
				faceCorners.push_back(FaceCorner());
			size_t faceIndex=0;
			for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt,++faceIndex)
				{
				/* Go around the face and calculate corner interior angles: */
				MFInt::ValueList::const_iterator fvIt=coordIndices.begin()+fIt->firstVertex;
				MFInt::ValueList::const_iterator fvEnd=fvIt+fIt->numVertices;
				int v0=fvEnd[-1];
				Vector e0=coords[v0]-coords[fvEnd[-2]];
				Scalar e0Sqr=e0.sqr();
				for(;fvIt!=fvEnd;++fvIt)
					{
					/* Calculate the corner's interior angle: */
					Vector e1=coords[*fvIt]-coords[v0];
					Scalar e1Sqr=e1.sqr();
					Scalar angle=Math::acos(Math::clamp(-(e0*e1)/Math::sqrt(e0Sqr*e1Sqr),Scalar(-1),Scalar(1)));
					
					/* Store the face corner with the corner's central vertex: */
					VertexFaces& vf=vertexFaces[v0-vertexIndexMin];
					faceCorners[vf.end].faceIndex=faceIndex;
					faceCorners[vf.end].cornerAngle=angle;
					++vf.end;
					
					/* Go to the next corner: */
					v0=*fvIt;
					e0=e1;
					e0Sqr=e1Sqr;
					}
				}
			
			/* Accumulate and upload vertex normal vectors: */
			Scalar creaseAngleCos=Math::cos(creaseAngle.getValue());
			std::vector<Vector> faceVertexNormals;
			faceVertexNormals.reserve(maxNumFaceVertices);
			faceIndex=0;
			for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt,++faceIndex)
				{
				/* Accumulate normal vectors for the face's vertices: */
				faceVertexNormals.clear();
				MFInt::ValueList::const_iterator fvIt=coordIndices.begin()+fIt->firstVertex;
				MFInt::ValueList::const_iterator fvEnd=fvIt+fIt->numVertices;
				for(;fvIt!=fvEnd;++fvIt)
					{
					VertexFaces& vf=vertexFaces[*fvIt-vertexIndexMin];
					Vector normal=Vector::zero;
					for(size_t fci=vf.begin;fci!=vf.end;++fci)
						{
						/* Check if the angle between the current face and the current shared face is less than the crease angle: */
						FaceCorner& fc=faceCorners[fci];
						if(faceNormals[faceIndex]*faceNormals[fc.faceIndex]>=creaseAngleCos)
							normal.addScaled(faceNormals[fc.faceIndex],fc.cornerAngle);
						}
					faceVertexNormals.push_back(normal.normalize());
					}
				
				/* Trivially triangulate the face: */
				for(size_t vi=2;vi<fIt->numVertices;++vi)
					{
					*reinterpret_cast<Vector*>(nPtr)=faceVertexNormals[0];
					nPtr+=dataItem->vertexSize;
					*reinterpret_cast<Vector*>(nPtr)=faceVertexNormals[vi-1];
					nPtr+=dataItem->vertexSize;
					*reinterpret_cast<Vector*>(nPtr)=faceVertexNormals[vi];
					nPtr+=dataItem->vertexSize;
					}
				}
			}
		else
			{
			/* Upload per-face normal vectors: */
			for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
				{
				/* Upload the face normal into all vertices generated by the face: */
				for(size_t i=(fIt->numVertices-2)*3;i>0;--i,nPtr+=dataItem->vertexSize)
					*reinterpret_cast<Vector*>(nPtr)=fIt->faceNormal;
				}
			}
		}
	
	/* Access the interleaved buffer's vertex positions: */
	GLubyte* cPtr=bufferPtr+dataItem->positionOffset;
	
	/* Check if there is a point transformation: */
	if(pointTransform.getValue()!=0)
		{
		/* Upload transformed vertex positions: */
		for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
			{
			/* Use the computed triangulation: */
			MFInt::ValueList::const_iterator ciIt=coordIndices.begin()+fIt->firstVertex;
			IndexList::iterator tvIt=triangleVertexIndices.begin()+fIt->firstTriangleVertex;
			for(size_t i=(fIt->numVertices-2)*3;i>0;--i,++tvIt,cPtr+=dataItem->vertexSize)
				*reinterpret_cast<Point*>(cPtr)=Point(pointTransform.getValue()->transformPoint(PointTransformNode::TPoint(coords[ciIt[*tvIt]])));
			}
		}
	else
		{
		/* Upload untransformed vertex positions: */
		for(std::vector<NCFace>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
			{
			/* Use the computed triangulation: */
			MFInt::ValueList::const_iterator ciIt=coordIndices.begin()+fIt->firstVertex;
			IndexList::iterator tvIt=triangleVertexIndices.begin()+fIt->firstTriangleVertex;
			for(size_t i=(fIt->numVertices-2)*3;i>0;--i,++tvIt,cPtr+=dataItem->vertexSize)
				*reinterpret_cast<Point*>(cPtr)=coords[ciIt[*tvIt]];
			}
		}
	}

IndexedFaceSetNode::IndexedFaceSetNode(void)
	:colorPerVertex(true),normalPerVertex(true),
	 ccw(true),convex(true),solid(true),creaseAngle(0),
	 haveColors(false),bbox(Box::empty),
	 numValidFaces(0),vertexIndexMin(0),vertexIndexMax(0),maxNumFaceVertices(0),totalNumFaceVertices(0),totalNumTriangles(0),
	 version(0)
	{
	}

const char* IndexedFaceSetNode::getClassName(void) const
	{
	return className;
	}

void IndexedFaceSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"texCoord")==0)
		{
		vrmlFile.parseSFNode(texCoord);
		}
	else if(strcmp(fieldName,"color")==0)
		{
		vrmlFile.parseSFNode(color);
		}
	else if(strcmp(fieldName,"normal")==0)
		{
		vrmlFile.parseSFNode(normal);
		}
	else if(strcmp(fieldName,"coord")==0)
		{
		vrmlFile.parseSFNode(coord);
		}
	else if(strcmp(fieldName,"texCoordIndex")==0)
		{
		vrmlFile.parseField(texCoordIndex);
		}
	else if(strcmp(fieldName,"colorIndex")==0)
		{
		vrmlFile.parseField(colorIndex);
		}
	else if(strcmp(fieldName,"colorPerVertex")==0)
		{
		vrmlFile.parseField(colorPerVertex);
		}
	else if(strcmp(fieldName,"normalIndex")==0)
		{
		vrmlFile.parseField(normalIndex);
		}
	else if(strcmp(fieldName,"normalPerVertex")==0)
		{
		vrmlFile.parseField(normalPerVertex);
		}
	else if(strcmp(fieldName,"coordIndex")==0)
		{
		vrmlFile.parseField(coordIndex);
		}
	else if(strcmp(fieldName,"ccw")==0)
		{
		vrmlFile.parseField(ccw);
		}
	else if(strcmp(fieldName,"convex")==0)
		{
		vrmlFile.parseField(convex);
		}
	else if(strcmp(fieldName,"solid")==0)
		{
		vrmlFile.parseField(solid);
		}
	else if(strcmp(fieldName,"creaseAngle")==0)
		{
		vrmlFile.parseField(creaseAngle);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void IndexedFaceSetNode::update(void)
	{
	/* Check if there are per-vertex colors: */
	haveColors=color.getValue()!=0;
	
	/* Access the face set's face vertex indices: */
	MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Calculate the number of valid faces, the range of vertex indices, the maximum number of vertices per face, the total number of face vertices, and the total number of generated triangles: */
	numValidFaces=0;
	vertexIndexMin=Math::Constants<int>::max;
	vertexIndexMax=-1;
	maxNumFaceVertices=0;
	totalNumFaceVertices=0;
	totalNumTriangles=0;
	for(MFInt::ValueList::iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Find the end of the current face: */
		MFInt::ValueList::iterator feIt;
		for(feIt=ciIt;feIt!=coordIndices.end()&&*feIt>=0;++feIt)
			;
		
		/* Check if the face is valid: */
		size_t numFaceVertices(feIt-ciIt);
		if(numFaceVertices>=3)
			{
			/* Count the face: */
			++numValidFaces;
			for(;ciIt!=feIt;++ciIt)
				{
				if(vertexIndexMin>*ciIt)
					vertexIndexMin=*ciIt;
				if(vertexIndexMax<*ciIt)
					vertexIndexMax=*ciIt;
				}
			if(maxNumFaceVertices<numFaceVertices)
				maxNumFaceVertices=numFaceVertices;
			totalNumFaceVertices+=numFaceVertices;
			totalNumTriangles+=numFaceVertices-2;
			}
		
		/* Go to the next face: */
		ciIt=feIt;
		if(ciIt!=coordIndices.end())
			++ciIt;
		}
	
	/* Update the face set's bounding box: */
	bbox=Box::empty;
	if(coord.getValue()!=0)
		{
		MFPoint::ValueList& coords=coord.getValue()->point.getValues();
		if(pointTransform.getValue()!=0)
			{
			/* Calculate the bounding box of the transformed point coordinates: */
			bbox=pointTransform.getValue()->calcBoundingBox(coords,coordIndices);
			}
		else
			{
			/* Calculate the bounding box of the untransformed point coordinates: */
			for(MFInt::ValueList::iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++ciIt)
				if(*ciIt>=0)
					bbox.addPoint(coords[*ciIt]);
			}
		}
	
	/* Bump up the indexed face set's version number: */
	++version;
	}

void IndexedFaceSetNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GeometryNode::read(reader);
	
	/* Read all fields: */
	reader.readSFNode(texCoord);
	reader.readSFNode(color);
	reader.readSFNode(normal);
	reader.readSFNode(coord);
	reader.readField(texCoordIndex);
	reader.readField(colorIndex);
	reader.readField(colorPerVertex);
	reader.readField(normalIndex);
	reader.readField(normalPerVertex);
	reader.readField(coordIndex);
	reader.readField(ccw);
	reader.readField(convex);
	reader.readField(solid);
	reader.readField(creaseAngle);
	}

void IndexedFaceSetNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GeometryNode::write(writer);
	
	/* Write all fields: */
	writer.writeSFNode(texCoord);
	writer.writeSFNode(color);
	writer.writeSFNode(normal);
	writer.writeSFNode(coord);
	writer.writeField(texCoordIndex);
	writer.writeField(colorIndex);
	writer.writeField(colorPerVertex);
	writer.writeField(normalIndex);
	writer.writeField(normalPerVertex);
	writer.writeField(coordIndex);
	writer.writeField(ccw);
	writer.writeField(convex);
	writer.writeField(solid);
	writer.writeField(creaseAngle);
	}

bool IndexedFaceSetNode::canCollide(void) const
	{
	return true;
	}

int IndexedFaceSetNode::getGeometryRequirementMask(void) const
	{
	int result=BaseAppearanceNode::HasSurfaces;
	if(!solid.getValue())
		result|=BaseAppearanceNode::HasTwoSidedSurfaces;
	if(haveColors)
		result|=BaseAppearanceNode::HasColors;
	
	return result;
	}

Box IndexedFaceSetNode::calcBoundingBox(void) const
	{
	/* Return the previously-calculated bounding box: */
	return bbox;
	}

void IndexedFaceSetNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Bail out if the sphere does not hit the face set's bounding box: */
	if(!collisionQuery.doesHitBox(bbox))
		return;
	
	/* Call the appropriate method for the face set's mode: */
	if(solid.getValue())
		{
		if(ccw.getValue())
			testCollisionSolidCcw(collisionQuery);
		else
			testCollisionSolidCw(collisionQuery);
		}
	else
		testCollisionNonSolid(collisionQuery);
	}

void IndexedFaceSetNode::glRenderAction(int appearanceRequirementMask,GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.uploadModelview();
	renderState.setFrontFace(ccw.getValue()?GL_CCW:GL_CW);
	if(solid.getValue())
		renderState.enableCulling(GL_BACK);
	else
		renderState.disableCulling();
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	if(dataItem->vertexBufferObjectId!=0&&dataItem->indexBufferObjectId!=0)
		{
		/*******************************************************************
		Render the indexed face set from the vertex and index buffers:
		*******************************************************************/
		
		/* Bind the face set's vertex and index buffer objects: */
		renderState.bindVertexBuffer(dataItem->vertexBufferObjectId);
		renderState.bindIndexBuffer(dataItem->indexBufferObjectId);
		
		if(dataItem->version!=version)
			{
			/* Calculate the memory layout of the in-buffer vertices: */
			dataItem->vertexArrayPartsMask=0x0;
			dataItem->vertexSize=0;
			dataItem->texCoordOffset=dataItem->vertexSize;
			if(numNeedsTexCoords!=0)
				{
				dataItem->vertexSize+=sizeof(TexCoord);
				dataItem->vertexArrayPartsMask|=GLVertexArrayParts::TexCoord;
				}
			typedef GLColor<GLubyte,4> BColor; // Type for colors uploaded to vertex buffers
			dataItem->colorOffset=dataItem->vertexSize;
			if(haveColors||numNeedsColors!=0)
				{
				dataItem->vertexSize+=sizeof(BColor);
				dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Color;
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
			
			/* Upload vertices and indices if there is anything to upload: */
			if(totalNumTriangles!=0)
				{
				/* Create the vertex buffer and prepare it for vertex data upload: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,totalNumTriangles*3*dataItem->vertexSize,0,GL_STATIC_DRAW_ARB);
				GLubyte* bufferPtr=static_cast<GLubyte*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Upload the new face set: */
				if(convex.getValue())
					uploadConvexFaceSet(dataItem,bufferPtr);
				else
					uploadNonConvexFaceSet(dataItem,bufferPtr);
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			
			/* Mark the vertex and index buffer objects as up-to-date: */
			dataItem->version=version;
			}
		
		/* Enable vertex buffer rendering: */
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
		
		/* Draw the vertex array: */
		glDrawArrays(GL_TRIANGLES,0,GLsizei(totalNumTriangles*3));
		}
	else
		{
		/*******************************************************************
		Render the indexed face set directly:
		*******************************************************************/
		
		// Not even going to bother...
		}
	}

void IndexedFaceSetNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}

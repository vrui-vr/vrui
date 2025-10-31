/***********************************************************************
CollisionTypes - Helper structures to work with collision testing.
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

#ifndef SCENEGRAPH_COLLISIONTYPES_INCLUDED
#define SCENEGRAPH_COLLISIONTYPES_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <SceneGraph/Geometry.h>

namespace SceneGraph {

struct CollisionEdge // Structure representing an edge
	{
	/* Elements: */
	public:
	Point center; // Center point of the edge
	Vector axis; // Axis direction of the edge, from the first to the second vertex
	Scalar axisSqr; // Squared length of the axis vector
	
	/* Constructors and destructors: */
	CollisionEdge(const Point& sCenter,const Vector& sAxis,Scalar sAxisSqr) // Element-wise constructor
		:center(sCenter),axis(sAxis),axisSqr(sAxisSqr)
		{
		}
	CollisionEdge(const Point& sCenter,const Vector& sAxis) // Ditto; calculates squared axis length on the fly
		:center(sCenter),axis(sAxis),axisSqr(axis.sqr())
		{
		}
	CollisionEdge(const Point& v0,const Point& v1) // Creates edge from to vertices
		:center(Geometry::mid(v0,v1)),axis(v1-v0),axisSqr(axis.sqr())
		{
		}
	};

struct CollisionTriangle // Structure representing a triangle
	{
	/* Elements: */
	public:
	Point v[3]; // The triangle's vertices
	Vector normal; // The triangle's normal vector, assuming counter-clockwise vertex order
	Scalar normalMag; // Length of the normal vector
	int axes[2]; // Indices of the two primary axes most parallel to the triangle's plane
	
	/* Constructors and destructors: */
	CollisionTriangle(const Point& sV0,const Point& sV1,const Point& sV2) // Creates a triangle from three vertices
		:normal((sV1-sV0)^(sV2-sV0)),normalMag(normal.mag())
		{
		/* Copy the vertices: */
		v[0]=sV0;
		v[1]=sV1;
		v[2]=sV2;
		
		/* Find the two parallel axes: */
		int a=Geometry::findParallelAxis(normal);
		axes[0]=(a+1)%3;
		axes[1]=(a+2)%3;
		}
	};

struct CollisionPolygon // Structure representing a polygon
	{
	/* Elements: */
	const std::vector<Point>& vertices; // Pointer to a list containing the polygon's vertices
	std::vector<int>::const_iterator begin,end; // Iterators to the beginning and end of the polygon's vertex indices
	Point center; // The polygon's center point
	Vector normal; // The polygon's normal vector
	Scalar normalMag; // Length of the normal vector
	int axes[2]; // Indices of the two primary axes most parallel to the polygon's plane
	
	/* Constructors and destructors: */
	CollisionPolygon(const std::vector<Point>& sVertices,std::vector<int>::const_iterator sBegin,std::vector<int>::const_iterator sEnd,const Point& sCenter,const Vector& sNormal) // Creates a polygon from a list of vertices, list subset of vertex indices, and normal vector
		:vertices(sVertices),begin(sBegin),end(sEnd),
		 center(sCenter),normal(sNormal),normalMag(normal.mag())
		{
		/* Find the two parallel axes: */
		int a=Geometry::findParallelAxis(normal);
		axes[0]=(a+1)%3;
		axes[1]=(a+2)%3;
		}
	};

}

#endif

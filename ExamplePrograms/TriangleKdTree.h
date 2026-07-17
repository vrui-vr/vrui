/***********************************************************************
TriangleKdTree - Class for kd-trees of triangles to support fast
intersection tests and distance calculations.
Copyright (c) 2009-2026 Oliver Kreylos

This file is part of the Virtual Clay Editing Package.

The Virtual Clay Editing Package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Clay Editing Package is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Clay Editing Package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef TRIANGLEKDTREE_INCLUDED
#define TRIANGLEKDTREE_INCLUDED

#include <vector>
#include <Math/Interval.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>

class TriangleKdTree
	{
	/* Embedded classes: */
	public:
	typedef unsigned int Card; // Type for triangle indices
	static const Card nil=~Card(0); // The invalid triangle index
	typedef std::vector<Card> CardList; // Type for lists of triangle indices
	typedef float Scalar; // Scalar type
	typedef Geometry::Point<Scalar,3> Point; // Type for affine points
	typedef Geometry::Vector<Scalar,3> Vector; // Type for affine vectors
	typedef Geometry::Box<Scalar,3> Box; // Type for axis-aligned bounding boxes
	
	struct Triangle // Structure for triangles defined by three vertices
		{
		/* Embedded classes: */
		public:
		typedef Math::Interval<Scalar> AxisExtent; // Type for triangle extents along a primary axis
		
		/* Elements: */
		Point vertices[3]; // The triangle's three vertices in counter-clockwise order
		
		/* Constructors and destructors: */
		Triangle(const Point& v0,const Point& v1,const Point& v2) // Creates triangle from three vertices
			{
			/* Copy the triangle vertices: */
			vertices[0]=v0;
			vertices[1]=v1;
			vertices[2]=v2;
			}
		
		/* Methods: */
		AxisExtent calcExtent(int dimension) const // Returns the extent of the triangle along the given primary axis
			{
			AxisExtent result(vertices[0][dimension]);
			result.addValue(vertices[1][dimension]);
			result.addValue(vertices[2][dimension]);
			
			return result;
			}
		};
	
	typedef std::vector<Triangle> TriangleList; // Type for lists of triangles
	
	struct RayIntersection // Structure to hold an intersection between a ray and a triangle
		{
		/* Elements: */
		public:
		Card triangleIndex; // Index of the intersected triangle
		Scalar lambda; // Ray parameter of the intersection point
		
		/* Constructors and destructors: */
		RayIntersection(Card sTriangleIndex,Scalar sLambda)
			:triangleIndex(sTriangleIndex),lambda(sLambda)
			{
			}
		};
	
	typedef std::vector<RayIntersection> RayIntersectionList; // Type for lists of ray intersections
	
	struct TriangleDistance // Structure to report the distance from a point to a triangle
		{
		/* Elements: */
		public:
		Card triangleIndex; // Index of the closest triangle
		Scalar dist2; // Squared distance from the query point to the closest triangle
		
		/* Constructors and destructors: */
		TriangleDistance(Card sTriangleIndex,Scalar sDist2)
			:triangleIndex(sTriangleIndex),dist2(sDist2)
			{
			}
		};
	
	private:
	struct TriangleFragment // Helper structure to represent triangle fragments during kd-tree creation
		{
		/* Elements: */
		public:
		Card originalIndex; // Index of triangle from which this fragment originated
		Triangle fragment; // The triangle fragment
		
		/* Constructors and destructors: */
		TriangleFragment(Card sOriginalIndex,const Point& sV0,const Point& sV1,const Point& sV2) // Creates a triangle fragment from an original triangle index and three vertices
			:originalIndex(sOriginalIndex),fragment(sV0,sV1,sV2)
			{
			}
		};
	
	typedef std::vector<TriangleFragment> TriangleFragmentList; // Type for lists of triangle fragments
	
	struct Node // Structure for kd-tree nodes
		{
		/* Elements: */
		public:
		Node* children; // Pointer to array of two child nodes for interior nodes
		int splitDimension; // Dimension along which the node is split
		Scalar plane; // Coordinate of splitting plane in node's dimension
		CardList triangleIndices; // List of triangles overlapping this leaf node's domain
		
		/* Constructors and destructors: */
		Node(void) // Creates empty leaf node
			:children(0)
			{
			}
		~Node(void) // Destroys the node and its descendants
			{
			delete[] children;
			}
		};
	
	/* Elements: */
	private:
	const TriangleList& triangles; // List of original triangles
	Box boundingBox; // Bounding box around all triangles
	Card maxTrianglesPerNode; // Maximum number of triangles per kd-tree node
	Node root; // Root node of kd-tree
	
	// DEBUGGING
	// mutable Card numTestedTriangles;
	// mutable Card numTraversedNodes;
	
	/* Private methods: */
	Scalar findBestSplit(const Box& domain,int dimension,const CardList& triangleIndices,const TriangleFragmentList& triangleFragments) const;
	void splitTriangle(Card triangleIndex,const Triangle& t,int dimension,Scalar splitPlane,TriangleFragmentList triangleFragments[2]);
	void initNode(Node& node,const Box& domain,const CardList& triangleIndices,const TriangleFragmentList& triangleFragments);
	void intersectXrayNode(const Node& node,const Point& start,RayIntersectionList& intersections) const;
	void calcTriangleDistanceNode(const Node& node,const Point& position,TriangleDistance& distance) const;
	
	/* Constructors and destructors: */
	public:
	TriangleKdTree(const TriangleList& sTriangles); // Creates kd-tree for given triangle list without initialization
	
	/* Methods: */
	void createTree(const Box& sBoundingBox,Card sMaxTrianglesPerNode,const CardList& triangleIndices); // Creates a kd-tree for the given bounding box, containing only the triangles whose indices are in the given list
	void createTree(const Box& sBoundingBox,Card sMaxTrianglesPerNode); // Ditto, using all triangles in the triangle list
	RayIntersectionList intersectXray(const Point& start) const; // Intersects a ray along the positive x axis starting at the given starting point with the kd-tree; returns list of all intersection x coordinates in increasing order
	};

#endif

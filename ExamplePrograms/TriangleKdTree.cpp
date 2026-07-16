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

#include "TriangleKdTree.h"

// DEBUGGING
#include <assert.h>
#include <iostream>
#include <Misc/Timer.h>

#include <algorithm>
#include <Misc/Utility.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/IntersectionTests.h>

namespace {

/****************
Helper functions:
****************/

template <class Scalar>
inline
Scalar
increment(
	Scalar value)
	{
	Scalar delta=Scalar(1);
	while(value+delta!=value)
		delta*=Scalar(0.5);
	if(delta==Scalar(0))
		delta=Math::Constants<Scalar>::smallest;
	while(value+delta==value)
		delta*=Scalar(2);
	Scalar newValue=value+delta;
	assert(newValue!=value);
	return newValue;
	}

template <class Scalar>
inline
Scalar
decrement(
	Scalar value)
	{
	Scalar delta=Scalar(1);
	while(value-delta!=value)
		delta*=Scalar(0.5);
	if(delta==Scalar(0))
		delta=Math::Constants<Scalar>::smallest;
	while(value-delta==value)
		delta*=Scalar(2);
	Scalar newValue=value-delta;
	assert(newValue!=value);
	return newValue;
	}

template <class Scalar>
inline
Geometry::Point<Scalar,3>
affCo(
	const Geometry::Point<Scalar,3>& p0,
	const Geometry::Point<Scalar,3>& p1,
	Scalar w1)
	{
	return Geometry::Point<Scalar,3>(p0[0]+(p1[0]-p0[0])*w1,p0[1]+(p1[1]-p0[1])*w1,p0[2]+(p1[2]-p0[2])*w1);
	}

}

/*******************************
Methods of class TriangleKdTree:
*******************************/

TriangleKdTree::Scalar
TriangleKdTree::findBestSplit(
	const TriangleKdTree::Box& domain,
	int dimension,
	const TriangleKdTree::CardList& triangleIndices,
	const TriangleKdTree::TriangleFragmentList& triangleFragments) const
	{
	/*******************************************************************
	Create sorted arrays of triangle intervals to find an optimal split
	plane:
	*******************************************************************/
	
	Card totalNumTriangles=triangleIndices.size()+triangleFragments.size();
	Scalar* starts=new Scalar[totalNumTriangles+1];
	Scalar* sPtr=starts;
	Scalar* ends=new Scalar[totalNumTriangles+1];
	Scalar* ePtr=ends;
	
	/* Process complete triangles: */
	for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt,++sPtr,++ePtr)
		{
		Triangle::AxisExtent ae=triangles[*tiIt].calcExtent(dimension);
		*sPtr=ae.getMin();
		*ePtr=ae.getMax();
		}
	
	/* Process triangle fragments: */
	for(TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();tfIt!=triangleFragments.end();++tfIt,++sPtr,++ePtr)
		{
		Triangle::AxisExtent ae=tfIt->fragment.calcExtent(dimension);
		*sPtr=ae.getMin();
		*ePtr=ae.getMax();
		}
	
	/* Sort both arrays and add sentinel elements: */
	std::sort(starts,starts+totalNumTriangles);
	starts[totalNumTriangles]=Math::Constants<Scalar>::max;
	std::sort(ends,ends+totalNumTriangles);
	ends[totalNumTriangles]=Math::Constants<Scalar>::max;
	
	/* Check whether it is better to split at the median or truncate empty space: */
	Scalar mid=Math::mid(domain.min[dimension],domain.max[dimension]);
	Scalar splitPlane;
	if(starts[0]>=mid)
		{
		/* Truncate empty space from the left: */
		splitPlane=decrement(starts[0]);
		}
	else if(ends[totalNumTriangles-1]<=mid)
		{
		/* Truncate empty space from the right: */
		splitPlane=increment(ends[totalNumTriangles-1]);
		}
	else
		{
		/* Find a split position that balances the subtree and minimizes triangle splits: */
		Card si=0;
		Card ei=0;
		Scalar pos=starts[0];
		Card numLeftTriangles=0;
		Card numRightTriangles=totalNumTriangles;
		double bestness=Math::Constants<Scalar>::min;
		while(ei<totalNumTriangles)
			{
			/* Process all "events" at the current position: */
			while(starts[si]==pos)
				{
				++numLeftTriangles;
				++si;
				}
			while(ends[ei]==pos)
				{
				--numRightTriangles;
				++ei;
				}
			
			/* Calculate the "goodness" of the tentative split position: */
			double goodness=-Math::sqr(double(numLeftTriangles)-double(numRightTriangles))-double(numLeftTriangles+numRightTriangles-totalNumTriangles)*double(totalNumTriangles);
			if(bestness<goodness)
				{
				splitPlane=pos;
				bestness=goodness;
				}
			
			/* Go to the next position: */
			pos=starts[si];
			if(pos>ends[ei])
				pos=ends[ei];
			}
		
		if(bestness>=Math::sqr(double(totalNumTriangles))*-0.4)
			{
			/* Nudge the split position to the right: */
			splitPlane=increment(splitPlane);
			}
		else
			{
			/* Signal an invalid split: */
			splitPlane=domain.min[dimension];
			}
		}
	
	/* Clean up: */
	delete[] starts;
	delete[] ends;
	
	return splitPlane;
	}

void
TriangleKdTree::splitTriangle(
	TriangleKdTree::Card triangleIndex,
	const TriangleKdTree::Triangle& t,
	int dimension,
	TriangleKdTree::Scalar splitPlane,
	TriangleKdTree::TriangleFragmentList triangleFragments[2])
	{
	/* Create arrays to hold the new front and back vertices: */
	Point front[4];
	Point* fPtr=front;
	Point back[4];
	Point* bPtr=back;
	
	/* Process the three triangle edges: */
	int e0=2;
	for(int e1=0;e1<3;e0=e1,++e1)
		{
		/* Process the edge: */
		const Point& v0=t.vertices[e0];
		const Point& v1=t.vertices[e1];
		
		if(v0[dimension]<=splitPlane)
			*(fPtr++)=v0;
		if(v0[dimension]>=splitPlane)
			*(bPtr++)=v0;
		if(v0[dimension]<splitPlane&&v1[dimension]>splitPlane)
			{
			/* Calculate the intersection point: */
			Point s=affCo(v0,v1,(splitPlane-v0[dimension])/(v1[dimension]-v0[dimension]));
			s[dimension]=splitPlane;
			*(fPtr++)=s;
			*(bPtr++)=s;
			}
		else if(v0[dimension]>splitPlane&&v1[dimension]<splitPlane)
			{
			/* Calculate the intersection point: */
			Point s=affCo(v1,v0,(splitPlane-v1[dimension])/(v0[dimension]-v1[dimension]));
			s[dimension]=splitPlane;
			*(fPtr++)=s;
			*(bPtr++)=s;
			}
		}
	
	/* Extend single points or line segments into zero-area triangles: */
	while(fPtr-front<3)
		{
		fPtr[0]=fPtr[-1];
		++fPtr;
		}
	while(bPtr-back<3)
		{
		bPtr[0]=bPtr[-1];
		++bPtr;
		}
	
	#if 0
	// DEBUGGING
	Box triDomain=Box::empty;
	for(int i=0;i<3;++i)
		triDomain.addPoint(t.vertices[i]);
	for(Point* pPtr=front;pPtr!=fPtr;++pPtr)
		assert(triDomain.contains(*pPtr));
	for(Point* pPtr=back;pPtr!=bPtr;++pPtr)
		assert(triDomain.contains(*pPtr));
	#endif
	
	/* Assemble the front result triangle(s): */
	triangleFragments[0].push_back(TriangleFragment(triangleIndex,front[0],front[1],front[2]));
	if(fPtr-front>3)
		triangleFragments[0].push_back(TriangleFragment(triangleIndex,front[2],front[3],front[0]));
	
	/* Assemble the back result triangle(s): */
	triangleFragments[1].push_back(TriangleFragment(triangleIndex,back[0],back[1],back[2]));
	if(bPtr-back>3)
		triangleFragments[1].push_back(TriangleFragment(triangleIndex,back[2],back[3],back[0]));
	}

void
TriangleKdTree::initNode(
	TriangleKdTree::Node& node,
	const TriangleKdTree::Box& domain,
	const TriangleKdTree::CardList& triangleIndices,
	const TriangleKdTree::TriangleFragmentList& triangleFragments)
	{
	#if 0
	// DEBUGGING
	for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt)
		checkTriangle(domain,triangles[*tiIt]);
	for(TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();tfIt!=triangleFragments.end();++tfIt)
		checkTriangle(domain,tfIt->fragment);
	#endif
	
	/* Count the total number of distinct original triangle indices in the triangle and fragment lists: */
	Card numDistinctTriangles=triangleIndices.size();
	if(!triangleFragments.empty())
		{
		TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();
		++numDistinctTriangles;
		Card lastIndex=tfIt->originalIndex;
		for(++tfIt;tfIt!=triangleFragments.end();++tfIt)
			if(lastIndex!=tfIt->originalIndex)
				{
				++numDistinctTriangles;
				lastIndex=tfIt->originalIndex;
				}
		}
	
	/* Check the number of distinct triangles against the limit: */
	if(numDistinctTriangles>maxTrianglesPerNode)
		{
		/*******************************************************************
		Create an interior node by distributing the given triangles and
		fragments between the node's two subdomains:  
		*******************************************************************/
		
		// DEBUGGING
		// std::cout<<"Creating interior node with "<<numDistinctTriangles<<" triangles"<<std::endl;
		
		/*******************************************************************
		Try at most three times to find a good split; if it fails, create a
		leaf node instead.
		*******************************************************************/
		
		int numTries;
		for(numTries=0;numTries<3;++numTries)
			{
			/* Find the optimal split plane: */
			node.plane=findBestSplit(domain,node.splitDimension,triangleIndices,triangleFragments);
			
			/* Check if the found split is beneficial: */
			if(node.plane>domain.min[node.splitDimension]&&node.plane<domain.max[node.splitDimension])
				break;
			
			/* Otherwise go to the next split dimension and try again: */
			node.splitDimension=(node.splitDimension+1)%3;
			}
		
		if(numTries<3)
			{
			/*******************************************************************
			Distribute complete triangles and triangle fragments to the two
			subdomains:
			*******************************************************************/
			
			CardList subTriangleIndices[2];
			TriangleFragmentList subTriangleFragments[2];
			
			/* Process all complete triangles: */
			for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt)
				{
				const Triangle& t=triangles[*tiIt];
				Triangle::AxisExtent ae=t.calcExtent(node.splitDimension);
				
				if(ae.contains(node.plane))
					{
					/* Split the triangle and assign the fragments to the subdomains: */
					splitTriangle(*tiIt,t,node.splitDimension,node.plane,subTriangleFragments);
					}
				else
					{
					/* Assign the complete triangle to one or both subdomains: */
					if(ae.getMin()<=node.plane)
						subTriangleIndices[0].push_back(*tiIt);
					if(ae.getMax()>=node.plane)
						subTriangleIndices[1].push_back(*tiIt);
					}
				}
			
			/* Process all triangle fragments: */
			for(TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();tfIt!=triangleFragments.end();++tfIt)
				{
				Triangle::AxisExtent ae=tfIt->fragment.calcExtent(node.splitDimension);
				
				if(ae.contains(node.plane))
					{
					/* Split the triangle fragment and assign the resulting fragments to the subdomains: */
					splitTriangle(tfIt->originalIndex,tfIt->fragment,node.splitDimension,node.plane,subTriangleFragments);
					}
				else
					{
					/* Assign the unchanged triangle fragment to one or both subdomains: */
					if(ae.getMin()<=node.plane)
						subTriangleFragments[0].push_back(*tfIt);
					if(ae.getMax()>=node.plane)
						subTriangleFragments[1].push_back(*tfIt);
					}
				}
			
			// DEBUGGING
			// std::cout<<"Creating subtree node with ";
			// std::cout<<subTriangleIndices[0].size()+subTriangleFragments[0].size()<<" left and ";
			// std::cout<<subTriangleIndices[1].size()+subTriangleFragments[1].size()<<" right triangles"<<std::endl;
			
			/* Create and initialize the node's children: */
			node.children=new Node[2];
			for(int i=0;i<2;++i)
				{
				Box subDomain=domain;
				if(i==0)
					subDomain.max[node.splitDimension]=node.plane;
				else
					subDomain.min[node.splitDimension]=node.plane;
				node.children[i].splitDimension=(node.splitDimension+1)%3;
				initNode(node.children[i],subDomain,subTriangleIndices[i],subTriangleFragments[i]);
				}
			
			return;
			}
		else
			{
			// DEBUGGING
			// std::cout<<"Punting on node with "<<numDistinctTriangles<<" triangles"<<std::endl;
			}
		}
	
	/*******************************************************************
	Create a leaf node containing the given triangles and fragments:
	*******************************************************************/
	
	// DEBUGGING
	// std::cout<<"Creating leaf node with "<<numDistinctTriangles<<" triangles"<<std::endl;
	
	node.triangleIndices.reserve(numDistinctTriangles);
	
	/* Merge the lists of complete and fragmented triangles, removing duplicates: */
	CardList::const_iterator tiIt=triangleIndices.begin();
	TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();
	Card lastIndex=nil;
	while(tiIt!=triangleIndices.end()&&tfIt!=triangleFragments.end())
		{
		/* Get the smaller triangle index from the two lists: */
		Card nextIndex;
		if(*tiIt<=tfIt->originalIndex)
			{
			/* Grab the next complete triangle: */
			nextIndex=*tiIt;
			++tiIt;
			}
		else
			{
			/* Grab the next triangle fragment: */
			nextIndex=tfIt->originalIndex;
			++tfIt;
			}
		
		/* Add the next index to the list unless it's a duplicate: */
		if(nextIndex!=lastIndex)
			node.triangleIndices.push_back(nextIndex);
		lastIndex=nextIndex;
		}
	while(tiIt!=triangleIndices.end())
		{
		/* Add the next complete triangle to the list unless it's a duplicate: */
		if(*tiIt!=lastIndex)
			node.triangleIndices.push_back(*tiIt);
		lastIndex=*tiIt;
		++tiIt;
		}
	while(tfIt!=triangleFragments.end())
		{
		/* Add the next triangle fragment to the list unless it's a duplicate: */
		if(tfIt->originalIndex!=lastIndex)
			node.triangleIndices.push_back(tfIt->originalIndex);
		lastIndex=tfIt->originalIndex;
		++tfIt;
		}
	}

void
TriangleKdTree::intersectXrayNode(
	const TriangleKdTree::Node& node,
	const TriangleKdTree::Point& start,
	std::vector<TriangleKdTree::Scalar>& intersections) const
	{
	/* Check if the node is a leaf: */
	if(node.children==0)
		{
		/* Intersect the ray with all triangles in the node: */
		for(CardList::const_iterator tiIt=node.triangleIndices.begin();tiIt!=node.triangleIndices.end();++tiIt)
			{
			/* Sort the triangle's vertices by their z values: */
			const Triangle& t=triangles[*tiIt];
			const Point* v0=&t.vertices[0];
			const Point* v1=&t.vertices[1];
			if((*v0)[2]>(*v1)[2])
				Misc::swap(v0,v1);
			const Point* v2=&t.vertices[2];
			if((*v1)[2]>(*v2)[2])
				Misc::swap(v1,v2);
			if((*v0)[2]>(*v1)[2])
				Misc::swap(v0,v1);
			
			/* Intersect the triangle with the plane orthogonal to the z axis and containing the start point: */
			Point e0,e1;
			bool haveEdge=false;
			if((*v0)[2]<start[2]&&(*v1)[2]<start[2]&&(*v2)[2]>=start[2])
				{
				e0=affCo(*v0,*v2,(start[2]-(*v0)[2])/((*v2)[2]-(*v0)[2]));
				e1=affCo(*v1,*v2,(start[2]-(*v1)[2])/((*v2)[2]-(*v1)[2]));
				haveEdge=true;
				}
			else if((*v0)[2]<start[2]&&(*v1)[2]>=start[2]&&(*v2)[2]>=start[2])
				{
				e0=affCo(*v0,*v1,(start[2]-(*v0)[2])/((*v1)[2]-(*v0)[2]));
				e1=affCo(*v0,*v2,(start[2]-(*v0)[2])/((*v2)[2]-(*v0)[2]));
				haveEdge=true;
				}
			if(haveEdge)
				{
				/* Intersect the edge in the intersection plane with the ray: */
				if(e0[1]<start[1]&&e1[1]>=start[1])
					{
					Scalar x=e0[0]+(e1[0]-e0[0])*(start[1]-e0[1])/(e1[1]-e0[1]);
					if(x>=start[0])
						{
						/* We have an intersection: */
						intersections.push_back(x);
						}
					}
				else if(e0[1]>=start[1]&&e1[1]<start[1])
					{
					Scalar x=e1[0]+(e0[0]-e1[0])*(start[1]-e1[1])/(e0[1]-e1[1]);
					if(x>=start[0])
						{
						/* We have an intersection: */
						intersections.push_back(x);
						}
					}
				}
			}
		}
	else
		{
		/* Intersect the ray with the node's children recursively: */
		if(node.splitDimension==0)
			{
			/* Intersect the ray with both children: */
			for(int i=0;i<2;++i)
				intersectXrayNode(node.children[i],start,intersections);
			}
		else
			{
			/* Intersect the ray with the node's child that intersects the ray: */
			if(node.plane>=start[node.splitDimension])
				intersectXrayNode(node.children[0],start,intersections);
			if(node.plane<=start[node.splitDimension])
				intersectXrayNode(node.children[1],start,intersections);
			}
		}
	}

TriangleKdTree::TriangleKdTree(
	const TriangleKdTree::TriangleList& sTriangles)
	:triangles(sTriangles)
	{
	}

void
TriangleKdTree::createTree(
	const TriangleKdTree::Box& sBoundingBox,
	TriangleKdTree::Card sMaxTrianglesPerNode,
	const TriangleKdTree::CardList& triangleIndices)
	{
	// DEBUGGING
	// std::cout<<"Creating kd-tree for "<<triangleIndices.size()<<" triangles"<<std::endl;
	
	/* Store tree creation parameters: */
	boundingBox=sBoundingBox;
	maxTrianglesPerNode=sMaxTrianglesPerNode;
	
	/* Extend the bounding box slightly outwards: */
	for(int i=0;i<3;++i)
		{
		boundingBox.min[i]=decrement(boundingBox.min[i]);
		boundingBox.max[i]=increment(boundingBox.max[i]);
		}
	
	/* Initialize the kd-tree: */
	TriangleFragmentList triangleFragments;
	root.splitDimension=0;
	initNode(root,boundingBox,triangleIndices,triangleFragments);
	}

std::vector<TriangleKdTree::Scalar>
TriangleKdTree::intersectXray(
	const TriangleKdTree::Point& start) const
	{
	/* Intersect the ray with the root node: */
	std::vector<Scalar> intersections;
	intersectXrayNode(root,start,intersections);
	
	if(!intersections.empty())
		{
		/* Sort the intersection list: */
		std::sort(intersections.begin(),intersections.end());
		
		/* Remove duplicates from the intersection list: */
		std::vector<Scalar> result;
		std::vector<Scalar>::iterator iIt=intersections.begin();
		result.push_back(*iIt);
		for(++iIt;iIt!=intersections.end();++iIt)
			if(*iIt!=result.back())
				result.push_back(*iIt);
		
		return result;
		}
	else
		return intersections;
	}

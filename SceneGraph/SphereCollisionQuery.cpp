/***********************************************************************
SphereCollisionQuery - Class to query the collision of a sliding sphere
with a scene graph.
Copyright (c) 2020-2025 Oliver Kreylos

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

#include <SceneGraph/SphereCollisionQuery.h>

#include <Math/Math.h>
#include <Geometry/Box.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/CollisionTypes.h>

namespace SceneGraph {

/*************************************
Methods of class SphereCollisionQuery:
*************************************/

SphereCollisionQuery::SphereCollisionQuery(const Point& sC0,const Vector& sC0c1,Scalar sRadius)
	:c0(sC0),c0c1(sC0c1),radius(sRadius),
	 c0c1Sqr(c0c1.sqr()),radiusSqr(radius*radius),
	 hitLambda(1)
	{
	}

Math::Interval<Scalar> SphereCollisionQuery::calcBoxInterval(const Box& box) const
	{
	/* Test the query's valid parameter interval against an outset box along all three axes: */
	Scalar lMin(0);
	Scalar lMax(hitLambda);
	for(int axis=0;axis<3&&lMin<lMax;++axis)
		{
		if(c0c1[axis]<Scalar(0))
			{
			/* Calculate the overlap interval of the sphere's path against the box axis: */
			lMin=Math::max(lMin,(box.max[axis]-c0[axis]+radius)/c0c1[axis]);
			lMax=Math::min(lMax,(box.min[axis]-c0[axis]-radius)/c0c1[axis]);
			}
		else if(c0c1[axis]>Scalar(0))
			{
			/* Calculate the overlap interval of the sphere's path against the box axis: */
			lMin=Math::max(lMin,(box.min[axis]-c0[axis]-radius)/c0c1[axis]);
			lMax=Math::min(lMax,(box.max[axis]-c0[axis]+radius)/c0c1[axis]);
			}
		else if(c0[axis]<box.min[axis]-radius||c0[axis]>box.max[axis]+radius)
			{
			/* Invalidate the interval: */
			lMin=lMax;
			}
		}
	
	/* Return the potentially empty resulting interval: */
	return Math::Interval<Scalar>(lMin,lMax);
	}

bool SphereCollisionQuery::doesHitBox(const Box& box) const
	{
	/* Test the query's valid parameter interval against an outset box along all three axes: */
	Scalar lMin(0);
	Scalar lMax(hitLambda);
	for(int axis=0;axis<3&&lMin<lMax;++axis)
		{
		if(c0c1[axis]<Scalar(0))
			{
			/* Calculate the overlap interval of the sphere's path against the box axis: */
			lMin=Math::max(lMin,(box.max[axis]-c0[axis]+radius)/c0c1[axis]);
			lMax=Math::min(lMax,(box.min[axis]-c0[axis]-radius)/c0c1[axis]);
			}
		else if(c0c1[axis]>Scalar(0))
			{
			/* Calculate the overlap interval of the sphere's path against the box axis: */
			lMin=Math::max(lMin,(box.min[axis]-c0[axis]-radius)/c0c1[axis]);
			lMax=Math::min(lMax,(box.max[axis]-c0[axis]+radius)/c0c1[axis]);
			}
		else if(c0[axis]<box.min[axis]-radius||c0[axis]>box.max[axis]+radius)
			{
			/* Invalidate the interval: */
			lMin=lMax;
			}
		}
	
	/* The sphere hits the box if the valid parameter interval is non-empty: */
	return lMin<lMax;
	}

bool SphereCollisionQuery::testVertexAndUpdate(const Point& vertex)
	{
	bool result=false;
	
	Vector vc0=c0-vertex; // Vector from vertex to sphere's starting point
	
	Scalar a=c0c1Sqr; // Quadratic coefficient from quadratic formula
	Scalar bh=vc0*c0c1; // Halved linear coefficient from quadratic formula
	Scalar c=vc0.sqr()-radiusSqr; // Constant coefficient from quadratic formula
	
	Scalar discq=bh*bh-a*c; // Quarter discriminant from quadratic formula
	if(discq>=Scalar(0))
		{
		/* Find the quadratic equation's smaller root: */
		// Scalar lambda=bh>=Scalar(0)?(-bh-Math::sqrt(discq))/a:c/(-bh+Math::sqrt(discq)); // Slightly more stable formula
		Scalar lambda=c/(-bh+Math::sqrt(discq)); // Stable formulation for negative bh, which is the only bh that counts
		
		/* Check whether this vertex will affect the collision: */
		if(lambda>=Scalar(0))
			{
			if(hitLambda>lambda)
				{
				/* Update the hit result: */
				hitLambda=lambda;
				hitNormal=Geometry::addScaled(vc0,c0c1,lambda);
				
				result=true;
				}
			}
		else if(c<Scalar(0)&&bh<Scalar(0)&&hitLambda>Scalar(0))
			{
			/* Sphere already penetrates the vertex; prevent it from getting worse: */
			hitLambda=Scalar(0);
			hitNormal=vc0;
			
			result=true;
			}
		}
	
	return result;
	}

bool SphereCollisionQuery::testEdgeAndUpdate(const Point& vertex0,const Point& vertex1)
	{
	bool result=false;
	
	Vector v0c0=c0-vertex0; // Vector from edge's first vertex to sphere's starting point
	Vector v0v1=vertex1-vertex0; // Vector from edge's first vertex to edge's second vertex
	Scalar v0v1Sqr=v0v1.sqr(); // Squared length of edge vector
	Vector v0c0xv0v1=v0c0^v0v1;
	Vector c0c1xv0v1=c0c1^v0v1;
	
	Scalar bh=v0c0xv0v1*c0c1xv0v1; // Halved linear coefficient from quadratic formula
	if(bh<Scalar(0))
		{
		Scalar a=c0c1xv0v1.sqr(); // Quadratic coefficient from quadratic formula
		Scalar c=v0c0xv0v1.sqr()-radiusSqr*v0v1Sqr; // Constant coefficient from quadratic formula
		
		Scalar discq=bh*bh-a*c; // Quarter discriminant from quadratic formula
		if(discq>=Scalar(0))
			{
			/* Find the quadratic equation's smaller root: */
			// Scalar lambda=bh>=Scalar(0)?(-bh-Math::sqrt(discq))/a:c/(-bh+Math::sqrt(discq)); // Slightly more stable formula
			Scalar lambda=c/(-bh+Math::sqrt(discq)); // Stable formulation for negative bh, which is the only bh that counts
			
			/* Check whether this edge will affect the collision: */
			if(lambda>=Scalar(0))
				{
				if(hitLambda>lambda)
					{
					/* Check whether the contact point is inside the edge's extents: */
					Vector cv=Geometry::addScaled(v0c0,c0c1,lambda);
					Scalar mu=cv*v0v1;
					if(mu>=Scalar(0)&&mu<=v0v1Sqr)
						{
						/* Update the hit result: */
						hitLambda=lambda;
						hitNormal=Geometry::subtractScaled(cv,v0v1,mu/v0v1Sqr);
						
						result=true;
						}
					}
				}
			else if(c<Scalar(0)&&hitLambda>Scalar(0))
				{
				/* Check whether the sphere's starting point is inside the edge's extents: */
				Scalar mu=v0c0*v0v1;
				if(mu>=Scalar(0)&&mu<=v0v1Sqr)
					{
					/* Sphere already penetrates the edge; prevent it from getting worse: */
					hitLambda=Scalar(0);
					hitNormal=Geometry::subtractScaled(v0c0,v0v1,mu/v0v1Sqr);
					
					result=true;
					}
				}
			}
		}
	
	return result;
	}

bool SphereCollisionQuery::testEdgeAndUpdate(const Point& center,const Vector& axis,Scalar axisSqr)
	{
	bool result=false;
	
	Vector v0c0=c0-center; // Vector from edge's center point to sphere's starting point
	Vector v0c0xv0v1=v0c0^axis;
	Vector c0c1xv0v1=c0c1^axis;
	
	Scalar bh=v0c0xv0v1*c0c1xv0v1; // Halved linear coefficient from quadratic formula
	if(bh<Scalar(0))
		{
		Scalar a=c0c1xv0v1.sqr(); // Quadratic coefficient from quadratic formula
		Scalar c=v0c0xv0v1.sqr()-radiusSqr*axisSqr; // Constant coefficient from quadratic formula
		
		Scalar discq=bh*bh-a*c; // Quarter discriminant from quadratic formula
		if(discq>=Scalar(0))
			{
			/* Find the quadratic equation's smaller root: */
			// Scalar lambda=bh>=Scalar(0)?(-bh-Math::sqrt(discq))/a:c/(-bh+Math::sqrt(discq)); // Slightly more stable formula
			Scalar lambda=c/(-bh+Math::sqrt(discq)); // Stable formulation for negative bh, which is the only bh that counts
			
			/* Check whether this edge will affect the collision: */
			if(lambda>=Scalar(0))
				{
				if(hitLambda>lambda)
					{
					/* Check whether the contact point is inside the edge's extents: */
					Vector cv=Geometry::addScaled(v0c0,c0c1,lambda);
					Scalar mu=cv*axis;
					if(Math::abs(mu)*Scalar(2)<=axisSqr)
						{
						/* Update the hit result: */
						hitLambda=lambda;
						hitNormal=Geometry::subtractScaled(cv,axis,mu/axisSqr);
						
						result=true;
						}
					}
				}
			else if(c<Scalar(0)&&hitLambda>Scalar(0))
				{
				/* Check whether the sphere's starting point is inside the edge's extents: */
				Scalar mu=v0c0*axis;
				if(Math::abs(mu)*Scalar(2)<=axisSqr)
					{
					/* Sphere already penetrates the edge; prevent it from getting worse: */
					hitLambda=Scalar(0);
					hitNormal=Geometry::subtractScaled(v0c0,axis,mu/axisSqr);
					
					result=true;
					}
				}
			}
		}
	
	return result;
	}

bool SphereCollisionQuery::testPlaneAndUpdate(const Point& center,const Vector& normal)
	{
	bool result=false;
	
	Vector cc0=c0-center; // Vector from plane's center to sphere's starting point
	Scalar cc0n=cc0*normal;
	Scalar c0c1n=c0c1*normal;
	
	if(c0c1n<Scalar(0))
		{
		/* Find the first intersection: */
		Scalar lambda=(-radius*normal.mag()-cc0n)/c0c1n;
		
		/* Check whether this plane will affect the collision: */
		if(lambda>=Scalar(0))
			{
			if(hitLambda>lambda)
				{
				/* Update the hit result: */
				hitLambda=lambda;
				hitNormal=normal;
				
				result=true;
				}
			}
		else if(cc0n>Scalar(0)&&hitLambda>Scalar(0))
			{
			/* Sphere already penetrates the plane; prevent it from getting worse: */
			hitLambda=Scalar(0);
			hitNormal=normal;
			
			result=true;
			}
		}
	else if(c0c1n>Scalar(0))
		{
		/* Find the first intersection: */
		Scalar lambda=(radius*normal.mag()-cc0n)/c0c1n;
		
		/* Check whether this plane will affect the collision: */
		if(lambda>=Scalar(0))
			{
			if(hitLambda>lambda)
				{
				/* Update the hit result: */
				hitLambda=lambda;
				hitNormal=-normal;
				
				result=true;
				}
			}
		else if(cc0n<Scalar(0)&&hitLambda>Scalar(0))
			{
			/* Sphere already penetrates the plane; prevent it from getting worse: */
			hitLambda=Scalar(0);
			hitNormal=-normal;
			
			result=true;
			}
		}
	
	return result;
	}

bool SphereCollisionQuery::testEdgeAndUpdate(const CollisionEdge& edge)
	{
	bool result=false;
	
	Vector v0c0=c0-edge.center; // Vector from edge's center point to sphere's starting point
	Vector v0c0xv0v1=v0c0^edge.axis;
	Vector c0c1xv0v1=c0c1^edge.axis;
	
	Scalar bh=v0c0xv0v1*c0c1xv0v1; // Halved linear coefficient from quadratic formula
	if(bh<Scalar(0))
		{
		Scalar a=c0c1xv0v1.sqr(); // Quadratic coefficient from quadratic formula
		Scalar c=v0c0xv0v1.sqr()-radiusSqr*edge.axisSqr; // Constant coefficient from quadratic formula
		
		Scalar discq=bh*bh-a*c; // Quarter discriminant from quadratic formula
		if(discq>=Scalar(0))
			{
			/* Find the quadratic equation's smaller root: */
			// Scalar lambda=bh>=Scalar(0)?(-bh-Math::sqrt(discq))/a:c/(-bh+Math::sqrt(discq)); // Slightly more stable formula
			Scalar lambda=c/(-bh+Math::sqrt(discq)); // Stable formulation for negative bh, which is the only bh that counts
			
			/* Check whether this edge will affect the collision: */
			if(lambda>=Scalar(0))
				{
				if(hitLambda>lambda)
					{
					/* Check whether the contact point is inside the edge's extents: */
					Vector cv=Geometry::addScaled(v0c0,c0c1,lambda);
					Scalar mu=cv*edge.axis;
					if(Math::abs(mu)*Scalar(2)<=edge.axisSqr)
						{
						/* Update the hit result: */
						hitLambda=lambda;
						hitNormal=Geometry::subtractScaled(cv,edge.axis,mu/edge.axisSqr);
						
						result=true;
						}
					}
				}
			else if(c<Scalar(0)&&hitLambda>Scalar(0))
				{
				/* Check whether the sphere's starting point is inside the edge's extents: */
				Scalar mu=v0c0*edge.axis;
				if(Math::abs(mu)*Scalar(2)<=edge.axisSqr)
					{
					/* Sphere already penetrates the edge; prevent it from getting worse: */
					hitLambda=Scalar(0);
					hitNormal=Geometry::subtractScaled(v0c0,edge.axis,mu/edge.axisSqr);
					
					result=true;
					}
				}
			}
		}
	
	return result;
	}

bool SphereCollisionQuery::testTriangleAndUpdate(const CollisionTriangle& triangle)
	{
	bool result=false;
	
	/* Test the sphere against the triangle's plane: */
	Scalar denominator=c0c1*triangle.normal;
	Scalar offset=(c0-triangle.v[0])*triangle.normal;
	if(denominator<Scalar(0)&&offset>=Scalar(0))
		{
		/* Calculate the intersection of the sphere's path with the triangle's plane: */
		Scalar counter=radius*triangle.normalMag-offset;
		Scalar lambda=counter<Scalar(0)?counter/denominator:Scalar(0); // Take care of the case where the sphere is already penetrating the triangle
		if(hitLambda>lambda)
			{
			/* Calculate the point where the sphere hits the triangle's plane: */
			Point hit3=c0;
			if(lambda>Scalar(0))
				hit3.addScaled(c0c1,lambda).subtractScaled(triangle.normal,radius/triangle.normalMag);
			else
				hit3.subtractScaled(triangle.normal,offset/Math::sqr(triangle.normalMag));
			
			/* Check whether the intersection point is inside the triangle by projecting both to the most parallel primary plane: */
			Scalar hx=hit3[triangle.axes[0]];
			Scalar hy=hit3[triangle.axes[1]];
			int intersects=0;
			int i0=2;
			for(int i1=0;i1<3;i0=i1,++i1)
				{
				Scalar x0=triangle.v[i0][triangle.axes[0]];
				Scalar y0=triangle.v[i0][triangle.axes[1]];
				Scalar x1=triangle.v[i1][triangle.axes[0]];
				Scalar y1=triangle.v[i1][triangle.axes[1]];
				if(y0<=hy&&y1>hy)
					{
					if(x0+(x1-x0)*(hy-y0)/(y1-y0)>=hx)
						++intersects;
					}
				else if(y0>hy&&y1<=hy)
					{
					if(x1+(x0-x1)*(hy-y1)/(y0-y1)>=hx)
						++intersects;
					}
				}
			if(intersects==1)
				{
				/* Update the hit result: */
				hitLambda=lambda;
				hitNormal=triangle.normal;
				
				result=true;
				}
			}
		}
	
	return result;
	}

bool SphereCollisionQuery::testPolygonAndUpdate(const CollisionPolygon& polygon)
	{
	bool result=false;
	
	/* Test the sphere against the polygon's plane: */
	Scalar denominator=c0c1*polygon.normal;
	Scalar offset=(c0-polygon.center)*polygon.normal;
	if(denominator<Scalar(0)&&offset>=Scalar(0))
		{
		/* Calculate the intersection of the sphere's path with the polygon's plane: */
		Scalar counter=radius*polygon.normalMag-offset;
		Scalar lambda=counter<Scalar(0)?counter/denominator:Scalar(0); // Take care of the case where the sphere is already penetrating the polygon
		if(hitLambda>lambda)
			{
			/* Calculate the point where the sphere hits the polygon's plane and project it to 2D: */
			Point hit3=c0;
			if(lambda>Scalar(0))
				hit3.addScaled(c0c1,lambda).subtractScaled(polygon.normal,radius/polygon.normalMag);
			else
				hit3.subtractScaled(polygon.normal,Math::sqr(offset/polygon.normalMag));
			Scalar hx=hit3[polygon.axes[0]];
			Scalar hy=hit3[polygon.axes[1]];
			
			/* Check if the plane hit point is inside the polygon using the even/odd rule: */
			bool inside=false;
			std::vector<int>::const_iterator it0=polygon.end-1;
			Scalar v0x=polygon.vertices[*it0][polygon.axes[0]];
			Scalar v0y=polygon.vertices[*it0][polygon.axes[1]];
			for(std::vector<int>::const_iterator it1=polygon.begin;it1!=polygon.end;it0=it1,++it1)
				{
				Scalar v1x=polygon.vertices[*it1][polygon.axes[0]];
				Scalar v1y=polygon.vertices[*it1][polygon.axes[1]];
				
				/* Check for an edge crossing: */
				bool cross;
				if(v0y<=v1y)
					cross=v0y<=hy&&hy<v1y;
				else
					cross=v1y<=hy&&hy<v0y;
				if(cross)
					{
					/* Calculate the edge intersection point: */
					Scalar w=(hy-v0y)/(v1y-v0y);
					if(v0x+(v1x-v0x)*w>=hx)
						inside=!inside;
					}
				
				/* Go to the next edge: */
				v0x=v1x;
				v0y=v1y;
				}
			
			if(inside)
				{
				/* This is the actual collision: */
				hitLambda=lambda;
				hitNormal=polygon.normal;
				
				result=true;
				}
			else
				{
				/* Test the face's vertices and edges: */
				std::vector<int>::const_iterator it0=polygon.end-1;
				for(std::vector<int>::const_iterator it1=polygon.begin;it1!=polygon.end;it0=it1,++it1)
					{
					/* Test the edge's starting vertex: */
					result=testVertexAndUpdate(polygon.vertices[*it0])||result;
					
					/* Test the edge: */
					result=testEdgeAndUpdate(polygon.vertices[*it0],polygon.vertices[*it1])||result;
					}
				}
			}
		}
	
	return result;
	}

SphereCollisionQuery SphereCollisionQuery::transform(const OGTransform& transform) const
	{
	/* Transform the query parameters by the inverse transformation: */
	SphereCollisionQuery result(transform.inverseTransform(c0),transform.inverseTransform(c0c1),radius/transform.getScaling());
	
	/* Copy the current hit result: */
	result.hitLambda=hitLambda;
	
	return result;
	}

SphereCollisionQuery& SphereCollisionQuery::updateFromTransform(const OGTransform& transform,const SphereCollisionQuery& transformedQuery)
	{
	/* Copy the updated hit result: */
	hitLambda=transformedQuery.hitLambda;
	
	/* Transform the hit normal by the transformation (which is orthogonal, so this is okay): */
	hitNormal=transform.transform(transformedQuery.hitNormal);
	
	return *this;
	}

}

/***********************************************************************
CapsuleCollisionQuery - Class to query the collision of a sliding
capsule (cylinder with hemispherical end caps) with a scene graph.
Copyright (c) 2020 Oliver Kreylos

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

#ifndef SCENEGRAPH_CAPSULECOLLISIONQUERY_INCLUDED
#define SCENEGRAPH_CAPSULECOLLISIONQUERY_INCLUDED

#include <SceneGraph/Geometry.h>

namespace SceneGraph {

class CapsuleCollisionQuery
	{
	/* Elements: */
	private:
	Point c0,c1; // Initial and final center point of capsule
	Vector axis; // Vector from the capsule's center point to the center of the top hemisphere
	Scalar radius; // Capsule's radius
	
	/* Transient processing state: */
	Scalar hitLambda; // Ratio of sliding vector at which the capsule hits geometry
	Vector hitNormal; // Normal vector of hit plane
	
	/* Constructors and destructors: */
	public:
	CapsuleCollisionQuery(const Point& sC0,const Point& sC1,const Vector& sAxis,Scalar sRadius); // Elementwise constructor
	
	/* Methods: */
	bool testVertex(const Point& vertex); // Tests capsule against a vertex; returns true if previous hit result changed
	bool testEdge(const Point& edge0,const Point& edge1); // Tests capsule against an edge; returns true if previous hit result changed
	bool testPlane(const Point& center,const Vector& normal); // Tests capsule against a plane; returns true if previous hit result changed
	CapsuleCollisionQuery transform(const OGTransform& transform) const; // Returns a transformed copy of the collision query
	};

}

#endif

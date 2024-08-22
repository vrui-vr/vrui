/***********************************************************************
GeometryNode - Base class for nodes that define renderable geometry.
Copyright (c) 2009-2022 Oliver Kreylos

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

#ifndef SCENEGRAPH_GEOMETRYNODE_INCLUDED
#define SCENEGRAPH_GEOMETRYNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/Node.h>
#include <SceneGraph/PointTransformNode.h>

/* Forward declarations: */
namespace SceneGraph {
class SphereCollisionQuery;
class GLRenderState;
}

namespace SceneGraph {

class GeometryNode:public Node
	{
	/* Embedded classes: */
	public:
	enum AppearanceRequirementFlags // Enumerated type for geometry components required by an appearance node
		{
		NeedsTexCoords=0x1, // The appearance requires vertex texture coordinates
		NeedsColors=0x2, // The appearance requires vertex colors
		NeedsNormals=0x4 // The appearance requires vertex normal vectors
		};
	
	typedef SF<PointTransformNodePointer> SFPointTransformNode;
	
	/* Elements: */
	static const char* className; // The class's name
	
	/* Fields: */
	SFPointTransformNode pointTransform;
	
	/* Derived state: */
	protected:
	unsigned int numNeedsTexCoords; // Number of appearance nodes that currently require vertex texture coordinates
	unsigned int numNeedsColors; // Number of appearance nodes that currently require vertex colors
	unsigned int numNeedsNormals; // Number of appearance nodes that currently require vertex normals
	
	/* Constructors and destructors: */
	public:
	GeometryNode(void); // Creates an empty geometry node
	
	/* Methods from class Node: */
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* New methods: */
	virtual bool canCollide(void) const =0; // Returns true if the geometry node supports collision detection
	virtual int getGeometryRequirementMask(void) const =0; // Returns the mask of requirements this geometry node has of appearance nodes
	virtual void addAppearanceRequirement(int appearanceRequirementMask); // Adds a mask of appearance requirement flags
	virtual void removeAppearanceRequirement(int appearanceRequirementMask); // Removes a mask of appearance requirement flags
	virtual Box calcBoundingBox(void) const =0; // Returns the bounding box of the geometry defined by the node
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const =0; // Tests the node for collision with a moving sphere
	virtual void glRenderAction(int appearanceRequirementMask,GLRenderState& renderState) const =0; // Renders the geometry defined by the node into the current OpenGL context, using at least the geometry components indicated in the given mask
	};

typedef Misc::Autopointer<GeometryNode> GeometryNodePointer;

}

#endif

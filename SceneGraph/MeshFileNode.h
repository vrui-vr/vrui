/***********************************************************************
MeshFileNode - Meta node class to represent the contents of a mesh file
in one of several supported formats as a sub-scene graph.
Copyright (c) 2018-2023 Oliver Kreylos

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

#ifndef SCENEGRAPH_MESHFILENODE_INCLUDED
#define SCENEGRAPH_MESHFILENODE_INCLUDED

#include <vector>
#include <IO/Directory.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GraphNodeParent.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/MaterialLibraryNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/PointTransformNode.h>

namespace SceneGraph {

class MeshFileNode:public GraphNodeParent
	{
	/* Embedded classes: */
	public:
	typedef SF<AppearanceNodePointer> SFAppearanceNode;
	typedef SF<MaterialLibraryNodePointer> SFMaterialLibraryNode;
	typedef SF<PointTransformNodePointer> SFPointTransformNode;
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	MFString url; //  Name of the mesh file to read
	SFAppearanceNode appearance; // Appearance node to be used for mesh files that don't define their own appearances
	SFBool disableTextures; // Flag to disable texture images when loading a material library
	SFMaterialLibraryNode materialLibrary; // Library of named materials to be used by the mesh file; will override materials in the mesh file if present
	SFPointTransformNode pointTransform; // A non-linear point transformation to apply to all shapes read from the mesh file
	SFBool ccw; // Flag whether the mesh file defines faces in counter-clockwise order
	SFBool convex; // Flag whether all faces defined in the mesh file can assumed to be convex
	SFBool solid; // Flag whether the mesh file defines a solid surfaces whose backfaces are not rendered
	SFFloat pointSize; // Cosmetic point size for rendering points
	SFFloat creaseAngle; // Maximum angle between adjacent faces to create a sharp edge
	
	/* Derived elements: */
	protected:
	bool fromBinary; // Flag if the node was most recently initialized from a binary file
	IO::DirectoryPtr baseDirectory; // Base directory for relative URLs
	std::vector<ShapeNodePointer> shapes; // List of shape nodes read from the mesh file
	
	/* Private methods: */
	void clearShapes(void); // Clears the list of represented shape nodes
	
	/* Constructors and destructors: */
	public:
	MeshFileNode(void); // Creates a default mesh file node
	virtual ~MeshFileNode(void);
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from class GraphNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	
	/* Methods from class GraphNodeParent: */
	virtual void passMaskUpdate(GraphNode& child,PassMask newPassMask);
	
	/* New methods: */
	void addShape(ShapeNode& newShape); // Adds a shape node to the representation
	};

}

#endif

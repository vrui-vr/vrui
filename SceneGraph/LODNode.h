/***********************************************************************
LODNode - Class for group nodes that select between their children based
on distance from the viewpoint.
Copyright (c) 2011-2025 Oliver Kreylos

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

#ifndef SCENEGRAPH_LODNODE_INCLUDED
#define SCENEGRAPH_LODNODE_INCLUDED

#include <vector>
#include <Misc/Autopointer.h>
#include <Geometry/Point.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GraphNodeParent.h>

namespace SceneGraph {

class LODNode:public GraphNodeParent
	{
	/* Embedded classes: */
	public:
	typedef MF<GraphNodePointer> MFGraphNode;
	
	/* Elements: */
	static const char* className; // The class's name
	
	/* Fields: */
	protected:
	MFGraphNode level;
	public:
	SFPoint center;
	MFFloat range;
	
	/* Constructors and destructors: */
	public:
	LODNode(void); // Creates an empty LOD node
	virtual ~LODNode(void);
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual EventOut* getEventOut(const char* fieldName) const;
	virtual EventIn* getEventIn(const char* fieldName);
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from class GraphNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	virtual void alRenderAction(ALRenderState& renderState) const;
	virtual void act(ActState& actState);
	
	/* Methods from class GraphNodeParent: */
	virtual void passMaskUpdate(GraphNode& child,PassMask newPassMask);
	
	/* New methods: */
	const MFGraphNode::ValueList& getLevels(void) const // Returns the vector of level nodes
		{
		return level.getValues();
		}
	void setLevel(int index,GraphNode& node); // Sets the level node for the given index to the given node
	void resetLevel(int index); // Sets the level node for the given index to null
	};

typedef Misc::Autopointer<LODNode> LODNodePointer;

}

#endif

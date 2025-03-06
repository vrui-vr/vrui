/***********************************************************************
GraphNode - Base class for nodes that can be parts of a scene graph.
Copyright (c) 2009-2025 Oliver Kreylos

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

#ifndef SCENEGRAPH_GRAPHNODE_INCLUDED
#define SCENEGRAPH_GRAPHNODE_INCLUDED

#include <Misc/SizedTypes.h>
#include <Misc/Autopointer.h>
#include <Geometry/Box.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/Node.h>

/* Forward declarations: */
namespace SceneGraph {
class SphereCollisionQuery;
class GLRenderState;
class ALRenderState;
class ActState;
class GraphNodeParent;
}

namespace SceneGraph {

class GraphNode:public Node
	{
	/* Embedded classes: */
	private:
	struct ParentLink // Structure linking a graph node to one of its parents
		{
		/* Elements: */
		public:
		GraphNodeParent* parent; // Non-reference counted pointer to the node's parent
		ParentLink* succ; // Pointer to the next parent link in a linked list
		
		/* Constructors and destructors: */
		ParentLink(GraphNodeParent* sParent,ParentLink* sSucc)
			:parent(sParent),succ(sSucc)
			{
			}
		};
	
	public:
	typedef Misc::UInt32 PassMask; // Type to represent bit masks of processing passes
	
	enum Pass // Enumerated type for processing or rendering passes in which a graph node can participate
		{
		CollisionPass=0x1, // Node participates in collision detection
		GLRenderPass=0x2, // Node participates in opaque OpenGL rendering
		GLTransparentRenderPass=0x4, // Node participates in transparent OpenGL rendering
		ALRenderPass=0x8, // Node participates in OpenAL audio rendering
		ActionPass=0x10 // Node wants to execute actions at regular intervals
		};
	
	/* Elements: */
	private:
	ParentLink parents; // Head of the node's linked list of parent pointers; if the node has no parents, the structure's parent pointer is null
	protected:
	PassMask passMask; // Bit mask of processing or rendering passes in which this node participates
	
	/* Protected methods: */
	void setPassMask(PassMask newPassMask); // Helper function to set the pass mask and cascade the change up the scene graph if necessary
	
	/* Constructors and destructors: */
	public:
	GraphNode(void); // Creates a parent-less graph node participating in collision detection and opaque OpenGL rendering
	virtual ~GraphNode(void);
	
	/* New methods: */
	public:
	void addParent(GraphNodeParent& parent); // Adds one occurrence of the given node to this node's parents
	void removeParent(GraphNodeParent& parent); // Removes one occurrence of the given node from this node's parents; throws exception if the given node isn't a parent
	PassMask getPassMask(void) const // Returns this node's pass mask
		{
		return passMask;
		}
	bool participatesInPass(PassMask queryPassMask) const // Returns true if this node participates in any of the passes contained in the given pass mask
		{
		return (passMask&queryPassMask)!=0x0U;
		}
	virtual Box calcBoundingBox(void) const; // Returns the bounding box of the node
	virtual void testCollision(SphereCollisionQuery& collisionQuery) const; // Tests the node for collision with a moving sphere
	virtual void glRenderAction(GLRenderState& renderState) const; // Renders the node into the given OpenGL context
	virtual void alRenderAction(ALRenderState& renderState) const; // Renders the node into the given OpenAL context
	virtual void act(ActState& actState); // Lets the node act in the given action traversal state
	};

typedef Misc::Autopointer<GraphNode> GraphNodePointer;

}

#endif

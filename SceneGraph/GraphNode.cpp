/***********************************************************************
GraphNode - Base class for nodes that can be parts of a scene graph.
Copyright (c) 2021-2025 Oliver Kreylos

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

#include <SceneGraph/GraphNode.h>

#include <Misc/StdError.h>
#include <Geometry/Box.h>
#include <SceneGraph/GraphNodeParent.h>

namespace SceneGraph {

/**************************
Methods of class GraphNode:
**************************/

void GraphNode::setPassMask(GraphNode::PassMask newPassMask)
	{
	/* Check if anything changed and the node has parents: */
	if(passMask!=newPassMask&&parents.parent!=0)
		{
		/* Notify the node's parents of the change: */
		for(ParentLink* plPtr=&parents;plPtr!=0;plPtr=plPtr->succ)
			plPtr->parent->passMaskUpdate(*this,newPassMask);
		}
	
	/* Update the pass mask: */
	passMask=newPassMask;
	}

GraphNode::GraphNode(void)
	:parents(0,0),
	 passMask(CollisionPass|GLRenderPass)
	{
	}

GraphNode::~GraphNode(void)
	{
	/* The parents list should be empty by the time a node is destroyed, but let's make sure: */
	ParentLink* plPtr=parents.succ;
	while(plPtr!=0)
		{
		ParentLink* succ=plPtr->succ;
		delete plPtr;
		plPtr=succ;
		}
	}

void GraphNode::addParent(GraphNodeParent& parent)
	{
	/* Check if the node already has parents: */
	if(parents.parent!=0)
		{
		/* Add the new parent as the second element in the linked list: */
		parents.succ=new ParentLink(&parent,parents.succ);
		}
	else
		{
		/* Set the first parent pointer: */
		parents.parent=&parent;
		}
	}

void GraphNode::removeParent(GraphNodeParent& parent)
	{
	/* Find the first occurrence of the parent node in the parents list: */
	if(parents.parent==&parent)
		{
		/* Check if there is another parent: */
		if(parents.succ!=0)
			{
			/* Replace the first parent pointer with the second in the list, then delete that: */
			ParentLink* succ=parents.succ;
			parents.parent=succ->parent;
			parents.succ=succ->succ;
			delete succ;
			}
		else
			{
			/* Set the list head's parent pointer to null: */
			parents.parent=0;
			}
		}
	else
		{
		/* Search the parents list: */
		ParentLink* pl0=&parents;
		ParentLink* pl1=parents.succ;
		while(pl1!=0&&pl1->parent!=&parent)
			{
			pl0=pl1;
			pl1=pl1->succ;
			}
		if(pl1==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Given node is not a parent of this node");
		
		/* Delete the found link: */
		pl0->succ=pl1->succ;
		delete pl1;
		}
	}

Box GraphNode::calcBoundingBox(void) const
	{
	return Box::empty;
	}

void GraphNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* A node that sets a pass mask and doesn't implement the corresponding method is a bug: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing implementation");
	}

void GraphNode::glRenderAction(GLRenderState& renderState) const
	{
	/* A node that sets a pass mask and doesn't implement the corresponding method is a bug: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing implementation");
	}

void GraphNode::alRenderAction(ALRenderState& renderState) const
	{
	/* A node that sets a pass mask and doesn't implement the corresponding method is a bug: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing implementation");
	}

void GraphNode::act(ActState& actState)
	{
	/* A node that sets a pass mask and doesn't implement the corresponding method is a bug: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing implementation");
	}

}

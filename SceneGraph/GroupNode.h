/***********************************************************************
GroupNode - Base class for nodes that contain child nodes.
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

#ifndef SCENEGRAPH_GROUPNODE_INCLUDED
#define SCENEGRAPH_GROUPNODE_INCLUDED

#include <vector>
#include <Misc/Autopointer.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GraphNodeParent.h>

namespace SceneGraph {

class GroupNode:public GraphNodeParent
	{
	/* Embedded classes: */
	public:
	typedef MF<GraphNodePointer> MFGraphNode;
	typedef MFGraphNode::ValueList ChildList;
	struct TreeNode;
	
	/* Elements: */
	static const char* className; // The class's name
	
	/* Fields: */
	
	/*********************************************************************
	To add or remove children from a group, append pointers to the
	children to the addChildren or removeChildren fields, respectively,
	and then call the update() method. Or call the addChild / removeChild
	methods, respectively, which will update more efficiently.
	*********************************************************************/
	
	MFGraphNode addChildren; // List of children to add on the next update
	MFGraphNode removeChildren; // List of children to remove on the next update
	protected:
	MFGraphNode children; // List of this node's children; null children can be added, but will not be retained
	public:
	SFPoint bboxCenter; // Center of explicit bounding box
	SFSize bboxSize; // Size of explicit bounding box
	
	/* Derived state: */
	protected:
	Box* explicitBoundingBox; // Pointer to the explicit bounding box; null if there is no explicit bounding box
	TreeNode* kdTreeRoot; // Pointer to the root of a collision acceleration tree
	
	/* Private methods: */
	void createKdTree(void); // Creates a collision acceleration tree
	
	/* Constructors and destructors: */
	public:
	GroupNode(void); // Creates an empty group node
	virtual ~GroupNode(void);
	
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
	const ChildList& getChildren(void) const // Returns the list of the group's children
		{
		return children.getValues();
		}
	virtual void addChild(GraphNode& child); // Adds the given child to the group
	virtual void removeChild(GraphNode& child); // Removes the given child from the group
	virtual void removeAllChildren(void); // Removes all children from the group
	};

typedef Misc::Autopointer<GroupNode> GroupNodePointer;

}

#endif

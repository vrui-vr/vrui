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

#include <SceneGraph/GroupNode.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <IO/File.h>
#include <Geometry/Box.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/**********************************
Static elements of class GroupNode:
**********************************/

const char* GroupNode::className="Group";

/**************************
Methods of class GroupNode:
**************************/

GroupNode::GroupNode(void)
	:bboxCenter(Point::origin),
	 bboxSize(Size(-1,-1,-1)),
	 explicitBoundingBox(0)
	{
	/* An empty group node does not participate in any processing: */
	passMask=0x0U;
	}

GroupNode::~GroupNode(void)
	{
	/* Remove this node as a parent of all child nodes: */
	for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		(*chIt)->removeParent(*this);
	
	delete explicitBoundingBox;
	}

const char* GroupNode::getClassName(void) const
	{
	return className;
	}

EventOut* GroupNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"children")==0)
		return makeEventOut(this,children);
	else
		return GraphNode::getEventOut(fieldName);
	}

EventIn* GroupNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"addChildren")==0)
		return makeEventIn(this,addChildren);
	else if(strcmp(fieldName,"removeChildren")==0)
		return makeEventIn(this,removeChildren);
	else if(strcmp(fieldName,"children")==0)
		return makeEventIn(this,children);
	else
		return GraphNode::getEventIn(fieldName);
	}

void GroupNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"children")==0)
		{
		vrmlFile.parseMFNode(children);
		
		/* Remove all null children from the children field: */
		ChildList::iterator ch0It=children.getValues().begin();
		for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			if(*chIt!=0)
				{
				if(ch0It!=chIt)
					*ch0It=*chIt;
				++ch0It;
				}
		if(ch0It!=children.getValues().end())
			children.getValues().erase(ch0It,children.getValues().end());
		
		/* Set this node as a parent of all children and initialize the pass mask based on the new contents of the children field: */
		PassMask newPassMask=0x0U;
		for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			{
			(*chIt)->addParent(*this);
			newPassMask|=(*chIt)->getPassMask();
			}
		setPassMask(newPassMask);
		}
	else if(strcmp(fieldName,"bboxCenter")==0)
		{
		vrmlFile.parseField(bboxCenter);
		}
	else if(strcmp(fieldName,"bboxSize")==0)
		{
		vrmlFile.parseField(bboxSize);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

void GroupNode::update(void)
	{
	/* Process the lists of children to add and children to remove: */
	MFGraphNode::ValueList& c=children.getValues();
	
	/* Keep track of the pass mask: */
	PassMask newPassMask=passMask;
	
	MFGraphNode::ValueList& ac=addChildren.getValues();
	if(!ac.empty())
		{
		for(MFGraphNode::ValueList::iterator acIt=ac.begin();acIt!=ac.end();++acIt)
			if(*acIt!=0)
				{
				/* Check if the child is already in the list: */
				MFGraphNode::ValueList::iterator cIt;
				for(cIt=c.begin();cIt!=c.end()&&*cIt!=*acIt;++cIt)
					;
				if(cIt==c.end())
					{
					/* Add the child to the list: */
					(*acIt)->addParent(*this);
					c.push_back(*acIt);
					
					/* Update the pass mask: */
					newPassMask|=(*acIt)->getPassMask();
					}
				}
		ac.clear();
		}
	
	MFGraphNode::ValueList& rc=removeChildren.getValues();
	if(!rc.empty())
		{
		bool removedAChild=false;
		for(MFGraphNode::ValueList::iterator rcIt=rc.begin();rcIt!=rc.end();++rcIt)
			{
			/* Remove all instances of the child from the list: */
			for(MFGraphNode::ValueList::iterator cIt=c.begin();cIt!=c.end();)
				{
				if(*cIt==*rcIt)
					{
					/* Remove the child: */
					(*cIt)->removeParent(*this);
					c.erase(cIt);
					removedAChild=true;
					}
				else
					++cIt;
				}
			}
		rc.clear();
		
		if(removedAChild)
			{
			/* Need to recalculate the pass mask by querying all remaining children: */
			newPassMask=0x0U;
			for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
				newPassMask|=(*chIt)->getPassMask();
			}
		}
	
	/* Set the new pass mask: */
	setPassMask(newPassMask);
	
	/* Calculate the explicit bounding box, if one is given: */
	if(bboxSize.getValue()[0]>=Scalar(0)&&bboxSize.getValue()[1]>=Scalar(0)&&bboxSize.getValue()[2]>=Scalar(0))
		{
		/* Create a new box if there isn't one yet: */
		if(explicitBoundingBox==0)
			explicitBoundingBox=new Box;
		
		/* Update the box: */
		explicitBoundingBox->min=bboxCenter.getValue();
		explicitBoundingBox->max=bboxCenter.getValue();
		for(int i=0;i<3;++i)
			{
			Scalar hs=Math::div2(bboxSize.getValue()[i]);
			explicitBoundingBox->min[i]-=hs;
			explicitBoundingBox->max[i]+=hs;
			}
		}
	else
		{
		/* Delete an existing box: */
		delete explicitBoundingBox;
		explicitBoundingBox=0;
		}
	}

void GroupNode::read(SceneGraphReader& reader)
	{
	/* Remove this node as a parent from all current child nodes: */
	for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		(*chIt)->removeParent(*this);
	
	/* Read all fields: */
	reader.readMFNode(addChildren);
	reader.readMFNode(removeChildren);
	reader.readMFNode(children);
	
	/* Add this node as a parent to all child nodes and initialize the pass mask based on the new contents of the children field: */
	PassMask newPassMask=0x0U;
	for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		{
		(*chIt)->addParent(*this);
		newPassMask|=(*chIt)->getPassMask();
		}
	
	/* Set the new pass mask: */
	setPassMask(newPassMask);
	
	/* Check if there is an explicit bounding box: */
	if(reader.getFile().read<Misc::UInt8>()!=0)
		{
		/* Read the explicit bounding box's fields: */
		reader.readField(bboxCenter);
		reader.readField(bboxSize);
		}
	else
		{
		/* Invalidate the explicit bounding box: */
		bboxCenter.setValue(Point::origin);
		bboxSize.setValue(Size(-1,-1,-1));
		}
	}

void GroupNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeMFNode(addChildren);
	writer.writeMFNode(removeChildren);
	writer.writeMFNode(children);
	
	/* Check if there is an explicit bounding box and write a flag to the file: */
	writer.getFile().write<Misc::UInt8>(explicitBoundingBox!=0?1:0);
	if(explicitBoundingBox!=0)
		{
		/* Write the explicit bounding box's fields: */
		writer.writeField(bboxCenter);
		writer.writeField(bboxSize);
		}
	}

Box GroupNode::calcBoundingBox(void) const
	{
	/* Return the explicit bounding box if there is one: */
	if(explicitBoundingBox!=0)
		return *explicitBoundingBox;
	else
		{
		/* Calculate the group's bounding box as the union of the children's boxes: */
		Box result=Box::empty;
		for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			result.addBox((*chIt)->calcBoundingBox());
		return result;
		}
	}

void GroupNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Could check here whether the collision query touches the bounding box: */
	// ...
	
	/* Apply the collision query to all child nodes that participate in the collision pass in order: */
	for(ChildList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		if((*chIt)->participatesInPass(CollisionPass))
			(*chIt)->testCollision(collisionQuery);
	}

void GroupNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Call the OpenGL render actions of all child nodes that participate in the current OpenGL rendering pass in order: */
	for(ChildList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		if((*chIt)->participatesInPass(renderState.getRenderPass()))
			(*chIt)->glRenderAction(renderState);
	}

void GroupNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Call the OpenAL render actions of all child nodes that participate in the OpenAL rendering pass in order: */
	for(ChildList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		if((*chIt)->participatesInPass(ALRenderPass))
			(*chIt)->alRenderAction(renderState);
	}

void GroupNode::act(ActState& actState)
	{
	/* Call the act methods of all child nodes that participate in the action pass in order: */
	for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		if((*chIt)->participatesInPass(ActionPass))
			(*chIt)->act(actState);
	}

void GroupNode::passMaskUpdate(GraphNode& child,GraphNode::PassMask newPassMask)
	{
	/* Check if the child only added passes: */
	if((child.getPassMask()&newPassMask)==child.getPassMask())
		{
		/* Augment the current pass mask: */
		setPassMask(passMask|newPassMask);
		}
	else
		{
		/* Add the pass masks of all other children to the child's new pass mask: */
		for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			if(chIt->getPointer()!=&child)
				newPassMask|=(*chIt)->getPassMask();
		setPassMask(newPassMask);
		}
	}

void GroupNode::addChild(GraphNode& child)
	{
	/* Add the child to the children field: */
	child.addParent(*this);
	children.appendValue(&child);
	
	/* Update the pass mask: */
	setPassMask(passMask|child.getPassMask());
	}

void GroupNode::removeChild(GraphNode& child)
	{
	/* Remove the first instance of the child from the children field: */
	if(children.removeFirstValue(&child))
		child.removeParent(*this);
	
	/* Need to recalculate the pass mask by querying all remaining children: */
	PassMask newPassMask=0x0U;
	for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		newPassMask|=(*chIt)->getPassMask();
	setPassMask(newPassMask);
	}

void GroupNode::removeAllChildren(void)
	{
	/* Remove this node as a parent of all child nodes: */
	for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		(*chIt)->removeParent(*this);
	
	/* Clear the children field: */
	children.clearValues();
	
	/* Reset the processing pass mask: */
	setPassMask(0x0U);
	}

}

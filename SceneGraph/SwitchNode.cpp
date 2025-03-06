/***********************************************************************
SwitchNode - Class for group nodes that traverse zero or one of their
children based on a selection field.
Copyright (c) 2018-2025 Oliver Kreylos

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

#include <SceneGraph/SwitchNode.h>

#include <string.h>
#include <Misc/StdError.h>
#include <Geometry/Box.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>

namespace SceneGraph {

/***********************************
Static elements of class SwitchNode:
***********************************/

const char* SwitchNode::className="Switch";

/***************************
Methods of class SwitchNode:
***************************/

SwitchNode::SwitchNode(void)
	:whichChoice(-1)
	{
	}

SwitchNode::~SwitchNode(void)
	{
	/* Remove this node as the parent of all choice nodes: */
	for(MFGraphNode::ValueList::iterator chIt=choice.getValues().begin();chIt!=choice.getValues().end();++chIt)
		(*chIt)->removeParent(*this);
	}

const char* SwitchNode::getClassName(void) const
	{
	return className;
	}

EventOut* SwitchNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"choice")==0)
		return makeEventOut(this,choice);
	else if(strcmp(fieldName,"whichChoice")==0)
		return makeEventOut(this,whichChoice);
	else
		return GraphNode::getEventOut(fieldName);
	}

EventIn* SwitchNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"choice")==0)
		return makeEventIn(this,choice);
	else if(strcmp(fieldName,"whichChoice")==0)
		return makeEventIn(this,whichChoice);
	else
		return GraphNode::getEventIn(fieldName);
	}

void SwitchNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"choice")==0)
		{
		vrmlFile.parseMFNode(choice);
		
		/* Add this node as the parent of all choice nodes: */
		for(MFGraphNode::ValueList::iterator chIt=choice.getValues().begin();chIt!=choice.getValues().end();++chIt)
			(*chIt)->addParent(*this);
		}
	else if(strcmp(fieldName,"whichChoice")==0)
		{
		vrmlFile.parseField(whichChoice);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

void SwitchNode::update(void)
	{
	/* Set this node's pass mask to the current choice's pass mask or nothing if the choice is invalid, or the chosen node is null: */
	PassMask newPassMask=0x0U;
	int wc=whichChoice.getValue();
	if(wc>=0&&wc<int(choice.getNumValues())&&choice.getValue(wc)!=0)
		newPassMask=choice.getValue(wc)->getPassMask();
	setPassMask(newPassMask);
	}

void SwitchNode::read(SceneGraphReader& reader)
	{
	/* Remove this node as the parent of all current choice nodes: */
	for(MFGraphNode::ValueList::iterator chIt=choice.getValues().begin();chIt!=choice.getValues().end();++chIt)
		(*chIt)->removeParent(*this);
	
	/* Read all fields: */
	reader.readMFNode(choice);
	reader.readField(whichChoice);
	
	/* Add this node as the parent of all new choice nodes: */
	for(MFGraphNode::ValueList::iterator chIt=choice.getValues().begin();chIt!=choice.getValues().end();++chIt)
		(*chIt)->addParent(*this);
	}

void SwitchNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeMFNode(choice);
	writer.writeField(whichChoice);
	}

Box SwitchNode::calcBoundingBox(void) const
	{
	/* Calculate the group's bounding box as the union of the choices' boxes: */
	Box result=Box::empty;
	for(MFGraphNode::ValueList::const_iterator cIt=choice.getValues().begin();cIt!=choice.getValues().end();++cIt)
		if(*cIt!=0)
			result.addBox((*cIt)->calcBoundingBox());
	return result;
	}

void SwitchNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Apply the collision query to the current choice (this wouldn't be called if the choice weren't valid or the chosen node were null): */
	choice.getValue(whichChoice.getValue())->testCollision(collisionQuery);
	}

void SwitchNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Call the render action of the current choice (this wouldn't be called if the choice weren't valid or the chosen node were null): */
	choice.getValue(whichChoice.getValue())->glRenderAction(renderState);
	}

void SwitchNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Call the render action of the current choice (this wouldn't be called if the choice weren't valid or the chosen node were null): */
	choice.getValue(whichChoice.getValue())->alRenderAction(renderState);
	}

void SwitchNode::act(ActState& actState)
	{
	/* Call the action method of the current choice (this wouldn't be called if the choice weren't valid or the chosen node were null): */
	choice.getValue(whichChoice.getValue())->act(actState);
	}

void SwitchNode::passMaskUpdate(GraphNode& child,GraphNode::PassMask newPassMask)
	{
	/* Check if the current choice is valid, and the given node is the current choice: */
	int wc=whichChoice.getValue();
	if(wc>=0&&wc<int(choice.getNumValues())&&choice.getValue(wc)==&child)
		{
		/* Set the pass mask to the child's new pass mask: */
		setPassMask(newPassMask);
		}
	}

void SwitchNode::setChoice(int index,GraphNode& node)
	{
	if(index<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid index");
	
	/* Ensure that the choice field has enough entries: */
	MFGraphNode::ValueList& choices=choice.getValues();
	while(index>=int(choices.size()))
		choices.push_back(0);
	
	/* Release the current choice node of the given index: */
	if(choices[index]!=0)
		choices[index]->removeParent(*this);
	
	/* Set the new choice node: */
	choices[index]=&node;
	node.addParent(*this);
	}

void SwitchNode::resetChoice(int index)
	{
	if(index<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid index");
	
	/*Release the choice node if the index is in bounds and the choice node is valid; non-existing choices are implicitly null: */
	MFGraphNode::ValueList& choices=choice.getValues();
	if(index<int(choices.size())&&choices[index]!=0)
		{
		choices[index]->removeParent(*this);
		choices[index]=0;
		}
	}

}

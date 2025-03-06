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

#include <SceneGraph/LODNode.h>

#include <string.h>
#include <Misc/StdError.h>
#include <AL/ALContextData.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/ALRenderState.h>
#include <SceneGraph/ActState.h>

namespace SceneGraph {

/********************************
Static elements of class LODNode:
********************************/

const char* LODNode::className="LOD";

/************************
Methods of class LODNode:
************************/

LODNode::LODNode(void)
	:center(Point::origin)
	{
	/* An empty LOD node does not participate in any processing: */
	passMask=0x0U;
	}

LODNode::~LODNode(void)
	{
	/* Remove this node as a parent of all level nodes: */
	for(MFGraphNode::ValueList::const_iterator lIt=level.getValues().begin();lIt!=level.getValues().end();++lIt)
		if(*lIt!=0)
			(*lIt)->removeParent(*this);
	}

const char* LODNode::getClassName(void) const
	{
	return className;
	}

EventOut* LODNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"level")==0)
		return makeEventOut(this,level);
	else if(strcmp(fieldName,"center")==0)
		return makeEventOut(this,center);
	else if(strcmp(fieldName,"range")==0)
		return makeEventOut(this,range);
	else
		return GraphNode::getEventOut(fieldName);
	}

EventIn* LODNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"level")==0)
		return makeEventIn(this,level);
	else if(strcmp(fieldName,"center")==0)
		return makeEventIn(this,center);
	else if(strcmp(fieldName,"range")==0)
		return makeEventIn(this,range);
	else
		return GraphNode::getEventIn(fieldName);
	}

void LODNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"level")==0)
		{
		vrmlFile.parseMFNode(level);
		
		/* Set this node as a parent of all level nodes: */
		for(MFGraphNode::ValueList::const_iterator lIt=level.getValues().begin();lIt!=level.getValues().end();++lIt)
			if(*lIt!=0)
				(*lIt)->addParent(*this);
		}
	else if(strcmp(fieldName,"center")==0)
		{
		vrmlFile.parseField(center);
		}
	else if(strcmp(fieldName,"range")==0)
		{
		vrmlFile.parseField(range);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

void LODNode::update(void)
	{
	/* Set the pass mask to the union of all levels' pass masks, for lack of a better approach: */
	PassMask newPassMask=0x0U;
	for(MFGraphNode::ValueList::const_iterator lIt=level.getValues().begin();lIt!=level.getValues().end();++lIt)
		if(*lIt!=0)
			newPassMask|=(*lIt)->getPassMask();
	setPassMask(newPassMask);
	}

void LODNode::read(SceneGraphReader& reader)
	{
	/* Remove this node as a parent of all current level nodes: */
	for(MFGraphNode::ValueList::const_iterator lIt=level.getValues().begin();lIt!=level.getValues().end();++lIt)
		if(*lIt!=0)
			(*lIt)->removeParent(*this);
	
	/* Read all fields: */
	reader.readMFNode(level);
	reader.readField(center);
	reader.readField(range);
	
	/* Set this node as a parent of all level nodes: */
	for(MFGraphNode::ValueList::const_iterator lIt=level.getValues().begin();lIt!=level.getValues().end();++lIt)
		if(*lIt!=0)
			(*lIt)->addParent(*this);
	}

void LODNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeMFNode(level);
	writer.writeField(center);
	writer.writeField(range);
	}

Box LODNode::calcBoundingBox(void) const
	{
	/* Calculate the group's bounding box as the union of the levels' boxes, for lack of a better approach: */
	Box result=Box::empty;
	for(MFGraphNode::ValueList::const_iterator lIt=level.getValues().begin();lIt!=level.getValues().end();++lIt)
		if(*lIt!=0)
			result.addBox((*lIt)->calcBoundingBox());
	return result;
	}

void LODNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Bail out if the level list is empty: */
	if(level.getValues().empty())
		return;
	
	/* Calculate the distance to the sphere's starting position: */
	Scalar viewDist2=Geometry::sqrDist(collisionQuery.getC0(),center.getValue());
	
	/* Find the appropriate level to test against: */
	unsigned int l=0;
	unsigned int r=range.getNumValues()+1;
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(Math::sqr(range.getValue(m-1))<=viewDist2)
			l=m;
		else
			r=m;
		}
	
	/* Apply the collision query to the selected level: */
	if(l>level.getNumValues()-1)
		l=level.getNumValues()-1;
	if(level.getValue(l)!=0)
		level.getValue(l)->testCollision(collisionQuery);
	}

void LODNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Bail out if the level list is empty: */
	if(level.getValues().empty())
		return;
	
	/* Calculate the distance to the viewer's position: */
	Scalar viewDist2=Geometry::sqrDist(renderState.getViewerPos(),center.getValue());
	
	/* Find the appropriate level to display: */
	unsigned int l=0;
	unsigned int r=range.getNumValues()+1;
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(Math::sqr(range.getValue(m-1))<=viewDist2)
			l=m;
		else
			r=m;
		}
	
	/* Call the render action of the selected level: */
	if(l>level.getNumValues()-1)
		l=level.getNumValues()-1;
	if(level.getValue(l)!=0)
		level.getValue(l)->glRenderAction(renderState);
	}

void LODNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Bail out if the level list is empty: */
	if(level.getValues().empty())
		return;
	
	/* Calculate the distance to the viewer's position: */
	Scalar viewDist2=Geometry::sqrDist(renderState.getViewerPos(),center.getValue());
	
	/* Find the appropriate level to display: */
	unsigned int l=0;
	unsigned int r=range.getNumValues()+1;
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(Math::sqr(range.getValue(m-1))<=viewDist2)
			l=m;
		else
			r=m;
		}
	
	/* Call the render action of the selected level: */
	if(l>level.getNumValues()-1)
		l=level.getNumValues()-1;
	if(level.getValue(l)!=0)
		level.getValue(l)->alRenderAction(renderState);
	}

void LODNode::act(ActState& actState)
	{
	/* Bail out if the level list is empty: */
	if(level.getValues().empty())
		return;
	
	/* Calculate the distance to the viewer's position: */
	Scalar viewDist2=Geometry::sqrDist(actState.getViewerPos(),center.getValue());
	
	/* Find the appropriate level to display: */
	unsigned int l=0;
	unsigned int r=range.getNumValues()+1;
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(Math::sqr(range.getValue(m-1))<=viewDist2)
			l=m;
		else
			r=m;
		}
	
	/* Call the render action of the selected level: */
	if(l>level.getNumValues()-1)
		l=level.getNumValues()-1;
	if(level.getValue(l)!=0)
		level.getValue(l)->act(actState);
	}

void LODNode::passMaskUpdate(GraphNode& child,GraphNode::PassMask newPassMask)
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
		for(MFGraphNode::ValueList::const_iterator lIt=level.getValues().begin();lIt!=level.getValues().end();++lIt)
			if(*lIt!=0&&lIt->getPointer()!=&child)
				newPassMask|=(*lIt)->getPassMask();
		setPassMask(newPassMask);
		}
	}

void LODNode::setLevel(int index,GraphNode& node)
	{
	if(index<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid index");
	
	/* Ensure that the level field has enough entries: */
	MFGraphNode::ValueList& levels=level.getValues();
	while(index>=int(levels.size()))
		levels.push_back(0);
	
	/* Release the current level node of the given index: */
	if(levels[index]!=0)
		levels[index]->removeParent(*this);
	
	/* Set the new level node: */
	levels[index]=&node;
	node.addParent(*this);
	}

void LODNode::resetLevel(int index)
	{
	if(index<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid index");
	
	/*Release the level node if the index is in bounds and the level node is valid; non-existing levels are implicitly null: */
	MFGraphNode::ValueList& levels=level.getValues();
	if(index<int(levels.size())&&levels[index]!=0)
		{
		levels[index]->removeParent(*this);
		levels[index]=0;
		}
	}

}

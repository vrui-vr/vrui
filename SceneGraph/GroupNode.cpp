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
#include <utility>
#include <algorithm>
#include <Misc/SizedTypes.h>
#include <IO/File.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Box.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>

#define GROUP_USE_KDTREE 0

namespace SceneGraph {

#if GROUP_USE_KDTREE

/****************************************
Declaration of class GroupNode::TreeNode:
****************************************/

struct GroupNode::TreeNode // Structure for interior and exterior kd-tree nodes
	{
	/* Embedded classes: */
	public:
	struct Node // Structure representing one of the group's children
		{
		/* Elements: */
		public:
		GraphNode* node; // Pointer to the group's child node
		unsigned int index; // Index of the child node among the group's children
		};
	
	struct CreatorNode // Structure representing one of the group's children during tree creation
		{
		/* Elements: */
		public:
		GraphNode* node; // Pointer to the group's child node
		unsigned int index; // Index of the child node among the group's children
		Box bbox; // The child node's bounding box
		};
	
	/* Elements: */
	
	/* State for interior nodes: */
	int splitAxis; // Index of primary axis along which this node was split
	Scalar split; // Coordinate of the split plane
	TreeNode* children[2]; // Pointers to the left and right child nodes
	
	/* State for exterior nodes: */
	std::vector<Node> nodes; // List of group's children overlapping this node's domain
	
	/* Constructors and destructors: */
	TreeNode(const Box& bbox,const std::vector<CreatorNode>& sNodes); // Creates a tree node from a list of the group's children overlapping its domain, defined by the given bounding box
	~TreeNode(void);
	
	/* Methods: */
	void testCollision(SphereCollisionQuery& collisionQuery,bool* childMap) const;
	};

/************************************
Methods of class GroupNode::TreeNode:
************************************/

GroupNode::TreeNode::TreeNode(const Box& bbox,const std::vector<GroupNode::TreeNode::CreatorNode>& sNodes)
	:splitAxis(-1)
	{
	/* Sort the primary axes by the extents of the bounding box: */
	int axes[3];
	Scalar sizes[3];
	for(int i=0;i<3;++i)
		{
		axes[i]=i;
		sizes[i]=bbox.max[i]-bbox.min[i];
		}
	for(int i=0;i<2;++i)
		for(int j=i+1;j<3;++j)
			{
			if(sizes[i]<sizes[j])
				{
				std::swap(axes[i],axes[j]);
				std::swap(sizes[i],sizes[j]);
				}
			}
	
	/* Try the split axes in order from biggest to smallest until a possible split is found: */
	for(int axisIndex=0;axisIndex<3;++axisIndex)
		{
		/* Determine the split position that minimizes the maximum number of child nodes on either side of the resulting splitting plane: */
		int a=axes[axisIndex];
		std::vector<Scalar> begins; // List of positions where child node domains begin
		begins.reserve(sNodes.size());
		std::vector<Scalar> ends; // List of positions where child node domains end
		ends.reserve(sNodes.size());
		for(std::vector<CreatorNode>::const_iterator nIt=sNodes.begin();nIt!=sNodes.end();++nIt)
			{
			/* Enter the child node's extents into the respective lists: */
			begins.push_back(nIt->bbox.min[a]);
			ends.push_back(nIt->bbox.max[a]);
			}
		
		/* Sort the beginning and end lists: */
		std::sort(begins.begin(),begins.end());
		std::sort(ends.begin(),ends.end());
		
		/* Put sentinels in the lists: */
		begins.push_back(Math::Constants<Scalar>::infinity);
		ends.push_back(Math::Constants<Scalar>::infinity);
		std::vector<Scalar>::iterator bEnd=begins.end()-1;
		std::vector<Scalar>::iterator eEnd=ends.end()-1;
		
		/* Sweep the lists from left to right using a merge sort approach to keep track of the numbers of left and right child nodes: */
		unsigned int numLeft=0;
		unsigned int numRight=sNodes.size();
		Scalar nextSplit=begins.front();
		unsigned int bestMax=numRight;
		Scalar bestSplit=nextSplit;
		std::vector<Scalar>::iterator bIt=begins.begin();
		std::vector<Scalar>::iterator eIt=ends.begin();
		while(true)
			{
			/* Update the child node counts before increasing the current split position: */
			while(*bIt==nextSplit)
				{
				++numLeft;
				++bIt;
				}
			while(*eIt==nextSplit)
				{
				--numRight;
				++eIt;
				}
			
			/* Bail out if there are no more possible split positions: */
			if(bIt==bEnd&&eIt==eEnd)
				break;
			
			/* Increase the current split position: */
			if(*bIt<=*eIt)
				nextSplit=*bIt;
			else
				nextSplit=*eIt;
			
			/* Check if this is the best split so far: */
			unsigned int max=Math::max(numLeft,numRight);
			if(bestMax>max)
				{
				bestMax=max;
				bestSplit=nextSplit;
				}
			}
		
		/* Bail out if it's a good split: */
		if(bestMax<(sNodes.size()*3)/4)
			{
			// DEBUGGING
			// std::cout<<"Selected split along axis "<<a<<" at "<<bestSplit<<" with "<<bestMax<<" triangles per side"<<std::endl;
			
			splitAxis=a;
			split=bestSplit;
			break;
			}
		}
	
	if(splitAxis>=0)
		{
		/* Create an interior node by splitting the list of child nodes at the split plane: */
		std::vector<CreatorNode> childNodes[2];
		for(std::vector<CreatorNode>::const_iterator nIt=sNodes.begin();nIt!=sNodes.end();++nIt)
			{
			if(nIt->bbox.min[splitAxis]<split)
				{
				CreatorNode cn=*nIt;
				cn.bbox.max[splitAxis]=split;
				childNodes[0].push_back(cn);
				}
			if(nIt->bbox.max[splitAxis]>split)
				{
				CreatorNode cn=*nIt;
				cn.bbox.min[splitAxis]=split;
				childNodes[1].push_back(cn);
				}
			}
		
		Box cBox0=bbox;
		cBox0.max[splitAxis]=split;
		children[0]=new TreeNode(cBox0,childNodes[0]);
		Box cBox1=bbox;
		cBox1.min[splitAxis]=split;
		children[1]=new TreeNode(cBox1,childNodes[1]);
		}
	else
		{
		/* Create an exterior node: */
		children[1]=children[0]=0;
		nodes.reserve(sNodes.size());
		for(std::vector<CreatorNode>::const_iterator nIt=sNodes.begin();nIt!=sNodes.end();++nIt)
			{
			Node n;
			n.node=nIt->node;
			n.index=nIt->index;
			nodes.push_back(n);
			}
		}
	}

GroupNode::TreeNode::~TreeNode(void)
	{
	for(int i=0;i<2;++i)
		delete children[i];
	}

void GroupNode::TreeNode::testCollision(SphereCollisionQuery& collisionQuery,bool* childMap) const
	{
	if(splitAxis>=0)
		{
		/* Check the position of the collision query's starting point against the split plane: */
		Scalar c0=collisionQuery.getC0()[splitAxis];
		Scalar c0c1=collisionQuery.getC0c1()[splitAxis];
		if(c0>=split)
			{
			/* Check in the second child first: */
			children[1]->testCollision(collisionQuery,childMap);
			
			/* Check whether the collision query is moving towards the first child: */
			if(c0c1<Scalar(0))
				{
				/* Check if the collision query still reaches the split plane: */
				if(c0+c0c1*collisionQuery.getHitLambda()-collisionQuery.getRadius()<split)
					children[0]->testCollision(collisionQuery,childMap);
				}
			else
				{
				/* The collision query is moving away from the first child; check if it's intersecting anyway: */
				if(c0-collisionQuery.getRadius()<split)
					children[0]->testCollision(collisionQuery,childMap);
				}
			}
		else
			{
			/* Check in the first child first: */
			children[0]->testCollision(collisionQuery,childMap);
			
			/* Check whether the collision query is moving towards the second child: */
			if(c0c1>Scalar(0))
				{
				/* Check if the collision query still reaches the split plane: */
				if(c0+c0c1*collisionQuery.getHitLambda()+collisionQuery.getRadius()>=split)
					children[1]->testCollision(collisionQuery,childMap);
				}
			else
				{
				/* The collision query is moving away from the second child; check if it's intersecting anyway: */
				if(c0+collisionQuery.getRadius()>=split)
					children[1]->testCollision(collisionQuery,childMap);
				}
			}
		}
	else
		{
		/* Test all child nodes that haven't been tested yet: */
		for(std::vector<Node>::const_iterator nIt=nodes.begin();nIt!=nodes.end();++nIt)
			if(nIt->node->participatesInPass(CollisionPass)&&!childMap[nIt->index])
				{
				/* Check the node for collision: */
				nIt->node->testCollision(collisionQuery);
				
				/* Mark the node as tested: */
				childMap[nIt->index]=true;
				}
		}
	}

#endif

/**********************************
Static elements of class GroupNode:
**********************************/

const char* GroupNode::className="Group";

/**************************
Methods of class GroupNode:
**************************/

void GroupNode::createKdTree(void)
	{
	#if GROUP_USE_KDTREE
	
	/* Remove the current kd-tree: */
	delete kdTreeRoot;
	kdTreeRoot=0;
	
	if(children.getNumValues()>=8) // Some arbitrary cut-off
		{
		/* Create a list of child nodes and their bounding boxes: */
		Box totalBox=Box::empty;
		std::vector<TreeNode::CreatorNode> cNodes;
		cNodes.reserve(children.getNumValues());
		unsigned int index=0;
		for(ChildList::iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt,++index)
			{
			TreeNode::CreatorNode cn;
			cn.node=chIt->getPointer();
			cn.index=index;
			cn.bbox=(*chIt)->calcBoundingBox();
			totalBox.addBox(cn.bbox);
			cNodes.push_back(cn);
			}
		
		/* Create the kd-tree: */
		kdTreeRoot=new TreeNode(totalBox,cNodes);
		}
	
	#endif
	}

GroupNode::GroupNode(void)
	:bboxCenter(Point::origin),
	 bboxSize(Size(-1,-1,-1)),
	 explicitBoundingBox(0),kdTreeRoot(0)
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
	#if GROUP_USE_KDTREE
	delete kdTreeRoot;
	#endif
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
	
	/* Create the acceleration kd-tree: */
	createKdTree();
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
	
	/* Create the acceleration kd-tree: */
	createKdTree();
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
	#if GROUP_USE_KDTREE
	if(kdTreeRoot!=0)
		{
		/* Create a node map: */
		bool* nodeMap=new bool[children.getNumValues()];
		for(unsigned int i=0;i<children.getNumValues();++i)
			nodeMap[i]=false;
		
		/* Traverse the collision acceleration tree: */
		kdTreeRoot->testCollision(collisionQuery,nodeMap);
		
		/* Destroy the node map: */
		delete[] nodeMap;
		}
	else
	#endif
		{
		/* Could check here whether the collision query touches the bounding box: */
		// ...
		
		/* Apply the collision query to all child nodes that participate in the collision pass in order: */
		for(ChildList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			if((*chIt)->participatesInPass(CollisionPass))
				(*chIt)->testCollision(collisionQuery);
		}
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
	
	/* Create the acceleration kd-tree: */
	createKdTree();
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
	
	/* Create the acceleration kd-tree: */
	createKdTree();
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
	
	/* Create the acceleration kd-tree: */
	createKdTree();
	}

}

/***********************************************************************
MeshFileNode - Meta node class to represent the contents of a mesh file
in one of several supported formats as a sub-scene graph.
Copyright (c) 2018-2024 Oliver Kreylos

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

#include <SceneGraph/MeshFileNode.h>

#include <string.h>
#include <Misc/StdError.h>
#include <Misc/VarIntMarshaller.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/Internal/ReadStlFile.h>
#include <SceneGraph/Internal/ReadPlyFile.h>
#include <SceneGraph/Internal/ReadLwoFile.h>
#include <SceneGraph/Internal/ReadObjFile.h>

namespace SceneGraph {

/*************************************
Static elements of class MeshFileNode:
*************************************/

const char* MeshFileNode::className="MeshFile";

/*****************************
Methods of class MeshFileNode:
*****************************/

void MeshFileNode::clearShapes(void)
	{
	/* Remove this node as a parent of all current shape nodes: */
	for(std::vector<ShapeNodePointer>::const_iterator sIt=shapes.begin();sIt!=shapes.end();++sIt)
		(*sIt)->removeParent(*this);
	
	/* Delete the current mesh file representation: */
	shapes.clear();
	}

MeshFileNode::MeshFileNode(void)
	:disableTextures(false),ccw(true),convex(true),solid(true),pointSize(1),
	 fromBinary(false)
	{
	}

MeshFileNode::~MeshFileNode(void)
	{
	/* Delete all current shapes: */
	clearShapes();
	}

const char* MeshFileNode::getClassName(void) const
	{
	return className;
	}

void MeshFileNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		/* Remember the VRML file's base directory: */
		baseDirectory=&vrmlFile.getBaseDirectory();
		}
	else if(strcmp(fieldName,"appearance")==0)
		vrmlFile.parseSFNode(appearance);
	else if(strcmp(fieldName,"disableTextures")==0)
		vrmlFile.parseField(disableTextures);
	else if(strcmp(fieldName,"materialLibrary")==0)
		vrmlFile.parseSFNode(materialLibrary);
	else if(strcmp(fieldName,"pointTransform")==0)
		vrmlFile.parseSFNode(pointTransform);
	else if(strcmp(fieldName,"ccw")==0)
		vrmlFile.parseField(ccw);
	else if(strcmp(fieldName,"convex")==0)
		vrmlFile.parseField(convex);
	else if(strcmp(fieldName,"solid")==0)
		vrmlFile.parseField(solid);
	else if(strcmp(fieldName,"creaseAngle")==0)
		vrmlFile.parseField(creaseAngle);
	else if(strcmp(fieldName,"pointSize")==0)
		vrmlFile.parseField(pointSize);
	else
		GraphNode::parseField(fieldName,vrmlFile);
	
	fromBinary=false;
	}

void MeshFileNode::update(void)
	{
	if(!fromBinary)
		{
		/* Delete all current shapes: */
		clearShapes();
		
		/* Do nothing if there is no mesh file name: */
		if(!url.getValues().empty())
			{
			/* Determine the mesh file's format by inspecting its file name extension: */
			std::string::const_iterator endIt=url.getValue(0).end();
			
			/* Check for and ignore a .gz file name extension: */
			if(endIt-url.getValue(0).begin()>=3&&strncasecmp(&*(endIt-3),".gz",3)==0)
				endIt-=3;
			
			/* Find the model file name's real extension: */
			std::string::const_iterator extIt;
			for(extIt=endIt;extIt>=url.getValue(0).begin()&&*extIt!='.';--extIt)
				;
			
			/* Read a mesh file: */
			if(extIt>url.getValue(0).begin()&&strncasecmp(&*extIt,".stl",endIt-extIt)==0)
				readStlFile(*baseDirectory,url.getValue(0),*this);
			else if(extIt>url.getValue(0).begin()&&strncasecmp(&*extIt,".ply",endIt-extIt)==0)
				readPlyFile(*baseDirectory,url.getValue(0),*this);
			else if(extIt>url.getValue(0).begin()&&strncasecmp(&*extIt,".lwo",endIt-extIt)==0)
				readLwoFile(*baseDirectory,url.getValue(0),*this);
			else if(extIt>url.getValue(0).begin()&&strncasecmp(&*extIt,".obj",endIt-extIt)==0)
				readObjFile(*baseDirectory,url.getValue(0),*this);
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mesh file %s has unknown format",url.getValue(0).c_str());
			}
		}
	
	/* Add this node as a parent of all shape nodes and calculate this node's pass mask as the union of the represented shapes' pass masks: */
	PassMask newPassMask=0x0U;
	for(std::vector<ShapeNodePointer>::iterator sIt=shapes.begin();sIt!=shapes.end();++sIt)
		{
		(*sIt)->addParent(*this);
		newPassMask|=(*sIt)->getPassMask();
		}
	setPassMask(newPassMask);
	}

void MeshFileNode::read(SceneGraphReader& reader)
	{
	/*********************************************************************
	Don't read the node's fields; read the created shape nodes instead.
	*********************************************************************/
	
	url.clearValues();
	
	/* Delete all current shapes: */
	clearShapes();
	
	/* Read the number of shape nodes followed by the shape nodes: */
	unsigned int numShapes=Misc::readVarInt32(reader.getFile());
	shapes.reserve(numShapes);
	for(unsigned int i=0;i<numShapes;++i)
		shapes.push_back(reader.readTypedNode<ShapeNode>());
	
	fromBinary=true;
	}

void MeshFileNode::write(SceneGraphWriter& writer) const
	{
	/*********************************************************************
	Don't write the node's fields; write the created shape nodes instead.
	*********************************************************************/
	
	/* Write the number of shape nodes followed by the shape nodes: */
	Misc::writeVarInt32(shapes.size(),writer.getFile());
	for(std::vector<ShapeNodePointer>::const_iterator sIt=shapes.begin();sIt!=shapes.end();++sIt)
		writer.writeNode(sIt->getPointer());
	}

Box MeshFileNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	/* Return the union of all represented shapes' bounding boxes: */
	for(std::vector<ShapeNodePointer>::const_iterator sIt=shapes.begin();sIt!=shapes.end();++sIt)
		result.addBox((*sIt)->calcBoundingBox());
	
	return result;
	}

void MeshFileNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Apply the collision query to all represented shapes in order: */
	for(std::vector<ShapeNodePointer>::const_iterator sIt=shapes.begin();sIt!=shapes.end();++sIt)
		(*sIt)->testCollision(collisionQuery);
	}

void MeshFileNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Render all represented shapes: */
	for(std::vector<ShapeNodePointer>::const_iterator sIt=shapes.begin();sIt!=shapes.end();++sIt)
		(*sIt)->glRenderAction(renderState);
	}

void MeshFileNode::passMaskUpdate(GraphNode& child,PassMask newPassMask)
	{
	/* No need to do anything here */
	}

void MeshFileNode::addShape(ShapeNode& newShape)
	{
	/* Store the new shape: */
	shapes.push_back(&newShape);
	}

}

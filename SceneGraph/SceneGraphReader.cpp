/***********************************************************************
SceneGraphReader - Class to read a complete scene graph from a binary
file for compact storage or network transmission.
Copyright (c) 2021-2024 Oliver Kreylos

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

#include <SceneGraph/SceneGraphReader.h>

#include <string.h>
#include <Misc/SizedTypes.h>
#include <Misc/VarIntMarshaller.h>
#include <Misc/Marshaller.h>
#include <Misc/StandardMarshallers.h>
#include <Geometry/GeometryMarshallers.h>
#include <GL/GLMarshallers.h>
#include <SceneGraph/Node.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/Internal/SceneGraphFile.h>
#include <SceneGraph/SceneGraphReader.icpp>

namespace SceneGraph {

/*********************************
Methods of class SceneGraphReader:
*********************************/

SceneGraphReader::SceneGraphReader(IO::FilePtr sSourceFile,NodeCreator& sNodeCreator)
	:sourceFile(sSourceFile),nodeCreator(sNodeCreator)
	{
	/* Read and check the scene graph file header string: */
	char header[SceneGraphFile::headerSize];
	sourceFile->read(header,SceneGraphFile::headerSize);
	if(memcmp(header,SceneGraphFile::headerString,SceneGraphFile::headerSize)!=0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"File is not a scene graph file");
	
	/* Read and check the major version number: */
	majorVersion=sourceFile->read<Misc::UInt16>();
	if(majorVersion!=SceneGraphFile::majorVersion)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scene graph file is wrong version");
	
	/* Read the minor version number: */
	minorVersion=sourceFile->read<Misc::UInt16>();
	}

SceneGraphReader::~SceneGraphReader(void)
	{
	}

Node* SceneGraphReader::readNode(void)
	{
	Node* result=0;
	
	/* Read the next node's combined type ID / node index: */
	unsigned int nodeIndex=Misc::readVarInt32(*sourceFile);
	
	if(nodeIndex>=nodeCreator.getNumNodeTypes())
		{
		/* It's a node that has appeared in the file before: */
		result=nodes[nodeIndex-nodeCreator.getNumNodeTypes()].getPointer();
		}
	else if(nodeIndex!=0)
		{
		/* Create a new node from the file: */
		result=nodeCreator.createNode(nodeIndex);
		result->read(*this);
		result->update();
		
		/* Store the new node in the node array: */
		nodes.push_back(result);
		}
	
	return result;
	}

/************************************************
Force instantiation of standard template methods:
************************************************/

template void SceneGraphReader::readField<bool>(SFBool&);
template void SceneGraphReader::readField<std::string>(SFString&);
template void SceneGraphReader::readField<Time>(SFTime&);
template void SceneGraphReader::readField<int>(SFInt&);
template void SceneGraphReader::readField<float>(SFFloat&);
template void SceneGraphReader::readField<Size>(SFSize&);
template void SceneGraphReader::readField<Point>(SFPoint&);
template void SceneGraphReader::readField<Vector>(SFVector&);
template void SceneGraphReader::readField<Rotation>(SFRotation&);
template void SceneGraphReader::readField<Color>(SFColor&);
template void SceneGraphReader::readField<TexCoord>(SFTexCoord&);
template void SceneGraphReader::readField<bool>(MFBool&);
template void SceneGraphReader::readField<std::string>(MFString&);
template void SceneGraphReader::readField<Time>(MFTime&);
template void SceneGraphReader::readField<int>(MFInt&);
template void SceneGraphReader::readField<float>(MFFloat&);
template void SceneGraphReader::readField<Size>(MFSize&);
template void SceneGraphReader::readField<Point>(MFPoint&);
template void SceneGraphReader::readField<Vector>(MFVector&);
template void SceneGraphReader::readField<Rotation>(MFRotation&);
template void SceneGraphReader::readField<Color>(MFColor&);
template void SceneGraphReader::readField<TexCoord>(MFTexCoord&);

}

/***********************************************************************
SceneGraphWriter - Class to write a complete scene graph to a binary
file for compact storage or network transmission.
Copyright (c) 2021 Oliver Kreylos

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

#include <SceneGraph/SceneGraphWriter.h>

#include <Misc/SizedTypes.h>
#include <Misc/VarIntMarshaller.h>
#include <Misc/Marshaller.h>
#include <Misc/StandardMarshallers.h>
#include <Geometry/GeometryMarshallers.h>
#include <GL/GLMarshallers.h>
#include <SceneGraph/Node.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/Internal/SceneGraphFile.h>
#include <SceneGraph/SceneGraphWriter.icpp>

namespace SceneGraph {

/*********************************
Methods of class SceneGraphWriter:
*********************************/

SceneGraphWriter::SceneGraphWriter(IO::FilePtr sDestFile,NodeCreator& sNodeCreator)
	:destFile(sDestFile),nodeCreator(sNodeCreator),
	 nextNodeIndex(0),nodeIndexMap(17)
	{
	/* Write the scene graph file header: */
	destFile->write(SceneGraphFile::headerString,SceneGraphFile::headerSize);
	destFile->write(Misc::UInt16(SceneGraphFile::majorVersion));
	destFile->write(Misc::UInt16(SceneGraphFile::minorVersion));
	}

SceneGraphWriter::~SceneGraphWriter(void)
	{
	}

void SceneGraphWriter::writeNode(const Node* node)
	{
	if(node!=0)
		{
		/* Check if this node has been written before: */
		NodeIndexMap::Iterator niIt=nodeIndexMap.findEntry(node);
		if(niIt.isFinished())
			{
			/* Write the node's type ID: */
			Misc::writeVarInt32(nodeCreator.getNodeTypeId(node),*destFile);
			
			/* Let the node write its own representation: */
			node->write(*this);
			
			/* Remember the node's index in the file and increase the index: */
			nodeIndexMap.setEntry(NodeIndexMap::Entry(node,nextNodeIndex));
			++nextNodeIndex;
			}
		else
			{
			/* Write the index at which the node first appeared in the file, plus the number of node types to disambiguate indices from node type IDs: */
			Misc::writeVarInt32(niIt->getDest()+nodeCreator.getNumNodeTypes(),*destFile);
			}
		}
	else
		{
		/* Write the ID for NULL nodes: */
		Misc::writeVarInt32(0,*destFile);
		}
	}

/************************************************
Force instantiation of standard template methods:
************************************************/

template void SceneGraphWriter::writeField<bool>(const SFBool&);
template void SceneGraphWriter::writeField<std::string>(const SFString&);
template void SceneGraphWriter::writeField<Time>(const SFTime&);
template void SceneGraphWriter::writeField<int>(const SFInt&);
template void SceneGraphWriter::writeField<float>(const SFFloat&);
template void SceneGraphWriter::writeField<Size>(const SFSize&);
template void SceneGraphWriter::writeField<Point>(const SFPoint&);
template void SceneGraphWriter::writeField<Vector>(const SFVector&);
template void SceneGraphWriter::writeField<Rotation>(const SFRotation&);
template void SceneGraphWriter::writeField<Color>(const SFColor&);
template void SceneGraphWriter::writeField<TexCoord>(const SFTexCoord&);
template void SceneGraphWriter::writeField<bool>(const MFBool&);
template void SceneGraphWriter::writeField<std::string>(const MFString&);
template void SceneGraphWriter::writeField<Time>(const MFTime&);
template void SceneGraphWriter::writeField<int>(const MFInt&);
template void SceneGraphWriter::writeField<float>(const MFFloat&);
template void SceneGraphWriter::writeField<Size>(const MFSize&);
template void SceneGraphWriter::writeField<Point>(const MFPoint&);
template void SceneGraphWriter::writeField<Vector>(const MFVector&);
template void SceneGraphWriter::writeField<Rotation>(const MFRotation&);
template void SceneGraphWriter::writeField<Color>(const MFColor&);
template void SceneGraphWriter::writeField<TexCoord>(const MFTexCoord&);

}

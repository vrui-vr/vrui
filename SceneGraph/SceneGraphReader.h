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

#ifndef SCENEGRAPH_SCENEGRAPHREADER_INCLUDED
#define SCENEGRAPH_SCENEGRAPHREADER_INCLUDED

#include <Misc/Autopointer.h>
#include <Misc/StdError.h>
#include <Misc/VarIntMarshaller.h>
#include <IO/File.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/Node.h>

/* Forward declarations: */
namespace SceneGraph {
class NodeCreator;
}

namespace SceneGraph {

class SceneGraphReader
	{
	/* Elements: */
	private:
	IO::FilePtr sourceFile; // Pointer to source file from which to read the scene graph
	unsigned int majorVersion,minorVersion; // Major and minor scene graph file versions
	NodeCreator& nodeCreator; // Node creator to create nodes based on node type IDs
	std::vector<NodePointer> nodes; // Array of nodes indexed by source file node indices
	
	/* Constructors and destructors: */
	public:
	SceneGraphReader(IO::FilePtr sSourceFile,NodeCreator& sNodeCreator); // Creates a scene graph reader for the given source file and node creator
	~SceneGraphReader(void); // Destroys the scene graph writer
	
	/* Methods: */
	IO::File& getFile(void) // Returns the source file
		{
		return *sourceFile;
		}
	unsigned int getMajorVersion(void) const // Returns the major scene graph file version number
		{
		return majorVersion;
		}
	unsigned int getMinorVersion(void) const // Returns the minor scene graph file version number
		{
		return minorVersion;
		}
	Node* readNode(void); // Reads the next node (which can be NULL) from the file and returns it
	template <class NodeParam>
	NodeParam* readTypedNode(void) // Reads the next node from the source file and casts it to the requested type
		{
		/* Read the node and cast it to the requested type: */
		Node* node=readNode();
		NodeParam* typedNode=dynamic_cast<NodeParam*>(node);
		if(node!=0&&typedNode==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching node type");
		return typedNode;
		}
	
	template <class ValueParam>
	void readField(SF<ValueParam>& field); // Reads the contents of the given single-valued field from the file
	template <class NodeParam>
	void readSFNode(SF<Misc::Autopointer<NodeParam> >& field) // Reads the contents of the given single-node-valued field from the file
		{
		/* Read the next node, cast it to the requested type, and store it in the field: */
		field.setValue(readTypedNode<NodeParam>());
		}
	template <class ValueParam>
	void readField(MF<ValueParam>& field); // Reads the contents of the given multi-valued field from the file
	template <class NodeParam>
	void readMFNode(MF<Misc::Autopointer<NodeParam> >& field) // Reads the contents of the given multi-node-valued field from the file
		{
		/* Read the number of nodes in the field followed by the field's nodes: */
		typename MF<Misc::Autopointer<NodeParam> >::ValueList& values=field.getValues();
		values.clear();
		unsigned int numValues=Misc::readVarInt32(*sourceFile);
		values.reserve(numValues);
		for(unsigned int i=0;i<numValues;++i)
			values.push_back(readTypedNode<NodeParam>());
		}
	
	NodePointer getNode(unsigned int nodeIndex) const // Returns the node of the given index
		{
		return nodes[nodeIndex];
		}
	template <class NodeParam>
	Misc::Autopointer<NodeParam> getTypedNode(unsigned int nodeIndex) const // Returns the node of the given index, cast to the requested type
		{
		/* Read the node and cast it to the requested type: */
		Misc::Autopointer<NodeParam> result(nodes[nodeIndex]);
		if(nodes[nodeIndex]!=0&&result==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching node type");
		return result;
		}
	};

}

#endif

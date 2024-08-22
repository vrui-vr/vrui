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

#ifndef SCENEGRAPH_SCENEGRAPHWRITER_INCLUDED
#define SCENEGRAPH_SCENEGRAPHWRITER_INCLUDED

#include <Misc/Autopointer.h>
#include <Misc/VarIntMarshaller.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <IO/File.h>
#include <SceneGraph/FieldTypes.h>

/* Forward declarations: */
namespace SceneGraph {
class Node;
class NodeCreator;
}

namespace SceneGraph {

class SceneGraphWriter
	{
	/* Embedded classes: */
	private:
	typedef Misc::HashTable<const Node*,unsigned int> NodeIndexMap; // Type for hash tables mapping nodes to their indices in the written file
	
	/* Elements: */
	IO::FilePtr destFile; // Pointer to destination file to which to write the scene graph
	NodeCreator& nodeCreator; // Node creator to query node's type IDs
	unsigned int nextNodeIndex; // Index that will be assigned to the next node written to the file
	NodeIndexMap nodeIndexMap; // Map of indices of written nodes
	
	/* Constructors and destructors: */
	public:
	SceneGraphWriter(IO::FilePtr sDestFile,NodeCreator& sNodeCreator); // Creates a scene graph writer for the given destination file and node creator
	~SceneGraphWriter(void); // Destroys the scene graph writer
	
	/* Methods: */
	IO::File& getFile(void) // Returns the destination file
		{
		return *destFile;
		}
	void writeNode(const Node* node); // Writes the given node, which can be NULL, to the file
	
	template <class ValueParam>
	void writeField(const SF<ValueParam>& field); // Writes the contents of the given single-valued field to the file
	template <class NodeParam>
	void writeSFNode(const SF<Misc::Autopointer<NodeParam> >& field) // Writes the contents of the given single-node-valued field to the file
		{
		/* Write the node referenced by the field: */
		writeNode(field.getValue().getPointer());
		}
	template <class ValueParam>
	void writeField(const MF<ValueParam>& field); // Writes the contents of the given multi-valued field to the file
	template <class NodeParam>
	void writeMFNode(const MF<Misc::Autopointer<NodeParam> >& field) // Writes the contents of the given multi-node-valued field to the file
		{
		/* Write the number of nodes in the field followed by the field's nodes: */
		const typename MF<Misc::Autopointer<NodeParam> >::ValueList& values=field.getValues();
		Misc::writeVarInt32(values.size(),*destFile);
		for(typename MF<Misc::Autopointer<NodeParam> >::ValueList::const_iterator vIt=values.begin();vIt!=values.end();++vIt)
			writeNode(vIt->getPointer());
		}
	
	unsigned int getNodeIndex(const Node* node) const // Returns the index that was assigned to the given node when it was first written to the file
		{
		/* Look up the node in the index map, assuming it's in there: */
		return nodeIndexMap.getEntry(node).getDest();
		}
	};

}

#endif

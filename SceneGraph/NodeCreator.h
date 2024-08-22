/***********************************************************************
NodeCreator - Class to create node objects based on a node type name.
Copyright (c) 2009-2022 Oliver Kreylos

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

#ifndef SCENEGRAPH_NODECREATOR_INCLUDED
#define SCENEGRAPH_NODECREATOR_INCLUDED

#include <string>
#include <vector>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <SceneGraph/Node.h>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace SceneGraph {
class NodeFactory;
}

namespace SceneGraph {

class NodeCreator
	{
	/* Embedded classes: */
	private:
	struct MapEntry // Structure for node factory map entries
		{
		/* Elements: */
		public:
		unsigned int id; // Unique ID for registered factories; 0 is reserved for NULL nodes
		NodeFactory* factory; // Pointer to node factory
		
		/* Constructors and destructors: */
		MapEntry(unsigned int sId,NodeFactory* sFactory) // Element-wise constructor
			:id(sId),factory(sFactory)
			{
			}
		};
	
	typedef Misc::HashTable<std::string,MapEntry> NodeFactoryMap; // Type for hash tables mapping node type names to factory IDs and node factories
	typedef Misc::HashTable<std::string,NodePointer> PrototypeMap; // Type for hash tables mapping prototype names to the scene graphs implementing them
	
	struct PrototypeScope // Structure for prototype definitions for a single VRML file
		{
		/* Elements: */
		public:
		PrototypeMap prototypes; // Map of all prototypes defined in the VRML file
		NodePointer firstPrototype; // First prototype defined in the VRML file (only used by external prototype scopes)
		bool passthrough; // Flag whether this scope allows prototype look-up in higher-level scopes
		
		/* Constructors and destructors: */
		PrototypeScope(bool sPassthrough)
			:prototypes(5),passthrough(sPassthrough)
			{
			}
		PrototypeScope(PrototypeScope&& source) // Move constructor
			:prototypes(std::move(source.prototypes)),
			 firstPrototype(source.firstPrototype),
			 passthrough(source.passthrough)
			{
			}
		PrototypeScope& operator=(PrototypeScope&& source) // Move assignment operator
			{
			if(this!=&source)
				{
				prototypes=std::move(source.prototypes);
				firstPrototype=source.firstPrototype;
				passthrough=source.passthrough;
				}
			
			return *this;
			}
		};
	
	typedef Misc::HashTable<std::string,PrototypeScope> ExternalPrototypeMap; // Type for hash tables from external prototype VRML file URLs to external prototype definitions
	
	/* Elements: */
	unsigned int nextFactoryId; // ID that will be assigned to the next registered node factory
	NodeFactoryMap nodeFactoryMap; // Hash table mapping node type names to node factories
	std::vector<NodeFactory*> nodeFactories; // List of node factories indexed by their factory ID
	ExternalPrototypeMap externalPrototypes; // Map of external prototype definitions loaded from VRML files
	std::vector<PrototypeScope> prototypeScopes; // Stack of prototype scopes for currently open VRML files
	
	/* Private methods: */
	ExternalPrototypeMap::Iterator loadExternalPrototypes(IO::Directory& baseDirectory,const std::string& vrmlFileName); // Loads all prototypes defined in the VRML file of the given name
	
	/* Constructors and destructors: */
	public:
	NodeCreator(void); // Creates a node creator for all standard node types
	~NodeCreator(void); // Destroys the node creator
	
	/* Methods: */
	void registerNodeType(NodeFactory* nodeFactory); // Registers a node factory for nodes of the given type; node creator inherits factory object
	unsigned int getNumNodeTypes(void) const // Returns the number of registered node factories
		{
		return nextFactoryId;
		}
	unsigned int getNodeTypeId(const Node* node) const; // Returns the ID of the type of the given node
	Node* createNode(unsigned int nodeTypeId); // Creates a new node of the type associated with the given ID
	Node* createNode(const char* nodeTypeName); // Creates a new node of the given type
	void startPrototypeScope(bool passthrough); // Starts a new prototype scope for a VRML file; creates pass-through scope if given flag is true
	void definePrototype(const char* name,Node& implementation); // Defines a new prototype implementation in the current scope
	void defineExternalPrototype(IO::Directory& baseDirectory,const char* name,const char* url); // Defines an external prototype
	void closePrototypeScope(void); // Closes the currently active prototype scope
	};

}

#endif


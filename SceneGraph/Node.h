/***********************************************************************
Node - Base class for nodes, i.e., shared elements of rendering or other
state.
Copyright (c) 2009-2023 Oliver Kreylos

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

#ifndef SCENEGRAPH_NODE_INCLUDED
#define SCENEGRAPH_NODE_INCLUDED

#include <stdexcept>
#include <Misc/Autopointer.h>
#include <Threads/RefCounted.h>

/* Forward declarations: */
namespace SceneGraph {
class EventIn;
class EventOut;
class VRMLFile;
class SceneGraphReader;
class SceneGraphWriter;
}

namespace SceneGraph {

class Node:public Threads::RefCounted
	{
	/* Embedded classes: */
	public:
	class FieldError:public std::runtime_error // Error class to signal undefined field or event in/out names
		{
		/* Constructors and destructors: */
		public:
		FieldError(const std::string& errorString);
		};
	
	/* Constructors and destructors: */
	public:
	virtual ~Node(void); // Destroys the node
	
	/* New methods: */
	virtual const char* getClassName(void) const =0; // Returns the class name of a node
	virtual EventOut* getEventOut(const char* fieldName) const; // Returns an event source for the given field
	virtual EventIn* getEventIn(const char* fieldName); // Returns an event sink for the given field
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile); // Sets the value of the given field by reading from the VRML 2.0 file
	virtual void update(void); // Called to update the node's derived state after some of its fields have been changed externally
	virtual void read(SceneGraphReader& reader); // Reads the node's state from a binary file using the given reader object; reader will call update() afterwards
	virtual void write(SceneGraphWriter& writer) const; // Writes the node's state to a binary file using the given writer object
	};

typedef Misc::Autopointer<Node> NodePointer;

}

#endif

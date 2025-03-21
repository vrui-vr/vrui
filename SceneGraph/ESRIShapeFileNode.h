/***********************************************************************
ESRIShapeFileNode - Class to represent an ESRI shape file as a
collection of line sets, point sets, or face sets (each shape file can
only contain a single type of primitives).
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

#ifndef SCENEGRAPH_ESRISHAPEFILENODE_INCLUDED
#define SCENEGRAPH_ESRISHAPEFILENODE_INCLUDED

#include <IO/Directory.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/FontStyleNode.h>

namespace SceneGraph {

class ESRIShapeFileNode:public GroupNode
	{
	/* Embedded classes: */
	public:
	typedef SF<AppearanceNodePointer> SFAppearanceNode;
	typedef SF<FontStyleNodePointer> SFFontStyleNode;
	
	/* Elements: */
	static const char* className; // The class's name
	
	/* Fields: */
	public:
	MFString url; // URL for the ESRI shape file to load
	SFAppearanceNode appearance; // Node defining appearance of geometry loaded from shape file
	SFString labelField; // Name of attribute table field to be used to label shape file records
	SFFontStyleNode fontStyle; // Font style for shape file record labels
	SFBool transformToCartesian; // Flag whether to use the projection defined in the shape file to transform all geometry to Cartesian coordinates
	SFFloat pointSize;
	SFFloat lineWidth;
	
	/* Derived state: */
	protected:
	bool fromBinary; // Flag if the node was most recently initialized from a binary file
	IO::DirectoryPtr baseDirectory; // Base directory for relative URLs
	
	/* Constructors and destructors: */
	public:
	ESRIShapeFileNode(void); // Creates an uninitialized ESRI shape file file node
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	};

}

#endif

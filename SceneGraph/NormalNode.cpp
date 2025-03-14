/***********************************************************************
NormalNode - Class for nodes defining normal vectors.
Copyright (c) 2009-2021 Oliver Kreylos

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

#include <SceneGraph/NormalNode.h>

#include <string.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>

namespace SceneGraph {

/***********************************
Static elements of class NormalNode:
***********************************/

const char* NormalNode::className="Normal";

/***************************
Methods of class NormalNode:
***************************/

NormalNode::NormalNode(void)
	{
	}

const char* NormalNode::getClassName(void) const
	{
	return className;
	}

void NormalNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"vector")==0)
		{
		vrmlFile.parseField(vector);
		}
	else
		Node::parseField(fieldName,vrmlFile);
	}

void NormalNode::read(SceneGraphReader& reader)
	{
	/* Read all fields: */
	reader.readField(vector);
	}

void NormalNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeField(vector);
	}

}

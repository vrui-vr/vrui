/***********************************************************************
TextureCoordinateNode - Class for nodes defining texture coordinates.
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

#include <SceneGraph/TextureCoordinateNode.h>

#include <string.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>

namespace SceneGraph {

/**********************************************
Static elements of class TextureCoordinateNode:
**********************************************/

const char* TextureCoordinateNode::className="TextureCoordinate";

/**************************************
Methods of class TextureCoordinateNode:
**************************************/

TextureCoordinateNode::TextureCoordinateNode(void)
	{
	}

const char* TextureCoordinateNode::getClassName(void) const
	{
	return className;
	}

void TextureCoordinateNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"point")==0)
		{
		vrmlFile.parseField(point);
		}
	else
		Node::parseField(fieldName,vrmlFile);
	}

void TextureCoordinateNode::read(SceneGraphReader& reader)
	{
	/* Read all fields: */
	reader.readField(point);
	}

void TextureCoordinateNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeField(point);
	}

}

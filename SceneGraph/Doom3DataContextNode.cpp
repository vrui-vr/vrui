/***********************************************************************
Doom3DataContextNode - Class for nodes representing a collection of pak
files, textures, and materials to render Doom3 static and animated
models.
Copyright (c) 2010-2023 Oliver Kreylos

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

#include <SceneGraph/Doom3DataContextNode.h>

#include <string.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/Internal/Doom3FileManager.h>
#include <SceneGraph/Internal/Doom3TextureManager.h>
#include <SceneGraph/Internal/Doom3MaterialManager.h>

namespace SceneGraph {

/*********************************************
Static elements of class Doom3DataContextNode:
*********************************************/

const char* Doom3DataContextNode::className="Doom3DataContext";

/*************************************
Methods of class Doom3DataContextNode:
*************************************/

Doom3DataContextNode::Doom3DataContextNode(void)
	:fileManager(0),textureManager(0),materialManager(0)
	{
	}

Doom3DataContextNode::~Doom3DataContextNode(void)
	{
	delete materialManager;
	delete textureManager;
	delete fileManager;
	}

const char* Doom3DataContextNode::getClassName(void) const
	{
	return className;
	}

void Doom3DataContextNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"baseUrl")==0)
		{
		vrmlFile.parseField(baseUrl);
		
		/* Remember the VRML file's base directory: */
		baseDirectory=&vrmlFile.getBaseDirectory();
		}
	else if(strcmp(fieldName,"pakFilePrefix")==0)
		{
		vrmlFile.parseField(pakFilePrefix);
		}
	else
		Node::parseField(fieldName,vrmlFile);
	}

void Doom3DataContextNode::update(void)
	{
	/* Delete all managers: */
	delete materialManager;
	delete textureManager;
	delete fileManager;
	
	/* Create a file manager: */
	fileManager=new Doom3FileManager(baseDirectory->openDirectory(baseUrl.getValue(0).c_str()),pakFilePrefix.getValue().c_str());
	
	/* Create texture and material managers: */
	textureManager=new Doom3TextureManager(*fileManager);
	materialManager=new Doom3MaterialManager(*textureManager);
	}

void Doom3DataContextNode::read(SceneGraphReader& reader)
	{
	/*********************************************************************
	This won't work because the Doom3 data context can't be expected to
	exist, let along in the same absolute paths. We'll 	probably never fix
	this, so there.
	*********************************************************************/
	
	/* Read all fields: */
	reader.readField(baseUrl);
	reader.readField(pakFilePrefix);
	}

void Doom3DataContextNode::write(SceneGraphWriter& writer) const
	{
	/*********************************************************************
	This won't work because the Doom3 data context can't be expected to
	exist on the other side, let along in the same absolute paths. We'll
	probably never fix this, so there.
	*********************************************************************/
	
	/* Write all fields: */
	writer.writeField(baseUrl);
	writer.writeField(pakFilePrefix);
	}

}

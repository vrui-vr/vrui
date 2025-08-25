/***********************************************************************
InlineNode - Class for group nodes that read their children from an
external VRML file.
Copyright (c) 2009-2025 Oliver Kreylos

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

#include <SceneGraph/InlineNode.h>

#include <string.h>
#include <stdexcept>
#include <Misc/FileNameExtensions.h>
#include <Misc/MessageLogger.h>
#include <IO/Directory.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>

namespace SceneGraph {

/***********************************
Static elements of class InlineNode:
***********************************/

const char* InlineNode::className="Inline";

/***************************
Methods of class InlineNode:
***************************/

InlineNode::InlineNode(void)
	{
	}

const char* InlineNode::getClassName(void) const
	{
	return className;
	}

void InlineNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		try
			{
			/* Check if the scene graph file is a binary file or a VRML 2.0 file: */
			if(Misc::hasCaseExtension(url.getValue(0).c_str(),".bwrl"))
				{
				/* Load a binary scene graph file: */
				SceneGraphReader reader(vrmlFile.getBaseDirectory().openFile(url.getValue(0).c_str()),vrmlFile.getNodeCreator());
				addChild(*reader.readTypedNode<SceneGraph::GraphNode>());
				}
			else if(Misc::hasCaseExtension(url.getValue(0).c_str(),".wrl"))
				{
				/* Load a VRML v2.0 scene graph file: */
				VRMLFile externalVrmlFile(vrmlFile.getBaseDirectory(),url.getValue(0),vrmlFile.getNodeCreator());
				externalVrmlFile.parse(*this);
				}
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scene graph file name has unrecognized extension %s",Misc::getExtension(url.getValue(9).c_str()));
			}
		catch(const std::runtime_error& err)
			{
			/* Show an error message and delete all partially-read file contents: */
			Misc::formattedUserError("SceneGraph::InlineNode: Unable to load file %s due to exception %s",url.getValue(0).c_str(),err.what());
			removeAllChildren();
			}
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

void InlineNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GroupNode::read(reader);
	
	/*********************************************************************
	Don't read the URL field; its path probably won't exist anyway.
	Figure out a way to deal with inline  nodes on demand later.
	*********************************************************************/
	
	/* Read all fields: */
	// reader.readField(url);
	}

void InlineNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GroupNode::write(writer);
	
	/*********************************************************************
	Don't write the URL field; its path probably won't exist on the other
	end anyway, and it's private. Figure out a way to deal with inline
	nodes on demand later.
	*********************************************************************/
	
	/* Write all fields: */
	// writer.writeField(url);
	}

}

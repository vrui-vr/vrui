/***********************************************************************
SceneGraphViewer - Vislet class to render a scene graph loaded from one
or more VRML 2.0 files.
Copyright (c) 2009-2022 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Vislets/SceneGraphViewer.h>

#include <string.h>
#include <Misc/FileNameExtensions.h>
#include <IO/OpenFile.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/VRMLFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/VisletManager.h>

namespace Vrui {

namespace Vislets {

/****************************************
Methods of class SceneGraphViewerFactory:
****************************************/

SceneGraphViewerFactory::SceneGraphViewerFactory(VisletManager& visletManager)
	:VisletFactory("SceneGraphViewer",visletManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Set tool class' factory pointer: */
	SceneGraphViewer::factory=this;
	}

SceneGraphViewerFactory::~SceneGraphViewerFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SceneGraphViewer::factory=0;
	}

Vislet* SceneGraphViewerFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new SceneGraphViewer(numArguments,arguments);
	}

void SceneGraphViewerFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveSceneGraphViewerDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createSceneGraphViewerFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SceneGraphViewerFactory* sceneGraphViewerFactory=new SceneGraphViewerFactory(*visletManager);
	
	/* Return factory object: */
	return sceneGraphViewerFactory;
	}

extern "C" void destroySceneGraphViewerFactory(VisletFactory* factory)
	{
	delete factory;
	}

/*****************************************
Static elements of class SceneGraphViewer:
*****************************************/

SceneGraphViewerFactory* SceneGraphViewer::factory=0;

/*********************************
Methods of class SceneGraphViewer:
*********************************/

SceneGraphViewer::SceneGraphViewer(int numArguments,const char* const arguments[])
	:navigational(true)
	{
	/* Create a node creator: */
	SceneGraph::NodeCreator nodeCreator;
	
	/* Create the scene graph's root node: */
	root=new SceneGraph::GroupNode;
	
	/* Load all VRML files from the command line: */
	for(int i=0;i<numArguments;++i)
		{
		if(arguments[i][0]=='-')
			{
			if(strcasecmp(arguments[i]+1,"physical")==0)
				navigational=false;
			}
		else if(Misc::hasCaseExtension(arguments[i],".bwrl"))
			{
			/* Load a scene graph from a binary file: */
			SceneGraph::SceneGraphReader reader(IO::openFile(arguments[i]),nodeCreator);
			SceneGraph::GraphNodePointer node=reader.readTypedNode<SceneGraph::GraphNode>();
			root->addChild(*node);
			}
		else
			{
			/* Load a scene graph from a VRML 2.0 file: */
			SceneGraph::VRMLFile vrmlFile(arguments[i],nodeCreator);
			vrmlFile.parse(*root);
			}
		}
	}

SceneGraphViewer::~SceneGraphViewer(void)
	{
	}

VisletFactory* SceneGraphViewer::getFactory(void) const
	{
	return factory;
	}

void SceneGraphViewer::enable(bool startup)
	{
	/* Add the scene graph to Vrui's central scene graph: */
	getSceneGraphManager()->addNode(navigational,*root);
	
	/* Call the base class method: */
	Vislet::enable(startup);
	}

void SceneGraphViewer::disable(bool shutdown)
	{
	if(!shutdown)
		{
		/* Remove the scene graph from Vrui's central scene graph: */
		getSceneGraphManager()->removeNode(navigational,*root);
		}
	
	/* Call the base class method: */
	Vislet::disable(shutdown);
	}

}

}

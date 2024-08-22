/***********************************************************************
NodeCreator - Class to create node objects based on a node type name.
Copyright (c) 2009-2024 Oliver Kreylos

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

#include <SceneGraph/NodeCreator.h>

#include <stdexcept>
#include <Misc/StdError.h>
#include <SceneGraph/Config.h>
#include <SceneGraph/NodeFactory.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/CollisionNode.h>
#include <SceneGraph/TransformNode.h>
#include <SceneGraph/ONTransformNode.h>
#include <SceneGraph/OGTransformNode.h>
#include <SceneGraph/DOGTransformNode.h>
#include <SceneGraph/BillboardNode.h>
#include <SceneGraph/LODNode.h>
#include <SceneGraph/SwitchNode.h>
#include <SceneGraph/ReferenceEllipsoidNode.h>
#include <SceneGraph/GeodeticToCartesianTransformNode.h>
#include <SceneGraph/InlineNode.h>
#include <SceneGraph/MaterialNode.h>
#include <SceneGraph/ImageTextureNode.h>
#include <SceneGraph/TextureTransformNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/PhongAppearanceNode.h>
#include <SceneGraph/MaterialLibraryNode.h>
#include <SceneGraph/AffinePointTransformNode.h>
#include <SceneGraph/GeodeticToCartesianPointTransformNode.h>
#include <SceneGraph/UTMPointTransformNode.h>
#include <SceneGraph/ImageProjectionNode.h>
#include <SceneGraph/BoxNode.h>
#include <SceneGraph/SphereNode.h>
#include <SceneGraph/ConeNode.h>
#include <SceneGraph/CylinderNode.h>
#include <SceneGraph/TextureCoordinateNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/ColorMapNode.h>
#include <SceneGraph/PointSetNode.h>
#include <SceneGraph/IndexedLineSetNode.h>
#include <SceneGraph/CurveSetNode.h>
#include <SceneGraph/ElevationGridNode.h>
#include <SceneGraph/QuadSetNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/FontStyleNode.h>
#include <SceneGraph/TextNode.h>
#if SCENEGRAPH_CONFIG_HAVE_FANCYTEXT
#include <SceneGraph/FancyFontStyleNode.h>
#include <SceneGraph/FancyTextNode.h>
#endif
#include <SceneGraph/LabelSetNode.h>
#include <SceneGraph/MeshFileNode.h>
#include <SceneGraph/ArcInfoExportFileNode.h>
#include <SceneGraph/ESRIShapeFileNode.h>
#include <SceneGraph/Doom3DataContextNode.h>
#include <SceneGraph/Doom3ModelNode.h>
#include <SceneGraph/Doom3MD5MeshNode.h>
#include <SceneGraph/BubbleNode.h>
#include <SceneGraph/AudioClipNode.h>
#include <SceneGraph/SoundNode.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/****************************
Methods of class NodeCreator:
****************************/

NodeCreator::ExternalPrototypeMap::Iterator NodeCreator::loadExternalPrototypes(IO::Directory& baseDirectory,const std::string& vrmlFileName)
	{
	/* Open the VRML file of the given name: */
	VRMLFile vrmlFile(baseDirectory,vrmlFileName,*this);
	
	/* Create a new root node (will be ignored later): */
	GroupNodePointer root=new GroupNode;
	
	/* Read the contents of the VRML file into the root node: */
	vrmlFile.parse(*root);
	
	/* Add a new external prototype definition: */
	ExternalPrototypeMap::Iterator epIt=externalPrototypes.setAndFindEntry(ExternalPrototypeMap::Entry(vrmlFileName,PrototypeScope(false)));
	
	/* Collect the VRML file's prototype definitions: */
	for(PrototypeMap::Iterator pIt=prototypeScopes.back().prototypes.begin();!pIt.isFinished();++pIt)
		epIt->getDest().prototypes.setEntry(PrototypeMap::Entry(pIt->getSource(),pIt->getDest()));
	epIt->getDest().firstPrototype=prototypeScopes.back().firstPrototype;
	
	return epIt;
	}

NodeCreator::NodeCreator(void)
	:nextFactoryId(1),
	 nodeFactoryMap(61),
	 externalPrototypes(5)
	{
	/* Push the special entry for NULL nodes: */
	nodeFactories.push_back(0);
	
	/* Register the standard node types: */
	registerNodeType(new GenericNodeFactory<GroupNode>());
	registerNodeType(new GenericNodeFactory<CollisionNode>());
	registerNodeType(new GenericNodeFactory<TransformNode>());
	registerNodeType(new GenericNodeFactory<ONTransformNode>());
	registerNodeType(new GenericNodeFactory<OGTransformNode>());
	registerNodeType(new GenericNodeFactory<DOGTransformNode>());
	registerNodeType(new GenericNodeFactory<BillboardNode>());
	registerNodeType(new GenericNodeFactory<LODNode>());
	registerNodeType(new GenericNodeFactory<SwitchNode>());
	registerNodeType(new GenericNodeFactory<ReferenceEllipsoidNode>());
	registerNodeType(new GenericNodeFactory<GeodeticToCartesianTransformNode>());
	registerNodeType(new GenericNodeFactory<InlineNode>());
	registerNodeType(new GenericNodeFactory<MaterialNode>());
	registerNodeType(new GenericNodeFactory<ImageTextureNode>());
	registerNodeType(new GenericNodeFactory<TextureTransformNode>());
	registerNodeType(new GenericNodeFactory<AppearanceNode>());
	registerNodeType(new GenericNodeFactory<PhongAppearanceNode>());
	registerNodeType(new GenericNodeFactory<MaterialLibraryNode>());
	registerNodeType(new GenericNodeFactory<AffinePointTransformNode>());
	registerNodeType(new GenericNodeFactory<GeodeticToCartesianPointTransformNode>());
	registerNodeType(new GenericNodeFactory<UTMPointTransformNode>());
	registerNodeType(new GenericNodeFactory<ImageProjectionNode>());
	registerNodeType(new GenericNodeFactory<BoxNode>());
	registerNodeType(new GenericNodeFactory<SphereNode>());
	registerNodeType(new GenericNodeFactory<ConeNode>());
	registerNodeType(new GenericNodeFactory<CylinderNode>());
	registerNodeType(new GenericNodeFactory<TextureCoordinateNode>());
	registerNodeType(new GenericNodeFactory<ColorNode>());
	registerNodeType(new GenericNodeFactory<NormalNode>());
	registerNodeType(new GenericNodeFactory<CoordinateNode>());
	registerNodeType(new GenericNodeFactory<ColorMapNode>());
	registerNodeType(new GenericNodeFactory<PointSetNode>());
	registerNodeType(new GenericNodeFactory<IndexedLineSetNode>());
	registerNodeType(new GenericNodeFactory<CurveSetNode>());
	registerNodeType(new GenericNodeFactory<ElevationGridNode>());
	registerNodeType(new GenericNodeFactory<QuadSetNode>());
	registerNodeType(new GenericNodeFactory<IndexedFaceSetNode>());
	registerNodeType(new GenericNodeFactory<ShapeNode>());
	registerNodeType(new GenericNodeFactory<FontStyleNode>());
	registerNodeType(new GenericNodeFactory<TextNode>());
	#if SCENEGRAPH_CONFIG_HAVE_FANCYTEXT
	registerNodeType(new GenericNodeFactory<FancyFontStyleNode>());
	registerNodeType(new GenericNodeFactory<FancyTextNode>());
	#endif
	registerNodeType(new GenericNodeFactory<LabelSetNode>());
	registerNodeType(new GenericNodeFactory<MeshFileNode>());
	registerNodeType(new GenericNodeFactory<ArcInfoExportFileNode>());
	registerNodeType(new GenericNodeFactory<ESRIShapeFileNode>());
	registerNodeType(new GenericNodeFactory<Doom3DataContextNode>());
	registerNodeType(new GenericNodeFactory<Doom3ModelNode>());
	registerNodeType(new GenericNodeFactory<Doom3MD5MeshNode>());
	registerNodeType(new GenericNodeFactory<BubbleNode>());
	registerNodeType(new GenericNodeFactory<AudioClipNode>());
	registerNodeType(new GenericNodeFactory<SoundNode>());
	}

NodeCreator::~NodeCreator(void)
	{
	/* Destroy all node factories: */
	for(std::vector<NodeFactory*>::iterator nfIt=nodeFactories.begin()+1;nfIt!=nodeFactories.end();++nfIt)
		delete *nfIt;
	}

void NodeCreator::registerNodeType(NodeFactory* nodeFactory)
	{
	/* Check if the factory's name has already been used: */
	std::string factoryName(nodeFactory->getClassName());
	if(nodeFactoryMap.isEntry(factoryName))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Node factory %s already registered",factoryName.c_str());
	
	/* Associate the node factory with the next factory ID and store them in the node factory map: */
	nodeFactoryMap.setEntry(NodeFactoryMap::Entry(factoryName,MapEntry(nextFactoryId,nodeFactory)));
	++nextFactoryId;
	
	/* Store the node factory in the node factory array: */
	nodeFactories.push_back(nodeFactory);
	}

unsigned int NodeCreator::getNodeTypeId(const Node* node) const
	{
	unsigned int result=0;
	
	if(node!=0)
		return nodeFactoryMap.getEntry(node->getClassName()).getDest().id;
	
	return result;
	}

Node* NodeCreator::createNode(unsigned int factoryId)
	{
	Node* result=0;
	if(factoryId!=0)
		{
		/* Check if the factory ID is valid: */
		if(factoryId<nodeFactories.size())
			result=nodeFactories[factoryId]->createNode();
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid node factory ID %u",factoryId);
		}
	
	return result;
	}

Node* NodeCreator::createNode(const char* nodeType)
	{
	/* Look for the requested node type in the node type map: */
	NodeFactoryMap::Iterator nfIt=nodeFactoryMap.findEntry(nodeType);
	if(!nfIt.isFinished())
		return nfIt->getDest().factory->createNode();
	
	/* Look for the requested node type in the currently active prototype scope and potentially higher-level scopes: */
	Node* result=0;
	if(!prototypeScopes.empty())
		{
		/* Look in the innermost scope first: */
		std::vector<PrototypeScope>::reverse_iterator psIt=prototypeScopes.rbegin();
		PrototypeMap::Iterator pIt=psIt->prototypes.findEntry(nodeType);
		if(!pIt.isFinished())
			result=pIt->getDest().getPointer();
		
		/* Look in higher-level scopes while no prototype is found: */
		bool passthrough=psIt->passthrough;
		for(++psIt;psIt!=prototypeScopes.rend()&&passthrough&&result==0;++psIt)
			{
			/* Look in the current scope: */
			PrototypeMap::Iterator pIt=psIt->prototypes.findEntry(nodeType);
			if(!pIt.isFinished())
				result=pIt->getDest().getPointer();
			
			passthrough=psIt->passthrough;
			}
		}
	
	return result;
	}

void NodeCreator::startPrototypeScope(bool passthrough)
	{
	/* Start a new prototype scope: */
	prototypeScopes.push_back(PrototypeScope(passthrough));
	}

void NodeCreator::definePrototype(const char* name,Node& implementation)
	{
	if(prototypeScopes.empty())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scope stack is empty");
	
	PrototypeScope& ps=prototypeScopes.back();
	
	/* Do not redefine already defined prototypes: */
	if(!ps.prototypes.isEntry(name))
		{
		/* Store the prototype implementation in the active scope: */
		ps.prototypes.setEntry(PrototypeMap::Entry(name,&implementation));
		if(ps.firstPrototype==0)
			ps.firstPrototype=&implementation;
		}
	}

void NodeCreator::defineExternalPrototype(IO::Directory& baseDirectory,const char* name,const char* url)
	{
	if(prototypeScopes.empty())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scope stack is empty");
	
	/* Split the given URL into VRML file name and prototype name: */
	const char* protoNamePtr=0;
	const char* uPtr;
	for(uPtr=url;*uPtr!='\0';++uPtr)
		if(*uPtr=='#')
			{
			if(protoNamePtr!=0)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"More than one '#' in url %s",url);
			protoNamePtr=uPtr;
			}
	std::string vrmlFileName(url,protoNamePtr!=0?protoNamePtr:uPtr);
	
	Node* implementation=0;
	
	/* Check if the requested VRML file has already been processed: */
	ExternalPrototypeMap::Iterator epIt=externalPrototypes.findEntry(vrmlFileName);
	if(epIt.isFinished())
		{
		/* Load the external prototype VRML file: */
		epIt=loadExternalPrototypes(baseDirectory,vrmlFileName);
		}
	
	/* Check if the URL contained a prototype name: */
	if(protoNamePtr!=0&&protoNamePtr[1]!='\0')
		{
		/* Retrieve the named prototype from the external prototype map: */
		PrototypeMap::Iterator pIt=epIt->getDest().prototypes.findEntry(protoNamePtr+1);
		if(!pIt.isFinished())
			implementation=pIt->getDest().getPointer();
		}
	else
		{
		/* Define the first prototype defined in the VRML file: */
		implementation=epIt->getDest().firstPrototype.getPointer();
		}
	
	/* Check if an implementation was found: */
	if(implementation==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No extermal prototype matches url %s",url);
	
	/* Store the prototype implementation in the active scope: */
	prototypeScopes.back().prototypes.setEntry(PrototypeMap::Entry(name,implementation));
	}

void NodeCreator::closePrototypeScope(void)
	{
	if(prototypeScopes.empty())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scope stack is empty");
	
	/* Close the current prototype scope: */
	prototypeScopes.pop_back();
	}

}

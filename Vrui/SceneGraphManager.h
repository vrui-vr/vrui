/***********************************************************************
SceneGraphManager - Class to manage a scene graph used to represent
renderable objects in physical and navigational space.
Copyright (c) 2021-2025 Oliver Kreylos

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

#ifndef VRUI_SCENEGRAPHMANAGER_INCLUDED
#define VRUI_SCENEGRAPHMANAGER_INCLUDED

#include <string>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/GraphNode.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/ONTransformNode.h>
#include <SceneGraph/DOGTransformNode.h>
#include <SceneGraph/ActState.h>
#include <SceneGraph/GLRenderState.h>
#include <Vrui/Types.h>

/* Forward declarations: */
namespace IO {
class File;
class Directory;
}
namespace SceneGraph {
class SphereCollisionQuery;
class ALRenderState;
}
namespace Vrui {
struct VruiState;
class InputDevice;
class InputGraphManager;
}

namespace Vrui {

class SceneGraphManager
	{
	friend struct VruiState;
	friend class InputGraphManager;
	
	/* Embedded classes: */
	private:
	struct DeviceSceneGraph // Structure associating a scene graph with an input device
		{
		/* Elements: */
		public:
		SceneGraph::ONTransformNodePointer root; // Pointer to the root of the device's scene graph
		bool enabled; // Flag whether the device is currently enabled, i.e., whether its scene graph root is a child of the physical scene graph root
		
		/* Constructors and destructors: */
		DeviceSceneGraph(SceneGraph::ONTransformNode& sRoot,bool sEnabled) // Element-wise constructor
			:root(&sRoot),enabled(sEnabled)
			{
			}
		};
	typedef Misc::HashTable<InputDevice*,DeviceSceneGraph> DeviceSceneGraphMap; // Type for hash tables mapping input device pointers to the scene graph nodes representing them
	
	class ClippedGroup; // Scene graph node class to apply clipping planes to a group of nodes
	
	/* Elements: */
	private:
	SceneGraph::NodeCreator nodeCreator; // A node creator to load scene graphs from VRML 2.0 or binary files
	SceneGraph::GroupNodePointer physicalRoot; // The root of the physical-space scene graph and also the root of the entire scene graph
	SceneGraph::DOGTransformNodePointer navigationalRoot; // The root of the navigational-space scene graph
	SceneGraph::GroupNodePointer clippedRoot; // The root of the clipped navigational-space scene graph
	DeviceSceneGraphMap deviceSceneGraphMap; // Map of scene graphs representing Vrui input devices
	SceneGraph::ActState actState; // Action traversal structure
	
	/* Private methods called by friends: */
	
	/* Methods called by Vrui kernel: */
	void setNavigationTransformation(const NavTransform& newNavigationTransformation); // Sets the navigation transformation
	void updateInputDevices(void); // Notifies the scene graph manager that input devices have (potentially) changed their tracking data
	void glRenderAction(SceneGraph::GLRenderState& renderState) const // Renders the scene graph into the current rendering pass
		{
		if(physicalRoot->participatesInPass(renderState.getRenderPass()))
			physicalRoot->glRenderAction(renderState);
		}
	void alRenderAction(SceneGraph::ALRenderState& renderState) const // Renders the scene graph into the OpenAL audio rendering pass
		{
		if(physicalRoot->participatesInPass(SceneGraph::GraphNode::ALRenderPass))
			physicalRoot->alRenderAction(renderState);
		}
	const SceneGraph::ActState& act(const Point& physViewerPos,const Vector& physUpVector,double time,double nextTime); // Calls the scene graph's action methods for the given time point and next expected frame; returns the persistent action traversal state
	
	/* Methods called by InputGraphManager: */
	void setInputDeviceState(InputDevice* device,bool newEnabled); // Notifies the scene graph manager that the given device changed state
	void removeInputDevice(InputDevice* device); // Notifies the scene graph manager that the given input device is being removed
	
	/* Constructors and destructors: */
	public:
	SceneGraphManager(void); // Creates a manager with empty physical- and navigational scene graphs
	
	/* Methods: */
	SceneGraph::GroupNode& getPhysicalRoot(void) // Returns the root node of the physical-space scene graph
		{
		return *physicalRoot;
		}
	void addPhysicalNode(SceneGraph::GraphNode& node); // Adds the given node to the physical-space scene graph
	void removePhysicalNode(SceneGraph::GraphNode& node); // Removes the given node from the physical-space scene graph
	
	SceneGraph::GroupNode& getNavigationalRoot(void) // Returns the root node of the navigational-space scene graph
		{
		return *navigationalRoot;
		}
	void addNavigationalNode(SceneGraph::GraphNode& node); // Adds the given node to the navigational-space scene graph
	void removeNavigationalNode(SceneGraph::GraphNode& node); // Removes the given node from the navigational-space scene graph
	
	void addUnclippedNavigationalNode(SceneGraph::GraphNode& node); // Adds the given node to the unclipped navigational-space scene graph
	void removeUnclippedNavigationalNode(SceneGraph::GraphNode& node); // Removes the given node from the unclipped navigational-space scene graph
	
	void addNode(bool navigational,SceneGraph::GraphNode& node) // Convenience method to add a node to the navigational- or physical-space scene graph
		{
		/* Add the node to the selected scene graph: */
		if(navigational)
			addNavigationalNode(node);
		else
			addPhysicalNode(node);
		}
	void removeNode(bool navigational,SceneGraph::GraphNode& node) // Convenience method to remove a node from the navigational- or physical-space scene graph
		{
		/* Remove the node from the selected scene graph: */
		if(navigational)
			removeNavigationalNode(node);
		else
			removePhysicalNode(node);
		}
	
	void addDeviceNode(InputDevice* device,SceneGraph::GraphNode& node); // Adds the given node to the scene graph of the given input device
	void removeDeviceNode(InputDevice* device,SceneGraph::GraphNode& node); // Removes the given node from the scene graph of the given input device
	
	/* Support methods: */
	const SceneGraph::NodeCreator& getNodeCreator(void) const // Returns the scene graph node creator
		{
		return nodeCreator;
		}
	SceneGraph::NodeCreator& getNodeCreator(void) // Ditto
		{
		return nodeCreator;
		}
	SceneGraph::GraphNodePointer loadBinarySceneGraph(IO::File& sceneGraphFile); // Loads a scene graph from an already open binary scene graph file; returns loaded scene graph's root node
	SceneGraph::GraphNodePointer loadVRMLSceneGraph(IO::Directory& baseDirectory,const std::string& sourceUrl); // Loads a scene graph from the VRML v2.0 file of the given name inside the given directory; returns loaded scene graph's root node
	SceneGraph::GraphNodePointer loadVRMLSceneGraph(const std::string& sourceUrl); // Loads a scene graph from the VRML v2.0 file of the given name; returns loaded scene graph's root node
	SceneGraph::GraphNodePointer loadSceneGraph(IO::Directory& baseDirectory,const std::string& sourceUrl); // Ditto, but opens file and determines file type based on extension
	SceneGraph::GraphNodePointer loadSceneGraph(const std::string& sourceUrl); // Ditto
	
	/* Query and processing methods: */
	SceneGraph::Box calcPhysicalBoundingBox(void) const // Returns the bounding box of the physical-space scene graph, including the navigational-space scene graph, in physical coordinates
		{
		return physicalRoot->calcBoundingBox();
		}
	SceneGraph::Box calcNavigationalBoundingBox(void) const; // Returns the bounding box of the navigational-space scene graph in navigational coordinates
	void testPhysicalCollision(SceneGraph::SphereCollisionQuery& collisionQuery) const // Tests the given sphere against the physical-space scene graph, including the navigational-space scene graph
		{
		if(physicalRoot->participatesInPass(SceneGraph::GraphNode::CollisionPass))
			physicalRoot->testCollision(collisionQuery);
		}
	void testNavigationalCollision(SceneGraph::SphereCollisionQuery& collisionQuery) const; // Tests the given sphere against the navigational-space scene graph
	};

}

#endif

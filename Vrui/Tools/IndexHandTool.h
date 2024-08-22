/***********************************************************************
IndexHandTool - Tool to attach an animated hand model to a Valve Index
controller and create a virtual input device following the index finger.
Copyright (c) 2021-2023 Oliver Kreylos

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

#ifndef VRUI_INDEXHANDTOOL_INCLUDED
#define VRUI_INDEXHANDTOOL_INCLUDED

#include <string>
#include <Math/Math.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/TransformNode.h>
#include <Vrui/TransformTool.h>

namespace Vrui {

class IndexHandTool;

class IndexHandToolFactory:public ToolFactory
	{
	friend class IndexHandTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Embedded classes: */
		public:
		struct Thumb // Structure representing the configuration of the thumb
			{
			/* Elements: */
			public:
			SceneGraph::Rotation rs[6][3]; // Rotations for the finger's joints in button-touching (0-4) and stretched (5) states
			};
		
		struct Finger // Structure representing the configuration of a finger other than the thumb
			{
			/* Elements: */
			public:
			SceneGraph::Rotation rs[2][3]; // Rotations for the finger's joints in stretched and grabbed states
			SceneGraph::Scalar bendThresholds[2]; // Finger valuator thresholds to activate and deactivate the finger's "grabbed" state
			};
		
		/* Elements: */
		Thumb thumb; // Configuration of the right-hand thumb; left hand is mirrored about x axis
		Finger fingers[4]; // Configuration of the four other right-hand fingers, index (0) to pinky (3); left hand is mirrored about x axis
		SceneGraph::DOGTransform indexTransform; // Transformation from index finger's third joint to index finger device
		ONTransform palmTransform; // Transformation from root device to palm device
		
		/* Constructors and destructors: */
		Configuration(void); // Creates default configuration
		
		/* Methods: */
		void read(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void write(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration configuration; // Default configuration for all tools
	
	/* Constructors and destructors: */
	public:
	IndexHandToolFactory(ToolManager& toolManager);
	virtual ~IndexHandToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual const char* getValuatorFunction(int valuatorSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class IndexHandTool:public TransformTool
	{
	friend class IndexHandToolFactory;
	
	/* Elements: */
	static IndexHandToolFactory* factory; // Pointer to the factory object for this class
	IndexHandToolFactory::Configuration configuration; // Private configuration of this tool
	SceneGraph::Vector fingerDrs[4][3]; // Differential rotation vectors for the fingers' joints
	std::string handModelFileName; // Name of the VRML file containing the hand model
	SceneGraph::TransformNodePointer hand; // Pointer to the hand scene graph's root node
	SceneGraph::TransformNodePointer thumbTransforms[3]; // Pointers to the thumb's three joint nodes
	SceneGraph::TransformNodePointer fingerTransforms[4][3]; // Pointers to the other fingers' three joint nodes
	int thumbButton; // Index of the controller button currently touched by the thumb, or 5 if no button is touched
	unsigned int gestureMask; // Bit mask of fingers currently in the "grabbed" or "touched" state
	TrackerState deviceT; // Current relative transformation from source device to index finger device
	InputDevice* palmDevice; // The palm device
	
	/* Private methods: */
	void updateGesture(unsigned int newGestureMask); // Updates the hand's current gesture
	void updateThumb(void); // Updates the position of the thumb
	void updateFinger(int fingerIndex,SceneGraph::Scalar fingerBend); // Updates the position of the given finger
	
	/* Constructors and destructors: */
	public:
	IndexHandTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~IndexHandTool(void);
	
	/* Methods from class Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData);
	virtual void frame(void);
	
	/* Methods from class DeviceForwarder: */
	virtual std::vector<InputDevice*> getForwardedDevices(void);
	virtual InputDeviceFeatureSet getSourceFeatures(const InputDeviceFeature& forwardedFeature);
	virtual InputDevice* getSourceDevice(const InputDevice* forwardedDevice);
	virtual InputDeviceFeatureSet getForwardedFeatures(const InputDeviceFeature& sourceFeature);
	};

}

#endif

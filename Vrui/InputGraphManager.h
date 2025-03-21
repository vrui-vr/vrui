/***********************************************************************
InputGraphManager - Class to maintain the bipartite input device / tool
graph formed by tools being assigned to input devices, and input devices
in turn being grabbed by tools.
Copyright (c) 2004-2024 Oliver Kreylos

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

#ifndef VRUI_INPUTGRAPHMANAGER_INCLUDED
#define VRUI_INPUTGRAPHMANAGER_INCLUDED

#include <vector>
#include <Misc/HashTable.h>
#include <Misc/CallbackList.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/ONTransformNode.h>
#include <SceneGraph/FontStyleNode.h>
#include <Vrui/Types.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace IO {
class Directory;
}
class GLContextData;
class ALContextData;
namespace Vrui {
class SceneGraphManager;
class VirtualInputDevice;
class Tool;
struct InputGraphManagerToolStackState;
}

namespace Vrui {

class InputGraphManager
	{
	/* Embedded classes: */
	public:
	struct CallbackData:public Misc::CallbackData // Generic callback data for input graph events
		{
		/* Elements: */
		public:
		InputGraphManager* inputGraphManager; // Pointer to the input graph manager
		
		/* Constructors and destructors: */
		CallbackData(InputGraphManager* sInputGraphManager)
			:inputGraphManager(sInputGraphManager)
			{
			}
		};
	
	struct InputDeviceStateChangeCallbackData:public CallbackData // Callback data when an input device changes state
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to the input device that changed state
		bool newEnabled; // The device's new state
		
		/* Constructors and destructors: */
		InputDeviceStateChangeCallbackData(InputGraphManager* sInputGraphManager,InputDevice* sInputDevice,bool sNewEnabled)
			:CallbackData(sInputGraphManager),
			 inputDevice(sInputDevice),newEnabled(sNewEnabled)
			{
			}
		};
	
	struct ToolStateChangeCallbackData:public CallbackData // Callback data when a tool changes state
		{
		/* Elements: */
		public:
		Tool* tool; // Pointer to the tool that changed state
		bool newEnabled; // The tool's new state
		
		/* Constructors and destructors: */
		ToolStateChangeCallbackData(InputGraphManager* sInputGraphManager,Tool* sTool,bool sNewEnabled)
			:CallbackData(sInputGraphManager),
			 tool(sTool),newEnabled(sNewEnabled)
			{
			}
		};
	
	private:
	struct GraphTool // Structure to represent a tool in the input graph
		{
		/* Elements: */
		public:
		Tool* tool; // Pointer to the tool
		int level; // Index of the graph level containing the tool
		GraphTool* levelPred; // Pointer to the previous tool in the same graph level
		GraphTool* levelSucc; // Pointer to the next tool in the same graph level
		bool enabled; // Flag whether this tool is enabled, i.e., receives input data and has frame and display called
		
		/* Constructors and destructors: */
		GraphTool(Tool* sTool,int sLevel); // Creates a graph wrapper for the given tool
		};
	
	struct ToolSlot // Structure to store assignments of input device features to tool input slots
		{
		/* Elements: */
		public:
		InputDeviceFeature feature; // The input device feature managed by this tool slot
		GraphTool* tool; // Pointer to the tool assigned to this feature slot
		bool active; // Flag whether the button or valuator are currently pressed or pushed
		bool preempted; // Flag whether a button press or valuator push event on this slot was preempted
		
		/* Constructors and destructors: */
		ToolSlot(void); // Creates an unassigned tool slot
		~ToolSlot(void); // Removes the slot's callbacks and destroys it
		
		/* Methods: */
		void initialize(InputDevice* sDevice,int sFeatureIndex); // Initializes a slot and installs callbacks
		void inputDeviceButtonCallback(InputDevice::ButtonCallbackData* cbData); // Callback for button events
		void inputDeviceValuatorCallback(InputDevice::ValuatorCallbackData* cbData); // Callback for valuator events
		bool activate(void); // Changes the tool slot's state to active
		bool deactivate(void); // Changes the tool slot's state to inactive
		};
	
	struct GraphInputDevice // Structure to represent an input device in the input graph
		{
		/* Elements: */
		public:
		InputDevice* device; // Pointer to the input device
		Glyph deviceGlyph; // Glyph used to visualize the device's position and orientation
		ToolSlot* toolSlots; // Array of tool slots for this device
		int level; // Index of the graph level containing the input device
		bool navigational; // Flag whether this device, if ungrabbed, follows the navigation transformation
		NavTrackerState fromNavTransform; // Transformation from navigation coordinates to device's coordinates while device is in navigational mode
		GraphInputDevice* levelPred; // Pointer to the previous input device in the same graph level
		GraphInputDevice* levelSucc; // Pointer to the next input device in the same graph level
		GraphTool* grabber; // Pointer to the tool currently holding a grab on the input device
		bool enabled; // Flag whether this input device is enabled, i.e., has valid tracking state and sends feature events
		
		/* Constructors and destructors: */
		GraphInputDevice(InputDevice* sDevice); // Creates a graph wrapper for the given input device
		~GraphInputDevice(void); // Destroys the graph wrapper
		};
	
	typedef Misc::HashTable<const InputDevice*,GraphInputDevice*> DeviceMap; // Hash table to map from input devices to graph input devices
	typedef Misc::HashTable<const Tool*,GraphTool*> ToolMap; // Hash table to map from tools to graph tools
	
	/* Elements: */
	GlyphRenderer* glyphRenderer; // Pointer to the glyph renderer
	SceneGraphManager* sceneGraphManager; // Pointer to the scene graph manager
	VirtualInputDevice* virtualInputDevice; // Pointer to helper class for handling ungrabbed virtual input devices
	SceneGraph::Scalar toolStackFontSize; // Font size to show tool stacks
	SceneGraph::FontStyleNodePointer toolStackSlotFont,toolStackToolFont; // Fonts to show tool stacks
	Misc::CallbackList inputDeviceStateChangeCallbacks; // List of callbacks called when an input device changes state
	Misc::CallbackList toolStateChangeCallbacks; // List of callbacks called when a tool changes state
	GraphTool inputDeviceManager; // A fake graph tool to grab physical input devices
	DeviceMap deviceMap; // Hash table mapping from input devices to graph input devices
	ToolMap toolMap; // Hash table mapping from tools to graph tools
	int maxGraphLevel; // Maximum level in the input graph that has input devices or tools
	std::vector<GraphInputDevice*> deviceLevels; // Vector of pointers to the first input device in each graph level
	std::vector<GraphTool*> toolLevels; // Vector of pointers to the first tool in each graph level
	SceneGraph::ONTransformNodePointer toolStackNode; // Scene graph node displaying an input device feature's tool stack
	InputDeviceFeature toolStackBaseFeature; // Base input device feature for the currently displayed tool stack
	InputDeviceFeature toolDeletionCandidate; // Input device feature currently flagged for tool deletion via the tool kill zone
	
	/* Private methods: */
	void linkInputDevice(GraphInputDevice* gid); // Links a graph input device to its current graph level
	void unlinkInputDevice(GraphInputDevice* gid); // Unlinks a graph input device from its current graph level
	void linkTool(GraphTool* gt); // Links a graph tool to its current graph level
	void unlinkTool(GraphTool* gt); // Unlinks a graph tool from its current graph level
	int calcToolLevel(const Tool* tool) const; // Returns the correct graph level for the given tool
	void growInputGraph(int level); // Grows the input graph to represent the given level
	void shrinkInputGraph(void); // Removes all empty levels from the end of the input graph
	void updateInputGraph(void); // Reorders graph levels after input device grab/release
	SceneGraph::ONTransformNodePointer showToolStack(const ToolSlot& ts,InputGraphManagerToolStackState& tss) const; // Returns a scene graph visualizing the given tool slot's tool stack
	
	/* Constructors and destructors: */
	public:
	InputGraphManager(SceneGraphManager* sSceneGraphManager); // Creates an empty input graph manager using the given glyph renderer and virtual input device
	private:
	InputGraphManager(const InputGraphManager& source); // Prohibit copy constructor
	InputGraphManager& operator=(const InputGraphManager& source); // Prohibit assignment operator
	public:
	~InputGraphManager(void);
	
	/* Methods: */
	void finalize(GlyphRenderer* sGlyphRenderer,VirtualInputDevice* sVirtualInputDevice); // Finishes initializing the input graph manager
	Misc::CallbackList& getInputDeviceStateChangeCallbacks(void) // Returns the list of callbacks called when an input device changes state
		{
		return inputDeviceStateChangeCallbacks;
		}
	Misc::CallbackList& getToolStateChangeCallbacks(void) // Returns the list of callbacks called when a tool changes state
		{
		return toolStateChangeCallbacks;
		}
	void clear(void); // Removes all tools and virtual input devices from the input graph
	void addInputDevice(InputDevice* newDevice); // Adds an ungrabbed input device to the graph
	void removeInputDevice(InputDevice* device); // Removes an input device from the graph
	void addTool(Tool* newTool); // Adds a tool to the input graph, based on its current input assignment
	void removeTool(Tool* tool); // Removes a tool from the input graph
	void loadInputGraph(const Misc::ConfigurationFileSection& configFileSection); // Loads all virtual input devices and tools defined in the given configuration file section
	void loadInputGraph(IO::Directory& directory,const char* configurationFileName,const char* baseSectionName); // Ditto
	void saveInputGraph(IO::Directory& directory,const char* configurationFileName,const char* baseSectionName) const; // Saves the current state of all virtual input devices and assigned tools to the given section in the given configuration file
	bool isNavigational(const InputDevice* device) const // Returns whether the given device will follow navigation coordinates while ungrabbed
		{
		/* Get pointer to the graph input device and return device's navigational flag: */
		return deviceMap.getEntry(device).getDest()->navigational;
		}
	void setNavigational(InputDevice* device,bool newNavigational); // Sets whether the given device will follow navigation coordinates while ungrabbed
	Glyph& getInputDeviceGlyph(InputDevice* device); // Returns the glyph associated with the given input device
	bool isReal(const InputDevice* device) const // Returns true if the given input device is a real device
		{
		/* Get pointer to the graph input device and return true if device is in graph level 0: */
		return deviceMap.getEntry(device).getDest()->level==0;
		}
	bool isGrabbed(const InputDevice* device) const // Returns true if the given input device is currently grabbed by a tool
		{
		/* Get pointer to the graph input device and return true if device has a grabber: */
		return deviceMap.getEntry(device).getDest()->grabber!=0;
		}
	bool isEnabled(const InputDevice* device) const // Returns true if the given input device is currently enabled
		{
		/* Get pointer to the graph input device and return device's enabled flag: */
		return deviceMap.getEntry(device).getDest()->enabled;
		}
	void disable(InputDevice* device); // Disables the given input device; does nothing if device is already disabled
	void enable(InputDevice* device); // Enables the given input device; does nothing if device is already enabled
	void setEnabled(InputDevice* device,bool newEnabled) // Sets the given device's enabled flag
		{
		/* Forward the call to the other methods: */
		if(newEnabled)
			enable(device);
		else
			disable(device);
		}
	InputDevice* getFirstInputDevice(void); // Returns pointer to the first ungrabbed and enabled input device
	InputDevice* getNextInputDevice(InputDevice* device); // Returns pointer to the next enabled input device in the same level after the given one
	InputDevice* findInputDevice(const Point& position,bool ungrabbedOnly =true); // Finds an ungrabbed and enabled input device based on a position in physical coordinates
	InputDevice* findInputDevice(const Ray& ray,bool ungrabbedOnly =true); // Finds an ungrabbed and enabled input device based on a ray in physical coordinates
	bool grabInputDevice(InputDevice* device,Tool* grabber); // Allows a tool (or physical layer if tool==0) to grab an input device; returns true on success
	void releaseInputDevice(InputDevice* device,Tool* grabber); // Allows the current grabber of an input device to release the grab
	InputDevice* getRootDevice(InputDevice* device); // Returns the input device forming the base of the transformation chain containing the given (virtual) input device
	InputDeviceFeature findFirstUnassignedFeature(const InputDeviceFeature& feature) const; // Returns the first unassigned input device feature forwarded from the given feature
	Tool* getFeatureTool(InputDevice* device,int featureIndex); // Returns the tool assigned to the feature of the given index on the given input device, or null if the feature has not assigned tool
	void showToolStack(const InputDeviceFeature& feature); // Displays the stack of tools assigned to the given input device feature
	void update(void); // Updates state of all tools and non-physical input devices in the graph
	void glRenderDevices(GLContextData& contextData) const; // Renders current state of all input devices
	void glRenderTools(GLContextData& contextData) const; // Renders current state of all tools
	void alRenderTools(ALContextData& contextData) const; // Renders current sound state of all tools
	};

}

#endif

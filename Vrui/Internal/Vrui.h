/***********************************************************************
Internal kernel interface of the Vrui virtual reality development
toolkit.
Copyright (c) 2000-2025 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_INCLUDED
#define VRUI_INTERNAL_INCLUDED

#include <vector>
#include <deque>
#include <Misc/Autopointer.h>
#include <Misc/RingBuffer.h>
#include <Misc/PriorityHeap.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <Misc/Timer.h>
#include <Misc/CommandDispatcher.h>
#include <Misc/CallbackList.h>
#include <Realtime/Time.h>
#include <Threads/Mutex.h>
#include <IO/Directory.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/TextField.h>
#include <GLMotif/DropdownBox.h>
#include <GLMotif/TextFieldSlider.h>
#include <GLMotif/HSVColorSelector.h>
#include <GLMotif/FileSelectionHelper.h>
#include <SceneGraph/GraphNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/WindowProperties.h>
#include <Vrui/DisplayState.h>
#include <Vrui/ToolManager.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
class CallbackData;
class TimerEventScheduler;
}
namespace Cluster {
class Multiplexer;
class MulticastPipe;
}
class GLContext;
class GLFont;
namespace GLMotif {
class Container;
class Popup;
class PopupMenu;
class PopupWindow;
class CascadeButton;
}
namespace SceneGraph {
class GLRenderState;
}
namespace Vrui {
class InputDeviceDataSaver;
class MultipipeDispatcher;
class Lightsource;
class ScaleBar;
class VisletManager;
class GUIInteractor;
class ScreenSaverInhibitor;
class ScreenProtectorArea;
}

namespace Vrui {

/********************
Global program state:
********************/

struct VruiState
	{
	/* Embedded classes: */
	public:
	struct ScreenProtectorDevice // Structure describing an input device that needs to be protected from bumping into a screen
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to input device
		Point center; // Center of protective sphere in input device's coordinates
		Scalar radius; // Radius of protective sphere around input device's position
		};
	
	struct HapticDevice // Structure describing an input device with a haptic feature, to check against the tool kill zone
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to input device
		bool inKillZone; // Flag whether the input device is currently inside the tool kill zone
		};
	
	class DisplayStateMapper:public GLObject // Helper class to associate DisplayState objects with each VRWindow's GL context
		{
		/* Embedded classes: */
		public:
		struct DataItem:public GLObject::DataItem
			{
			/* Elements: */
			public:
			DisplayState displayState; // The display state object
			GLuint screenProtectorDisplayListId; // ID of display list to render screen protector grids
			
			/* Constructors and destructors: */
			DataItem(GLContext& context);
			virtual ~DataItem(void);
			};
		
		/* Methods from GLObject: */
		virtual void initContext(GLContextData& contextData) const;
		};
	
	struct FrameCallbackSlot // Structure holding a frame callback
		{
		/* Elements: */
		public:
		FrameCallback callback; // The callback function
		void* userData; // User-specified argument
		};
	
	struct MessageDialog // Structure keeping track of a message dialog that was popped up by showErrorMessage
		{
		/* Elements: */
		public:
		GLMotif::PopupWindow* dialog; // Pointer to the dialog window
		double timeout; // Application time at which the dialog should be closed automatically
		
		/* Constructors and destructors: */
		MessageDialog(GLMotif::PopupWindow* sDialog,double sTimeout) // Elementwise constructor
			:dialog(sDialog),timeout(sTimeout)
			{
			}
		
		/* Methods: */
		static bool lessEqual(const MessageDialog& md1,const MessageDialog& md2) // Comparison function for priority heap
			{
			/* Sort message dialogs by time-out: */
			return md1.timeout<=md2.timeout;
			}
		};
	
	typedef Misc::PriorityHeap<MessageDialog,MessageDialog> MessageDialogHeap; // Type for heaps of message dialogs, sorted by time-out
	
	class ApplicationDisplayFunctionNode:public SceneGraph::GraphNode // Custom scene graph node class to call the application's display function from inside the central scene graph
		{
		/* Elements: */
		public:
		static const char* className;
		private:
		DisplayFunctionType displayFunction; // The display function
		void* displayFunctionData; // An opaque parameter passes to the display function
		
		/* Constructors and destructors: */
		public:
		ApplicationDisplayFunctionNode(DisplayFunctionType sDisplayFunction,void* sDisplayFunctionData);
		
		/* Methods from class SceneGraph::Node: */
		virtual const char* getClassName(void) const;
		
		/* Methods from class SceneGraph::GraphNode: */
		virtual void glRenderAction(SceneGraph::GLRenderState& renderState) const;
		};
	
	/* Elements: */
	
	/* Desktop environment management: */
	ScreenSaverInhibitor* screenSaverInhibitor;
	
	/* Multipipe management: */
	Cluster::Multiplexer* multiplexer;
	bool master;
	Cluster::MulticastPipe* pipe;
	
	/* Random number management: */
	unsigned int randomSeed; // Seed value for random number generator
	
	/* Scene graph management: */
	SceneGraphManager* sceneGraphManager;
	
	/* Input graph management: */
	InputGraphManager* inputGraphManager;
	GLMotif::FileSelectionHelper inputGraphSelectionHelper; // Helper to load and save input graph files
	bool loadInputGraph; // Flag whether to replace the current input graph with one loaded from the given file at the next opportune moment
	std::string inputGraphFileName; // Name of input graph file to load if loadInputGraph is true
	
	/* Input device management: */
	TextEventDispatcher* textEventDispatcher;
	InputDeviceManager* inputDeviceManager;
	MultipipeDispatcher* multipipeDispatcher;
	InputDeviceDataSaver* inputDeviceDataSaver;
	
	/* Definition of physical environment: */
	EnvironmentDefinition environmentDefinition;
	Scalar inchFactor,meterFactor; // Cached because they're used a lot
	Misc::CallbackList environmentDefinitionChangedCallbacks;
	
	/* Glyph management: */
	GlyphRenderer* glyphRenderer;
	
	/* Virtual input device management: */
	Point newInputDevicePosition;
	VirtualInputDevice* virtualInputDevice;
	
	/* Light source management: */
	LightsourceManager* lightsourceManager;
	Lightsource* sunLightsource; // A directional lightsource in physical space
	float sunAzimuth,sunElevation,sunIntensity;
	
	/* Clipping plane management: */
	ClipPlaneManager* clipPlaneManager;
	
	/* Viewer management: */
	int numViewers;
	Viewer* viewers;
	Viewer* mainViewer;
	
	/* Screen management: */
	int numScreens;
	VRScreen* screens;
	VRScreen* mainScreen;
	
	/* Screen protection management: */
	int numProtectorAreas;
	ScreenProtectorArea* protectorAreas;
	int numProtectorDevices;
	ScreenProtectorDevice* protectorDevices;
	bool protectScreens;
	bool alwaysRenderProtection; // Flag to request always rendering the screen protectors
	Scalar renderProtection; // If >0.0, screen protectors need to be drawn during the current frame
	Color protectorGridColor; // Color to draw screen protectors
	Scalar protectorGridSpacing; // Spacing between grid lines when drawing screen protectors
	int numHapticDevices;
	HapticDevice* hapticDevices;
	
	/* Window management: */
	WindowProperties windowProperties;
	DisplayStateMapper displayStateMapper;
	
	/* Listener management: */
	int numListeners;
	Listener* listeners;
	Listener* mainListener;
	
	/* Rendering parameters: */
	Scalar frontplaneDist;
	Scalar backplaneDist;
	Color backgroundColor;
	Color foregroundColor;
	Color ambientLightColor;
	Misc::CallbackList renderingParametersChangedCallbacks;
	GLFont* pixelFont; // Font to be used when writing in pixel space
	
	/* Sound rendering parameters: */
	bool useSound;
	
	/* Widget management: */
	GLMaterial widgetMaterial;
	GLMotif::StyleSheet uiStyleSheet;
	Misc::TimerEventScheduler* timerEventScheduler; // Scheduler for timer events
	GLMotif::WidgetManager* widgetManager;
	UIManager* uiManager;
	GLMotif::PopupMenu* dialogsMenu;
	std::vector<GLMotif::PopupWindow*> poppedDialogs;
	GLMotif::PopupMenu* systemMenu; // Vrui system menu as top-level pop-up if the application does not define its own main menu
	bool systemMenuTopLevel; // Flag whether the system menu is a top-level pop-up (and must be deleted)
	GLMotif::Widget* quitSeparator; // Separator between system menu entries and the quit entry
	GLMotif::CascadeButton* dialogsMenuCascade;
	GLMotif::CascadeButton* visletsMenuCascade;
	GLMotif::ToggleButton* fixOrientationToggle;
	GLMotif::ToggleButton* fixVerticalToggle;
	GLMotif::Button* undoViewButton;
	GLMotif::Button* redoViewButton;
	MutexMenu* mainMenu;
	GLMotif::FileSelectionHelper viewSelectionHelper; // Helper to load and save view files
	GLMotif::PopupWindow* settingsDialog; // The Vrui settings dialog
	GLMotif::Pager* settingsPager; // The pager widget holding individual settings pages
	GLMotif::TextFieldSlider* sunAzimuthSlider;
	GLMotif::TextFieldSlider* sunElevationSlider;
	GLMotif::TextFieldSlider* sunIntensitySlider;
	bool userMessagesToConsole; // Flag whether to route user messages, normally displayed as dialog boxes, to the console instead
	MessageDialogHeap messageDialogs; // Heap containing currently-open message dialogs, sorted by time-out
	
	/* 3D picking management: */
	Scalar pointPickDistance;
	Scalar rayPickCosine;
	
	/* Navigation transformation management: */
	std::string viewpointFileName;
	bool fixOrientation,fixVertical; // Flags whether the entire orientation or the vertical alignment should stay fixed during interactive navigation
	Rotation fixedOrientation;
	Vector fixedVertical;
	bool delayNavigationTransformation; // Flag whether changes to the navigation transformation are delayed until the beginning of the next frame
	int navigationTransformationChangedMask; // Bit mask for changed parts of the navigation transformation (0x1-transform,0x2-display center/size)
	NavTransform newNavigationTransformation;
	NavTransform navigationTransformation,inverseNavigationTransformation;
	Misc::RingBuffer<NavTransform> navigationUndoBuffer;
	Misc::RingBuffer<NavTransform>::iterator navigationUndoCurrent;
	Misc::CallbackList navigationTransformationChangedCallbacks; // List of callbacks called when the navigation transformation changes
	CoordinateManager* coordinateManager;
	ScaleBar* scaleBar;
	
	/* Tool management: */
	ToolManager* toolManager;
	
	/* Vislet management: */
	VisletManager* visletManager;
	
	/* Application function callbacks: */
	PrepareMainLoopFunctionType prepareMainLoopFunction;
	void* prepareMainLoopFunctionData;
	FrameFunctionType frameFunction;
	void* frameFunctionData;
	Misc::Autopointer<ApplicationDisplayFunctionNode> applicationDisplayFunction;
	SoundFunctionType soundFunction;
	void* soundFunctionData;
	ResetNavigationFunctionType resetNavigationFunction;
	void* resetNavigationFunctionData;
	FinishMainLoopFunctionType finishMainLoopFunction;
	void* finishMainLoopFunctionData;
	
	/* Time management: */
	Misc::Timer appTime; // Free-running application timer
	double minimumFrameTime; // Lower limit on frame times; Vrui's main loop will block to pad frame times to this minimum
	double lastFrame; // Application time at which the last frame was started
	double lastFrameDelta; // Duration of last frame
	double nextFrameTime; // Scheduled time to start next frame, or 0.0 if no frame scheduled
	double synchFrameTime; // Precise time to be used for next frame
	bool synchWait; // Flag whether to delay the next frame until wallclock time matches synch time
	int numRecentFrameTimes; // Number of recent frame times to average from
	double* recentFrameTimes; // Array of recent times to complete a frame
	int nextFrameTimeIndex; // Index at which the next frame time is stored in the array
	double* sortedFrameTimes; // Helper array to calculate median of frame times
	double currentFrameTime; // Current frame time average
	double animationFrameInterval; // Suggested frame interval to be used for animations
	Threads::Mutex frameCallbacksMutex; // Mutex protecting the list of extra frame callbacks
	std::vector<FrameCallbackSlot> frameCallbacks; // List of extra frame callbacks
	Misc::CallbackList preRenderingCallbacks; // List of callbacks called for each window group before anything is rendered
	Misc::CallbackList postRenderingCallbacks; // List of callbacks called after all window groups have finished rendering
	Misc::CommandDispatcher commandDispatcher; // Dispatcher for pipe and console commands
	
	/* Transient dragging/moving/scaling state: */
	Misc::CallbackList navigationToolActivationCallbacks;
	const Tool* activeNavigationTool;
	
	/* List of created virtual input devices: */
	std::deque<InputDevice*> createdVirtualInputDevices;
	
	/* Rendering management state: */
	bool updateContinuously; // Flag if the inner Vrui loop never blocks
	bool synced; // Flag whether Vrui frames are synchronized to some display
	TimePoint nextVsync; // Predicted time at which the current frame's vsync event will occur
	TimeVector vsyncPeriod; // Current estimate of time interval between subsequent vsync events
	TimeVector exposureDelay; // Time interval between a vsync event and the time a frame submitted before that vsync will actually be visible to the user
	
	/* Private methods: */
	GLMotif::PopupMenu* buildDialogsMenu(void); // Builds the dialogs submenu
	GLMotif::PopupMenu* buildAlignViewMenu(void); // Builds the align view submenu
	GLMotif::PopupMenu* buildViewMenu(void); // Builds the view submenu
	GLMotif::PopupMenu* buildDevicesMenu(void); // Builds the input devices submenu
	void buildSystemMenu(GLMotif::Container* parent); // Builds the Vrui system menu inside the given container widget
	void pushNavigationTransformation(void); // Pushes the current navigation transformation into the navigation undo buffer
	void updateNavigationTransformation(const NavTransform& newTransform); // Updates the working version of the navigation transformation
	void loadViewpointFile(IO::Directory& directory,const char* viewpointFileName); // Overrides the navigation transformation with viewpoint data stored in the given viewpoint file
	void saveViewpointFile(IO::Directory& directory,const char* viewpointFileName); // Saves the current viewpoint data to the given viewpoint file
	
	/* Constructors and destructors: */
	VruiState(Cluster::Multiplexer* sMultiplexer,Cluster::MulticastPipe* sPipe); // Initializes basic Vrui state
	~VruiState(void);
	
	/* Methods: */
	
	/* Initialization methods: */
	void initialize(const Misc::ConfigurationFileSection& configFileSection); // Initializes complete Vrui state
	void createSystemMenu(void); // Creates Vrui's system menu
	void createSettingsDialog(void); // Creates Vrui's settings dialog window
	DisplayState* registerContext(GLContext& context) const; // Registers a newly-created OpenGL context with the Vrui state object
	void prepareMainLoop(void); // Performs last steps of initialization before main loop is run
	
	/* Frame processing methods: */
	void update(void); // Update Vrui state for current frame
	void display(DisplayState* displayState,GLContextData& contextData) const; // Vrui display function
	void sound(SceneGraph::ALRenderState& renderState) const; // Vrui sound function
	
	/* De-initialization methods: */
	void finishMainLoop(void); // Performs first steps of shutdown after mainloop finishes
	
	/* Pipe command callback methods: */
	static void listCommandsCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	static void showMessageCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	static void resetViewCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	static void loadViewCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	static void saveViewCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	static void loadInputGraphCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	static void saveScreenshotCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	static void quitCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData);
	
	/* System menu callback methods: */
	void dialogsMenuCallback(GLMotif::Button::SelectCallbackData* cbData,GLMotif::PopupWindow* const& dialog);
	void widgetPopCallback(GLMotif::WidgetManager::WidgetPopCallbackData* cbData);
	void loadViewCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void saveViewCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void resetViewCallback(Misc::CallbackData* cbData);
	void alignViewCallback(Misc::CallbackData* cbData);
	void fixOrientationCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void fixVerticalCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void undoViewCallback(Misc::CallbackData* cbData);
	void redoViewCallback(Misc::CallbackData* cbData);
	void createInputDeviceCallback(Misc::CallbackData* cbData,const int& numButtons);
	void destroyInputDeviceCallback(Misc::CallbackData* cbData);
	void loadInputGraphCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void saveInputGraphCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void toolKillZoneActiveCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void showToolKillZoneCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void protectScreensCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void showSettingsDialogCallback(Misc::CallbackData* cbData);
	void showScaleBarToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void quitCallback(Misc::CallbackData* cbData);
	
	/* Settings dialog callback methods: */
	void navigationUnitScaleValueChangedCallback(GLMotif::TextField::ValueChangedCallbackData* cbData);
	void navigationUnitValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void ambientIntensityValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void viewerHeadlightValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& viewerIndex);
	void updateSunLightsource(void);
	void sunValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void sunAzimuthValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void sunElevationValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void sunIntensityValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void backgroundColorValueChangedCallback(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData);
	void foregroundColorValueChangedCallback(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData);
	void backplaneValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void frontplaneValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void globalGainValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	};

extern VruiState* vruiState;

/*****************************
Private Vrui global variables:
*****************************/

/* Helper class to print a prefix to error messages: */
class VruiErrorHeader
	{
	};
std::ostream& operator<<(std::ostream& os,const VruiErrorHeader& veh);

extern bool vruiVerbose; // Flag whether Vrui should be verbose about its operations
extern bool vruiMaster; // Flag whether a Vrui instance is on a single host, or the head node of a cluster
extern VruiErrorHeader vruiErrorHeader; // Object to print error message headers

/********************************
Private Vrui function prototypes:
********************************/

/* Forward declarations: */
struct VruiWindowGroup;

extern void setRandomSeed(unsigned int newRandomSeed); // Sets Vrui's random seed; can only be called by InputDeviceAdapterPlayback during its initialization
extern EnvironmentDefinition& modifyEnvironmentDefinition(void); // Allows caller to modify Vrui's environment definition; can only be called by InputDeviceAdapterDeviceDaemon during its initialization
extern void vruiDelay(double interval);
extern double peekApplicationTime(void); // Returns the (approximate) application time that will be used by the next Vrui frame; can only be called by input device adapters during event handling
extern void synchronize(double firstFrameTime); // Gives a precise time value to use for the initial frame time; can only be called by InputDeviceAdapterPlayback during initialization
extern void synchronize(double nextFrameTime,bool wait); // Gives a precise time value to use for the next frame; delays frame until wall-clock time matches if wait is true; can only be called by InputDeviceAdapterPlayback during playback
extern void resetNavigation(void); // Calls the application-provided function to reset the navigation transformation
extern void setDisplayCenter(const Point& newDisplayCenter,Scalar newDisplaySize); // Sets the center and size of Vrui's display environment
extern void resizeWindow(VruiWindowGroup* windowGroup,const VRWindow* window,const ISize& newViewportSize,const ISize& newFrameSize); // Notifies the run-time environment that a window has changed viewport and/or frame buffer size
extern void getMaxWindowSizes(VruiWindowGroup* windowGroup,ISize& viewportSize,ISize& frameSize); // Returns the maximum viewport and frame buffer sizes for the given window group
extern void vsync(const TimePoint& newNextVsync,const TimeVector& newVsyncPeriod,const TimeVector& newExposureDelay); // Updates the kernel's frame synchronization state for the next frame

}

#endif

/***********************************************************************
PlaceViewerTool - Class for tools that move a viewer to the tool's
current position when their button is pressed.
Copyright (c) 2022-2023 Oliver Kreylos

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

#ifndef VRUI_PLACEVIEWERTOOL_INCLUDED
#define VRUI_PLACEVIEWERTOOL_INCLUDED

#include <Vrui/UtilityTool.h>

/* Forward declarations: */
namespace Vrui {
class Viewer;
}

namespace Vrui {

class PlaceViewerTool;

class PlaceViewerToolFactory:public ToolFactory
	{
	friend class PlaceViewerTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		std::string viewerName; // Name of the viewer affected by this tool
		Point deviceOffset; // Additional offset vector in device space
		bool dragViewer; // Flag if the tool drags the associated viewer while the tool button is pressed
		
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
	PlaceViewerToolFactory(ToolManager& toolManager);
	virtual ~PlaceViewerToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class PlaceViewerTool:public UtilityTool
	{
	friend class PlaceViewerToolFactory;
	
	/* Elements: */
	private:
	static PlaceViewerToolFactory* factory; // Pointer to the factory object for this class
	PlaceViewerToolFactory::Configuration configuration; // Configuration for this tool
	Viewer* viewer; // The viewer affected by this tool
	
	/* Private methods: */
	void updateViewer(void); // Updates the controlled viewer based on the current tool position
	
	/* Constructors and destructors: */
	public:
	PlaceViewerTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif

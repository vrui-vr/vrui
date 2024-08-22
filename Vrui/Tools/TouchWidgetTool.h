/***********************************************************************
TouchWidgetTool - Class for tools that can interact with GLMotif GUI
widgets by touch.
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

#ifndef VRUI_TOUCHWIDGETTOOL_INCLUDED
#define VRUI_TOUCHWIDGETTOOL_INCLUDED

#include <Geometry/Plane.h>
#include <Vrui/GUIInteractor.h>
#include <Vrui/MenuTool.h>

/* Forward declarations: */
namespace GLMotif {
class Widget;
}
namespace Vrui {
class ToolManager;
}

namespace Vrui {

class TouchWidgetTool;

class TouchWidgetToolFactory:public ToolFactory
	{
	friend class TouchWidgetTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		bool popUpMainMenu; // Flag whether the tool will pop up the main menu if it doesn't point at a UI widget on activation
		bool alignWidgets; // Flag whether to align widgets to a GUI transformation during dragging
		
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
	TouchWidgetToolFactory(ToolManager& toolManager);
	virtual ~TouchWidgetToolFactory(void);
	
	/* Methods from class ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class TouchWidgetTool:public MenuTool,public GUIInteractor
	{
	friend class TouchWidgetToolFactory;
	
	/* Elements: */
	private:
	static TouchWidgetToolFactory* factory; // Pointer to the factory object for this class
	TouchWidgetToolFactory::Configuration configuration; // Private configuration of this tool
	
	bool active; // Flag if the tool is currently sending UI events to the widget manager
	bool touching; // Flag if the tool is currently touching a widget
	GLMotif::Widget* target; // Pointer to the currently selected widget
	Plane touchPlane; // Plane aligned with the currently selected widget; interaction ends when device moves in front of plane
	
	/* Private methods: */
	void calcTouchPlane(GLMotif::Widget* widget,const Point& toolPos); // Calculates a touch plane for the given widget that includes the given tool position
	Ray calcTouchRay(const Point& toolPos) const; // Calculates a ray for touch interactions based on the given tool position
	
	/* Constructors and destructors: */
	public:
	TouchWidgetTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from class Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif

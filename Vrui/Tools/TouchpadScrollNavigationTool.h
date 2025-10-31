/***********************************************************************
TouchpadScrollNavigationTool - Class to scroll using a linear touch pad
device.
Copyright (c) 2025 Oliver Kreylos

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

#ifndef VRUI_TOUCHPADSCROLLNAVIGATIONTOOL_INCLUDED
#define VRUI_TOUCHPADSCROLLNAVIGATIONTOOL_INCLUDED

#include <Geometry/Vector.h>
#include <Vrui/NavigationTool.h>

namespace Vrui {

class TouchpadScrollNavigationTool;

class TouchpadScrollNavigationToolFactory:public ToolFactory
	{
	friend class TouchpadScrollNavigationTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		Vector scrollDirection; // Scrolling direction in physical space
		Scalar scrollFactor; // Scale factor from valuator value to physical space units
		
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
	TouchpadScrollNavigationToolFactory(ToolManager& toolManager);
	virtual ~TouchpadScrollNavigationToolFactory(void);
	
	/* Methods from class ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getValuatorFunction(int valuatorSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class TouchpadScrollNavigationTool:public NavigationTool
	{
	friend class TouchpadScrollNavigationToolFactory;
	
	/* Elements: */
	private:
	static TouchpadScrollNavigationToolFactory* factory; // Pointer to the factory object for this class
	TouchpadScrollNavigationToolFactory::Configuration configuration; // Private configuration of this tool
	
	/* Transient navigation state: */
	Scalar lastScrollValue; // Scroll valuator value on previous frame
	
	/* Constructors and destructors: */
	public:
	TouchpadScrollNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from class Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual const ToolFactory* getFactory(void) const;
	virtual void valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif

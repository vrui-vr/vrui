/***********************************************************************
LookMenuTool - Class for menu tools that show the program's main menu
when the main viewer is looking at it and allow any widget interaction
tool to select items from it.
Copyright (c) 2023 Oliver Kreylos

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

#ifndef VRUI_LOOKMENUTOOL_INCLUDED
#define VRUI_LOOKMENUTOOL_INCLUDED

#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/MenuTool.h>

namespace Vrui {

class LookMenuTool;

class LookMenuToolFactory:public ToolFactory
	{
	friend class LookMenuTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		Scalar maxActivationAngleCos; // Cosine of the maximum angle between the device's pointing direction and the view direction to show the menu
		Scalar menuOffset; // Distance from device's position to center of menu along device's pointing direction
		bool trackDevice; // Flag whether the menu tracks the position of the input device while it is being shown
		
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
	LookMenuToolFactory(ToolManager& toolManager);
	virtual ~LookMenuToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class LookMenuTool:public MenuTool
	{
	friend class LookMenuToolFactory;
	
	/* Elements: */
	private:
	static LookMenuToolFactory* factory; // Pointer to the factory object for this class
	LookMenuToolFactory::Configuration configuration; // Configuration for this tool
	ONTransform menuTransform; // Transformation from tool's input device to the main menu
	
	/* Constructors and destructors: */
	public:
	LookMenuTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~LookMenuTool(void);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif

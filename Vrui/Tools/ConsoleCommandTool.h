/***********************************************************************
ConsoleCommandTool - Class for tools to dispatch commands to Vrui's
internal text command interface when a button is pressed.
Copyright (c) 2026 Oliver Kreylos

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

#ifndef VRUI_CONSOLECOMMANDTOOL_INCLUDED
#define VRUI_CONSOLECOMMANDTOOL_INCLUDED

#include <string>
#include <Vrui/UtilityTool.h>

namespace Vrui {

class ConsoleCommandTool;

class ConsoleCommandToolFactory:public ToolFactory
	{
	friend class ConsoleCommandTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		std::string command; // The command, including parameters, that will be submitted to the command interface when a button is pressed
		
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
	ConsoleCommandToolFactory(ToolManager& toolManager);
	virtual ~ConsoleCommandToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ConsoleCommandTool:public UtilityTool
	{
	friend class ConsoleCommandToolFactory;
	
	/* Elements: */
	private:
	static ConsoleCommandToolFactory* factory; // Pointer to the factory object for this class
	ConsoleCommandToolFactory::Configuration configuration; // Private configuration of this tool
	
	/* Constructors and destructors: */
	public:
	ConsoleCommandTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	};

}

#endif

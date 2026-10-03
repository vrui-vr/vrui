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

#include <Vrui/Tools/ConsoleCommandTool.h>

#include <Misc/MessageLogger.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/CommandDispatcher.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/*********************************************************
Methods of class ConsoleCommandToolFactory::Configuration:
*********************************************************/

ConsoleCommandToolFactory::Configuration::Configuration(void)
	{
	}

void ConsoleCommandToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateString("./command",command);
	}

void ConsoleCommandToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeString("./command",command);
	}

/******************************************
Methods of class ConsoleCommandToolFactory:
******************************************/

ConsoleCommandToolFactory::ConsoleCommandToolFactory(ToolManager& toolManager)
	:ToolFactory("ConsoleCommandTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UtilityTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	ConsoleCommandTool::factory=this;
	}

ConsoleCommandToolFactory::~ConsoleCommandToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ConsoleCommandTool::factory=0;
	}

const char* ConsoleCommandToolFactory::getName(void) const
	{
	return "Command Executor";
	}

const char* ConsoleCommandToolFactory::getButtonFunction(int) const
	{
	return "Execute Command";
	}

Tool* ConsoleCommandToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ConsoleCommandTool(this,inputAssignment);
	}

void ConsoleCommandToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveConsoleCommandToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createConsoleCommandToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ConsoleCommandToolFactory* scriptExecutorToolFactory=new ConsoleCommandToolFactory(*toolManager);
	
	/* Return factory object: */
	return scriptExecutorToolFactory;
	}

extern "C" void destroyConsoleCommandToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*******************************************
Static elements of class ConsoleCommandTool:
*******************************************/

ConsoleCommandToolFactory* ConsoleCommandTool::factory=0;

/***********************************
Methods of class ConsoleCommandTool:
***********************************/

ConsoleCommandTool::ConsoleCommandTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(factory,inputAssignment),
	 configuration(ConsoleCommandTool::factory->configuration)
	{
	}

void ConsoleCommandTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void ConsoleCommandTool::initialize(void)
	{
	/* Bring up a dialog to set the command if there is no pre-configured command: */
	if(configuration.command.empty())
		{
		// IMPLEMENT ME!
		}
	}

void ConsoleCommandTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

const ToolFactory* ConsoleCommandTool::getFactory(void) const
	{
	return factory;
	}

void ConsoleCommandTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		try
			{
			/* Dispatch the command to Vrui's command dispatcher: */
			getCommandDispatcher().dispatchCommand(configuration.command.c_str(),configuration.command.c_str()+configuration.command.size());
			}
		catch(const std::runtime_error& err)
			{
			Misc::sourcedUserError(__PRETTY_FUNCTION__,"Command %s caused exception %s",configuration.command.c_str(),err.what());
			}
		}
	}

}

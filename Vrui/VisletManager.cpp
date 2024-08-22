/***********************************************************************
VisletManager - Class to manage vislet classes.
Copyright (c) 2006-2023 Oliver Kreylos

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

#include <Vrui/VisletManager.h>

#include <ctype.h>
#include <string>
#include <vector>
#include <Misc/ConfigurationFile.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/CommandDispatcher.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/CascadeButton.h>
#include <Vrui/Vrui.h>
#include <Vrui/Internal/Vrui.h>
#include <Vrui/Internal/Config.h>

namespace Vrui {

/******************************
Methods of class VisletManager:
******************************/

void VisletManager::setVisletState(const std::string& visletClassName,int visletIndex,bool newState)
	{
	if(visletIndex>=0)
		{
		/* Set the activation state of the requested vislet of the requested class: */
		for(size_t i=0;i<vislets.size();++i)
			if(visletClassName==vislets[i]->getFactory()->getClassName())
				{
				/* Check if this is the requested vislet: */
				if(visletIndex==0)
					{
					/* Change the vislet's activation state: */
					if(vislets[i]->isActive()!=newState)
						{
						if(newState)
							vislets[i]->enable(false);
						else
							vislets[i]->disable(false);
						}
					
					/* Update the vislet's toggle button in the vislet menu: */
					static_cast<GLMotif::ToggleButton*>(visletMenu->getMenu()->getChild(i))->setToggle(vislets[i]->isActive());
					}
				
				--visletIndex;
				}
		}
	else
		{
		/* Set the activation state of all vislets of the requested class: */
		for(size_t i=0;i<vislets.size();++i)
			if(visletClassName==vislets[i]->getFactory()->getClassName())
				{
				/* Change the vislet's activation state: */
				if(vislets[i]->isActive()!=newState)
					{
					if(newState)
						vislets[i]->enable(false);
					else
						vislets[i]->disable(false);
					}
				
				/* Update the vislet's toggle button in the vislet menu: */
				static_cast<GLMotif::ToggleButton*>(visletMenu->getMenu()->getChild(i))->setToggle(vislets[i]->isActive());
				}
		}
	}

void VisletManager::addVisletCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	VisletManager* thisPtr=static_cast<VisletManager*>(userData);
	
	/* Get the name of the vislet class to add and create a new vislet factory: */
	std::string visletClassName=Misc::ValueCoder<std::string>::decode(argumentBegin,argumentEnd,&argumentBegin);
	VisletFactory* factory=thisPtr->loadClass(visletClassName.c_str());
	
	/* Collect vislet arguments: */
	std::vector<std::string> arguments;
	std::vector<const char*> argumentStrings;
	while(true)
		{
		/* Find the start of the next argument: */
		for(;argumentBegin!=argumentEnd&&isspace(*argumentBegin);++argumentBegin)
			;
		
		/* Bail out if there are no more arguments: */
		if(argumentBegin==argumentEnd)
			break;
		
		/* Get and store the next argument: */
		arguments.push_back(Misc::ValueCoder<std::string>::decode(argumentBegin,argumentEnd,&argumentBegin));
		argumentStrings.push_back(arguments.back().c_str());
		}
	
	/* Create a new vislet of the requested class with the given arguments: */
	Vislet* newVislet=factory->createVislet(argumentStrings.size(),argumentStrings.data());
	
	/* Enable the new vislet for the first time: */
	newVislet->enable(true);
	
	/* Add the new vislet to the list: */
	thisPtr->vislets.push_back(newVislet);
	
	/* Create a toggle button for the new vislet in the vislet menu: */
	char toggleButtonName[40];
	snprintf(toggleButtonName,sizeof(toggleButtonName),"Vislet%u",(unsigned int)(thisPtr->vislets.size())-1);
	GLMotif::ToggleButton* toggleButton=new GLMotif::ToggleButton(toggleButtonName,thisPtr->visletMenu,factory->getClassName());
	toggleButton->setToggle(newVislet->isActive());
	toggleButton->getValueChangedCallbacks().add(thisPtr,&VisletManager::visletMenuToggleButtonCallback);
	
	/* Enable the vislet submenu just in case: */
	vruiState->visletsMenuCascade->setEnabled(thisPtr->vislets.size()!=0);
	}

void VisletManager::removeVisletCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	VisletManager* thisPtr=static_cast<VisletManager*>(userData);
	
	/* Get the name of the vislet class to remove: */
	std::string visletClassName=Misc::ValueCoder<std::string>::decode(argumentBegin,argumentEnd,&argumentBegin);
	}

void VisletManager::enableVisletCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	VisletManager* thisPtr=static_cast<VisletManager*>(userData);
	
	/* Get the name of the vislet class to enable: */
	std::string visletClassName=Misc::ValueCoder<std::string>::decode(argumentBegin,argumentEnd,&argumentBegin);
	
	/* Check if there is an optional index argument: */
	int visletIndex=-1; // Enable all vislets of the given class by default
	for(;argumentBegin!=argumentEnd&&isspace(*argumentBegin);++argumentBegin)
		;
	if(argumentBegin!=argumentEnd)
		visletIndex=Misc::ValueCoder<int>::decode(argumentBegin,argumentEnd,&argumentBegin);
	
	/* Make it so: */
	thisPtr->setVisletState(visletClassName,visletIndex,true);
	}

void VisletManager::disableVisletCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	VisletManager* thisPtr=static_cast<VisletManager*>(userData);
	
	/* Get the name of the vislet class to disable: */
	std::string visletClassName=Misc::ValueCoder<std::string>::decode(argumentBegin,argumentEnd,&argumentBegin);
	
	/* Check if there is an optional index argument: */
	int visletIndex=-1; // Disable all vislets of the given class by default
	for(;argumentBegin!=argumentEnd&&isspace(*argumentBegin);++argumentBegin)
		;
	if(argumentBegin!=argumentEnd)
		visletIndex=Misc::ValueCoder<int>::decode(argumentBegin,argumentEnd,&argumentBegin);
	
	/* Make it so: */
	thisPtr->setVisletState(visletClassName,visletIndex,false);
	}

void VisletManager::visletMenuToggleButtonCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Get the toggle button's index in its container: */
	GLMotif::RowColumn* visletMenu=dynamic_cast<GLMotif::RowColumn*>(cbData->toggle->getParent());
	
	if(visletMenu!=0)
		{
		int toggleIndex=visletMenu->getChildIndex(cbData->toggle);
		if(toggleIndex>=0&&toggleIndex<int(vislets.size()))
			{
			/* Activate or deactivate the vislet: */
			if(cbData->set)
				{
				vislets[toggleIndex]->enable(false);
				
				/* Check if the vislet enabled successfully: */
				if(!vislets[toggleIndex]->isActive())
					{
					/* Turn the toggle back off: */
					cbData->toggle->setToggle(false);
					}
				}
			else
				{
				vislets[toggleIndex]->disable(false);
				
				/* Check if the vislet disabled successfully: */
				if(vislets[toggleIndex]->isActive())
					{
					/* Turn the toggle back on: */
					cbData->toggle->setToggle(true);
					}
				}
			}
		}
	}

VisletManager::VisletManager(const Misc::ConfigurationFileSection& sConfigFileSection)
	:Plugins::FactoryManager<VisletFactory>(sConfigFileSection.retrieveString("./visletDsoNameTemplate",VRUI_INTERNAL_CONFIG_VISLETDIR "/" VRUI_INTERNAL_CONFIG_VISLETNAMETEMPLATE)),
	 configFileSection(sConfigFileSection),
	 visletMenu(0)
	{
	typedef std::vector<std::string> StringList;
	
	/* Get additional search paths from configuration file section and add them to the factory manager: */
	StringList visletSearchPaths=configFileSection.retrieveValue<StringList>("./visletSearchPaths",StringList());
	for(StringList::const_iterator vspIt=visletSearchPaths.begin();vspIt!=visletSearchPaths.end();++vspIt)
		{
		/* Add the path: */
		getDsoLocator().addPath(*vspIt);
		}
	
	/* Register commands to add/remove vislets: */
	getCommandDispatcher().addCommandCallback("VisletManager::addVislet",&VisletManager::addVisletCommandCallback,this,"<vislet class name> <argument>*","Adds a new vislet of the requested class with the given list of arguments");
	getCommandDispatcher().addCommandCallback("VisletManager::removeVislet",&VisletManager::removeVisletCommandCallback,this,"<vislet class name> [vislet index]","Removes the vislet of the given index within the requested class");
	
	/* Register commands to enable/disable vislets: */
	getCommandDispatcher().addCommandCallback("VisletManager::enableVislet",&VisletManager::enableVisletCommandCallback,this,"<vislet class name> [vislet index]","Enables the vislet of the given index within the requested class, or all vislets of the requested class if no index is given");
	getCommandDispatcher().addCommandCallback("VisletManager::disableVislet",&VisletManager::disableVisletCommandCallback,this,"<vislet class name> [vislet index]","Disables the vislet of the given index within the requested class, or all vislets of the requested class if no index is given");
	}

VisletManager::~VisletManager(void)
	{
	/* Destroy all loaded vislets: */
	for(VisletList::iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		{
		/* Delete the vislet: */
		(*vIt)->getFactory()->destroyVislet(*vIt);
		}
	}

Misc::ConfigurationFileSection VisletManager::getVisletClassSection(const char* visletClassName) const
	{
	/* Return the section of the same name under the vislet manager's section: */
	return configFileSection.getSection(visletClassName);
	}

Vislet* VisletManager::createVislet(VisletFactory* factory,int numVisletArguments,const char* const visletArguments[])
	{
	/* Create vislet of given class: */
	Vislet* newVislet=factory->createVislet(numVisletArguments,visletArguments);
	
	/* Add the vislet to the list: */
	vislets.push_back(newVislet);
	
	return newVislet;
	}

GLMotif::PopupMenu* VisletManager::buildVisletMenu(void)
	{
	/* Create the popup menu: */
	visletMenu=new GLMotif::PopupMenu("VisletsMenu",getWidgetManager());
	
	/* Create a toggle button for each vislet: */
	for(unsigned int i=0;i<vislets.size();++i)
		{
		char toggleButtonName[40];
		snprintf(toggleButtonName,sizeof(toggleButtonName),"Vislet%u",i);
		GLMotif::ToggleButton* toggleButton=new GLMotif::ToggleButton(toggleButtonName,visletMenu,vislets[i]->getFactory()->getClassName());
		toggleButton->setToggle(vislets[i]->isActive());
		toggleButton->getValueChangedCallbacks().add(this,&VisletManager::visletMenuToggleButtonCallback);
		}
	
	/* Finalize and return the vislet menu popup: */
	visletMenu->manageMenu();
	return visletMenu;
	}

void VisletManager::enable(void)
	{
	/* Enable all vislets for the first time: */
	for(size_t i=0;i<vislets.size();++i)
		if(!vislets[i]->isActive())
			{
			vislets[i]->enable(true);
			static_cast<GLMotif::ToggleButton*>(visletMenu->getMenu()->getChild(i))->setToggle(vislets[i]->isActive());
			}
	}

void VisletManager::disable(void)
	{
	/* Disable all vislets for the last time, even those that are already inactive: */
	for(size_t i=0;i<vislets.size();++i)
		vislets[i]->disable(true);
	}

void VisletManager::updateVisletMenu(const Vislet* vislet)
	{
	/* Find the vislet in the vislet list: */
	int visletIndex=0;
	for(VisletList::iterator vIt=vislets.begin();vIt!=vislets.end();++vIt,++visletIndex)
		if(*vIt==vislet)
			{
			/* Update the vislet's toggle button: */
			static_cast<GLMotif::ToggleButton*>(visletMenu->getMenu()->getChild(visletIndex))->setToggle(vislet->isActive());
			}
	}

void VisletManager::frame(void)
	{
	/* Call all vislet's frame functions: */
	for(VisletList::iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		if((*vIt)->isActive())
			(*vIt)->frame();
	}

void VisletManager::display(GLContextData& contextData) const
	{
	/* Call all vislet's display functions: */
	for(VisletList::const_iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		if((*vIt)->isActive())
			(*vIt)->display(contextData);
	}

void VisletManager::sound(ALContextData& contextData) const
	{
	/* Call all vislet's sound functions: */
	for(VisletList::const_iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		if((*vIt)->isActive())
			(*vIt)->sound(contextData);
	}

}

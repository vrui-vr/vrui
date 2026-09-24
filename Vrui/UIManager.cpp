/***********************************************************************
UIManager - Base class for managers arranging user interface components,
mapping user interface devices and tools, and create user-aligned
displays in physical space.
Copyright (c) 2015-2026 Oliver Kreylos

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

#include <Vrui/UIManager.h>

#include <GLMotif/StyleSheet.h>
#include <GLMotif/ClickRepeatWidget.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <Vrui/Vrui.h>
#include <Vrui/GUIInteractor.h>

#include <Vrui/Internal/Vrui.h>

namespace Vrui {
	
/**************************
Methods of class UIManager:
**************************/

bool UIManager::popupPrimaryWidgetAt(GLMotif::Widget* topLevelWidget,const GLMotif::WidgetManager::Transformation& widgetToWorld)
	{
	/* Call the base class method: */
	bool result=this->GLMotif::WidgetManager::popupPrimaryWidgetAt(topLevelWidget,widgetToWorld);
	
	/* Check if the widget was not previously popped up and the dialogs menu has already been built: */
	if(result&&dialogsMenu!=0)
		{
		/* Check if the newly popped-up widget is a dialog: */
		GLMotif::PopupWindow* dialog=dynamic_cast<GLMotif::PopupWindow*>(topLevelWidget);
		if(dialog!=0)
			{
			/* Append the newly popped-up dialog to the dialogs menu: */
			GLMotif::Button* button=dialogsMenu->addEntry(dialog->getTitleString());
			button->getSelectCallbacks().add(this,&UIManager::dialogsMenuCallback,dialog);
			poppedDialogs.push_back(dialog);
			
			/* Enable the dialogs menu cascade button if this is the first open dialog window: */
			if(poppedDialogs.size()==1)
				dialogsMenuCascade->setEnabled(true);
			}
		}
	
	return result;
	}

void UIManager::dialogsMenuCallback(Misc::CallbackData* cbData,GLMotif::PopupWindow* const& dialog)
	{
	/* Check if the dialog is currently visible or hidden: */
	if(isVisible(dialog))
		{
		/* Initialize the pop-up position: */
		Point hotSpot=getHotSpot();
		
		/* Move the dialog window to the hot spot position: */
		ONTransform transform=calcUITransform(hotSpot);
		transform*=ONTransform::translate(-Vector(dialog->calcHotSpot().getXyzw()));
		setPrimaryWidgetTransformation(dialog,transform);
		}
	else
		{
		/* Show the hidden dialog window at its previous position: */
		show(dialog);
		}
	}

void UIManager::clickRepeatEvent(Threads::TimerEvent& event,GLMotif::ClickRepeatWidget* widget)
	{
	/* Call the widget's click repeat method: */
	widget->clickRepeat();
	}

UIManager::UIManager(const Misc::ConfigurationFileSection& configFileSection)
	:dialogsMenuCascade(0),dialogsMenu(0),
	 activeGuiInteractor(0),
	 mostRecentGuiInteractor(0),
	 mostRecentHotSpot(getDisplayCenter()),mostRecentDirection(getForwardDirection())
	{
	}

bool UIManager::popdownWidget(GLMotif::Widget* widget)
	{
	/* Call the base class method: */
	bool result=GLMotif::WidgetManager::popdownWidget(widget);
	
	/* Check if the widget was not previously popped up and the dialogs menu has already been built: */
	if(result&&dialogsMenu!=0)
		{
		/* Find the given widget's root widget and check whether it's a dialog: */
		GLMotif::PopupWindow* dialog=dynamic_cast<GLMotif::PopupWindow*>(widget->getRoot());
		if(dialog!=0)
			{
			/* Find the popped-down dialog in the dialogs menu: */
			int menuIndex=0;
			for(std::vector<GLMotif::PopupWindow*>::iterator dIt=poppedDialogs.begin();dIt!=poppedDialogs.end();++dIt,++menuIndex)
				if(*dIt==dialog)
					{
					/* Remove the popped-down dialog from the popped dialogs list and delete the button widget: */
					poppedDialogs.erase(dIt);
					delete dialogsMenu->removeEntry(menuIndex);
					
					/* Disable the dialogs menu if it has become empty: */
					if(poppedDialogs.empty())
						dialogsMenuCascade->setEnabled(false);
					
					break;
					}
			}
		}
	
	return result;
	}

void UIManager::requestClickRepeat(GLMotif::ClickRepeatWidget* widget)
	{
	/* Create an active click repeat structure for the widget with a new timer: */
	Threads::EventTime first=vruiState->runLoop.getDispatchTime()+Threads::EventInterval(styleSheet->clickRepeatDelay);
	Threads::EventInterval repeat(styleSheet->clickRepeatInterval);
	activeClickRepeats.push_back(ActiveClickRepeat(widget,*new Threads::Timer(vruiState->runLoop,first,repeat,true,*Threads::createFunctionCall(this,&UIManager::clickRepeatEvent,widget))));
	}

void UIManager::cancelClickRepeat(GLMotif::ClickRepeatWidget* widget)
	{
	/* Find the active click repeat associated with the given widget: */
	for(ActiveClickRepeatList::iterator acrIt=activeClickRepeats.begin();acrIt!=activeClickRepeats.end();++acrIt)
		if(acrIt->widget==widget)
			{
			/* Delete the active click request and stop looking: */
			activeClickRepeats.erase(acrIt);
			break;
			}
	}

GLMotif::PopupMenu* UIManager::createDialogsMenu(GLMotif::Container* dialogsButtonParent)
	{
	/* Create the dialogs submenu: */
	dialogsMenu=new GLMotif::PopupMenu("DialogsMenu",this);
	
	/* Add menu buttons for all popped-up dialog boxes: */
	poppedDialogs.clear();
	for(PoppedWidgetIterator wIt=beginPrimaryWidgets();wIt!=endPrimaryWidgets();++wIt)
		{
		/* Check if the primary widget is a dialog box: */
		GLMotif::PopupWindow* dialog=dynamic_cast<GLMotif::PopupWindow*>(*wIt);
		if(dialog!=0)
			{
			/* Add an entry to the dialogs submenu: */
			GLMotif::Button* button=dialogsMenu->addEntry(dialog->getTitleString());
			
			/* Add a callback to the button: */
			button->getSelectCallbacks().add(this,&UIManager::dialogsMenuCallback,dialog);
			
			/* Save a pointer to the dialog window: */
			poppedDialogs.push_back(dialog);
			}
		}
	
	/* Manage and the submenu: */
	dialogsMenu->manageMenu();
	
	/* Create the dialogs submenu cascade button: */
	dialogsMenuCascade=new GLMotif::CascadeButton("DialogsMenuCascade",dialogsButtonParent,"Dialogs");
	dialogsMenuCascade->setPopup(dialogsMenu);
	dialogsMenuCascade->setEnabled(!poppedDialogs.empty());
	
	return dialogsMenu;
	}

bool UIManager::activateGuiInteractor(GUIInteractor* guiInteractor)
	{
	/* Check if the GUI interactor can be activated, or is already active: */
	if(activeGuiInteractor==0||activeGuiInteractor==guiInteractor)
		{
		/* Activate the GUI interactor: */
		activeGuiInteractor=guiInteractor;
		
		/* Remember it as the most recently active GUI interactor: */
		mostRecentGuiInteractor=guiInteractor;
		
		return true;
		}
	else
		return false;
	}

void UIManager::deactivateGuiInteractor(GUIInteractor* guiInteractor)
	{
	/* Check if the GUI interactor is active: */
	if(activeGuiInteractor==guiInteractor)
		{
		/* Reset the active GUI interactor: */
		activeGuiInteractor=0;
		}
	}

void UIManager::destroyGuiInteractor(GUIInteractor* guiInteractor)
	{
	/* If this GUI interactor is the current most recent one, reset it and remember its hotspot: */
	if(mostRecentGuiInteractor==guiInteractor)
		{
		mostRecentGuiInteractor=0;
		mostRecentHotSpot=guiInteractor->calcHotSpot();
		mostRecentDirection=guiInteractor->getRay().getDirection();
		}
	}

Point UIManager::getHotSpot(void) const
	{
	/* Check if there is a most-recently used GUI interactor: */
	if(mostRecentGuiInteractor!=0)
		{
		/* Return the GUI interactor's current hotspot: */
		return mostRecentGuiInteractor->calcHotSpot();
		}
	else
		{
		/* Return the most recent hotspot: */
		return mostRecentHotSpot;
		}
	}

Vector UIManager::getDirection(void) const
	{
	/* Check if there is a most-recently used GUI interactor: */
	if(mostRecentGuiInteractor!=0)
		{
		/* Return the GUI interactor's current interaction direction: */
		return mostRecentGuiInteractor->getRay().getDirection();
		}
	else
		{
		/* Return a zero vector to disable direction matching: */
		return Vector::zero;
		}
	}

void UIManager::shutdown(void)
	{
	/* Forget about the dialogs submenu so we don't have to do clean-up nobody will see: */
	dialogsMenu=0;
	}

}

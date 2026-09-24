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

#ifndef VRUI_UIMANAGER_INCLUDED
#define VRUI_UIMANAGER_INCLUDED

#include <vector>
#include <Threads/Timer.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/Types.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
class CallbackData;
}
namespace GLMotif {
class Container;
class PopupWindow;
class PopupMenu;
class CascadeButton;
}
namespace Vrui {
class InputDevice;
class GUIInteractor;
class VruiState;
}

namespace Vrui {

class UIManager:public GLMotif::WidgetManager
	{
	/* Embedded classes: */
	private:
	struct ActiveClickRepeat // Structure holding currently active click repeats
		{
		/* Elements: */
		public:
		GLMotif::ClickRepeatWidget* widget; // The widget receiving click repeat events
		Threads::TimerOwner timer; // A timer to schedule click repeat events
		
		/* Constructors and destructors: */
		ActiveClickRepeat(GLMotif::ClickRepeatWidget* sWidget,Threads::Timer& sTimer)
			:widget(sWidget),timer(&sTimer)
			{
			}
		};
	
	typedef std::vector<ActiveClickRepeat> ActiveClickRepeatList; // Type for lists of active click repeats
	
	/* Elements: */
	protected:
	GLMotif::CascadeButton* dialogsMenuCascade; // The cascade button to open the dialogs submenu
	GLMotif::PopupMenu* dialogsMenu; // Submenu with buttons to show all currently open top-level widgets
	std::vector<GLMotif::PopupWindow*> poppedDialogs; // The list of currently open top-level widgets
	ActiveClickRepeatList activeClickRepeats; // List of currently active click repeats
	GUIInteractor* activeGuiInteractor; // The currently active GUI interactor
	GUIInteractor* mostRecentGuiInteractor; // Pointer to the most-recently used GUI interactor, to calculate an appropriate position to pop up dialog windows
	Point mostRecentHotSpot; // Final hot spot position when the most-recently used GUI interactor is destroyed
	Vector mostRecentDirection; // Final interaction direction when the most-recently used GUI interactor is destroyed
	
	/* Protected methods from class GLMotif::WidgetManager: */
	virtual bool popupPrimaryWidgetAt(GLMotif::Widget* topLevelWidget,const GLMotif::WidgetManager::Transformation& widgetToWorld);
	
	/* New protected methods: */
	void dialogsMenuCallback(Misc::CallbackData* cbData,GLMotif::PopupWindow* const& dialog); // Callback called when one of the buttons in the dialogs submenu is selected
	void clickRepeatEvent(Threads::TimerEvent& event,GLMotif::ClickRepeatWidget* widget); // Timer event method calling the given widget's clickRepeat method
	
	/* Constructors and destructors: */
	public:
	UIManager(const Misc::ConfigurationFileSection& configFileSection); // Initializes UI manager from the given configuration file section
	
	/* Methods from class GLMotif::WidgetManager: */
	virtual bool popdownWidget(GLMotif::Widget* widget);
	virtual void requestClickRepeat(GLMotif::ClickRepeatWidget* widget);
	virtual void cancelClickRepeat(GLMotif::ClickRepeatWidget* widget);
	
	/* New methods: */
	GLMotif::PopupMenu* createDialogsMenu(GLMotif::Container* dialogsButtonParent); // Creates the sub-menu of currently open top-level widgets and attaches it to a new cascade button underneath the given parent
	bool canActivateGuiInteractor(const GUIInteractor* guiInteractor) const // Returns true if the given GUI interaction tool can be activated, or is already active
		{
		return activeGuiInteractor==0||activeGuiInteractor==guiInteractor;
		}
	bool activateGuiInteractor(GUIInteractor* guiInteractor); // Tries activating the given GUI interaction tool; returns true if successful
	void deactivateGuiInteractor(GUIInteractor* guiInteractor); // Deactivates the given GUI interaction tool; does nothing if it's not active
	void destroyGuiInteractor(GUIInteractor* guiInteractor); // Called to notify the UI manager of the destruction of a GUI interaction tool
	Point getHotSpot(void) const; // Returns a hot spot for newly-opened top-level widgets
	Vector getDirection(void) const; // Returns an interaction direction for newly-opened top-level widgets
	virtual Point projectRay(const Ray& ray) const =0; // Projects a ray onto the UI surface
	virtual void projectDevice(InputDevice* device,const TrackerState& proposedTransform) const =0; // Projects an input device onto the UI surface from the given proposed device transformation based on its device ray
	virtual ONTransform calcUITransform(const Point& point) const =0; // Returns a transformation to align a UI component at the given position
	virtual ONTransform calcUITransform(const Ray& ray) const =0; // Returns a transformation to align a UI component along the given ray
	virtual ONTransform calcUITransform(const InputDevice* device) const =0; // Returns a transformation to align a UI component for interaction with the given device
	virtual ONTransform calcHUDTransform(const Point& point) const =0; // Returns a transformation to align a display or non-interactive UI component at the given position
	virtual void shutdown(void); // Shuts down the UI manager at the end of Vrui's main loop
	};

}

#endif

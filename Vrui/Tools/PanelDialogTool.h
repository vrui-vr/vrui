/***********************************************************************
PanelDialogTool - Class for tools that can pick up popped-up primary
widgets and attach them to an input device.
Copyright (c) 2017 Oliver Kreylos

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

#ifndef VRUI_PANELDIALOGTOOL_INCLUDED
#define VRUI_PANELDIALOGTOOL_INCLUDED

#include <Vrui/UserInterfaceTool.h>

/* Forward declarations: */
namespace Vrui {
class ToolManager;
}

namespace Vrui {

class PanelDialogTool;

class PanelDialogToolFactory:public ToolFactory
	{
	friend class PanelDialogTool;
	
	/* Constructors and destructors: */
	public:
	PanelDialogToolFactory(ToolManager& toolManager);
	virtual ~PanelDialogToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class PanelDialogTool:public UserInterfaceTool
	{
	friend class PanelDialogToolFactory;
	
	/* Elements: */
	private:
	static PanelDialogToolFactory* factory; // Pointer to the factory object for this class
	GLMotif::PopupWindow* grabbedDialog; // Pointer to the grabbed dialog window, or 0 if none is grabbed
	
	/* Constructors and destructors: */
	public:
	PanelDialogTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif

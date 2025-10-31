/***********************************************************************
StylusTool - Class to represent styluses on touchscreen-like devices,
where a set of selector buttons changes the function triggered by the
activation of a main button.
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

#ifndef VRUI_STYLUSTOOL_INCLUDED
#define VRUI_STYLUSTOOL_INCLUDED

#include <Vrui/TransformTool.h>

namespace Vrui {

class StylusTool;

class StylusToolFactory:public ToolFactory
	{
	friend class StylusTool;
	
	/* Elements: */
	private:
	int numComponents; // Number of component tools (pen, eraser, ...) represented by the input device
	bool chordModifiers; // Flag if a tool's modifier buttons can be chorded
	
	/* Constructors and destructors: */
	public:
	StylusToolFactory(ToolManager& toolManager);
	virtual ~StylusToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class StylusTool:public TransformTool
	{
	friend class StylusToolFactory;
	
	/* Elements: */
	private:
	static StylusToolFactory* factory; // Pointer to the factory object for this class
	int numComponentButtons; // Number of buttons per component tool
	int component; // Index of the active component tool
	int modifierValue; // Current modifier button value
	
	/* Constructors and destructors: */
	public:
	StylusTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~StylusTool(void);
	
	/* Methods from class Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	
	/* Methods from class DeviceForwarder: */
	virtual InputDeviceFeatureSet getSourceFeatures(const InputDeviceFeature& forwardedFeature);
	virtual InputDeviceFeatureSet getForwardedFeatures(const InputDeviceFeature& sourceFeature);
	};

}

#endif

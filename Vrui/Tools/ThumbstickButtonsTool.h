/***********************************************************************
ThumbstickButtonsTool - Transform an analog stick to multiple buttons
arranged around a circle.
Copyright (c) 2021 Oliver Kreylos

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

#ifndef VRUI_THUMBSTICKBUTTONSTOOL_INCLUDED
#define VRUI_THUMBSTICKBUTTONSTOOL_INCLUDED

#include <Vrui/TransformTool.h>

namespace Vrui {

class ThumbstickButtonsTool;

class ThumbstickButtonsToolFactory:public ToolFactory
	{
	friend class ThumbstickButtonsTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		int numButtons; // Number of buttons to arrange around the circumference of the thumbstick
		double activationThresholds[2]; // Thresholds to deactivate and activate a button
		
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
	ThumbstickButtonsToolFactory(ToolManager& toolManager);
	virtual ~ThumbstickButtonsToolFactory(void);
	
	/* Methods from class ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getValuatorFunction(int valuatorSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ThumbstickButtonsTool:public TransformTool
	{
	friend class ThumbstickButtonsToolFactory;
	
	/* Elements: */
	static ThumbstickButtonsToolFactory* factory; // Pointer to the factory object for this class
	ThumbstickButtonsToolFactory::Configuration configuration; // Private configuration of this tool
	double anglePerButton; // Angle allocated for each perimeter button in radians
	int pressedButton; // Index of currently pressed button, or -1
	
	/* Constructors and destructors: */
	public:
	ThumbstickButtonsTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~ThumbstickButtonsTool(void);
	
	/* Methods from class Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData);
	
	/* Methods from class DeviceForwarder: */
	virtual InputDeviceFeatureSet getSourceFeatures(const InputDeviceFeature& forwardedFeature);
	virtual InputDeviceFeatureSet getForwardedFeatures(const InputDeviceFeature& sourceFeature);
	};

}

#endif

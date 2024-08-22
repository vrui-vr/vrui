/***********************************************************************
ButtonToValuatorTool - Class to convert a single button or two buttons
into a two- or three-state valuator, respectively.
Copyright (c) 2011-2024 Oliver Kreylos

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

#ifndef VRUI_BUTTONTOVALUATORTOOL_INCLUDED
#define VRUI_BUTTONTOVALUATORTOOL_INCLUDED

#include <Vrui/TransformTool.h>

namespace Vrui {

class ButtonToValuatorTool;

class ButtonToValuatorToolFactory:public ToolFactory
	{
	friend class ButtonToValuatorTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Embedded classes: */
		public:
		enum Mode // Enumerated type for valuator control modes
			{
			Immediate=0, // Pressing the button immediately sets the valuator to +1 or -1
			Ramped, // Pressing and holding the button increases/decreases the valuator over time
			Incremental // Pressing the button increments or decrements the valuator value
			};
		
		/* Elements: */
		public:
		std::string deviceName; // Name of the created virtual device
		Mode mode; // Button mode
		double step; // Step by which the valuator value increments or decrements
		double exponent; // Exponent to convert raw valuator value to reported value
		
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
	ButtonToValuatorToolFactory(ToolManager& toolManager);
	virtual ~ButtonToValuatorToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ButtonToValuatorTool:public TransformTool
	{
	friend class ButtonToValuatorToolFactory;
	
	/* Elements: */
	private:
	static ButtonToValuatorToolFactory* factory; // Pointer to the factory object for this class
	ButtonToValuatorToolFactory::Configuration configuration; // Private configuration of this tool
	double rawValue; // Current "raw" valuator value
	
	/* Constructors and destructors: */
	public:
	ButtonToValuatorTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~ButtonToValuatorTool(void);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	
	/* Methods from class DeviceForwarder: */
	virtual InputDeviceFeatureSet getSourceFeatures(const InputDeviceFeature& forwardedFeature);
	virtual InputDeviceFeatureSet getForwardedFeatures(const InputDeviceFeature& sourceFeature);
	};

}

#endif

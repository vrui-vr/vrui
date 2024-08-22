/***********************************************************************
SceneGraphViewerSurfaceTouchTransformTool - Transform tool to check an
input device for collision with a surface in a VRML model and press a
virtual button if a collision is detected.
Copyright (c) 2021-2023 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPHVIEWERSURFACETOUCHTRANSFORMTOOL_INCLUDED
#define SCENEGRAPHVIEWERSURFACETOUCHTRANSFORMTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Vrui/Types.h>
#include <Vrui/TransformTool.h>

#include "SceneGraphViewer.h"

/****************
Embedded classes:
****************/

class SceneGraphViewer::SurfaceTouchTransformToolFactory:public Vrui::ToolFactory
	{
	friend class SurfaceTouchTransformTool;
	
	/* Embedded classes: */
	private:
	typedef Vrui::Scalar Scalar;
	
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		Scalar probeRadius; // Radius of probe sphere in physical-space units
		Scalar probeOffset; // Offset of probe sphere's starting point along the source device's ray in physical-space units
		Scalar probeLength; // Length of probe sphere's travel from the source device's ray start in physical-space units
		Scalar hapticTickDistance; // Distance between haptic ticks while the device is touching a surface in physical-space units
		
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
	SurfaceTouchTransformToolFactory(Vrui::ToolManager& toolManager);
	virtual ~SurfaceTouchTransformToolFactory(void);
	
	/* Methods from Vrui::ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Vrui::Tool* createTool(const Vrui::ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Vrui::Tool* tool) const;
	};

class SceneGraphViewer::SurfaceTouchTransformTool:public Vrui::TransformTool
	{
	friend class SurfaceTouchTransformToolFactory;
	
	/* Embedded classes: */
	private:
	typedef Vrui::Scalar Scalar;
	typedef Vrui::Point Point;
	typedef Vrui::Vector Vector;
	
	/* Elements: */
	private:
	static SurfaceTouchTransformToolFactory* factory; // Pointer to the factory object for this class
	SurfaceTouchTransformToolFactory::Configuration configuration; // Private configuration of this tool
	
	Vrui::InputDevice* rootDevice; // Pointer to the root input device to which this tool is attached
	bool hasHapticFeature; // Flag whether the root input device has a haptic feature
	bool active; // Flag whether the tool is active, i.e., looking for surface collisions
	bool touching; // Flag if the tool is currently touching a surface
	Point lastTouchPos; // Last position at which the tool touched a surface
	Scalar lastHapticDist; // Distance the tool has traveled since the last haptic tick was generated
	
	/* Constructors and destructors: */
	public:
	SurfaceTouchTransformTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const Vrui::ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

#endif

/***********************************************************************
SceneGraphViewerSurfaceTouchTransformTool - Transform tool to check an
input device for collision with a surface in a VRML model and press a
virtual button if a collision is detected.
Copyright (c) 2021-2022 Oliver Kreylos

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

#include "SceneGraphViewerSurfaceTouchTransformTool.h"

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Ray.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/SceneGraphManager.h>

/**********************************************************************************
Methods of class SceneGraphViewer::SurfaceTouchTransformToolFactory::Configuration:
**********************************************************************************/

SceneGraphViewer::SurfaceTouchTransformToolFactory::Configuration::Configuration(void)
	:probeRadius(Vrui::getUiSize()*Scalar(0.5)),
	 probeOffset(-Vrui::getInchFactor()*Scalar(2.0)),
	 probeLength(Vrui::getInchFactor()*Scalar(2.0)),
	 hapticTickDistance(Vrui::getInchFactor()*Scalar(2.0))
	{
	}

void SceneGraphViewer::SurfaceTouchTransformToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./probeRadius",probeRadius);
	cfs.updateValue("./probeOffset",probeOffset);
	cfs.updateValue("./probeLength",probeLength);
	cfs.updateValue("./hapticTickDistance",hapticTickDistance);
	}

void SceneGraphViewer::SurfaceTouchTransformToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./probeRadius",probeRadius);
	cfs.storeValue("./probeOffset",probeOffset);
	cfs.storeValue("./probeLength",probeLength);
	cfs.storeValue("./hapticTickDistance",hapticTickDistance);
	}

/*******************************************************************
Methods of class SceneGraphViewer::SurfaceTouchTransformToolFactory:
*******************************************************************/

SceneGraphViewer::SurfaceTouchTransformToolFactory::SurfaceTouchTransformToolFactory(Vrui::ToolManager& toolManager)
	:Vrui::ToolFactory("SceneGraphViewerSurfaceTouchTransformTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	layout.setNumValuators(0);
	
	/* Insert class into class hierarchy: */
	Vrui::ToolFactory* transformToolFactory=toolManager.loadClass("TransformTool");
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	configuration.read(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	SurfaceTouchTransformTool::factory=this;
	}

SceneGraphViewer::SurfaceTouchTransformToolFactory::~SurfaceTouchTransformToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SurfaceTouchTransformTool::factory=0;
	}

const char* SceneGraphViewer::SurfaceTouchTransformToolFactory::getName(void) const
	{
	return "Touch Scene Graph";
	}

const char* SceneGraphViewer::SurfaceTouchTransformToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Forwarded Button";
			break;
		
		default:
			return 0;
		}
	}

Vrui::Tool* SceneGraphViewer::SurfaceTouchTransformToolFactory::createTool(const Vrui::ToolInputAssignment& inputAssignment) const
	{
	return new SurfaceTouchTransformTool(this,inputAssignment);
	}

void SceneGraphViewer::SurfaceTouchTransformToolFactory::destroyTool(Vrui::Tool* tool) const
	{
	delete tool;
	}

/********************************************************************
Static elements of class SceneGraphViewer::SurfaceTouchTransformTool:
********************************************************************/

SceneGraphViewer::SurfaceTouchTransformToolFactory* SceneGraphViewer::SurfaceTouchTransformTool::factory=0;

/************************************************************
Methods of class SceneGraphViewer::SurfaceTouchTransformTool:
************************************************************/

SceneGraphViewer::SurfaceTouchTransformTool::SurfaceTouchTransformTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::TransformTool(factory,inputAssignment),
	 configuration(SurfaceTouchTransformTool::factory->configuration),
	 rootDevice(0),hasHapticFeature(false),
	 active(false),touching(false)
	{
	/* This tool does not have private buttons: */
	numPrivateButtons=0;
	}

void SceneGraphViewer::SurfaceTouchTransformTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void SceneGraphViewer::SurfaceTouchTransformTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void SceneGraphViewer::SurfaceTouchTransformTool::initialize(void)
	{
	/* Let the base class do its thing: */
	TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	Vrui::getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Retrieve this tool's root input device: */
	rootDevice=Vrui::getInputGraphManager()->getRootDevice(getButtonDevice(0));
	
	/* Check if the root device has a haptic feature: */
	hasHapticFeature=Vrui::getInputDeviceManager()->hasHapticFeature(rootDevice);
	}

const Vrui::ToolFactory* SceneGraphViewer::SurfaceTouchTransformTool::getFactory(void) const
	{
	return factory;
	}

void SceneGraphViewer::SurfaceTouchTransformTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Forward button events unless there is a tool attached to the transformed device's button: */
	if(Vrui::getInputGraphManager()->getFeatureTool(transformedDevice,0)==0)
		{
		transformedDevice->setButtonState(0,cbData->newButtonState);
		active=false;
		}
	else
		{
		/* Activate or deactivate the tool: */
		active=cbData->newButtonState;
		if(!active)
			{
			/* Snap the transformed device back to the source device: */
			resetDevice();
			
			if(touching)
				{
				/* Release the button on the transformed device: */
				transformedDevice->setButtonState(0,false);
				
				/* Request a haptic tick: */
				Vrui::getInputDeviceManager()->hapticTick(rootDevice,5,100,128);
				
				touching=false;
				}
			}
		}
	}

void SceneGraphViewer::SurfaceTouchTransformTool::frame(void)
	{
	/* Issue collision requests only if the tool is active: */
	if(active)
		{
		/* Issue a sphere collision request against Vrui's central scene graph: */
		const Vrui::NavTransform& invNav=Vrui::getInverseNavigationTransformation();
		Scalar probeRadius=invNav.getScaling()*configuration.probeRadius;
		Vrui::Ray ray=getButtonDeviceRay(0);
		Point probeStart=ray(configuration.probeOffset);
		Vector probeStep=ray.getDirection()*configuration.probeLength;
		SceneGraph::SphereCollisionQuery probeQuery(invNav.transform(probeStart),invNav.transform(probeStep),probeRadius);
		Vrui::getSceneGraphManager()->testNavigationalCollision(probeQuery);
		if(probeQuery.isHit())
			{
			/* Position the transformed device at the site of the collision: */
			Point hitPos(Geometry::addScaled(probeStart,probeStep,Scalar(probeQuery.getHitLambda())));
			transformedDevice->setTransformation(Vrui::TrackerState(hitPos-Point::origin,getButtonDeviceTransformation(0).getRotation()));
			
			if(!touching)
				{
				/* Press the button on the transformed device: */
				transformedDevice->setButtonState(0,true);
				
				if(hasHapticFeature)
					{
					/* Request a haptic tick: */
					Vrui::getInputDeviceManager()->hapticTick(rootDevice,5,100,128);
					lastTouchPos=hitPos;
					lastHapticDist=Scalar(0);
					}
				
				touching=true;
				}
			else if(hasHapticFeature)
				{
				/* Keep track of traveled distance since the last haptic tick: */
				lastHapticDist+=Geometry::dist(hitPos,lastTouchPos);
				lastTouchPos=hitPos;
				if(lastHapticDist>=configuration.hapticTickDistance)
					{
					/* Request a haptic tick: */
					Vrui::getInputDeviceManager()->hapticTick(rootDevice,1,1,128);
					
					/* Reset the haptic distance: */
					lastHapticDist-=Math::floor(lastHapticDist/configuration.hapticTickDistance)*configuration.hapticTickDistance;
					}
				}
			}
		else
			{
			/* Snap the transformed device back to the source device: */
			resetDevice();
			
			if(touching)
				{
				/* Release the button on the transformed device: */
				transformedDevice->setButtonState(0,false);
				
				/* Request a haptic tick: */
				Vrui::getInputDeviceManager()->hapticTick(rootDevice,5,100,128);
				
				touching=false;
				}
			}
		}
	else
		{
		/* Lock the transformed device to the source device: */
		resetDevice();
		}
	}

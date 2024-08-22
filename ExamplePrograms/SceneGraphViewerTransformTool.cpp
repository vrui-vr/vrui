/***********************************************************************
SceneGraphViewerTransformTool - Transform tool to place a virtual input
device at the intersection of a ray with a scene graph's geometry.
Copyright (c) 2023 Oliver Kreylos

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

#include "SceneGraphViewerTransformTool.h"

#include <SceneGraph/SphereCollisionQuery.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/SceneGraphManager.h>

/*******************************************************
Methods of class SceneGraphViewer::TransformToolFactory:
*******************************************************/

SceneGraphViewer::TransformToolFactory::TransformToolFactory(Vrui::ToolManager& toolManager)
	:Vrui::ToolFactory("SceneGraphViewerTransformTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(0,true);
	layout.setNumValuators(0,true);
	
	/* Insert class into class hierarchy: */
	Vrui::ToolFactory* transformToolFactory=toolManager.loadClass("TransformTool");
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Set tool class's factory pointer: */
	TransformTool::factory=this;
	}

SceneGraphViewer::TransformToolFactory::~TransformToolFactory(void)
	{
	/* Reset tool class's factory pointer: */
	TransformTool::factory=0;
	}

const char* SceneGraphViewer::TransformToolFactory::getName(void) const
	{
	return "Project to Scene Graph";
	}

const char* SceneGraphViewer::TransformToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	return "Forwarded Button";
	}

const char* SceneGraphViewer::TransformToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	return "Forwarded Valuator";
	}

Vrui::Tool* SceneGraphViewer::TransformToolFactory::createTool(const Vrui::ToolInputAssignment& inputAssignment) const
	{
	return new TransformTool(this,inputAssignment);
	}

void SceneGraphViewer::TransformToolFactory::destroyTool(Vrui::Tool* tool) const
	{
	delete tool;
	}

/********************************************************
Static elements of class SceneGraphViewer::TransformTool:
********************************************************/

SceneGraphViewer::TransformToolFactory* SceneGraphViewer::TransformTool::factory=0;

/************************************************
Methods of class SceneGraphViewer::TransformTool:
************************************************/

SceneGraphViewer::TransformTool::TransformTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::TransformTool(factory,inputAssignment)
	{
	/* This tool does not have private buttons: */
	numPrivateButtons=0;
	}

void SceneGraphViewer::TransformTool::initialize(void)
	{
	/* Let the base class do its thing: */
	Vrui::TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	Vrui::getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	}

const Vrui::ToolFactory* SceneGraphViewer::TransformTool::getFactory(void) const
	{
	return factory;
	}

void SceneGraphViewer::TransformTool::frame(void)
	{
	/* Issue a zero-radius sphere collision request against Vrui's central scene graph: */
	const Vrui::NavTransform& invNav=Vrui::getInverseNavigationTransformation();
	Vrui::Ray ray=getButtonDeviceRay(0);
	Point probeStart=ray.getOrigin();
	Vector probeStep=ray.getDirection()*Vrui::getBackplaneDist();
	SceneGraph::SphereCollisionQuery probeQuery(invNav.transform(probeStart),invNav.transform(probeStep),Vrui::Scalar(0));
	Vrui::getSceneGraphManager()->testNavigationalCollision(probeQuery);
	if(probeQuery.isHit())
		{
		/* Position the transformed device at the site of the collision and align its y direction with the surface normal: */
		Point hitPos(Geometry::addScaled(probeStart,probeStep,Scalar(probeQuery.getHitLambda())));
		Vrui::TrackerState transform=Vrui::TrackerState::translateFromOriginTo(hitPos);
		Vector physNormal=Vrui::getNavigationTransformation().transform(probeQuery.getHitNormal());
		transform*=Vrui::TrackerState::rotate(Vrui::Rotation::rotateFromTo(getButtonDeviceTransformation(0).getDirection(1),-physNormal));
		transform*=Vrui::TrackerState::rotate(getButtonDeviceTransformation(0).getRotation());
		transform.renormalize();
		transformedDevice->setTransformation(transform);
		}
	else
		{
		/* Snap the transformed device back to the source device: */
		resetDevice();
		}
	}

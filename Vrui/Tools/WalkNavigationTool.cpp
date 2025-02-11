/***********************************************************************
WalkNavigationTool - Class to navigate in a VR environment by walking
around a fixed center position.
Copyright (c) 2007-2025 Oliver Kreylos

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

#include <Vrui/Tools/WalkNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/GLValueCoders.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/LineSetNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/Viewer.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/******************************************
Methods of class WalkNavigationToolFactory:
******************************************/

WalkNavigationToolFactory::WalkNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("WalkNavigationTool",toolManager),
	 centerOnActivation(false),
	 centerPoint(getDisplayCenter()),
	 moveSpeed(getDisplaySize()),
	 innerRadius(getDisplaySize()*Scalar(0.5)),outerRadius(getDisplaySize()*Scalar(0.75)),
	 centerViewDirection(getForwardDirection()),
	 rotateSpeed(Math::rad(Scalar(120))),
	 innerAngle(Math::rad(Scalar(30))),outerAngle(Math::rad(Scalar(120))),
	 drawMovementCircles(true),
	 movementCircleColor(0.0f,1.0f,0.0f)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	const EnvironmentDefinition& ed=getEnvironmentDefinition();
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	cfs.updateValue("./centerOnActivation",centerOnActivation);
	cfs.updateValue("./centerPoint",centerPoint);
	centerPoint=ed.calcFloorPoint(centerPoint);
	cfs.updateValue("./moveSpeed",moveSpeed);
	cfs.updateValue("./innerRadius",innerRadius);
	cfs.updateValue("./outerRadius",outerRadius);
	cfs.updateValue("./centerViewDirection",centerViewDirection);
	centerViewDirection.orthogonalize(ed.up).normalize();
	rotateSpeed=Math::rad(cfs.retrieveValue("./rotateSpeed",Math::deg(rotateSpeed)));
	innerAngle=Math::rad(cfs.retrieveValue("./innerAngle",Math::deg(innerAngle)));
	outerAngle=Math::rad(cfs.retrieveValue("./outerAngle",Math::deg(outerAngle)));
	cfs.updateValue("./drawMovementCircles",drawMovementCircles);
	cfs.updateValue("./movementCircleColor",movementCircleColor);
	
	/* Set tool class' factory pointer: */
	WalkNavigationTool::factory=this;
	}

WalkNavigationToolFactory::~WalkNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WalkNavigationTool::factory=0;
	}

const char* WalkNavigationToolFactory::getName(void) const
	{
	return "Walk";
	}

const char* WalkNavigationToolFactory::getButtonFunction(int) const
	{
	return "Start / Stop";
	}

Tool* WalkNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new WalkNavigationTool(this,inputAssignment);
	}

void WalkNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveWalkNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createWalkNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	WalkNavigationToolFactory* walkNavigationToolFactory=new WalkNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return walkNavigationToolFactory;
	}

extern "C" void destroyWalkNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*******************************************
Static elements of class WalkNavigationTool:
*******************************************/

WalkNavigationToolFactory* WalkNavigationTool::factory=0;

/***********************************
Methods of class WalkNavigationTool:
***********************************/

void WalkNavigationTool::showMovementCircles(void)
	{
	/* Calculate a rotation to align the movement circles with the horizontal plane: */
	const EnvironmentDefinition& ed=getEnvironmentDefinition();
	Rotation frame=ed.calcStandardRotation();
	
	/* Rotate the movement circles around the vertical axis to align the angle wedge to the center viewing direction: */
	Vector frameCvd=frame.inverseTransform(centerViewDirection);
	frame*=Rotation::rotateZ(Math::atan2(-frameCvd[0],frameCvd[1]));
	
	/* Add the movement circles to Vrui's physical-space scene graph: */
	circleRoot->setTransform(ONTransform(centerPoint-Point::origin,frame));
	getSceneGraphManager()->addPhysicalNode(*circleRoot);
	}

WalkNavigationTool::WalkNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment)
	{
	}

void WalkNavigationTool::initialize(void)
	{
	if(!factory->centerOnActivation)
		{
		/* Set the fixed center point and center view direction: */
		centerPoint=factory->centerPoint;
		centerViewDirection=factory->centerViewDirection;
		}
	
	if(factory->drawMovementCircles)
		{
		/* Create the scene graph to draw the movement circles: */
		circleRoot=new SceneGraph::ONTransformNode;
		
		SceneGraph::ShapeNodePointer shape=new SceneGraph::ShapeNode;
		circleRoot->addChild(*shape);
		
		SceneGraph::LineSetNodePointer movementCircles=new SceneGraph::LineSetNode;
		shape->geometry.setValue(movementCircles);
		movementCircles->lineWidth.setValue(1.0f);
		movementCircles->setColor(SceneGraph::LineSetNode::VertexColor(factory->movementCircleColor));
		
		/* Draw the inner and outer circles: */
		SceneGraph::Scalar tolerance(getMeterFactor()*Scalar(0.0005));
		movementCircles->addCircle(SceneGraph::Point::origin,SceneGraph::Rotation::identity,factory->innerRadius,tolerance);
		movementCircles->addCircle(SceneGraph::Point::origin,SceneGraph::Rotation::identity,factory->outerRadius,tolerance);
		
		/* Check if view direction rotation is enabled: */
		if(factory->rotateSpeed>Scalar(0))
			{
			/* Draw the inner angle: */
			SceneGraph::LineSetNode::VertexIndex base=movementCircles->getNextVertexIndex();
			movementCircles->addVertex(Point::origin);
			movementCircles->addVertex(Point(-Math::sin(factory->innerAngle)*factory->innerRadius,Math::cos(factory->innerAngle)*factory->innerRadius,0));
			movementCircles->addVertex(Point(Math::sin(factory->innerAngle)*factory->innerRadius,Math::cos(factory->innerAngle)*factory->innerRadius,0));
			movementCircles->addLine(base+1,base);
			movementCircles->addLine(base,base+2);
			
			/* Draw the outer angle: */
			movementCircles->addVertex(Point(-Math::sin(factory->outerAngle)*factory->outerRadius,Math::cos(factory->outerAngle)*factory->outerRadius,0));
			movementCircles->addVertex(Point(Math::sin(factory->outerAngle)*factory->outerRadius,Math::cos(factory->outerAngle)*factory->outerRadius,0));
			movementCircles->addLine(base+3,base);
			movementCircles->addLine(base,base+4);
			}
		
		movementCircles->update();
		
		/* Add fixed movement circles to Vrui's physical-space scene graph: */
		if(!factory->centerOnActivation)
			showMovementCircles();
		}
	}

void WalkNavigationTool::deinitialize(void)
	{
	/* Remove fixed movement circles from Vrui's physical-space scene graph: */
	if(factory->drawMovementCircles&&!factory->centerOnActivation)
		getSceneGraphManager()->removePhysicalNode(*circleRoot);
	}

const ToolFactory* WalkNavigationTool::getFactory(void) const
	{
	return factory;
	}

void WalkNavigationTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Act depending on this tool's current state: */
		if(isActive())
			{
			/* Deactivate this tool: */
			deactivate();
			
			/* Remove dynamic movement circles from Vrui's physical-space scene graph: */
			if(factory->centerOnActivation&&factory->drawMovementCircles)
				getSceneGraphManager()->removePhysicalNode(*circleRoot);
			}
		else
			{
			/* Try activating this tool: */
			if(activate())
				{
				if(factory->centerOnActivation)
					{
					/* Store the center point and center viewing direction for this navigation sequence: */
					const EnvironmentDefinition& ed=getEnvironmentDefinition();
					centerPoint=ed.calcFloorPoint(getMainViewer()->getHeadPosition());
					centerViewDirection=getMainViewer()->getViewDirection();
					centerViewDirection.orthogonalize(ed.up).normalize();
					
					/* Add dynamic movement circles to Vrui's physical-space scene graph: */
					if(factory->drawMovementCircles)
						showMovementCircles();
					}
				
				/* Initialize the navigation transformation: */
				preScale=Vrui::getNavigationTransformation();
				translation=Vector::zero;
				azimuth=Scalar(0);
				}
			}
		}
	}

void WalkNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		bool animating=false;
		const EnvironmentDefinition& ed=getEnvironmentDefinition();
		
		/* Calculate azimuth angle change based on the current viewing direction if rotation is enabled: */
		if(factory->rotateSpeed>Scalar(0))
			{
			Vector viewDir=getMainViewer()->getViewDirection();
			viewDir.orthogonalize(ed.up);
			Scalar viewDir2=Geometry::sqr(viewDir);
			if(viewDir2!=Scalar(0))
				{
				/* Calculate the rotation speed: */
				Scalar viewAngle=Math::acos(Math::clamp((viewDir*centerViewDirection)/Math::sqrt(viewDir2),Scalar(-1),Scalar(1)));
				Scalar rotateSpeed=factory->rotateSpeed*Math::clamp((viewAngle-factory->innerAngle)/(factory->outerAngle-factory->innerAngle),Scalar(0),Scalar(1));
				Vector x=centerViewDirection^ed.up;
				if(viewDir*x<Scalar(0))
					rotateSpeed=-rotateSpeed;
				
				if(rotateSpeed!=Scalar(0))
					{
					/* Update the accumulated rotation angle: */
					azimuth+=rotateSpeed*getFrameTime();
					if(azimuth<-Math::Constants<Scalar>::pi)
						azimuth+=Scalar(2)*Math::Constants<Scalar>::pi;
					else if(azimuth>=Math::Constants<Scalar>::pi)
						azimuth-=Scalar(2)*Math::Constants<Scalar>::pi;
					
					animating=true;
					}
				}
			}
		
		/* Calculate the movement direction and speed: */
		Point footPos=ed.calcFloorPoint(getMainViewer()->getHeadPosition());
		Vector moveDir=centerPoint-footPos;
		Scalar moveDirLen=moveDir.mag();
		if(moveDirLen>Scalar(0))
			{
			Scalar speed=factory->moveSpeed*Math::clamp((moveDirLen-factory->innerRadius)/(factory->outerRadius-factory->innerRadius),Scalar(0),Scalar(1));
			moveDir*=speed/moveDirLen;
			if(speed!=Scalar(0))
				animating=true;
			}
		
		/* Accumulate the transformation: */
		NavTransform::Rotation rot=NavTransform::Rotation::rotateAxis(ed.up,azimuth);
		translation+=rot.inverseTransform(moveDir*getFrameTime());
		
		/* Set the navigation transformation: */
		NavTransform nav=NavTransform::identity;
		nav*=Vrui::NavTransform::translateFromOriginTo(centerPoint);
		nav*=Vrui::NavTransform::rotate(rot);
		nav*=Vrui::NavTransform::translateToOriginFrom(centerPoint);
		nav*=Vrui::NavTransform::translate(translation);
		nav*=preScale;
		setNavigationTransformation(nav);
		
		/* Request another frame if animating: */
		if(animating)
			scheduleUpdate(getNextAnimationTime());
		}
	}

}

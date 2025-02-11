/***********************************************************************
WalkSurfaceNavigationTool - Version of the WalkNavigationTool that lets
a user navigate along an application-defined surface.
Copyright (c) 2009-2025 Oliver Kreylos

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

#include <Vrui/Tools/WalkSurfaceNavigationTool.h>

#include <Misc/PrintInteger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLFont.h>
#include <GL/GLValueCoders.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/LineSetNode.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/Viewer.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/****************************************************************
Methods of class WalkSurfaceNavigationToolFactory::Configuration:
****************************************************************/

WalkSurfaceNavigationToolFactory::Configuration::Configuration(void)
	:centerOnActivation(false),
	 centerPoint(getDisplayCenter()),
	 moveSpeed(getDisplaySize()),
	 innerRadius(getDisplaySize()*Scalar(0.5)),outerRadius(getDisplaySize()*Scalar(0.75)),
	 centerViewDirection(getForwardDirection()),
	 rotateSpeed(Math::rad(Scalar(120))),
	 innerAngle(Math::rad(Scalar(30))),outerAngle(Math::rad(Scalar(120))),
	 fallAcceleration(getMeterFactor()*Scalar(9.81)),
	 jetpackAcceleration(fallAcceleration*Scalar(1.5)),
	 probeSize(getInchFactor()*Scalar(12)),
	 maxClimb(getInchFactor()*Scalar(12)),
	 fixAzimuth(false),
	 drawMovementCircles(true),
	 movementCircleColor(0.0f,1.0f,0.0f),
	 drawHud(true),
	 hudRadius(getDisplaySize()*Scalar(2)),
	 hudFontSize(Scalar(getUiFont()->getTextHeight()))
	{
	}

void WalkSurfaceNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./centerOnActivation",centerOnActivation);
	cfs.updateValue("./centerPoint",centerPoint);
	centerPoint=calcFloorPoint(centerPoint);
	cfs.updateValue("./moveSpeed",moveSpeed);
	cfs.updateValue("./innerRadius",innerRadius);
	cfs.updateValue("./outerRadius",outerRadius);
	cfs.updateValue("./centerViewDirection",centerViewDirection);
	centerViewDirection.orthogonalize(getUpDirection()).normalize();
	rotateSpeed=Math::rad(cfs.retrieveValue("./rotateSpeed",Math::deg(rotateSpeed)));
	innerAngle=Math::rad(cfs.retrieveValue("./innerAngle",Math::deg(innerAngle)));
	outerAngle=Math::rad(cfs.retrieveValue("./outerAngle",Math::deg(outerAngle)));
	cfs.updateValue("./fallAcceleration",fallAcceleration);
	jetpackAcceleration=cfs.retrieveValue("./jetpackAcceleration",fallAcceleration*Scalar(1.5));
	cfs.updateValue("./probeSize",probeSize);
	cfs.updateValue("./maxClimb",maxClimb);
	cfs.updateValue("./fixAzimuth",fixAzimuth);
	cfs.updateValue("./drawMovementCircles",drawMovementCircles);
	cfs.updateValue("./movementCircleColor",movementCircleColor);
	cfs.updateValue("./drawHud",drawHud);
	cfs.updateValue("./hudRadius",hudRadius);
	cfs.updateValue("./hudFontSize",hudFontSize);
	}

void WalkSurfaceNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./centerOnActivation",centerOnActivation);
	cfs.storeValue("./centerPoint",centerPoint);
	cfs.storeValue("./moveSpeed",moveSpeed);
	cfs.storeValue("./innerRadius",innerRadius);
	cfs.storeValue("./outerRadius",outerRadius);
	cfs.storeValue("./centerViewDirection",centerViewDirection);
	cfs.storeValue("./rotateSpeed",Math::deg(rotateSpeed));
	cfs.storeValue("./innerAngle",Math::deg(innerAngle));
	cfs.storeValue("./outerAngle",Math::deg(outerAngle));
	cfs.storeValue("./fallAcceleration",fallAcceleration);
	cfs.storeValue("./jetpackAcceleration",fallAcceleration*Scalar(1.5));
	cfs.storeValue("./probeSize",probeSize);
	cfs.storeValue("./maxClimb",maxClimb);
	cfs.storeValue("./fixAzimuth",fixAzimuth);
	cfs.storeValue("./drawMovementCircles",drawMovementCircles);
	cfs.storeValue("./movementCircleColor",movementCircleColor);
	cfs.storeValue("./drawHud",drawHud);
	cfs.storeValue("./hudRadius",hudRadius);
	cfs.storeValue("./hudFontSize",hudFontSize);
	}

/*************************************************
Methods of class WalkSurfaceNavigationToolFactory:
*************************************************/

WalkSurfaceNavigationToolFactory::WalkSurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("WalkSurfaceNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	layout.setNumValuators(0,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	configuration.read(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	WalkSurfaceNavigationTool::factory=this;
	}

WalkSurfaceNavigationToolFactory::~WalkSurfaceNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WalkSurfaceNavigationTool::factory=0;
	}

const char* WalkSurfaceNavigationToolFactory::getName(void) const
	{
	return "Walk";
	}

const char* WalkSurfaceNavigationToolFactory::getButtonFunction(int) const
	{
	return "Start / Stop";
	}

const char* WalkSurfaceNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Fire Jetpack";
		
		default:
			return "Unused";
		}
	}

Tool* WalkSurfaceNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new WalkSurfaceNavigationTool(this,inputAssignment);
	}

void WalkSurfaceNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveWalkSurfaceNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createWalkSurfaceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	WalkSurfaceNavigationToolFactory* walkSurfaceNavigationToolFactory=new WalkSurfaceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return walkSurfaceNavigationToolFactory;
	}

extern "C" void destroyWalkSurfaceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************************
Static elements of class WalkSurfaceNavigationTool:
**************************************************/

WalkSurfaceNavigationToolFactory* WalkSurfaceNavigationTool::factory=0;

/******************************************
Methods of class WalkSurfaceNavigationTool:
******************************************/

void WalkSurfaceNavigationTool::showMovementCircles(void)
	{
	/* Calculate a rotation to align the movement circles with the horizontal plane: */
	const EnvironmentDefinition& ed=getEnvironmentDefinition();
	
	/* Rotate the movement circles around the vertical axis to align the angle wedge to the center viewing direction: */
	Rotation frame=hudFrame;
	Vector frameCvd=frame.inverseTransform(centerViewDirection);
	frame*=Rotation::rotateZ(Math::atan2(-frameCvd[0],frameCvd[1]));
	
	/* Add the movement circles to Vrui's physical-space scene graph: */
	circleRoot->setTransform(ONTransform(centerPoint-Point::origin,frame));
	getSceneGraphManager()->addPhysicalNode(*circleRoot);
	}

void WalkSurfaceNavigationTool::applyNavState(void) const
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotateAround(Point(0,0,headHeight),Rotation::rotateX(elevation));
	nav*=NavTransform::rotate(Rotation::rotateZ(azimuth));
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void WalkSurfaceNavigationTool::initNavState(void)
	{
	/* Calculate the main viewer's current head and foot positions: */
	Point headPos=getMainViewer()->getHeadPosition();
	footPos=calcFloorPoint(headPos);
	headHeight=Geometry::dist(headPos,footPos);
	
	/* Set up a physical navigation frame around the main viewer's current head position: */
	calcPhysicalFrame(headPos);
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	NavTransform newSurfaceFrame=surfaceFrame;
	
	/* Reset the falling velocity: */
	fallVelocity=Scalar(0);
	
	/* Align the initial frame with the application's surface and calculate Euler angles: */
	AlignmentData ad(surfaceFrame,newSurfaceFrame,configuration.probeSize,configuration.maxClimb);
	Scalar roll;
	align(ad,azimuth,elevation,roll);
	
	/* Limit the elevation angle to the horizontal: */
	elevation=Scalar(0);
	
	/* If the initial surface frame was above the surface, lift it back up and start falling: */
	Scalar z=newSurfaceFrame.inverseTransform(surfaceFrame.getOrigin())[2];
	if(z>Scalar(0))
		{
		newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
		fallVelocity=-configuration.fallAcceleration*getFrameTime();
		}
	
	/* Move the physical frame to the foot position, and adjust the surface frame accordingly: */
	newSurfaceFrame*=Geometry::invert(physicalFrame)*NavTransform::translate(footPos-headPos)*physicalFrame;
	physicalFrame.leftMultiply(NavTransform::translate(footPos-headPos));
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

WalkSurfaceNavigationTool::WalkSurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 configuration(WalkSurfaceNavigationTool::factory->configuration),
	 hudFrame(getEnvironmentDefinition().calcStandardRotation()),
	 jetpack(0)
	{
	}

void WalkSurfaceNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void WalkSurfaceNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void WalkSurfaceNavigationTool::initialize(void)
	{
	if(!configuration.centerOnActivation)
		{
		/* Set the fixed center point and center view direction: */
		centerPoint=configuration.centerPoint;
		centerViewDirection=configuration.centerViewDirection;
		}
	
	if(configuration.drawMovementCircles)
		{
		/* Create the scene graph to draw the movement circles: */
		circleRoot=new SceneGraph::ONTransformNode;
		
		SceneGraph::ShapeNodePointer shape=new SceneGraph::ShapeNode;
		circleRoot->addChild(*shape);
		
		SceneGraph::LineSetNodePointer movementCircles=new SceneGraph::LineSetNode;
		shape->geometry.setValue(movementCircles);
		movementCircles->lineWidth.setValue(1.0f);
		movementCircles->setColor(SceneGraph::LineSetNode::VertexColor(configuration.movementCircleColor));
		
		/* Draw the inner and outer circles: */
		SceneGraph::Scalar tolerance(getMeterFactor()*Scalar(0.0005));
		movementCircles->addCircle(SceneGraph::Point::origin,SceneGraph::Rotation::identity,configuration.innerRadius,tolerance);
		movementCircles->addCircle(SceneGraph::Point::origin,SceneGraph::Rotation::identity,configuration.outerRadius,tolerance);
		
		/* Check if view direction rotation is enabled: */
		if(configuration.rotateSpeed>Scalar(0))
			{
			/* Draw the inner angle: */
			SceneGraph::LineSetNode::VertexIndex base=movementCircles->getNextVertexIndex();
			movementCircles->addVertex(Point::origin);
			movementCircles->addVertex(Point(-Math::sin(configuration.innerAngle)*configuration.innerRadius,Math::cos(configuration.innerAngle)*configuration.innerRadius,0));
			movementCircles->addVertex(Point(Math::sin(configuration.innerAngle)*configuration.innerRadius,Math::cos(configuration.innerAngle)*configuration.innerRadius,0));
			movementCircles->addLine(base+1,base);
			movementCircles->addLine(base,base+2);
			
			/* Draw the outer angle: */
			movementCircles->addVertex(Point(-Math::sin(configuration.outerAngle)*configuration.outerRadius,Math::cos(configuration.outerAngle)*configuration.outerRadius,0));
			movementCircles->addVertex(Point(Math::sin(configuration.outerAngle)*configuration.outerRadius,Math::cos(configuration.outerAngle)*configuration.outerRadius,0));
			movementCircles->addLine(base+3,base);
			movementCircles->addLine(base,base+4);
			}
		
		movementCircles->update();
		
		/* Add fixed movement circles to Vrui's physical-space scene graph: */
		if(!configuration.centerOnActivation)
			showMovementCircles();
		}
	
	if(configuration.drawHud)
		{
		/* Create the scene graph to draw the compass HUD: */
		hudRoot=new SceneGraph::ONTransformNode;
		
		SceneGraph::ShapeNodePointer shape=new SceneGraph::ShapeNode;
		hudRoot->addChild(*shape);
		
		SceneGraph::LineSetNodePointer hud=new SceneGraph::LineSetNode;
		shape->geometry.setValue(hud);
		hud->lineWidth.setValue(1.0f);
		hud->setColor(SceneGraph::LineSetNode::VertexColor(configuration.movementCircleColor));
		
		/* Draw the azimuth tick marks: */
		for(int az=0;az<360;az+=10)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(az)/Scalar(360);
			Scalar c=Math::cos(angle)*configuration.hudRadius;
			Scalar s=Math::sin(angle)*configuration.hudRadius;
			hud->addLine(SceneGraph::Point(s,c,0),SceneGraph::Point(s,c,az%30==0?configuration.hudFontSize*Scalar(2):configuration.hudFontSize));
			}
		
		/* Draw the azimuth labels: */
		for(int az=0;az<360;az+=30)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(az)/Scalar(360);
			Scalar c=Math::cos(angle)*configuration.hudRadius;
			Scalar s=Math::sin(angle)*configuration.hudRadius;
			char azString[4];
			Rotation rot=Rotation::rotateZ(-angle);
			rot*=Rotation::rotateX(Math::div2(Math::Constants<Scalar>::pi));
			hud->addNumber(SceneGraph::Point(s,c,configuration.hudFontSize*Scalar(2.5)),rot,configuration.hudFontSize,0,-1,Misc::print(az,azString+3));
			}
		
		hud->update();
		}
	}

void WalkSurfaceNavigationTool::deinitialize(void)
	{
	/* Remove fixed movement circles from Vrui's physical-space scene graph: */
	if(configuration.drawMovementCircles&&!configuration.centerOnActivation)
		getSceneGraphManager()->removePhysicalNode(*circleRoot);
	}

const ToolFactory* WalkSurfaceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void WalkSurfaceNavigationTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Act depending on this tool's current state: */
		if(isActive())
			{
			/* Deactivate this tool: */
			deactivate();
			
			/* Remove dynamic movement circles from Vrui's physical-space scene graph: */
			if(configuration.centerOnActivation&&configuration.drawMovementCircles)
				getSceneGraphManager()->removePhysicalNode(*circleRoot);
			
			/* Remove the heads-up display from Vrui's physical-space scene graph: */
			if(configuration.drawHud)
				getSceneGraphManager()->removePhysicalNode(*hudRoot);
			}
		else
			{
			/* Try activating this tool: */
			if(activate())
				{
				/* Add the heads-up display to Vrui's physical-space scene graph: */
				if(configuration.drawHud)
					getSceneGraphManager()->addPhysicalNode(*hudRoot);
				
				if(configuration.centerOnActivation)
					{
					/* Store the center point and center viewing direction for this navigation sequence: */
					const EnvironmentDefinition& ed=getEnvironmentDefinition();
					centerPoint=ed.calcFloorPoint(getMainViewer()->getHeadPosition());
					centerViewDirection=getMainViewer()->getViewDirection();
					centerViewDirection.orthogonalize(ed.up).normalize();
					
					/* Add dynamic movement circles to Vrui's physical-space scene graph: */
					if(configuration.drawMovementCircles)
						showMovementCircles();
					}
				
				/* Initialize the navigation state: */
				initNavState();
				}
			}
		}
	}

void WalkSurfaceNavigationTool::valuatorCallback(int,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Update the jetpack acceleration value: */
	jetpack=Scalar(cbData->newValuatorValue)*configuration.jetpackAcceleration;
	}

void WalkSurfaceNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		bool animating=false;
		const EnvironmentDefinition& ed=getEnvironmentDefinition();
		
		/* Calculate azimuth angle change based on the current viewing direction if rotation is enabled: */
		if(configuration.rotateSpeed>Scalar(0))
			{
			Vector viewDir=getMainViewer()->getViewDirection();
			viewDir.orthogonalize(ed.up);
			Scalar viewDir2=Geometry::sqr(viewDir);
			if(viewDir2!=Scalar(0))
				{
				/* Calculate the rotation speed: */
				Scalar viewAngle=Math::acos(Math::clamp((viewDir*centerViewDirection)/Math::sqrt(viewDir2),Scalar(-1),Scalar(1)));
				Scalar rotateSpeed=configuration.rotateSpeed*Math::clamp((viewAngle-configuration.innerAngle)/(configuration.outerAngle-configuration.innerAngle),Scalar(0),Scalar(1));
				Vector x=centerViewDirection^ed.up;
				if(viewDir*x<Scalar(0))
					rotateSpeed=-rotateSpeed;
				
				/* Update the azimuth angle: */
				if(rotateSpeed!=Scalar(0))
					{
					azimuth=wrapAngle(azimuth+rotateSpeed*getFrameTime());
					animating=true;
					}
				}
			}
		
		/* Calculate the new head and foot positions: */
		Point headPos=getMainViewer()->getHeadPosition();
		Point newFootPos=ed.calcFloorPoint(headPos);
		headHeight=Geometry::dist(headPos,newFootPos);
		
		/* Create a physical navigation frame around the new foot position: */
		calcPhysicalFrame(newFootPos);
		
		/* Calculate the movement from walking: */
		Vector move=newFootPos-footPos;
		footPos=newFootPos;
		
		/* Calculate movement from virtual joystick: */
		Vector moveDir=footPos-centerPoint;
		Scalar moveDirLen=moveDir.mag();
		if(moveDirLen>Scalar(0))
			{
			Scalar speed=configuration.moveSpeed*Math::clamp((moveDirLen-configuration.innerRadius)/(configuration.outerRadius-configuration.innerRadius),Scalar(0),Scalar(1));
			moveDir*=speed/moveDirLen;
			if(speed!=Scalar(0))
				animating=true;
			}
		
		/* Add the current flying and falling velocities: */
		if(jetpack!=Scalar(0))
			{
			moveDir+=getValuatorDeviceRayDirection(0)*jetpack;
			animating=true;
			}
		moveDir+=ed.up*fallVelocity;
		
		/* Calculate the complete movement vector: */
		move+=moveDir*getFrameTime();
		
		/* Transform the movement vector from physical space to the physical navigation frame: */
		move=physicalFrame.inverseTransform(move);
		
		/* Rotate by the current azimuth angle: */
		move=Rotation::rotateZ(-azimuth).transform(move);
		
		/* Move the surface frame: */
		NavTransform newSurfaceFrame=surfaceFrame;
		newSurfaceFrame*=NavTransform::translate(move);
		
		/* Re-align the surface frame with the surface: */
		Point initialOrigin=newSurfaceFrame.getOrigin();
		Rotation initialOrientation=newSurfaceFrame.getRotation();
		AlignmentData ad(surfaceFrame,newSurfaceFrame,configuration.probeSize,configuration.maxClimb);
		align(ad);
		
		if(!configuration.fixAzimuth)
			{
			/* Have the azimuth angle track changes in the surface frame's rotation: */
			Rotation rot=Geometry::invert(initialOrientation)*newSurfaceFrame.getRotation();
			rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
			Vector x=rot.getDirection(0);
			azimuth=wrapAngle(azimuth+Math::atan2(x[1],x[0]));
			}
		
		/* Check if the initial surface frame is above the surface: */
		Scalar z=newSurfaceFrame.inverseTransform(initialOrigin)[2];
		if(z>Scalar(0))
			{
			/* Lift the aligned frame back up to the original altitude and continue flying: */
			newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
			fallVelocity-=configuration.fallAcceleration*getFrameTime();
			animating=true;
			}
		else
			{
			/* Stop falling: */
			fallVelocity=Scalar(0);
			}
		
		/* Apply the newly aligned surface frame: */
		surfaceFrame=newSurfaceFrame;
		applyNavState();
		
		/* Update the heads-up display: */
		if(configuration.drawHud)
			{
			/* Update the heads-up display's transformation: */
			Rotation frame=hudFrame;
			frame*=Rotation::rotateZ(azimuth);
			hudRoot->setTransform(ONTransform(headPos-Point::origin,frame));
			}
		
		/* Request another frame if animating: */
		if(animating)
			scheduleUpdate(getNextAnimationTime());
		}
	}

}

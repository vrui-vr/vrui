/***********************************************************************
InteractionDemo - Simple demonstration of 6-DOF dragging interactions
using the Vrui tool mechanism.
Copyright (c) 2026 Oliver Kreylos

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

#include <vector>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/LinearUnit.h>
#include <SceneGraph/ONTransformNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/BoxNode.h>
#include <SceneGraph/SphereNode.h>
#include <SceneGraph/CylinderNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/ToolManager.h>

class InteractionDemo:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef SceneGraph::Scalar Scalar; // Scalar type for geometry objects
	typedef SceneGraph::Point Point; // Type for points
	typedef SceneGraph::Vector Vector; // Type for vectors
	typedef SceneGraph::ONTransformNode::ONTransform Transform; // Type for 6-DOF transformation nodes
	typedef SceneGraph::ONTransformNodePointer TransformPointer; // Type for pointers to 6-DOF transformation nodes
	typedef std::vector<TransformPointer> TransformList; // List of pointers to SceneGraph::ONTransformNodes representing movable objects
	
	class MoveTool:public Vrui::Tool,public Vrui::Application::Tool<InteractionDemo> // Class for tools that can pick up and move objects
		{
		/* Embedded classes: */
		private:
		typedef Vrui::GenericToolFactory<MoveTool> Factory; // Type for factory objects creating objects of this tool class
		friend class Vrui::GenericToolFactory<MoveTool>;
		
		/* Elements: */
		private:
		static Factory* factory; // The factory object for this tool class
		TransformPointer movedTransform; // Pointer to the transformation node holding the object that is currently being moved, or null
		Vrui::OGTransform offsetTransform; // An offset transformation from the moved object's initial transformation to the input device's initial transformation
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initialize the tool class and register it with Vrui's tool manager
		MoveTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment); // Creates an instance of the tool class
		
		/* Methods from class Vrui::Tool: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		};
	
	friend class MoveTool;
	
	/* Elements: */
	TransformList transforms; // A list of movable objects
	
	/* Constructors and destructors: */
	public:
	InteractionDemo(int& argc,char**& argv);
	
	/* Methods from class Vrui::Application: */
	virtual void resetNavigation(void);
	};

/**************************************************
Static elements of class InteractionDemo::MoveTool:
**************************************************/

InteractionDemo::MoveTool::Factory* InteractionDemo::MoveTool::factory=0;

/******************************************
Methods of class InteractionDemo::MoveTool:
******************************************/

void InteractionDemo::MoveTool::initClass(void)
	{
	/* Create a factory object for the custom tool class: */
	factory=new Factory("InteractionDemo::MoveTool","Move Objects",0,*Vrui::getToolManager());
	
	/* Set the tool class's input layout: */
	factory->setNumButtons(1); // Needs exactly one button
	factory->setButtonFunction(0,"Pick/Move Object");
	
	/* Register the tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

InteractionDemo::MoveTool::MoveTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment)
	{
	}

const Vrui::ToolFactory* InteractionDemo::MoveTool::getFactory(void) const
	{
	return factory;
	}

void InteractionDemo::MoveTool::buttonCallback(int,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Check if the button was pressed or released: */
	if(cbData->newButtonState)
		{
		/* Get the current navigational-space transformation of the input device to which this tool is bound: */
		Vrui::OGTransform deviceTransform=getButtonDeviceNavTransformation(0);
		
		/* Get the device's position: */
		Point devicePos(deviceTransform.getOrigin());
		
		/* Check if any movable objects were picked: */
		for(TransformList::iterator tIt=application->transforms.begin();movedTransform==0&&tIt!=application->transforms.end();++tIt)
			{
			/* Transform the device position to the transformation's local coordinate space: */
			Point objPos=(*tIt)->transform.getValue().inverseTransform(devicePos);
			
			/* Get the transform node's children and check the transformed device position against every child: */
			const SceneGraph::GroupNode::ChildList& children=(*tIt)->getChildren();
			for(SceneGraph::GroupNode::ChildList::const_iterator chIt=children.begin();movedTransform==0&&chIt!=children.end();++chIt)
				{
				/* Check if the child was picked: */
				if((*chIt)->calcBoundingBox().contains(objPos))
					{
					/* Mark the child's parent transformation as being moved: */
					movedTransform=*tIt;
					
					/* Attach the transformation's children to the tool's input device by calculating an incremental transformation from the picked transformation to the current device transformation: */
					offsetTransform=Geometry::invert(deviceTransform)*Vrui::OGTransform(movedTransform->transform.getValue());
					}
				}
			}
		}
	else
		{
		/* Stop moving the currently moved object: */
		movedTransform=0;
		}
	}

void InteractionDemo::MoveTool::frame(void)
	{
	/* Check if there is an object currently being moved: */
	if(movedTransform!=0)
		{
		/* Get the current navigational-space transformation of the input device to which this tool is bound: */
		Vrui::OGTransform deviceTransform=getButtonDeviceNavTransformation(0);
		
		/* Calculate the new object transformation by multiplying the current navigational-space device transformation with the offset transformation: */
		deviceTransform*=offsetTransform;
		
		/* Update the object's transformation with the translation and rotation components of the device transformation, ignoring scaling: */
		movedTransform->setTransform(Transform(deviceTransform.getTranslation(),deviceTransform.getRotation()));
		}
	}

/********************************
Methods of class InteractionDemo:
********************************/

InteractionDemo::InteractionDemo(int& argc,char**& argv)
	:Vrui::Application(argc,argv)
	{
	/* Get a reference to Vrui's scene graph manager: */
	Vrui::SceneGraphManager& sgm=*Vrui::getSceneGraphManager();
	
	/* Create a simple material for our objects: */
	SceneGraph::AppearanceNodePointer appearance=SceneGraph::createPhongAppearance(SceneGraph::Color(1.0,0.667,0.0),SceneGraph::Color(1.0,1.0,0.25),0.9);
	
	/* Pick a size for our objects in some arbitrary unit of measurement; let's use meters: */
	SceneGraph::Scalar size(0.1); // 10cm
	
	/*********************************************
	Programmatically create a few movable objects:
	*********************************************/
	
	{
	/* Create a transform node to place a movable object in navigational space: */
	SceneGraph::ONTransformNodePointer transform=new SceneGraph::ONTransformNode;
	transform->setTransform(Transform::translate(Vector(-0.1,0.0,0.0)));
	
	/* Create a shape node: */
	SceneGraph::ShapeNodePointer shape=new SceneGraph::ShapeNode;
	shape->appearance.setValue(appearance);
	
	/* Create a cube: */
	SceneGraph::BoxNodePointer box=new SceneGraph::BoxNode;
	box->center.setValue(SceneGraph::Point::origin);
	box->size.setValue(SceneGraph::Size(size));
	box->update();
	
	shape->geometry.setValue(box);
	shape->update();
	
	/* Add the shape node to the transform node: */
	transform->addChild(*shape);
	
	/* Add the transform node to the list and to Vrui's navigational-space scene graph: */
	transforms.push_back(transform);
	sgm.addNavigationalNode(*transform);
	}
	
	{
	/* Create a transform node to place a movable object in navigational space: */
	SceneGraph::ONTransformNodePointer transform=new SceneGraph::ONTransformNode;
	transform->setTransform(Transform::translate(SceneGraph::Vector(0.1,0.0,0.0)));
	
	/* Create a shape node: */
	SceneGraph::ShapeNodePointer shape=new SceneGraph::ShapeNode;
	shape->appearance.setValue(appearance);
	
	/* Create a sphere: */
	SceneGraph::SphereNodePointer sphere=new SceneGraph::SphereNode;
	sphere->center.setValue(SceneGraph::Point::origin);
	sphere->radius.setValue(size*Math::pow(Scalar(3)/(Scalar(4)*Math::Constants<Scalar>::pi),Scalar(1)/Scalar(3))); // Give the sphere the same volume as the cube
	sphere->latLong.setValue(false);
	sphere->update();
	
	shape->geometry.setValue(sphere);
	shape->update();
	
	/* Add the shape node to the transform node: */
	transform->addChild(*shape);
	
	/* Add the transform node to the list and to Vrui's navigational-space scene graph: */
	transforms.push_back(transform);
	sgm.addNavigationalNode(*transform);
	}
	
	{
	/* Create a transform node to place a movable object in navigational space: */
	SceneGraph::ONTransformNodePointer transform=new SceneGraph::ONTransformNode;
	transform->setTransform(Transform::translate(SceneGraph::Vector(0.0,0.0,0.1)));
	
	/* Create a shape node: */
	SceneGraph::ShapeNodePointer shape1=new SceneGraph::ShapeNode;
	shape1->appearance.setValue(appearance);
	
	/* Create a cylinder: */
	Misc::Autopointer<SceneGraph::CylinderNode> cylinder1=new SceneGraph::CylinderNode;
	cylinder1->height.setValue(size);
	cylinder1->radius.setValue(size*Scalar(0.125));
	cylinder1->numSegments.setValue(16);
	cylinder1->update();
	
	shape1->geometry.setValue(cylinder1);
	shape1->update();
	
	/* Add the shape node to the transform node: */
	transform->addChild(*shape1);
	
	/* Create another transform node: */
	SceneGraph::ONTransformNodePointer transform1=new SceneGraph::ONTransformNode;
	transform1->setTransform(Transform::translate(SceneGraph::Vector(0.0,size*0.75,0.0)));
	
	/* Create a shape node: */
	SceneGraph::ShapeNodePointer shape2=new SceneGraph::ShapeNode;
	shape2->appearance.setValue(appearance);
	
	/* Create a cylinder: */
	Misc::Autopointer<SceneGraph::CylinderNode> cylinder2=new SceneGraph::CylinderNode;
	cylinder2->height.setValue(size*0.5);
	cylinder2->radius.setValue(size*0.5);
	cylinder2->numSegments.setValue(32);
	cylinder2->update();
	
	shape2->geometry.setValue(cylinder2);
	shape2->update();
	
	/* Add the shape node to the transform node and the transform node to the root transform node: */
	transform1->addChild(*shape2);
	transform->addChild(*transform1);
	
	/* Create another transform node: */
	SceneGraph::ONTransformNodePointer transform2=new SceneGraph::ONTransformNode;
	transform2->setTransform(Transform::translate(SceneGraph::Vector(0.0,-size*0.75,0.0)));
	
	/* Add the shape node to the transform node and the transform node to the root transform node: */
	transform2->addChild(*shape2);
	transform->addChild(*transform2);
	
	/* Add the root transform node to the list and to Vrui's navigational-space scene graph: */
	transforms.push_back(transform);
	sgm.addNavigationalNode(*transform);
	}
	
	/* Tell Vrui that navigational space is using meters as its unit of measurement (this is only relevant to the scale bar): */
	Vrui::getCoordinateManager()->setUnit(Geometry::LinearUnit(Geometry::LinearUnit::METER,1));
	
	/* Initialize the move tool class: */
	MoveTool::initClass();
	}

void InteractionDemo::resetNavigation(void)
	{
	/* Query the definition of Vrui's environment, i.e., the layout of its physical-space coordinate system and its unit of measurement: */
	const Vrui::EnvironmentDefinition& ed=Vrui::getEnvironmentDefinition();
	
	/* Center the display around the origin, and scale it so that our objects are displayed at 1:1 scale: */
	Vrui::NavTransform nav=Vrui::NavTransform::translateFromOriginTo(ed.center); // Move the origin of navigational space to the center of the environment
	nav*=Vrui::NavTransform::rotate(ed.calcStandardRotation()); // Navigational-space x points to the right, and navigational-space z points up
	nav*=Vrui::NavTransform::scale(ed.getMeterFactor()); // 1 unit in navigational space = whatever 1 meter is in physical space
	Vrui::setNavigationTransformation(nav);
	}

/****************
Main entry point:
****************/

VRUI_APPLICATION_RUN(InteractionDemo)

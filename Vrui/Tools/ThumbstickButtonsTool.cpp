/***********************************************************************
ThumbstickButtonsTool - Transform an analog stick to multiple buttons
arranged around a circle.
Copyright (c) 2021-2026 Oliver Kreylos

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

#include <Vrui/Tools/ThumbstickButtonsTool.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/FixedArray.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/GLValueCoders.h>
#include <SceneGraph/MaterialNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/LineSetNode.h>
#include <SceneGraph/TriangleSetNode.h>
#include <SceneGraph/ShapeNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/SceneGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/************************************************************
Methods of class ThumbstickButtonsToolFactory::Configuration:
************************************************************/

ThumbstickButtonsToolFactory::Configuration::Configuration(void)
	:numButtons(4),drawOnTouch(false),thumbstickRadius(1),thumbstickColor(0.0f,1.0f,0.0f,0.333f)
	{
	activationThresholds[0]=0.25;
	activationThresholds[1]=0.75;
	}

void ThumbstickButtonsToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	cfs.updateValue("./numButtons",numButtons);
	Misc::FixedArray<double,2> ats(activationThresholds);
	cfs.updateValue("./activationThresholds",ats);
	ats.writeElements(activationThresholds);
	cfs.updateValue("./drawOnTouch",drawOnTouch);
	if(drawOnTouch)
		{
		cfs.updateValue("./thumbstickTransform",thumbstickTransform);
		cfs.updateValue("./thumbstickRadius",thumbstickRadius);
		cfs.updateValue("./thumbstickColor",thumbstickColor);
		}
	}

void ThumbstickButtonsToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue("./numButtons",numButtons);
	cfs.storeValue("./activationThresholds",Misc::FixedArray<double,2>(activationThresholds));
	cfs.storeValue("./drawOnTouch",drawOnTouch);
	if(drawOnTouch)
		{
		cfs.storeValue("./thumbstickTransform",thumbstickTransform);
		cfs.storeValue("./thumbstickRadius",thumbstickRadius);
		cfs.storeValue("./thumbstickColor",thumbstickColor);
		}
	}

/*********************************************
Methods of class ThumbstickButtonsToolFactory:
*********************************************/

ThumbstickButtonsToolFactory::ThumbstickButtonsToolFactory(ToolManager& toolManager)
	:ToolFactory("ThumbstickButtonsTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(0,true);
	layout.setNumValuators(2);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	ThumbstickButtonsTool::factory=this;
	}

ThumbstickButtonsToolFactory::~ThumbstickButtonsToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ThumbstickButtonsTool::factory=0;
	}

const char* ThumbstickButtonsToolFactory::getName(void) const
	{
	return "Thumbstick -> Buttons";
	}

const char* ThumbstickButtonsToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Thumbstick Touch";
		
		default:
			return "Unused";
		}
	}

const char* ThumbstickButtonsToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	static const char* valuatorNames[2]=
		{
		"Thumbstick X Axis","Thumbstick Y Axis"
		};
	return valuatorNames[valuatorSlotIndex];
	}

Tool* ThumbstickButtonsToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ThumbstickButtonsTool(this,inputAssignment);
	}

void ThumbstickButtonsToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveThumbstickButtonsToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createThumbstickButtonsToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ThumbstickButtonsToolFactory* factory=new ThumbstickButtonsToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyThumbstickButtonsToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**********************************************
Static elements of class ThumbstickButtonsTool:
**********************************************/

ThumbstickButtonsToolFactory* ThumbstickButtonsTool::factory=0;

/**************************************
Methods of class ThumbstickButtonsTool:
**************************************/

int ThumbstickButtonsTool::calcButtonIndex(double x,double y,double threshold) const
	{
	/* Check if the thumbstick is in the activation area: */
	if(x*x+y*y>Math::sqr(threshold))
		{
		/* Calculate and return the index of the selected button: */
		double angle=Math::atan2(-x,y);
		if(angle<0.0)
			angle+=2.0*Math::Constants<double>::pi;
		return int(Math::floor(angle/anglePerButton+0.5))%configuration.numButtons;
		}
	else
		return -1;
	}

ThumbstickButtonsTool::ThumbstickButtonsTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 configuration(ThumbstickButtonsTool::factory->configuration),
	 anglePerButton(0),pressedButton(-1),buttonActive(false)
	{
	pos[1]=pos[0]=0.0;
	}

ThumbstickButtonsTool::~ThumbstickButtonsTool(void)
	{
	}

void ThumbstickButtonsTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Update the class configuration: */
	configuration.read(configFileSection);
	}

void ThumbstickButtonsTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Store the tool configuration: */
	configuration.write(configFileSection);
	}

void ThumbstickButtonsTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	std::string buttonsDeviceName=sourceDevice->getDeviceName();
	buttonsDeviceName.append("ThumbstickButtons");
	transformedDevice=addVirtualInputDevice(buttonsDeviceName.c_str(),configuration.numButtons,0);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	
	/* Calculate the angle per button: */
	anglePerButton=2.0*Math::Constants<double>::pi/double(configuration.numButtons);
	
	/* Check if the tool is supposed to draw the virtual button ring: */
	if(configuration.drawOnTouch)
		{
		/* Create the visualization root node: */
		buttonRingRoot=new SceneGraph::ONTransformNode;
		ONTransform thumbstickTransform=configuration.thumbstickTransform;
		thumbstickTransform*=ONTransform::rotate(ONTransform::Rotation::rotateZ(Math::div2(Math::Constants<Scalar>::pi-anglePerButton)));
		thumbstickTransform.renormalize();
		buttonRingRoot->setTransform(thumbstickTransform);
		
		/* Create the active button visualization using a material with transparency to force it into the transparent rendering pass (this is a bit ugly): */
		SceneGraph::MaterialNodePointer material=new SceneGraph::MaterialNode;
		material->diffuseColor.setValue(SceneGraph::Color(0.0f,0.0f,0.0f));
		material->specularColor.setValue(SceneGraph::Color(0.0f,0.0f,0.0f));
		material->shininess.setValue(0.0f);
		material->emissiveColor.setValue(SceneGraph::Color(configuration.thumbstickColor.getRgba()));
		material->transparency.setValue(1.0f-configuration.thumbstickColor[3]);
		material->update();
		
		SceneGraph::AppearanceNodePointer appearance=new SceneGraph::AppearanceNode;
		appearance->material.setValue(material);
		appearance->update();
		
		/* Create the button ring visualization: */
		SceneGraph::LineSetNodePointer lineSet=new SceneGraph::LineSetNode;
		lineSet->setColor(SceneGraph::LineSetNode::VertexColor(configuration.thumbstickColor));
		
		/* Calculate a tessellation for the visualization's outer circle that is a multiple of the number of buttons: */
		unsigned int tessellation=24;
		tessellation+=configuration.numButtons-1;
		tessellation-=tessellation%configuration.numButtons;
		double outerRadius=lineSet->calcCircleAdjustedRadius(configuration.thumbstickRadius,tessellation);
		double innerRadius=lineSet->calcCircleAdjustedRadius(Math::div2(configuration.thumbstickRadius),tessellation);
		
		/* Add the inner and outer circles: */
		lineSet->addTessellatedCircle(SceneGraph::Point::origin,SceneGraph::Rotation::identity,innerRadius,tessellation);
		lineSet->addTessellatedCircle(SceneGraph::Point::origin,SceneGraph::Rotation::identity,outerRadius,tessellation);
		
		/* Add separators between the button areas: */
		for(int i=0;i<configuration.numButtons;++i)
			{
			unsigned int vertexIndex=i*tessellation/configuration.numButtons;
			lineSet->addLine(vertexIndex,tessellation+vertexIndex);
			}
		
		lineSet->update();
		
		SceneGraph::ShapeNodePointer buttonRing=new SceneGraph::ShapeNode;
		buttonRing->appearance.setValue(appearance);
		buttonRing->geometry.setValue(lineSet);
		buttonRing->update();
		
		buttonRingRoot->addChild(*buttonRing);
		
		/* Create the active button ring segment: */
		SceneGraph::TriangleSetNodePointer activeButton=new SceneGraph::TriangleSetNode;
		activeButton->setColor(SceneGraph::TriangleSetNode::VertexColor(configuration.thumbstickColor));
		activeButton->setNormal(SceneGraph::Vector(0,0,1));
		for(unsigned int i=0;i<=tessellation/configuration.numButtons;++i)
			{
			double angle=2.0*Math::Constants<double>::pi*double(i)/double(tessellation);
			activeButton->addVertex(SceneGraph::Point(Math::cos(angle)*innerRadius,Math::sin(angle)*innerRadius,0));
			activeButton->addVertex(SceneGraph::Point(Math::cos(angle)*outerRadius,Math::sin(angle)*outerRadius,0));
			}
		activeButton->addTriangleStrip(0,activeButton->getNextVertexIndex());
		activeButton->update();
		
		SceneGraph::ShapeNodePointer activeButtonShape=new SceneGraph::ShapeNode;
		activeButtonShape->appearance.setValue(appearance);
		activeButtonShape->geometry.setValue(activeButton);
		activeButtonShape->update();
		
		activeButtonRoot=new SceneGraph::ONTransformNode;
		activeButtonRoot->addChild(*activeButtonShape);
		
		/* If the tool does not have a "touch" binding, add the visualization permanently to the source device's scene graph: */
		if(getNumButtons()==0)
			getSceneGraphManager()->addDeviceNode(getValuatorDevice(0),*buttonRingRoot);
		}
	}

void ThumbstickButtonsTool::deinitialize(void)
	{
	/* Call the base class method: */
	TransformTool::deinitialize();
	
	/* If the tool draws the virtual button ring, remove the visualization from the source device's scene graph just in case: */
	if(configuration.drawOnTouch)
		getSceneGraphManager()->removeDeviceNode(getValuatorDevice(0),*buttonRingRoot);
	}

const ToolFactory* ThumbstickButtonsTool::getFactory(void) const
	{
	return factory;
	}

void ThumbstickButtonsTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonSlotIndex==0)
		{
		/* Show or hide the thumbstick visualization: */
		if(cbData->newButtonState)
			getSceneGraphManager()->addDeviceNode(getButtonDevice(0),*buttonRingRoot);
		else
			getSceneGraphManager()->removeDeviceNode(getButtonDevice(0),*buttonRingRoot);
		}
	}

void ThumbstickButtonsTool::valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Update the thumbstick position: */
	pos[valuatorSlotIndex]=cbData->newValuatorValue;
	
	/* Check if one of the buttons is currently pressed: */
	if(pressedButton>=0)
		{
		/* Release the button if the thumbstick returned to the center position: */
		if(Math::sqr(pos[0])+Math::sqr(pos[1])<Math::sqr(configuration.activationThresholds[0]))
			{
			transformedDevice->setButtonState(pressedButton,false);
			pressedButton=-1;
			if(buttonActive)
				buttonRingRoot->removeChild(*activeButtonRoot);
			buttonActive=false;
			}
		}
	else
		{
		/* Calculate the index of the button that will be pressed: */
		pressedButton=calcButtonIndex(pos[0],pos[1],configuration.activationThresholds[1]);
		
		/* Press the button if it is valid: */
		if(pressedButton>=0)
			{
			transformedDevice->setButtonState(pressedButton,true);
			activeButtonRoot->setTransform(ONTransform::rotate(ONTransform::Rotation::rotateZ(double(pressedButton)*anglePerButton)));
			if(!buttonActive)
				buttonRingRoot->addChild(*activeButtonRoot);
			buttonActive=true;
			}
		#if 0 // Show which button would be pressed if the stick were fully pushed
		else
			{
			/* Calculate the index of the button that might be pressed: */
			int touchedButton=calcButtonIndex(pos[0],pos[1],configuration.activationThresholds[0]);
			if(touchedButton>=0)
				{
				activeButtonRoot->setTransform(ONTransform::rotate(ONTransform::Rotation::rotateZ(double(touchedButton)*anglePerButton)));
				if(!buttonActive)
					buttonRingRoot->addChild(*activeButtonRoot);
				buttonActive=true;
				}
			else if(buttonActive)
				{
				buttonRingRoot->removeChild(*activeButtonRoot);
				buttonActive=false;
				}
			}
		#endif
		}
	}

InputDeviceFeatureSet ThumbstickButtonsTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on transformed device");
	
	/* Return all input feature slots: */
	InputDeviceFeatureSet result;
	for(int i=0;i<2;++i)
		result.push_back(input.getValuatorSlotFeature(i));
	
	return result;
	}

InputDeviceFeatureSet ThumbstickButtonsTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Source feature is not part of tool's input assignment");
	
	/* Return the index of the button that would be pressed: */
	InputDeviceFeatureSet result;
	int button=calcButtonIndex(getValuatorState(0),getValuatorState(1),0.0);
	if(button>=0)
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,button));
	
	return result;
	}

}

/***********************************************************************
IndexHandTool - Tool to attach an animated hand model to a Valve Index
controller and create a virtual input device following the index finger.
Copyright (c) 2021-2024 Oliver Kreylos

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

#include <Vrui/Tools/IndexHandTool.h>

#include <string>
#include <vector>
#include <Misc/StdError.h>
#include <Misc/FixedArray.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GroupNode.h>
#include <Vrui/Vrui.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/SceneGraphManager.h>

namespace Vrui {

/****************************************************
Methods of class IndexHandToolFactory::Configuration:
****************************************************/

IndexHandToolFactory::Configuration::Configuration(void)
	:indexTransform(SceneGraph::DOGTransform::identity),
	 palmTransform(ONTransform::identity)
	{
	/* Configure finger bend thresholds for gesture detection: */
	for(int finger=0;finger<4;++finger)
		{
		fingers[finger].bendThresholds[0]=SceneGraph::Scalar(0.75);
		fingers[finger].bendThresholds[1]=SceneGraph::Scalar(0.25);
		}
	
	/* Set the default palm transformation: */
	palmTransform=ONTransform::translate(Vector(0.01,-0.09,-0.04));
	palmTransform*=ONTransform::rotate(Rotation(Vector(-0.927122, 0.365876, -0.081113),Math::rad(Scalar(26.896238))));
	palmTransform*=ONTransform::rotate(Rotation::rotateZ(Math::rad(Scalar(90.0))));
	palmTransform.renormalize();
	}

namespace {

/****************
Helper variables:
****************/

/* Names of buttons that can be touched by the thumb: */
static const char* buttonNames[6]=
	{
	"System","A","B","Trackpad","Thumbstick","Stretched"
	};

/* Names of the other four fingers: */
static const char* fingerNames[4]=
	{
	"index","middle","ring","pinky"
	};

/* State names for the other four fingers: */
static const char* stateNames[2]=
	{
	"Rest","Grab"
	};

}

void IndexHandToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	typedef Misc::FixedArray<SceneGraph::ONTransform,3> Pose; // Type for finger poses
	
	/* Read the thumb joint rotations: */
	for(int button=0;button<6;++button)
		{
		/* Read the thumb's joint rotations to touch the current button: */
		Pose p;
		for(int joint=0;joint<3;++joint)
			p[joint]=SceneGraph::ONTransform::rotate(thumb.rs[button][joint]);
		char settingName[64];
		snprintf(settingName,sizeof(settingName),"./thumb%sRots",buttonNames[button]);
		cfs.updateValue(settingName,p);
		for(int joint=0;joint<3;++joint)
			thumb.rs[button][joint]=p[joint].getRotation();
		}
	
	/* Read the other finger's joint rotations for stretched and grabbed states and bend thresholds: */
	for(int finger=0;finger<4;++finger)
		{
		for(int state=0;state<2;++state)
			{
			/* Read the current finger's joint rotations for the current state: */
			Pose p;
			for(int joint=0;joint<3;++joint)
				p[joint]=SceneGraph::ONTransform::rotate(fingers[finger].rs[state][joint]);
			char settingName[64];
			snprintf(settingName,sizeof(settingName),"./%s%sRots",fingerNames[finger],stateNames[state]);
			cfs.updateValue(settingName,p);
			for(int joint=0;joint<3;++joint)
				fingers[finger].rs[state][joint]=p[joint].getRotation();
			}
		
		/* Read the current finger's bend thresholds: */
		Misc::FixedArray<SceneGraph::Scalar,2> fbt(fingers[finger].bendThresholds);
		char settingName[64];
		snprintf(settingName,sizeof(settingName),"./%sBendThresholds",fingerNames[finger]);
		cfs.updateValue(settingName,fbt);
		fbt.writeElements(fingers[finger].bendThresholds);
		}
	
	/* Retrieve the index finger device transformation: */
	cfs.updateValue("./indexTransform",indexTransform);
	
	/* Retrieve the palm device transformation: */
	cfs.updateValue("./palmTransform",palmTransform);
	}

void IndexHandToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	typedef Misc::FixedArray<SceneGraph::ONTransform,3> Pose; // Type for finger poses
	
	/* Write the thumb joint rotations: */
	for(int button=0;button<6;++button)
		{
		/* Write the thumb's joint rotations to touch the current button: */
		Pose p;
		for(int joint=0;joint<3;++joint)
			p[joint]=SceneGraph::ONTransform::rotate(thumb.rs[button][joint]);
		char settingName[64];
		snprintf(settingName,sizeof(settingName),"./thumb%sRots",buttonNames[button]);
		cfs.storeValue(settingName,p);
		}
	
	/* Write the other finger's joint rotations for stretched and grabbed states and bend thresholds: */
	for(int finger=0;finger<4;++finger)
		{
		for(int state=0;state<2;++state)
			{
			/* Write the current finger's joint rotations for the current state: */
			Pose p;
			for(int joint=0;joint<3;++joint)
				p[joint]=SceneGraph::ONTransform::rotate(fingers[finger].rs[state][joint]);
			char settingName[64];
			snprintf(settingName,sizeof(settingName),"./%s%sRots",fingerNames[finger],stateNames[state]);
			cfs.storeValue(settingName,p);
			}
		
		/* Write the current finger's bend thresholds: */
		Misc::FixedArray<SceneGraph::Scalar,2> fbt(fingers[finger].bendThresholds);
		char settingName[64];
		snprintf(settingName,sizeof(settingName),"./%sBendThresholds",fingerNames[finger]);
		cfs.storeValue(settingName,fbt);
		}
	
	/* Write the index finger device transformation: */
	cfs.storeValue("./indexTransform",indexTransform);
	
	/* Write the palm device transformation: */
	cfs.storeValue("./palmTransform",palmTransform);
	}

/*************************************
Methods of class IndexHandToolFactory:
*************************************/

IndexHandToolFactory::IndexHandToolFactory(ToolManager& toolManager)
	:ToolFactory("IndexHandTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(5);
	layout.setNumValuators(4);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	IndexHandTool::factory=this;
	}

IndexHandToolFactory::~IndexHandToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	IndexHandTool::factory=0;
	}

const char* IndexHandToolFactory::getName(void) const
	{
	return "Index Hand Model";
	}

const char* IndexHandToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	static const char* buttonFunctions[5]=
		{
		"System Touch","A Touch","B Touch","Trackpad Touch","Thumbstick Touch"
		};
	return buttonFunctions[buttonSlotIndex];
	}

const char* IndexHandToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	static const char* valuatorFunctions[4]=
		{
		"Index Bend","Middle Bend","Ring Bend","Pinky Bend"
		};
	return valuatorFunctions[valuatorSlotIndex];
	}

Tool* IndexHandToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new IndexHandTool(this,inputAssignment);
	}

void IndexHandToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveIndexHandToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createIndexHandToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	IndexHandToolFactory* indexHandToolFactory=new IndexHandToolFactory(*toolManager);
	
	/* Return factory object: */
	return indexHandToolFactory;
	}

extern "C" void destroyIndexHandToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************
Static elements of class IndexHandTool:
**************************************/

IndexHandToolFactory* IndexHandTool::factory=0;

/******************************
Methods of class IndexHandTool:
******************************/

void IndexHandTool::updateGesture(unsigned int newGestureMask)
	{
	/* Set the state of the index finger device's button: */
	transformedDevice->setButtonState(0,newGestureMask==0x1cU);
	
	/* Set the state of the palm device's button: */
	palmDevice->setButtonState(0,newGestureMask==0x0U);
	
	gestureMask=newGestureMask;
	}

void IndexHandTool::updateThumb(void)
	{
	/* Determine the highest-priority touched button: */
	thumbButton=5;
	for(int i=0;i<5&&thumbButton==5;++i)
		if(getButtonState(i))
			thumbButton=i;
	
	/* Set the thumb's position according to the currently touched button: */
	for(int joint=0;joint<3;++joint)
		{
		thumbTransforms[joint]->rotation.setValue(configuration.thumb.rs[thumbButton][joint]);
		thumbTransforms[joint]->update();
		}
	
	/* Update the current gesture: */
	if(thumbButton<5)
		updateGesture(gestureMask|0x1U);
	else
		updateGesture(gestureMask&~0x1U);
	}

void IndexHandTool::updateFinger(int fingerIndex,SceneGraph::Scalar fingerBend)
	{
	/* Update the finger according to the new valuator data: */
	for(int joint=0;joint<3;++joint)
		{
		SceneGraph::Rotation r(fingerDrs[fingerIndex][joint]*fingerBend);
		r*=configuration.fingers[fingerIndex].rs[0][joint];
		fingerTransforms[fingerIndex][joint]->rotation.setValue(r);
		fingerTransforms[fingerIndex][joint]->update();
		}
	
	/* Check if the index finger was updated: */
	if(fingerIndex==0)
		{
		/* Calculate the new index finger device transformation: */
		SceneGraph::DOGTransform dt=hand->getTransform();
		for(int joint=0;joint<3;++joint)
			dt*=fingerTransforms[fingerIndex][joint]->getTransform();
		dt*=configuration.indexTransform;
		deviceT=TrackerState(dt.getTranslation(),dt.getRotation());
		}
	
	/* Update the current gesture: */
	unsigned int fingerBit=0x1U<<(1+fingerIndex);
	if((gestureMask&fingerBit)!=0x0U)
		{
		/* Clear the finger bit if the current value is below the release threshold: */
		if(fingerBend<configuration.fingers[fingerIndex].bendThresholds[1])
			updateGesture(gestureMask&~fingerBit);
		}
	else
		{
		/* Set the finger bit if the current value is above the grab threshold: */
		if(fingerBend>configuration.fingers[fingerIndex].bendThresholds[0])
			updateGesture(gestureMask|fingerBit);
		}
	}

IndexHandTool::IndexHandTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 configuration(IndexHandTool::factory->configuration),
	 thumbButton(5),gestureMask(0x0U),
	 palmDevice(0)
	{
	}

IndexHandTool::~IndexHandTool(void)
	{
	}

namespace {

/****************
Helper functions:
****************/

/* Function to mirror a rotation along the x axis: */
template <class RotationParam>
inline
RotationParam&
mirrorRotation(
	RotationParam& rotation)
	{
	/* Invert the y and z components of the rotation's quaternion: */
	const typename RotationParam::Scalar* q=rotation.getQuaternion();
	rotation=RotationParam(q[0],-q[1],-q[2],q[3]);
	return rotation;
	}

/* Function to mirror an orthonormal transformation along the x axis: */
template <class ScalarParam>
inline
Geometry::OrthonormalTransformation<ScalarParam,3>&
mirrorTransform(
	Geometry::OrthonormalTransformation<ScalarParam,3>& transform)
	{
	typedef Geometry::OrthonormalTransformation<ScalarParam,3> Transform;
	typedef typename Transform::Vector Vector;
	typedef typename Transform::Rotation Rotation;
	
	/* Invert the y and z components of the rotation's quaternion: */
	const Vector& t=transform.getTranslation();
	const ScalarParam* q=transform.getRotation().getQuaternion();
	transform=Transform(Vector(-t[0],t[1],t[2]),Rotation(q[0],-q[1],-q[2],q[3]));
	return transform;
	}

/* Function to mirror an orthogonal transformation along the x axis: */
template <class ScalarParam>
inline
Geometry::OrthogonalTransformation<ScalarParam,3>&
mirrorTransform(
	Geometry::OrthogonalTransformation<ScalarParam,3>& transform)
	{
	typedef Geometry::OrthogonalTransformation<ScalarParam,3> Transform;
	typedef typename Transform::Vector Vector;
	typedef typename Transform::Rotation Rotation;
	
	/* Invert the y and z components of the rotation's quaternion: */
	const Vector& t=transform.getTranslation();
	const ScalarParam* q=transform.getRotation().getQuaternion();
	transform=Transform(Vector(-t[0],t[1],t[2]),Rotation(q[0],-q[1],-q[2],q[3]),transform.getScaling());
	return transform;
	}

/* Thumb and finger joint names: */
static const char* jointTransformNameTemplates[5]=
	{
	"Thumb_","Index_","Middle_","Ring_","Pinky_"
	};

const char* makeJointTransformName(int fingerIndex,int jointIndex)
	{
	static char result[21];
	char* rPtr=result;
	const char* nPtr=jointTransformNameTemplates[fingerIndex];
	while(*nPtr!='\0')
		{
		if(*nPtr!='_')
			*rPtr=*nPtr;
		else
			*rPtr='0'+jointIndex;
		++rPtr;
		++nPtr;
		}
	*rPtr='\0';
	return result;
	}

}

void IndexHandTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Check if the default tool configuration should be mirrored along the x axis for the left hand: */
	if(configFileSection.retrieveValue<bool>("./leftHand",false))
		{
		/* Mirror all thumb joint rotations: */
		for(int button=0;button<6;++button)
			for(int joint=0;joint<3;++joint)
				mirrorRotation(configuration.thumb.rs[button][joint]);
		
		/* Mirror all other finger joint rotations: */
		for(int finger=0;finger<4;++finger)
			for(int state=0;state<2;++state)
				for(int joint=0;joint<3;++joint)
					mirrorRotation(configuration.fingers[finger].rs[state][joint]);
		
		/* Mirror the index finger device's transformation: */
		mirrorTransform(configuration.indexTransform);
		
		/* Mirror the palm device's transformation: */
		mirrorTransform(configuration.palmTransform);
		}
	
	/* Update the tool configuration: */
	configuration.read(configFileSection);
	
	/* Calculate all finger joints' differential rotations: */
	for(int finger=0;finger<4;++finger)
		for(int joint=0;joint<3;++joint)
			fingerDrs[finger][joint]=(configuration.fingers[finger].rs[1][joint]/configuration.fingers[finger].rs[0][joint]).getScaledAxis();
	
	/* Open the hand model VRML file: */
	handModelFileName=configFileSection.retrieveString("./handModelFileName");
	SceneGraph::NodeCreator nodeCreator;
	SceneGraph::VRMLFile handModelFile(*IO::openDirectory(VRUI_INTERNAL_CONFIG_SHAREDIR "/Resources"),handModelFileName,nodeCreator);
	SceneGraph::GroupNodePointer handModel=new SceneGraph::GroupNode;
	handModelFile.parse(*handModel);
	
	/* Retrieve the hand model's root transform node: */
	if(handModel->getChildren().size()!=1U)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Wrong number root nodes in hand model");
	hand=SceneGraph::TransformNodePointer(handModel->getChildren().front());
	
	/* Access the thumb's joint transformation nodes: */
	for(int joint=0;joint<3;++joint)
		thumbTransforms[joint]=SceneGraph::TransformNodePointer(handModelFile.useNode(makeJointTransformName(0,joint)));
	
	/* Access the other fingers' joint transformation nodes: */
	for(int finger=0;finger<4;++finger)
		for(int joint=0;joint<3;++joint)
			fingerTransforms[finger][joint]=SceneGraph::TransformNodePointer(handModelFile.useNode(makeJointTransformName(1+finger,joint)));
	}

void IndexHandTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write the tool configuration: */
	configuration.write(configFileSection);
	
	/* Write the hand model file name: */
	configFileSection.storeString("./handModelFileName",handModelFileName);
	}

void IndexHandTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	std::string indexFingerDeviceName=sourceDevice->getDeviceName();
	indexFingerDeviceName.append("IndexFinger");
	transformedDevice=addVirtualInputDevice(indexFingerDeviceName.c_str(),1,0);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Initialize the virtual input device: */
	transformedDevice->setDeviceRay(Vector(0,1,0),0);
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Create a virtual input device for the palm: */
	std::string palmDeviceName=sourceDevice->getDeviceName();
	palmDeviceName.append("Palm");
	palmDevice=addVirtualInputDevice(palmDeviceName.c_str(),1,0);
	palmDevice->setTrackType(sourceDevice->getTrackType());
	palmDevice->setDeviceRay(Vector(0,1,0),0);
	getInputGraphManager()->getInputDeviceGlyph(palmDevice).disable();
	getInputGraphManager()->grabInputDevice(palmDevice,this);
	
	/* Initialize the position of the thumb and the other fingers: */
	updateThumb();
	for(int finger=0;finger<4;++finger)
		updateFinger(finger,getValuatorState(finger));
	
	/* Attach the hand model scene graph to the source input device's scene graph: */
	getSceneGraphManager()->addDeviceNode(sourceDevice,*hand);
	}

void IndexHandTool::deinitialize(void)
	{
	/* Release and delete the palm device: */
	getInputGraphManager()->releaseInputDevice(palmDevice,this);
	getInputDeviceManager()->destroyInputDevice(palmDevice);
	palmDevice=0;
	
	/* Remove the hand model from the controller's scene graph: */
	Vrui::getSceneGraphManager()->removeDeviceNode(sourceDevice,*hand);
	
	/* Call the base class method: */
	TransformTool::deinitialize();
	}

const ToolFactory* IndexHandTool::getFactory(void) const
	{
	return factory;
	}

void IndexHandTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	updateThumb();
	}

void IndexHandTool::valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	updateFinger(valuatorSlotIndex,SceneGraph::Scalar(cbData->newValuatorValue));
	}

void IndexHandTool::frame(void)
	{
	/* Get the source device's state: */
	TrackerState t=sourceDevice->getTransformation();
	Vector lv=sourceDevice->getLinearVelocity();
	Vector av=sourceDevice->getAngularVelocity();
	
	/* Calculate the transformed device's tracking state: */
	TrackerState indexT=t;
	indexT*=deviceT;
	indexT.renormalize();
	Vector indexLv=lv+(av^deviceT.getTranslation());
	Vector indexAv=deviceT.transform(av);
	
	/* Update the transformed device: */
	transformedDevice->setTrackingState(indexT,indexLv,indexAv);
	
	/* Update the palm device: */
	TrackerState palmT=t;
	palmT*=configuration.palmTransform;
	palmT.renormalize();
	Vector palmLv=lv+(av^configuration.palmTransform.getTranslation());
	Vector palmAv=configuration.palmTransform.transform(av);
	palmDevice->setTrackingState(palmT,palmLv,palmAv);
	}

std::vector<InputDevice*> IndexHandTool::getForwardedDevices(void)
	{
	std::vector<InputDevice*> result;
	result.push_back(transformedDevice);
	result.push_back(palmDevice);
	return result;
	}

InputDeviceFeatureSet IndexHandTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed devices: */
	if(forwardedFeature.getDevice()!=transformedDevice&&forwardedFeature.getDevice()!=palmDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded feature is not on transformed devices");
	
	/* Return all input feature slots: */
	InputDeviceFeatureSet result;
	result.push_back(input.getValuatorSlotFeature(0));
	
	return result;
	}

InputDevice* IndexHandTool::getSourceDevice(const InputDevice* forwardedDevice)
	{
	/* Paranoia: Check if the forwarded device is the same as the transformed device: */
	if(forwardedDevice!=transformedDevice&&forwardedDevice!=palmDevice)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Forwarded device is not transformed devices");
	
	/* Return the designated source device: */
	return sourceDevice;
	}

InputDeviceFeatureSet IndexHandTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	InputDeviceFeatureSet result;
	
	/* Check if the source feature is being forwarded: */
	if(sourceFeature.isValuator()&&input.findFeature(sourceFeature)==0)
		{
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,0));
		result.push_back(InputDeviceFeature(palmDevice,InputDevice::BUTTON,0));
		}
	
	return result;
	}

}

/***********************************************************************
VRScreen - Class for display screens (fixed and head-mounted) in VR
environments.
Copyright (c) 2004-2024 Oliver Kreylos

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

#include <Vrui/VRScreen.h>

#include <string.h>
#include <Misc/StdError.h>
#include <Misc/CommandDispatcher.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Ray.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLMatrixTemplates.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Vrui.h>

namespace Vrui {

/*************************
Methods of class VRScreen:
*************************/

void VRScreen::setDeviceCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	VRScreen* thisPtr=static_cast<VRScreen*>(userData);
	
	/* Parse the new device name: */
	std::string newDeviceName(argumentsBegin,argumentsEnd);
	
	/* Attach the screen to the new device, or detach from devices if device name is empty: */
	if(newDeviceName.empty())
		thisPtr->attachToDevice(0);
	else
		{
		Vrui::InputDevice* newDevice=findInputDevice(newDeviceName.c_str());
		if(newDevice==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Device \"%s\" not found",newDeviceName.c_str());
		thisPtr->attachToDevice(newDevice);
		}
	}

void VRScreen::setTransformCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	VRScreen* thisPtr=static_cast<VRScreen*>(userData);
	
	/* Parse the new transformation: */
	ONTransform newTransform=Misc::ValueCoder<ONTransform>::decode(argumentsBegin,argumentsEnd);
	
	/* Override the transformation: */
	thisPtr->setTransform(newTransform);
	}

void VRScreen::inputDeviceStateChangeCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData)
	{
	/* Set screen state if this is our tracking device: */
	if(deviceMounted&&cbData->inputDevice==device)
		enabled=cbData->newEnabled;
	}

VRScreen::VRScreen(void)
	:screenName(0),
	 deviceMounted(false),device(0),
	 transform(ONTransform::identity),inverseTransform(ONTransform::identity),
	 offAxis(false),screenHomography(PTransform2::identity),inverseClipHomography(PTransform::identity),
	 intersect(true),
	 enabled(true)
	{
	screenSize[0]=screenSize[1]=Scalar(0);
	
	/* Register callbacks with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().add(this,&VRScreen::inputDeviceStateChangeCallback);
	}

VRScreen::~VRScreen(void)
	{
	delete[] screenName;
	
	/* Unregister callbacks with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().remove(this,&VRScreen::inputDeviceStateChangeCallback);
	}

void VRScreen::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read the screen's name: */
	std::string name=configFileSection.retrieveString("./name",configFileSection.getName());
	screenName=new char[name.size()+1];
	strcpy(screenName,name.c_str());
	
	/* Determine whether screen is device-mounted: */
	deviceMounted=configFileSection.retrieveValue("./deviceMounted",false);
	if(deviceMounted)
		{
		/* Retrieve the input device this screen is attached to: */
		std::string deviceName=configFileSection.retrieveString("./deviceName");
		device=findInputDevice(deviceName.c_str());
		if(device==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mounting device \"%s\" not found",deviceName.c_str());
		attachToDevice(device);
		}
	
	/* Retrieve screen position/orientation in physical or device coordinates: */
	try
		{
		/* Try reading the screen transformation directly: */
		transform=configFileSection.retrieveValue<ONTransform>("./transform");
		}
	catch(const std::runtime_error&)
		{
		/* Fall back to reading the screen's origin and axis directions: */
		Point origin=configFileSection.retrieveValue<Point>("./origin");
		Vector horizontalAxis=configFileSection.retrieveValue<Vector>("./horizontalAxis");
		Vector verticalAxis=configFileSection.retrieveValue<Vector>("./verticalAxis");
		ONTransform::Rotation rot=ONTransform::Rotation::fromBaseVectors(horizontalAxis,verticalAxis);
		transform=ONTransform(origin-Point::origin,rot);
		}
	
	/* Read the screen's size: */
	screenSize[0]=configFileSection.retrieveValue<Scalar>("./width");
	screenSize[1]=configFileSection.retrieveValue<Scalar>("./height");
	
	/* Apply a rotation around a single axis: */
	Point rotateCenter=configFileSection.retrieveValue("./rotateCenter",Point::origin);
	Vector rotateAxis=configFileSection.retrieveValue("./rotateAxis",Vector(1,0,0));
	Scalar rotateAngle=configFileSection.retrieveValue("./rotateAngle",Scalar(0));
	if(rotateAngle!=Scalar(0))
		{
		ONTransform screenRotation=ONTransform::translateFromOriginTo(rotateCenter);
		screenRotation*=ONTransform::rotate(ONTransform::Rotation::rotateAxis(rotateAxis,Math::rad(rotateAngle)));
		screenRotation*=ONTransform::translateToOriginFrom(rotateCenter);
		transform.leftMultiply(screenRotation);
		}
	
	/* Apply an arbitrary pre-transformation: */
	ONTransform preTransform=configFileSection.retrieveValue("./preTransform",ONTransform::identity);
	transform.leftMultiply(preTransform);
	
	/* Finalize the screen transformation: */
	transform.renormalize();
	inverseTransform=Geometry::invert(transform);
	
	/* Check if the screen is projected off-axis: */
	configFileSection.updateValue("./offAxis",offAxis);
	if(offAxis)
		{
		/* Create the inverse of the 2D homography from clip space to rectified screen space in screen coordinates: */
		PTransform2 sHomInv=PTransform2::identity;
		sHomInv.getMatrix()(0,0)=Scalar(2)/screenSize[0];
		sHomInv.getMatrix()(0,2)=Scalar(-1);
		sHomInv.getMatrix()(1,1)=Scalar(2)/screenSize[1];
		sHomInv.getMatrix()(1,2)=Scalar(-1);
		sHomInv.getMatrix()(2,2)=Scalar(1);
		
		/* Retrieve the 2D homography from clip space to projected screen space in screen coordinates: */
		PTransform2 pHom=configFileSection.retrieveValue<PTransform2>("./homography");
		
		/* Calculate the screen space homography: */
		screenHomography=pHom*sHomInv;
		
		/* Calculate the inverse clip space homography: */
		PTransform2 hom=sHomInv*pHom;
		for(int i=0;i<3;++i)
			for(int j=0;j<3;++j)
				inverseClipHomography.getMatrix()(i<2?i:3,j<2?j:3)=hom.getMatrix()(i,j);
		inverseClipHomography.doInvert();
		
		/* Find the maximum z value of the morphed far plane quadrilateral to scale the homography's z axis: */
		double maxFarZ=-1.0;
		maxFarZ=Math::max(maxFarZ,inverseClipHomography.transform(Point(-1,-1,1))[2]);
		maxFarZ=Math::max(maxFarZ,inverseClipHomography.transform(Point( 1,-1,1))[2]);
		maxFarZ=Math::max(maxFarZ,inverseClipHomography.transform(Point(-1, 1,1))[2]);
		maxFarZ=Math::max(maxFarZ,inverseClipHomography.transform(Point( 1, 1,1))[2]);
		
		/* Scale the inverse clip space homography to bring the far plane quadrilateral entirely into the frustum: */
		for(int j=0;j<4;++j)
			inverseClipHomography.getMatrix()(2,j)/=maxFarZ;
		}
	
	/* Read the intersect flag: */
	configFileSection.updateValue("./intersect",intersect);
	
	/* Register pipe command callbacks: */
	getCommandDispatcher().addCommandCallback(("Screen("+name+").setDevice").c_str(),&VRScreen::setDeviceCallback,this,"[device name]","Attaches the screen to the tracked input device of the given name, or detaches the screen from a device if no device name is given");
	getCommandDispatcher().addCommandCallback(("Screen("+name+").setTransform").c_str(),&VRScreen::setTransformCallback,this,"<transformation string>","Sets the screen's transformation relative to its device or physical space");
	}

InputDevice* VRScreen::attachToDevice(InputDevice* newDevice)
	{
	/* Return the previous mounting device: */
	InputDevice* result=deviceMounted?device:0;
	
	/* Set the device to which the screen is mounted, and update the mounted flag: */
	deviceMounted=newDevice!=0;
	device=newDevice;
	
	/* Update the screen's state: */
	if(deviceMounted)
		{
		/* Check if the mounting device is currently enabled: */
		enabled=getInputGraphManager()->isEnabled(device);
		}
	else
		enabled=true;
	
	return result;
	}

void VRScreen::setSize(Scalar newWidth,Scalar newHeight)
	{
	if(screenSize[0]!=newWidth||screenSize[1]!=newHeight)
		{
		/* Call the size changed callbacks: */
		SizeChangedCallbackData cbData(this,newWidth,newHeight);
		sizeChangedCallbacks.call(&cbData);
		
		/* Adjust the screen's origin in its own coordinate system: */
		transform*=ONTransform::translate(Vector(Math::div2(screenSize[0]-newWidth),Math::div2(screenSize[1]-newHeight),0));
		inverseTransform=Geometry::invert(transform);
		
		/* Adjust the screen's sizes: */
		screenSize[0]=newWidth;
		screenSize[1]=newHeight;
		}
	}

void VRScreen::setTransform(const ONTransform& newTransform)
	{
	/* Update the screen-to-physical/device transformation and its inverse: */
	transform=newTransform;
	inverseTransform=Geometry::invert(transform);
	}

ONTransform VRScreen::getScreenTransformation(void) const
	{
	ONTransform result=transform;
	if(deviceMounted)
		result.leftMultiply(device->getTransformation());
	return result;
	}

void VRScreen::setScreenTransform(void) const
	{
	/* Save the current matrix mode: */
	glPushAttrib(GL_TRANSFORM_BIT);
	
	/* Save the modelview matrix: */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	/* Modify the modelview matrix: */
	if(deviceMounted)
		glMultMatrix(device->getTransformation());
	glMultMatrix(transform);
	
	/* Restore the current matrix mode: */
	glPopAttrib();
	}

void VRScreen::resetScreenTransform(void) const
	{
	/* Save the current matrix mode: */
	glPushAttrib(GL_TRANSFORM_BIT);
	
	/* Restore the modelview matrix: */
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	/* Restore the current matrix mode: */
	glPopAttrib();
	}

}

/***********************************************************************
HIDPositionerPenPad - Base class for HID positioners representing stylus
and touchscreen devices.
Copyright (c) 2023-2025 Oliver Kreylos

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

#include <Vrui/Internal/HIDPositionerPenPad.h>

#include <string>
#include <Misc/StdError.h>
#include <Misc/ConfigurationFile.h>
#include <RawHID/EventDevice.h>
#include <Math/Math.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/UIManager.h>
#include <Vrui/Internal/PenPadCalibrator.h>
#include <Vrui/Internal/PenPadCalibratorRectilinear.h>
#include <Vrui/Internal/PenPadCalibratorAffine.h>
#include <Vrui/Internal/PenPadCalibratorProjective.h>
#include <Vrui/Internal/PenPadCalibratorBSpline.h>

namespace Vrui {

/************************************
Methods of class HIDPositionerPenPad:
************************************/

HIDPositionerPenPad::HIDPositionerPenPad(RawHID::EventDevice& sHid,const Misc::ConfigurationFileSection& configFileSection,bool* ignoredFeatures)
	:HIDPositioner(sHid),
	 config(hid),
	 calibrator(0),screen(0)
	{
	/* Check if the HID is a pen pad: */
	if(!config.valid)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Given HID %s is not a pen pad device",hid.getDeviceName().c_str());
	
	/* Ignore HID features directly related to pen pad operation: */
	#if 0 // We're not ignoring the hover buttons; let a Vrui tool sort it out
	for(std::vector<unsigned int>::iterator hkiIt=config.hoverKeyIndices.begin();hkiIt!=config.hoverKeyIndices.end();++hkiIt)
		ignoredFeatures[*hkiIt]=true;
	#endif
	unsigned int nkf=hid.getNumKeyFeatures();
	for(int i=0;i<2;++i)
		ignoredFeatures[nkf+config.posAxisIndices[i]]=true;
	if(config.haveTilt)
		for(int i=0;i<2;++i)
			ignoredFeatures[nkf+config.tiltAxisIndices[i]]=true;
	
	/* Retrieve the domain of the pen pad's position axes: */
	PenPadCalibrator::Box2 rawDomain;
	for(int i=0;i<2;++i)
		{
		const RawHID::EventDevice::AbsAxisConfig& axisCfg=hid.getAbsAxisFeatureConfig(config.posAxisIndices[i]);
		rawDomain.min[i]=Scalar(axisCfg.min);
		rawDomain.max[i]=Scalar(axisCfg.max);
		}
	
	/* Create a pen pad calibrator: */
	std::string calibratorType=configFileSection.retrieveString("./calibratorType","Rectilinear");
	if(calibratorType=="Rectilinear")
		calibrator=new PenPadCalibratorRectilinear(configFileSection,rawDomain);
	else if(calibratorType=="Affine")
		calibrator=new PenPadCalibratorAffine(configFileSection,rawDomain);
	else if(calibratorType=="Projective")
		calibrator=new PenPadCalibratorProjective(configFileSection,rawDomain);
	else if(calibratorType=="BSpline")
		calibrator=new PenPadCalibratorBSpline(configFileSection,rawDomain);
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid calibrator type %s",calibratorType.c_str());
	
	/* Read the name of the VR screen mapping pen pad positions into physical space: */
	screenName=configFileSection.retrieveString("./screenName");
	}

HIDPositionerPenPad::~HIDPositionerPenPad(void)
	{
	delete calibrator;
	}

int HIDPositionerPenPad::getTrackType(void) const
	{
	/* Pen pads support full orientational tracking: */
	return InputDevice::TRACK_POS|InputDevice::TRACK_DIR|InputDevice::TRACK_ORIENT;
	}

void HIDPositionerPenPad::prepareMainLoop(void)
	{
	/* Retrieve the mapping screen: */
	screen=findScreen(screenName.c_str());
	if(screen==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown VR screen %s",screenName.c_str());
	}

void HIDPositionerPenPad::updateDevice(InputDevice* device)
	{
	/* Retrieve the current pen state: */
	RawHID::PenDeviceConfig::PenState penState=config.getPenState(hid);
	if(penState.valid)
		{
		/* Calculate the device's calibrated position in normalized screen space: */
		PenPadCalibrator::Point2 calibrated=calibrator->calibrate(PenPadCalibrator::Point2(Scalar(penState.pos[0]),Scalar(penState.pos[1])));
		
		/* Transform the calibrated position to scaled screen space: */
		Vector screenOffset(calibrated[0]*screen->getScreenSize()[0],calibrated[1]*screen->getScreenSize()[1],Scalar(0));
		
		if(config.haveTilt)
			{
			/* Apply tilt correction: */
			Scalar x=Math::sin(Math::rad(Scalar(penState.tilt[0])));
			Scalar y=Math::sin(Math::rad(Scalar(penState.tilt[1])));
			// screenOffset[0]-=x*Scalar(0.05);
			// screenOffset[1]+=y*Scalar(0.05);
			}
		
		/* Calculate the device transformation: */
		ONTransform transform=screen->getScreenTransformation();
		transform*=ONTransform::translate(screenOffset);
		transform*=ONTransform::rotate(Rotation::rotateX(Math::rad(Scalar(-90))));
		
		/* Update the device's tracking state: */
		device->setTransformation(transform);
		
		/* Project the copied tracking state with the UI manager if requested: */
		if(project)
			getUiManager()->projectDevice(device,device->getTransformation());
		
		/* We should calculate linear and angular velocities here... */
		// ...
		
		/* Transform the physical-space head position to device coordinates: */
		Point deviceHeadPos=device->getTransformation().inverseTransform(getMainViewer()->getHeadPosition());
		
		/* Calculate the ray direction and ray origin offset in device coordinates: */
		Vector deviceRayDir=Point::origin-deviceHeadPos;
		Scalar deviceRayDirLen=Geometry::mag(deviceRayDir);
		deviceRayDir/=deviceRayDirLen;
		Scalar deviceRayStart=-(deviceHeadPos[1]+getFrontplaneDist())*deviceRayDirLen/deviceHeadPos[1];
		
		/* Update the device's ray: */
		device->setDeviceRay(deviceRayDir,deviceRayStart);
		
		/* Enable the device: */
		getInputGraphManager()->enable(device);
		}
	else
		{
		/* Disable the device: */
		getInputGraphManager()->disable(device);
		}
	}

}

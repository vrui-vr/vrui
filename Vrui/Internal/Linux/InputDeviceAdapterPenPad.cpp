/***********************************************************************
InputDeviceAdapterPenPad - Input device adapter for pen pads or pen
displays represented by one or more component HIDs.
Copyright (c) 2023-2024 Oliver Kreylos

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

#include <Vrui/Internal/Linux/InputDeviceAdapterPenPad.h>

#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>

namespace Vrui {

/*****************************************
Methods of class InputDeviceAdapterPenPad:
*****************************************/

void InputDeviceAdapterPenPad::synReportCallback(RawHID::EventDevice::CallbackData* cbData)
	{
	/* Update the relevant HID features: */
	{
	Threads::Mutex::Lock featureLock(featureMutex);
	for(int i=0;i<2;++i)
		posAxes[i].update();
	hoverButton.update();
	touchButton.update();
	for(unsigned int i=0;i<numButtons;++i)
		buttons[i].update();
	}
	
	/* Request a new Vrui frame: */
	requestUpdate();
	}

InputDeviceAdapterPenPad::InputDeviceAdapterPenPad(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager),
	 numButtons(0),buttons(0),
	 penScreen(0),pressedButtonIndex(-1)
	{
	/* Initialize input device adapter: */
	numInputDevices=1;
	inputDevices=new InputDevice*[numInputDevices];
	inputDevices[0]=0;
	
	/* Connect to the pen pad's component HIDs: */
	devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Pen"));
	devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Pad"));
	// devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Touch Strip"));
	// devices.push_back(RawHID::EventDevice(0x256cU,0x006dU,"Tablet Monitor Dial"));
	
	/* Register callbacks with the component HIDs and start dispatching events on the HIDs' device nodes: */
	for(std::vector<RawHID::EventDevice>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		{
		/* Try grabbing the device for exclusive access: */
		if(!dIt->grabDevice())
			Misc::sourcedConsoleWarning(__PRETTY_FUNCTION__,"Cannot grab device %s",dIt->getDeviceName().c_str());
		
		/* Register callbacks and start dispatching events from the device: */
		dIt->getSynReportEventCallbacks().add(this,&InputDeviceAdapterPenPad::synReportCallback);
		dIt->registerEventHandler(eventDispatcher);
		}
	
	/* Set up relevant device features: */
	for(int i=0;i<2;++i)
		posAxes[i]=RawHID::EventDevice::AbsAxisFeature(&devices[0],i);
	hoverButton=RawHID::EventDevice::KeyFeature(&devices[0],0);
	touchButton=RawHID::EventDevice::KeyFeature(&devices[0],1);
	
	/* Set up other buttons on the device: */
	numButtons=7;
	buttons=new RawHID::EventDevice::KeyFeature[numButtons];
	buttons[0]=RawHID::EventDevice::KeyFeature(&devices[0],2);
	buttons[1]=RawHID::EventDevice::KeyFeature(&devices[0],3);
	buttons[2]=RawHID::EventDevice::KeyFeature(&devices[1],0);
	buttons[3]=RawHID::EventDevice::KeyFeature(&devices[1],1);
	buttons[4]=RawHID::EventDevice::KeyFeature(&devices[1],2);
	buttons[5]=RawHID::EventDevice::KeyFeature(&devices[1],3);
	buttons[6]=RawHID::EventDevice::KeyFeature(&devices[1],4);
	buttonStates=new bool[numButtons];
	for(unsigned int i=0;i<numButtons;++i)
		buttonStates[i]=buttons[i].getValue();
	
	/* Set up the calibration B-spline mesh: */
	degrees[0]=2;
	degrees[1]=2;
	numPoints[0]=10;
	numPoints[1]=8;
	
	cps=new PPoint[numPoints[1]*numPoints[0]];
	cps[0]=PPoint(-0.367294,7.05492);
	cps[1]=PPoint(0.878565,7.05671);
	cps[2]=PPoint(2.4063,7.03613);
	cps[3]=PPoint(3.77085,6.99666);
	cps[4]=PPoint(5.24197,7.07477);
	cps[5]=PPoint(6.48573,6.94771);
	cps[6]=PPoint(8.15327,7.05621);
	cps[7]=PPoint(9.4919,7.00932);
	cps[8]=PPoint(11.0069,7.04781);
	cps[9]=PPoint(12.4864,6.93925);
	cps[10]=PPoint(-0.815232,5.91834);
	cps[11]=PPoint(0.714512,5.95462);
	cps[12]=PPoint(2.11873,5.93483);
	cps[13]=PPoint(3.60833,5.96898);
	cps[14]=PPoint(5.03114,5.95009);
	cps[15]=PPoint(6.56796,6.00374);
	cps[16]=PPoint(7.95475,5.95487);
	cps[17]=PPoint(9.44357,5.97989);
	cps[18]=PPoint(10.845,5.97524);
	cps[19]=PPoint(12.3068,5.96333);
	cps[20]=PPoint(-0.710181,4.85191);
	cps[21]=PPoint(0.711603,4.86181);
	cps[22]=PPoint(2.1832,4.86818);
	cps[23]=PPoint(3.62013,4.84536);
	cps[24]=PPoint(5.06256,4.86829);
	cps[25]=PPoint(6.51765,4.85935);
	cps[26]=PPoint(7.96183,4.8802);
	cps[27]=PPoint(9.40193,4.86861);
	cps[28]=PPoint(10.8575,4.87205);
	cps[29]=PPoint(12.4019,4.88515);
	cps[30]=PPoint(-0.681566,3.74711);
	cps[31]=PPoint(0.701734,3.7819);
	cps[32]=PPoint(2.17176,3.7689);
	cps[33]=PPoint(3.62334,3.79381);
	cps[34]=PPoint(5.05533,3.77539);
	cps[35]=PPoint(6.53399,3.79272);
	cps[36]=PPoint(7.94245,3.7982);
	cps[37]=PPoint(9.41997,3.80676);
	cps[38]=PPoint(10.9048,3.8052);
	cps[39]=PPoint(12.2869,3.84031);
	cps[40]=PPoint(-0.736064,2.66726);
	cps[41]=PPoint(0.730041,2.6865);
	cps[42]=PPoint(2.15252,2.68591);
	cps[43]=PPoint(3.63192,2.68479);
	cps[44]=PPoint(5.05574,2.69744);
	cps[45]=PPoint(6.50473,2.68907);
	cps[46]=PPoint(7.96682,2.69255);
	cps[47]=PPoint(9.40494,2.69087);
	cps[48]=PPoint(10.9038,2.70338);
	cps[49]=PPoint(12.3261,2.69114);
	cps[50]=PPoint(-0.731887,1.6474);
	cps[51]=PPoint(0.723596,1.62765);
	cps[52]=PPoint(2.17541,1.61777);
	cps[53]=PPoint(3.62517,1.62409);
	cps[54]=PPoint(5.06253,1.62771);
	cps[55]=PPoint(6.52659,1.63911);
	cps[56]=PPoint(7.98279,1.62969);
	cps[57]=PPoint(9.40622,1.63766);
	cps[58]=PPoint(10.8952,1.63771);
	cps[59]=PPoint(12.3686,1.65756);
	cps[60]=PPoint(-0.755756,0.459018);
	cps[61]=PPoint(0.716726,0.508476);
	cps[62]=PPoint(2.16866,0.493923);
	cps[63]=PPoint(3.60848,0.517135);
	cps[64]=PPoint(5.07003,0.490409);
	cps[65]=PPoint(6.51846,0.49623);
	cps[66]=PPoint(7.92949,0.512641);
	cps[67]=PPoint(9.41185,0.502721);
	cps[68]=PPoint(10.8536,0.517348);
	cps[69]=PPoint(12.3286,0.489287);
	cps[70]=PPoint(-0.496829,-0.454219);
	cps[71]=PPoint(1.03916,-0.609654);
	cps[72]=PPoint(2.45038,-0.568321);
	cps[73]=PPoint(3.89966,-0.61178);
	cps[74]=PPoint(5.29743,-0.549268);
	cps[75]=PPoint(6.74032,-0.580693);
	cps[76]=PPoint(8.2295,-0.604217);
	cps[77]=PPoint(9.63254,-0.552498);
	cps[78]=PPoint(10.9806,-0.587548);
	cps[79]=PPoint(12.6232,-0.434537);
	
	xs=new PPoint[degrees[0]+1];
	ys=new PPoint[degrees[1]+1];
	
	/* Create the list of pen feature names: */
	featureNames.push_back("Touch");
	featureNames.push_back("Pen1");
	featureNames.push_back("Pen2");
	featureNames.push_back("Pad1");
	featureNames.push_back("Pad2");
	featureNames.push_back("Pad3");
	featureNames.push_back("Pad4");
	featureNames.push_back("Pad5");
	
	/* Create the pen input device: */
	inputDevices[0]=inputDeviceManager->createInputDevice("Pen",InputDevice::TRACK_POS|InputDevice::TRACK_DIR,8,0,true);
	}

InputDeviceAdapterPenPad::~InputDeviceAdapterPenPad(void)
	{
	/* Release allocated resources: */
	delete[] buttons;
	delete[] buttonStates;
	delete[] cps;
	delete[] xs;
	delete[] ys;
	}

std::string InputDeviceAdapterPenPad::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Check the device is really ours: */
	if(feature.getDevice()!=inputDevices[0])
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",feature.getDevice()->getDeviceName());
	if(!feature.isButton()||feature.getIndex()>=8)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown feature");
	
	/* Return the button feature name: */
	return featureNames[feature.getIndex()];
	}

int InputDeviceAdapterPenPad::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Check the device is really ours: */
	if(device!=inputDevices[0])
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unknown device %s",device->getDeviceName());
	
	/* Find the feature name: */
	for(int i=0;i<8;++i)
		if(featureNames[i]==featureName)
			return device->getButtonFeatureIndex(i);
	
	return -1;
	}

void InputDeviceAdapterPenPad::prepareMainLoop(void)
	{
	/* Connect to the pen screen: */
	penScreen=findScreen("Screen");
	if(penScreen==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Screen Screen not found");
	}

void InputDeviceAdapterPenPad::updateInputDevices(void)
	{
	/* Grab relevant pen device data: */
	bool hoverState,touchState;
	Scalar pos[2];
	{
	Threads::Mutex::Lock featureLock(featureMutex);
	for(int i=0;i<2;++i)
		pos[i]=Scalar(posAxes[i].getNormalizedValueOneSide());
	hoverState=hoverButton.getValue();
	touchState=touchButton.getValue();
	for(unsigned int i=0;i<numButtons;++i)
		buttonStates[i]=buttons[i].getValue();
	}
	
	/* Check if the pen position is valid: */
	if(hoverState)
		{
		/* Retrieve the pen's uncalibrated position values in B-spline domain space: */
		Scalar tx=pos[0]*Scalar(numPoints[0]-degrees[0])+Scalar(degrees[0]);
		Scalar ty=pos[1]*Scalar(numPoints[1]-degrees[1])+Scalar(degrees[1]);
		
		/* Find the x and y knot intervals containing the evaluation parameters: */
		int ivx=Math::clamp(int(Math::floor(tx)),degrees[0],numPoints[0]-1);
		int ivy=Math::clamp(int(Math::floor(ty)),degrees[1],numPoints[1]-1);
		
		/* Evaluate x-direction B-spline curves: */
		for(int y=0;y<=degrees[1];++y)
			{
			/* Copy the partial control point array: */
			for(int x=0;x<=degrees[0];++x)
				xs[x]=cps[(ivy-degrees[1]+y)*numPoints[0]+(ivx-degrees[0]+x)];
			
			/* Run Cox-de Boor's algorithm on the partial array: */
			for(int k=0;k<degrees[0];++k)
				{
				int subDeg=degrees[0]-k;
				for(int x=0;x<subDeg;++x)
					xs[x]=Geometry::affineCombination(xs[x],xs[x+1],(tx-double(ivx-subDeg+1+x))/double(subDeg));
				}
			
			/* Put the final point into the y-direction B-spline curve evaluation array: */
			ys[y]=xs[0];
			}
		
		/* Evaluate the y-direction B-spline curve: */
		for(int k=0;k<degrees[1];++k)
			{
			int subDeg=degrees[1]-k;
			for(int y=0;y<subDeg;++y)
				ys[y]=Geometry::affineCombination(ys[y],ys[y+1],(ty-double(ivy-subDeg+1+y))/double(subDeg));
			}
		
		/* Transform the pen position from screen space to physical space: */
		TrackerState penTransform(Vector(ys[0][0],ys[0][1],Scalar(0)),Rotation::rotateX(Math::rad(Scalar(-90))));
		penTransform.leftMultiply(penScreen->getScreenTransformation());
		inputDevices[0]->setTransformation(penTransform);
		
		/* Calculate the pen's ray direction: */
		Vector rayDir=penTransform.getOrigin()-getMainViewer()->getHeadPosition();
		Scalar rayLen=Scalar(rayDir.mag());
		rayDir/=rayLen;
		inputDevices[0]->setDeviceRay(penTransform.inverseTransform(rayDir),-rayLen);
		
		/* Set the pen's button and valuator states: */
		int buttonIndex=0;
		for(unsigned int i=0;i<numButtons;++i)
			if(buttonStates[i])
				{
				buttonIndex=i+1;
				break;
				}
		if(pressedButtonIndex!=-1&&pressedButtonIndex!=buttonIndex)
			inputDevices[0]->setButtonState(pressedButtonIndex,false);
		inputDevices[0]->setButtonState(buttonIndex,touchState);
		pressedButtonIndex=touchState?buttonIndex:-1;
		
		/* Enable the pen device: */
		getInputGraphManager()->enable(inputDevices[0]);
		}
	else
		{
		/* Disable the pen device: */
		getInputGraphManager()->disable(inputDevices[0]);
		}
	}

}

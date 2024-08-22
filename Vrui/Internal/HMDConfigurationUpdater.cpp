/***********************************************************************
HMDConfigurationUpdater - Class to connect a rendering window for HMDs
to HMD configuration updates.
Copyright (c) 2024 Oliver Kreylos

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

#include <Vrui/Internal/HMDConfigurationUpdater.h>

#include <Misc/StdError.h>
#include <Misc/FunctionCalls.h>
#include <Threads/FunctionCalls.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/TextField.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/HMDConfiguration.h>
#include <Vrui/Internal/VRDeviceClient.h>
#include <Vrui/Internal/InputDeviceAdapterDeviceDaemon.h>

namespace Vrui {

/****************************************
Methods of class HMDConfigurationUpdater:
****************************************/

bool HMDConfigurationUpdater::hmdConfigurationUpdatedFrame(void* userData)
	{
	HMDConfigurationUpdater* thisPtr=static_cast<HMDConfigurationUpdater*>(userData);
	
	/* Lock the HMD configuration: */
	VRDeviceClient& dc=thisPtr->hmdAdapter->getDeviceClient();
	dc.lockHmdConfigurations();
	
	/* Check if the eye position changed: */
	if(thisPtr->eyePosVersion!=thisPtr->hmdConfiguration->getEyePosVersion())
		{
		/* Calculate the new IPD in mm: */
		Scalar newIpd=Geometry::dist(thisPtr->hmdConfiguration->getEyePosition(0),thisPtr->hmdConfiguration->getEyePosition(1))*getMeterFactor()*Scalar(1000);
		bool ipdDifferent=Math::abs(newIpd-thisPtr->lastShownIpd)>=(thisPtr->ipdDisplayDialog!=0?Scalar(0.2):Scalar(0.5));
		if(thisPtr->ipdDisplayDialog!=0)
			{
			/* Update the IPD display field: */
			GLMotif::TextField* ipdDisplay=static_cast<GLMotif::TextField*>(static_cast<GLMotif::RowColumn*>(thisPtr->ipdDisplayDialog->getChild())->getChild(1));
			ipdDisplay->setValue(newIpd);
			
			/* Extend the dialog's display time if the IPD is different enough: */
			if(ipdDifferent)
				{
				thisPtr->ipdDisplayDialogTakedownTime=getApplicationTime()+thisPtr->ipdDisplayDialogTimeout;
				thisPtr->lastShownIpd=newIpd;
				}
			}
		else if(ipdDifferent)
			{
			/* Create the IPD display dialog: */
			thisPtr->ipdDisplayDialog=new GLMotif::PopupWindow("IpdDisplayDialog",getWidgetManager(),"IPD Update");
			thisPtr->ipdDisplayDialog->setHideButton(false);
			
			GLMotif::RowColumn* ipdDisplayBox=new GLMotif::RowColumn("IpdDisplayBox",thisPtr->ipdDisplayDialog,false);
			ipdDisplayBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
			ipdDisplayBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
			ipdDisplayBox->setNumMinorWidgets(1);
			
			new GLMotif::Label("IpdDisplayLabel",ipdDisplayBox,"IPD");
			
			GLMotif::TextField* ipdDisplay=new GLMotif::TextField("IpdDisplay",ipdDisplayBox,6);
			ipdDisplay->setFieldWidth(5);
			ipdDisplay->setPrecision(1);
			ipdDisplay->setFloatFormat(GLMotif::TextField::FIXED);
			ipdDisplay->setValue(newIpd);
			
			new GLMotif::Label("IpdUnitLabel",ipdDisplayBox,"mm");
			
			ipdDisplayBox->manageChild();
			
			/* Pop up the IPD display dialog in the viewer's sight line: */
			Point hotspot=thisPtr->hmdViewer->getHeadPosition()+thisPtr->hmdViewer->getViewDirection()*(Scalar(24)*getInchFactor());
			popupPrimaryWidget(thisPtr->ipdDisplayDialog,hotspot,false);
			
			/* Set the dialog's display time: */
			thisPtr->ipdDisplayDialogTakedownTime=getApplicationTime()+thisPtr->ipdDisplayDialogTimeout;
			thisPtr->lastShownIpd=newIpd;
			}
		
		thisPtr->eyePosVersion=thisPtr->hmdConfiguration->getEyePosVersion();
		}
	
	/* Call the configuration updated callback: */
	(*thisPtr->configurationChangedCallback)(*thisPtr->hmdConfiguration);
	
	/* Unlock the HMD configuration: */
	dc.unlockHmdConfigurations();
	
	/* Check if an active IPD display dialog needs to be taken down: */
	if(thisPtr->ipdDisplayDialog!=0&&getApplicationTime()>=thisPtr->ipdDisplayDialogTakedownTime)
		{
		popdownPrimaryWidget(thisPtr->ipdDisplayDialog);
		delete thisPtr->ipdDisplayDialog;
		thisPtr->ipdDisplayDialog=0;
		
		/* Remove this callback: */
		return true;
		}
	else
		{
		/* Request another Vrui frame at the takedown time: */
		scheduleUpdate(thisPtr->ipdDisplayDialogTakedownTime);
		
		/* Keep this callback active: */
		return false;
		}
	}

void HMDConfigurationUpdater::hmdConfigurationUpdated(const HMDConfiguration& hmdConfiguration)
	{
	/* Hook a callback into Vrui's frame processing (will be ignored if the callback is already active): */
	addFrameCallback(hmdConfigurationUpdatedFrame,this);
	}

HMDConfigurationUpdater::HMDConfigurationUpdater(Viewer* sHmdViewer,HMDConfigurationUpdater::ConfigurationChangedCallback& sConfigurationChangedCallback)
	:hmdViewer(sHmdViewer),
	 hmdAdapter(0),hmdTrackerIndex(-1),
	 hmdConfiguration(0),configurationChangedCallback(&sConfigurationChangedCallback),
	 ipdDisplayDialogTimeout(5),ipdDisplayDialog(0)
	{
	/* Find the VRDeviceDaemon input device adapter connected to the given viewer: */
	InputDeviceAdapter* ida=getInputDeviceManager()->findInputDeviceAdapter(hmdViewer->getHeadDevice());
	hmdAdapter=dynamic_cast<InputDeviceAdapterDeviceDaemon*>(ida);
	if(hmdAdapter==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Viewer %s is not tracked by a VRDeviceDaemon client",hmdViewer->getName());
	
	/* Find the tracker index and HMD configuration associated with the head device: */
	hmdTrackerIndex=hmdAdapter->findInputDevice(hmdViewer->getHeadDevice());
	hmdConfiguration=hmdAdapter->findHmdConfiguration(hmdViewer->getHeadDevice());
	if(hmdConfiguration==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Viewer %s does not have an associated HMD configuration",hmdViewer->getName());
	
	/* Lock the HMD configuration: */
	VRDeviceClient& dc=hmdAdapter->getDeviceClient();
	dc.lockHmdConfigurations();
	
	/* Install an HMD configuration update callback with the input device adapter: */
	dc.setHmdConfigurationUpdatedCallback(hmdTrackerIndex,Misc::createFunctionCall(this,&HMDConfigurationUpdater::hmdConfigurationUpdated));
	
	/* Get the current eye position version and calculate the current IPD in mm: */
	eyePosVersion=hmdConfiguration->getEyePosVersion();
	lastShownIpd=Geometry::dist(hmdConfiguration->getEyePosition(0),hmdConfiguration->getEyePosition(1))*getMeterFactor()*Scalar(1000);
	
	/* Unlock the HMD configuration: */
	dc.unlockHmdConfigurations();
	}

HMDConfigurationUpdater::~HMDConfigurationUpdater(void)
	{
	delete ipdDisplayDialog;
	}

}

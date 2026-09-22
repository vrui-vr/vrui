/***********************************************************************
HMDConfigurationUpdater - Class to connect a rendering window for HMDs
to HMD configuration updates.
Copyright (c) 2024-2026 Oliver Kreylos

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

void HMDConfigurationUpdater::ipdDialogActiveFunction(Threads::ProcessFunction& processFunction)
	{
	/* Lock the HMD configuration: */
	VRDeviceClient& dc=hmdAdapter->getDeviceClient();
	dc.lockHmdConfigurations();
	
	/* Check if the eye position changed: */
	if(eyePosVersion!=hmdConfiguration->getEyePosVersion())
		{
		/* Calculate the new IPD in mm: */
		Scalar newIpd=Geometry::dist(hmdConfiguration->getEyePosition(0),hmdConfiguration->getEyePosition(1))*getMeterFactor()*Scalar(1000);
		bool ipdDifferent=Math::abs(newIpd-lastShownIpd)>=(ipdDisplayDialog!=0?Scalar(0.2):Scalar(0.5));
		
		/* Check if the IPD display dialog needs to be shown or updated: */
		if(ipdDisplayDialog!=0)
			{
			/* Update the IPD display field: */
			GLMotif::TextField* ipdDisplay=static_cast<GLMotif::TextField*>(static_cast<GLMotif::RowColumn*>(ipdDisplayDialog->getChild())->getChild(1));
			ipdDisplay->setValue(newIpd);
			}
		else if(ipdDifferent)
			{
			/* Create the IPD display dialog: */
			ipdDisplayDialog=new GLMotif::PopupWindow("IpdDisplayDialog",getWidgetManager(),"IPD Update");
			ipdDisplayDialog->setHideButton(false);
			
			GLMotif::RowColumn* ipdDisplayBox=new GLMotif::RowColumn("IpdDisplayBox",ipdDisplayDialog,false);
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
			Point hotspot=hmdViewer->getHeadPosition()+hmdViewer->getViewDirection()*(Scalar(24)*getInchFactor());
			popupPrimaryWidget(ipdDisplayDialog,hotspot,false);
			}
		
		/* Extend the dialog's display time if the IPD is different enough: */
		if(ipdDifferent)
			{
			ipdDisplayDialogTakedownTime=getApplicationTime()+ipdDisplayDialogTimeout;
			lastShownIpd=newIpd;
			}
		
		/* Mark the eye position as up-to-date: */
		eyePosVersion=hmdConfiguration->getEyePosVersion();
		}
	
	/* Call the configuration updated callback: */
	(*configurationChangedCallback)(*hmdConfiguration);
	
	/* Unlock the HMD configuration: */
	dc.unlockHmdConfigurations();
	
	/* Check if an active IPD display dialog needs to be taken down: */
	if(ipdDisplayDialog!=0&&getApplicationTime()>=ipdDisplayDialogTakedownTime)
		{
		popdownPrimaryWidget(ipdDisplayDialog);
		delete ipdDisplayDialog;
		ipdDisplayDialog=0;
		
		/* Disable this process function: */
		ipdDialogActive->disable();
		}
	else
		{
		/* Request another Vrui frame at the takedown time: */
		scheduleUpdate(ipdDisplayDialogTakedownTime);
		}
	}

void HMDConfigurationUpdater::hmdConfigurationUpdated(const HMDConfiguration& hmdConfiguration)
	{
	/* Enable the IPD dialog update function: */
	ipdDialogActive->enable();
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
	dc.setHmdConfigurationUpdatedCallback(hmdTrackerIndex,*Threads::createFunctionCall(this,&HMDConfigurationUpdater::hmdConfigurationUpdated));
	
	/* Get the current eye position version and calculate the current IPD in mm: */
	eyePosVersion=hmdConfiguration->getEyePosVersion();
	lastShownIpd=Geometry::dist(hmdConfiguration->getEyePosition(0),hmdConfiguration->getEyePosition(1))*getMeterFactor()*Scalar(1000);
	
	/* Unlock the HMD configuration: */
	dc.unlockHmdConfigurations();
	
	/* Create a process function to manage the IPD display dialog: */
	ipdDialogActive=new Threads::ProcessFunction(getRunLoop(),false,false,*Threads::createFunctionCall(this,&HMDConfigurationUpdater::ipdDialogActiveFunction));
	}

HMDConfigurationUpdater::~HMDConfigurationUpdater(void)
	{
	delete ipdDisplayDialog;
	}

}

/***********************************************************************
PenDeviceConfig - Structure defining the layout of an event device
representing a touchscreen, graphics tablet, etc.
Copyright (c) 2025 Oliver Kreylos

This file is part of the Raw HID Support Library (RawHID).

The Raw HID Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Raw HID Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Raw HID Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <RawHID/PenDeviceConfig.h>

#include <linux/input.h>
#include <RawHID/EventDevice.h>

namespace RawHID {

/*********************************
Methods of struct PenDeviceConfig:
*********************************/

PenDeviceConfig::PenDeviceConfig(void)
	:pressureAxisIndex(~0U),
	 touchKeyIndex(~0U),
	 valid(false),haveTilt(false),havePressure(false)
	{
	/* Initialize the absolute axis indices to invalid: */
	for(int i=0;i<2;++i)
		{
		posAxisIndices[i]=~0U;
		tiltAxisIndices[i]=~0U;
		}
	}

PenDeviceConfig::PenDeviceConfig(EventDevice& device)
	:pressureAxisIndex(~0U),
	 touchKeyIndex(~0U),
	 valid(false),haveTilt(false),havePressure(false)
	{
	/* Initialize the absolute axis indices to invalid: */
	for(int i=0;i<2;++i)
		{
		posAxisIndices[i]=~0U;
		tiltAxisIndices[i]=~0U;
		}
	
	/* Find the absolute axes and keys that define a pen device: */
	unsigned int featureMask=0x0U;
	
	/* Check for absolute axis features: */
	unsigned int numAxes=device.getNumAbsAxisFeatures();
	for(unsigned int i=0;i<numAxes;++i)
		{
		switch(device.getAbsAxisFeatureConfig(i).code)
			{
			case ABS_X:
				posAxisIndices[0]=i;
				featureMask|=0x1U;
				break;
			
			case ABS_Y:
				posAxisIndices[1]=i;
				featureMask|=0x2U;
				break;
			
			case ABS_TILT_X:
				tiltAxisIndices[0]=i;
				featureMask|=0x10U;
				break;
			
			case ABS_TILT_Y:
				tiltAxisIndices[1]=i;
				featureMask|=0x20U;
				break;
			
			case ABS_PRESSURE:
				pressureAxisIndex=i;
				havePressure=true;
				break;
			}
		}
	
	/* Check for key features: */
	unsigned int numKeys=device.getNumKeyFeatures();
	for(unsigned int i=0;i<numKeys;++i)
		{
		switch(device.getKeyFeatureCode(i))
			{
			case BTN_TOOL_PEN:
			case BTN_TOOL_RUBBER:
			case BTN_TOOL_BRUSH:
			case BTN_TOOL_PENCIL:
			case BTN_TOOL_AIRBRUSH:
			case BTN_TOOL_FINGER:
			case BTN_TOOL_MOUSE:
			case BTN_TOOL_LENS:
				hoverKeyIndices.push_back(i);
				featureMask|=0x04U;
				break;
			
			case BTN_TOUCH:
				touchKeyIndex=i;
				featureMask|=0x08U;
				break;
			
			default:
				/* Add the key as an additional key: */
				otherKeyIndices.push_back(i);
			}
		}
	
	/* Check if all required and optional features are present: */
	valid=(featureMask&0x0fU)==0x0fU;
	haveTilt=(featureMask&0x30U)==0x30U;
	}

PenDeviceConfig::PenState PenDeviceConfig::getPenState(const EventDevice& device) const
	{
	PenState result;
	
	/* Find the index of the first pen sub-component that is in range of the device: */
	result.valid=false;
	result.toolIndex=0U;
	for(std::vector<unsigned int>::const_iterator hkiIt=hoverKeyIndices.begin();hkiIt!=hoverKeyIndices.end()&&!result.valid;++hkiIt,++result.toolIndex)
		result.valid=device.getKeyFeatureValue(*hkiIt);
	
	if(result.valid)
		{
		/* Extract the pen position: */
		for(int i=0;i<2;++i)
			result.pos[i]=device.getAbsAxisFeatureValue(posAxisIndices[i]);
		
		/* Extract the pen tilt angles if supported: */
		if(haveTilt)
			for(int i=0;i<2;++i)
				result.tilt[i]=device.getAbsAxisFeatureValue(tiltAxisIndices[i]);
		
		/* Check if the pen is touching the device: */
		result.touching=device.getKeyFeatureValue(touchKeyIndex);
		
		/* Extract the touch pressure if the pen is touching and pressure is supported: */
		if(result.touching&&havePressure)
			result.pressure=device.getAbsAxisFeatureValue(pressureAxisIndex);
		}
	
	return result;
	}

}

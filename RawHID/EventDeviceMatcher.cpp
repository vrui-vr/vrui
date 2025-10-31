/***********************************************************************
EventDeviceMatcher - Helper class to match event devices on the host
against user specifications.
Copyright (c) 2023-2025 Oliver Kreylos

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

#include <RawHID/EventDeviceMatcher.h>

#include <stdio.h>
#include <linux/input.h>
#include <Misc/PrintInteger.h>

namespace RawHID {

/***********************************
Methods of class EventDeviceMatcher:
***********************************/

EventDeviceMatcher::~EventDeviceMatcher(void)
	{
	}

/*****************************************
Methods of class SelectEventDeviceMatcher:
*****************************************/

namespace {

/****************
Helper functions:
****************/

std::string ushortToString(unsigned short value)
	{
	char buffer[5];
	snprintf(buffer,sizeof(buffer),"%04x",(unsigned int)(value));
	return std::string(buffer);
	}

}

std::string SelectEventDeviceMatcher::getMatchSpec(void) const
	{
	std::string result;
	
	if((matchMask&BusType)!=0x0U)
		{
		result+="bus=";
		switch(matchBusType)
			{
			case BUS_PCI:
				result+="PCI";
				break;
			
			case BUS_ISAPNP:
				result+="ISAPNP";
				break;
			
			case BUS_USB:
				result+="USB";
				break;
			
			case BUS_HIL:
				result+="HIL";
				break;
			
			case BUS_BLUETOOTH:
				result+="Bluetooth";
				break;
			
			case BUS_VIRTUAL:
				result+="Virtual";
				break;
			
			default:
				result+=ushortToString(matchBusType);
			}
		}
	
	if((matchMask&(VendorID|ProductID))!=0x0U)
		{
		if(!result.empty())
			result+=", ";
		result+="ID=";
		
		if((matchMask&VendorID)!=0x0U)
			result+=ushortToString(matchVendorId);
		result.push_back(':');
		if((matchMask&ProductID)!=0x0U)
			result+=ushortToString(matchProductId);
		}
	
	if((matchMask&Version)!=0x0U)
		{
		if(!result.empty())
			result+=", ";
		result+="version=";
		result+=ushortToString(matchVersion);
		}
	
	if((matchMask&DeviceName)!=0x0U)
		{
		if(!result.empty())
			result+=", ";
		result+="name=";
		result.push_back('"');
		result+=matchDeviceName;
		result.push_back('"');
		}
	
	if((matchMask&SerialNumber)!=0x0U)
		{
		if(!result.empty())
			result+=", ";
		result+="serial number=";
		result.push_back('"');
		result+=matchSerialNumber;
		result.push_back('"');
		}
	
	if(!result.empty())
		result+=", ";
	result+="index=";
	char indexBuffer[16];
	result+=Misc::print(matchIndex,indexBuffer+15);
	
	return result;
	}

bool SelectEventDeviceMatcher::match(unsigned short busType,unsigned short vendorId,unsigned short productId,unsigned short version,const char* deviceName,const char* serialNumber)
	{
	return ((matchMask&BusType)==0x0U||matchBusType==busType)&&
	       ((matchMask&VendorID)==0x0U||matchVendorId==vendorId)&&
	       ((matchMask&ProductID)==0x0U||matchProductId==productId)&&
	       ((matchMask&Version)==0x0U||matchVersion==version)&&
	       ((matchMask&DeviceName)==0x0U||matchDeviceName==deviceName)&&
	       ((matchMask&SerialNumber)==0x0U||matchSerialNumber==serialNumber)&&
	       numMatches++==matchIndex;
	}

void SelectEventDeviceMatcher::setBusType(unsigned short newBusType)
	{
	matchMask|=BusType;
	matchBusType=newBusType;
	}

void SelectEventDeviceMatcher::setVendorId(unsigned short newVendorId)
	{
	matchMask|=VendorID;
	matchVendorId=newVendorId;
	}

void SelectEventDeviceMatcher::setProductId(unsigned short newProductId)
	{
	matchMask|=ProductID;
	matchProductId=newProductId;
	}

void SelectEventDeviceMatcher::setVersion(unsigned short newVersion)
	{
	matchMask|=Version;
	matchVersion=newVersion;
	}

void SelectEventDeviceMatcher::setDeviceName(const char* newDeviceName)
	{
	matchMask|=DeviceName;
	matchDeviceName=newDeviceName;
	}

void SelectEventDeviceMatcher::setDeviceName(const std::string& newDeviceName)
	{
	matchMask|=DeviceName;
	matchDeviceName=newDeviceName;
	}

void SelectEventDeviceMatcher::setSerialNumber(const char* newSerialNumber)
	{
	matchMask|=SerialNumber;
	matchSerialNumber=newSerialNumber;
	}

void SelectEventDeviceMatcher::setSerialNumber(const std::string& newSerialNumber)
	{
	matchMask|=SerialNumber;
	matchSerialNumber=newSerialNumber;
	}

void SelectEventDeviceMatcher::setIndex(unsigned int newIndex)
	{
	matchIndex=newIndex;
	}

}

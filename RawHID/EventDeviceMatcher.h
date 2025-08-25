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

#ifndef RAWHID_EVENTDEVICEMATCHER_INCLUDED
#define RAWHID_EVENTDEVICEMATCHER_INCLUDED

#include <string>

namespace RawHID {

class EventDeviceMatcher // Abstract base class for event device matchers
	{
	/* Constructors and destructors: */
	public:
	virtual ~EventDeviceMatcher(void); // Destroys the event device matcher
	
	/* Methods: */
	virtual std::string getMatchSpec(void) const =0; // Returns a description of the matcher's match specification
	virtual bool match(unsigned short busType,unsigned short vendorId,unsigned short productId,unsigned short version,const char* deviceName,const char* serialNumber) =0; // Returns true if the given device properties match the request
	};

class SelectEventDeviceMatcher:public EventDeviceMatcher // Class to match event devices against optional specification components and an index
	{
	/* Embedded classes: */
	public:
	enum MatchMask
		{
		BusType=0x1,
		VendorID=0x2,
		ProductID=0x4,
		Version=0x8,
		DeviceName=0x10,
		SerialNumber=0x20
		};
	
	/* Elements: */
	private:
	unsigned int matchMask; // Mask of event device specifier components to match against
	unsigned short matchBusType,matchVendorId,matchProductId,matchVersion; // Requested bus type, vendor and product ID, and device version
	std::string matchDeviceName; // Requested device name
	std::string matchSerialNumber; // Requested serial number
	unsigned int matchIndex; // Index of requested device among all matching devices
	unsigned int numMatches; // Number of matching devices already found
	
	/* Constructors and destructors: */
	public:
	SelectEventDeviceMatcher(void) // Creates a device matcher that matches the first instance of any device
		:matchMask(0x0U),
		 matchBusType(0U),matchVendorId(0U),matchProductId(0U),matchVersion(0U),
		 matchIndex(0),numMatches(0U)
		{
		}
	
	/* Methods from class EventDevice::DeviceMatcher: */
	virtual std::string getMatchSpec(void) const;
	virtual bool match(unsigned short busType,unsigned short vendorId,unsigned short productId,unsigned short version,const char* deviceName,const char* serialNumber);
	
	/* New methods: */
	void setBusType(unsigned short newBusType); // Sets the requested bus type
	void setVendorId(unsigned short newVendorId); // Sets the requested vendor ID
	void setProductId(unsigned short newProductId); // Sets the requested product ID
	void setVersion(unsigned short newVersion); // Sets the requested version number
	void setDeviceName(const char* newDeviceName); // Sets the requested device name
	void setDeviceName(const std::string& newDeviceName); // Ditto
	void setSerialNumber(const char* newSerialNumber); // Sets the requested serial number
	void setSerialNumber(const std::string& newSerialNumber); // Ditto
	void setIndex(unsigned int newIndex); // Sets the requested match index
	};

}

#endif

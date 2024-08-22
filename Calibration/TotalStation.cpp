/***********************************************************************
TotalStation - Class to represent a Leica Total Station survey
instrument to measure points for coordinate system calibration.
Copyright (c) 2009-2024 Oliver Kreylos

This file is part of the Vrui calibration utility package.

The Vrui calibration utility package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Vrui calibration utility package is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui calibration utility package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include "TotalStation.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <Misc/StdError.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>

namespace {

/****************
Helper functions:
****************/

unsigned int readUInt(std::string::const_iterator& sIt,int numDigits)
	{
	if(numDigits==0)
		return 0;
	std::string::const_iterator end=sIt+numDigits;
	unsigned int result=0;
	while(sIt!=end&&*sIt>='0'&&*sIt<='9')
		{
		result*=10;
		result+=(unsigned int)(*sIt-'0');
		++sIt;
		}
	return result;
	}

int readInt(std::string::const_iterator& sIt,int numDigits)
	{
	if(numDigits==0)
		return 0;
	std::string::const_iterator end=sIt+numDigits;
	int result=0;
	bool sign=false;
	if(*sIt=='-')
		{
		sign=true;
		++sIt;
		}
	else if(*sIt=='+')
		++sIt;
	while(sIt!=end&&*sIt>='0'&&*sIt<='9')
		{
		result*=10;
		result+=int(*sIt-'0');
		++sIt;
		}
	return sign?-result:result;
	}

}

/*****************************
Methods of class TotalStation:
*****************************/

void TotalStation::sendRequest(const char* format,...)
	{
	/* Assemble the request message: */
	char requestBuffer[256];
	va_list ap;
	va_start(ap,format);
	vsnprintf(requestBuffer,sizeof(requestBuffer),format,ap);
	va_end(ap);
	
	/* Send the message to the device: */
	devicePort.writeRaw(requestBuffer,strlen(requestBuffer));
	devicePort.flush();
	}

std::string TotalStation::readReply(void)
	{
	std::string result;
	int byte;
	while(true)
		{
		byte=devicePort.getChar();
		if(byte=='\r')
			{
			/* Skip the \n as well: */
			byte=devicePort.getChar();
			break;
			}
		else if(byte=='\n')
			break;
		result.push_back(byte);
		}
	
	/* Check for an error indicator: */
	if(result[0]=='@')
		{
		if(result[1]=='W')
			{
			/* Define a warning message table: */
			static const unsigned int warningCodes[4]=
				{
				100U,127U,139U,158U
				};
			static const char* warningMessages[5]=
				{
				"instrument busy","invalid command","EDM error","invalid sensor corrections",
				"unknown warning"
				};
			
			/* Find the warning message and throw an exception: */
			std::string::const_iterator cIt=result.begin()+2;
			unsigned int code=readUInt(cIt,3);
			int i;
			for(i=0;i<4&&code!=warningCodes[i];++i)
				;
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Warning %d (%s)",code,warningMessages[i]);
			}
		else if(result[1]=='E')
			{
			/* Define an error message table: */
			static const unsigned int errorCodes[8]=
				{
				101U,103U,112U,114U,139U,144U,155U,158U
				};
			static const char* errorMessages[9]=
				{
				"value out of range","invalid value","battery low","invalid command","EDM error",
				"collimation error","weak EDM signal","invalid sensor corrections",
				"unknown error"
				};
			
			/* Find the error message and throw an exception: */
			std::string::const_iterator cIt=result.begin()+2;
			unsigned int code=readUInt(cIt,3);
			int i;
			for(i=0;i<8&&code!=errorCodes[i];++i)
				;
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error %d (%s)",code,errorMessages[i]);
			}
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unrecognized '@' reply");
		}
	
	/* Return the reply: */
	return result;
	}

unsigned int TotalStation::conf(unsigned int confId)
	{
	/* Send the CONF request: */
	sendRequest("CONF/%04u\r\n",confId);
	
	/* Parse the CONF reply: */
	std::string confReply=readReply();
	std::string::const_iterator crIt=confReply.begin();
	unsigned int confReplyId=readUInt(crIt,4);
	if(crIt==confReply.end()||*crIt!='/')
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Malformed CONF reply \"%s\"",confReply.c_str());
	if(confReplyId!=confId)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching ID in CONF reply; expected %u, got %u",confId,confReplyId);
	
	/* Read the CONF reply value: */
	++crIt;
	return readUInt(crIt,4);
	}

std::string TotalStation::getString(unsigned int getId,bool requestMeasurement)
	{
	return "";
	}

unsigned int TotalStation::getUInt(unsigned int getId,bool requestMeasurement)
	{
	/* Send the GET request: */
	sendRequest("GET/%c/WI%02u\r\n",requestMeasurement?'M':'I',getId);
	
	/* Parse the GET reply: */
	std::string getReply=readReply();
	if(getReply[0]!='*')
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Loss of synchronization in GET requests");
	unsigned int getReplyId=(unsigned int)(getReply[1]-'0')*10+(unsigned int)(getReply[2]-'0');
	if(getReplyId!=getId)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching ID in GET reply; expected %u, got %u",getId,getReplyId);
	
	std::string::const_iterator valueIt=getReply.begin()+8;
	return readUInt(valueIt,16);
	}

int TotalStation::getInt(unsigned int getId,bool requestMeasurement)
	{
	/* Send the GET request: */
	sendRequest("GET/%c/WI%02u\r\n",requestMeasurement?'M':'I',getId);
	
	/* Parse the GET reply: */
	std::string getReply=readReply();
	if(getReply[0]!='*')
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Loss of synchronization in GET requests");
	unsigned int getReplyId=(unsigned int)(getReply[1]-'0')*10+(unsigned int)(getReply[2]-'0');
	if(getReplyId!=getId)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching ID in GET reply; expected %u, got %u",getId,getReplyId);
	
	std::string::const_iterator valueIt=getReply.begin()+7;
	return readInt(valueIt,17);
	}

namespace {

/***************
Helper function:
***************/

double parseMeasurement(std::string::const_iterator& sIt)
	{
	/* Skip the irrelevant byte: */
	++sIt;
	
	/* Skip the automatic index information: */
	++sIt;
	
	/* Skip the input mode: */
	++sIt;
	
	/* Extract the measurement unit: */
	int unitCode=*sIt-'0';
	++sIt;
	
	/* Convert to meters or radians using the reported measurement unit: */
	double result=0.0;
	switch(unitCode)
		{
		case 0: // Distance in milimeters
			result=double(readInt(sIt,17))/1000.0;
			break;
		
		case 1: // Distance in milifeet
			result=double(readInt(sIt,17))*0.0254*12.0/1000.0;
			break;
		
		case 2: // Angle in gons
			result=double(readInt(sIt,17))*2.0*Math::Constants<double>::pi/400.0;
			break;
		
		case 3: // Angle in decimal degrees
			result=double(readInt(sIt,17))*2.0*Math::Constants<double>::pi/360.0;
			break;
		
		case 4: // Angle in sexagesimal degrees
			{
			bool sign=*sIt=='-';
			++sIt;
			double degrees=double(readInt(sIt,11));
			double minutes=double(readInt(sIt,2));
			double seconds=double(readInt(sIt,3));
			result=(degrees+minutes/60.0+seconds/36000.0)*2.0*Math::Constants<double>::pi/360.0;
			if(sign)
				result=-result;
			break;
			}
		
		case 5: // Angle in mils
			result=double(readInt(sIt,17))*2.0*Math::Constants<double>::pi/6400.0;
			break;
		
		case 6: // Distance in 0.1 milimeters
			result=double(readInt(sIt,17))/10000.0;
			break;
		
		case 7: // Distance in 0.1 milifeet
			result=double(readInt(sIt,17))*0.0254*12.0/10000.0;
			break;
		
		case 8: // Distance in 0.01 milimeters
			result=double(readInt(sIt,17))/100000.0;
			break;
		}
	
	return result;
	}

}

double TotalStation::getMeasurement(unsigned int getId,bool requestMeasurement)
	{
	/* Send the GET request: */
	sendRequest("GET/%c/WI%02u\r\n",requestMeasurement?'M':'I',getId);
	
	/* Parse the GET reply: */
	std::string getReply=readReply();
	std::string::const_iterator grIt=getReply.begin();
	if(*grIt!='*')
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Loss of synchronization in GET requests");
	++grIt;
	unsigned int getReplyId=readUInt(grIt,2);
	if(getReplyId!=getId)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching ID in GET reply; expected %u, got %u",getId,getReplyId);
	
	return parseMeasurement(grIt);
	}

void TotalStation::getMultiMeasurement(size_t numGets,const unsigned int getIds[],double values[],bool requestMeasurement)
	{
	if(numGets==0)
		return;
	
	/* Send the GET request: */
	devicePort.writeRaw("GET/",4);
	devicePort.putChar(requestMeasurement?'M':'I');
	for(size_t i=0;i<numGets;++i)
		{
		char getBuffer[20];
		snprintf(getBuffer,sizeof(getBuffer),"/WI%02u",getIds[i]);
		devicePort.writeRaw(getBuffer,strlen(getBuffer));
		}
	devicePort.writeRaw("\r\n",2);
	devicePort.flush();
	
	/* Parse the GET reply: */
	std::string getReply=readReply();
	std::string::const_iterator grIt=getReply.begin();
	if(*grIt!='*')
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Loss of synchronization in GET requests");
	++grIt;
	for(size_t i=0;i<numGets;++i)
		{
		if(grIt==getReply.end())
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing value in GET reply; expected %u, got %u",numGets,i);
		unsigned int getReplyId=readUInt(grIt,2);
		if(getReplyId!=getIds[i])
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mismatching ID in GET reply; expected %u, got %u",getIds[i],getReplyId);
		values[i]=parseMeasurement(grIt);
		
		/* Skip the separator: */
		++grIt;
		}
	}

TotalStation::Point TotalStation::calcCartesianPoint(const double measurements[3]) const
	{
	/* Convert the distance to reported units: */
	double distance=measurements[2]*unitScale;
	
	/* Calculate the Cartesian point relative to the instrument: */
	Point result;
	result[0]=Math::sin(measurements[0])*Math::sin(measurements[1])*distance;
	if(horizAngleDirection==COUNTER_CLOCKWISE)
		result[0]=-result[0];
	result[1]=Math::cos(measurements[0])*Math::sin(measurements[1])*distance;
	result[2]=Math::cos(measurements[1])*distance;
	return result;
	}

TotalStation::TotalStation(const char* devicePortName,int deviceBaudRate)
	:devicePort(devicePortName,true),
	 angleUnit(GON),
	 horizAngleDirection(CLOCKWISE),
	 distanceUnit(METER),
	 unitScale(1.0),
	 prismOffset(0.0)
	{
	/* Initialize the device port: */
	devicePort.ref();
	devicePort.setPortSettings(Comm::SerialPort::Blocking);
	devicePort.setSerialSettings(deviceBaudRate,8,Comm::SerialPort::NoParity,1,false);
	devicePort.setRawMode(1,0);
	// devicePort.setLineControl(false,false);
	
	/* Power on Total Station: */
	sendRequest("a\r\n");
	if(readReply()!="?")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot communicate with device on port %s with baud rate %d",devicePortName,deviceBaudRate);
	
	/* Query the Total Station's angle unit: */
	unsigned int angleUnitCode=conf(40);
	switch(angleUnitCode)
		{
		case 0:
			angleUnit=GON;
			break;
		
		case 1:
			angleUnit=DEGREE_DECIMAL;
			break;
		
		case 2:
			angleUnit=DEGREE_SEXAGESIMAL;
			break;
		
		case 3:
			angleUnit=MIL;
			break;
		
		case 4:
			angleUnit=RADIAN;
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unrecognized angle unit %u",angleUnitCode);
		}
	
	/* Query the Total Station's horizontal angle direction: */
	unsigned int horizAngleDirectionCode=conf(171);
	switch(horizAngleDirectionCode)
		{
		case 0:
			horizAngleDirection=CLOCKWISE;
			break;
		
		case 1:
			horizAngleDirection=COUNTER_CLOCKWISE;
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unrecognized horizontal angle direction %u",horizAngleDirectionCode);
		}
	
	/* Query the Total Station's distance unit: */
	unsigned int distanceUnitCode=conf(40);
	switch(distanceUnitCode)
		{
		case 0:
			distanceUnit=METER;
			break;
		
		case 1:
			distanceUnit=USFOOT_DECIMAL;
			break;
		
		case 2:
			distanceUnit=INTLFOOT_DECIMAL;
			break;
		
		case 3:
			distanceUnit=USFOOT_INCH;
			break;
		
		case 4:
			distanceUnit=INTLFOOT_INCH;
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unrecognized distance unit %u",distanceUnitCode);
		}
	
	/* Set the Total Station's GIS packet type to GIS16: */
	sendRequest("SET/137/1\r\n");
	if(readReply()!="?")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set RS232 packet format to GIS16");
	
	/* Get the Total Station's current prism offset: */
	prismOffset=getMeasurement(58,false);
	}

TotalStation::~TotalStation(void)
	{
	}

TotalStation::Scalar TotalStation::getPrismOffset(void)
	{
	/* Get the Total Station's current prism offset: */
	return getMeasurement(58,false);
	}

unsigned int TotalStation::getEDMMode(void)
	{
	/* Get the Total Station's EDM mode: */
	return conf(161);
	}

void TotalStation::setUnitScale(TotalStation::Scalar newUnitScale)
	{
	/* Set the unit scale: */
	unitScale=newUnitScale;
	}

void TotalStation::setPrismOffset(TotalStation::Scalar newPrismOffset)
	{
	/* Set the Total Station's prism offset: */
	sendRequest("PUT/58..16%+09d \r\n",int(Math::floor(newPrismOffset*10.0+0.5)));
	if(readReply()!="?")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set prism offset");
	
	/* Get the Total Station's current prism offset: */
	prismOffset=getMeasurement(58,false);
	}

void TotalStation::setEDMMode(unsigned int newEDMMode)
	{
	sendRequest("SET/161/%u\r\n",newEDMMode);
	if(readReply()!="?")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot set EDM mode");
	}

TotalStation::Point TotalStation::getLastMeasurement(void)
	{
	/* Check if there is a valid current measurement: */
	if(conf(160)==1)
		{
		/* Read the last horizontal and vertical angles and distance: */
		unsigned int getIds[3]={21,22,31};
		double values[3];
		getMultiMeasurement(3,getIds,values,false);
		
		/* Calculate the Cartesian point relative to the instrument: */
		return calcCartesianPoint(values);
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Total Station has no valid measurement");
	}

TotalStation::Point TotalStation::requestMeasurement(void)
	{
	/* Read the last horizontal and vertical angles and distance: */
	unsigned int getIds[3]={21,22,31};
	double values[3];
	getMultiMeasurement(3,getIds,values,true);
	if(horizAngleDirection==COUNTER_CLOCKWISE)
		values[0]=-values[0];
	
	/* Calculate the Cartesian point relative to the instrument: */
	return calcCartesianPoint(values);
	}

void TotalStation::startRecording(void)
	{
	/* Set the Total Station's data recording device to RS232: */
	sendRequest("SET/75/0\r\n");
	if(readReply()!="?")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot start recording mode");
	sendRequest("SET/76/1\r\n");
	if(readReply()!="?")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot start recording mode");
	}

void TotalStation::stopRecording(void)
	{
	/* Set the Total Station's data recording device to back to its internal memory: */
	sendRequest("SET/76/0\r\n");
	if(readReply()!="?")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot stop recording mode");
	}

TotalStation::Point TotalStation::readNextMeasurement(void)
	{
	/* Wait for and read the next measurement: */
	std::string measurement=readReply();
	std::string::const_iterator mIt=measurement.begin();
	if(*mIt!='*')
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Loss of synchronization");
	++mIt;
	double values[3]={0.0,0.0,0.0};
	int componentMask=0x0;
	while(mIt!=measurement.end())
		{
		unsigned int mId=readUInt(mIt,2);
		switch(mId)
			{
			case 21: // Horizontal angle
				values[0]=parseMeasurement(mIt);
				if(horizAngleDirection==COUNTER_CLOCKWISE)
					values[0]=-values[0];
	
				componentMask|=0x1;
				break;
			
			case 22: // Vertical angle
				values[1]=parseMeasurement(mIt);
				componentMask|=0x2;
				break;
			
			case 31: // Distance
				values[2]=parseMeasurement(mIt);
				componentMask|=0x4;
				break;
			
			case 51: // Special case; need to parse differently
				{
				/* Just skip the damn thing: */
				mIt+=21;
				break;
				}
			
			default:
				parseMeasurement(mIt);
			}
		
		/* Skip the separator: */
		++mIt;
		}
	
	/* Check if all components have been read: */
	if(componentMask!=0x7)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing measurement component");
	
	/* Calculate the Cartesian point relative to the instrument: */
	return calcCartesianPoint(values);
	}


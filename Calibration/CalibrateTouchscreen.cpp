/***********************************************************************
CalibrateTouchscreen - Vrui utility to calibrate a touchscreen or pen
display or similar device.
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

#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Misc/Size.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Misc/ConfigurationFile.h>
#include <Realtime/Time.h>
#include <Threads/Mutex.h>
#include <Threads/EventDispatcherThread.h>
#include <RawHID/EventDevice.h>
#include <RawHID/EventDeviceMatcher.h>
#include <RawHID/PenDeviceConfig.h>
#include <Geometry/Point.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OutputOperators.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Internal/PenPadCalibrator.h>
#include <Vrui/Internal/PenPadCalibratorRectilinear.h>
#include <Vrui/Internal/PenPadCalibratorAffine.h>
#include <Vrui/Internal/PenPadCalibratorProjective.h>
#include <Vrui/Internal/PenPadCalibratorBSpline.h>

class CalibrateTouchscreen:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef Misc::Size<2> Size; // Type for mesh sizes etc.
	typedef Vrui::Scalar Scalar; // Scalar type
	typedef Vrui::PenPadCalibrator::Point2 Point2; // Type for points in screen space
	typedef Vrui::PenPadCalibrator::Box2 Box2; // Type for boxes in screen space
	typedef Vrui::PenPadCalibrator::TiePoint TiePoint; // Type for tie points between touchscreen measurement space and screen space
	typedef Vrui::PenPadCalibrator::TiePointList TiePointList; // Type for lists of tie points
	typedef RawHID::EventDevice::AbsAxisConfig AxisConfig;
	
	/* Elements: */
	RawHID::EventDevice* penDevice; // The input device associated with the touchscreen's pen device
	RawHID::PenDeviceConfig penDeviceConfig; // The pen device's configuration
	Threads::EventDispatcherThread dispatcher; // Event dispatcher for events on the pen device
	Box2 posDomain; // Domain of pen's position axes
	Vrui::VRScreen* screen; // The screen associated with the touchscreen
	Scalar screenSize[2]; // Width and height of the touchscreen in physical coordinate units
	Scalar gridGap; // Gap between the outer edges of the screen and the grid in grid tile units
	Size numPoints; // Number of calibration points in x and y
	unsigned int currentPoint; // Current calibration point
	Realtime::TimePointMonotonic measureStart; // Earliest time at which a new measurement can start
	Scalar pointAccum[2]; // Absolute axis accumulators for the current point
	Scalar tiltAccum[2]; // Tilt axis accumulators for the current point
	unsigned int numAccum; // Number of accumulated points
	TiePointList tiePoints; // List of already collected calibration tie points
	int calibrationType; // Type of calibration method
	Size splineDegree; // Degree for B-Spline calibrator
	Size splineSize; // Mesh size for B-Spline calibrator
	Vrui::PenPadCalibrator* calibrator; // Pointer to a calibration object, or 0 if calibration is incomplete
	mutable Threads::Mutex penStateMutex; // Mutex protecting the current pen state
	bool hovering; // Flag whether the pen is currently close enough to the touch screen for the pen position to be valid
	Point2 penPos; // Calibration pen position when a calibration has been calculated
	
	/* Private methods: */
	void keyCallback(RawHID::EventDevice::KeyFeatureEventCallbackData* cbData);
	void absAxisCallback(RawHID::EventDevice::AbsAxisFeatureEventCallbackData* cbData);
	void synReportCallback(Misc::CallbackData* cbData);
	void calibrate(void);
	static void listPenDevices(void);
	
	/* Constructors and destructors: */
	public:
	CalibrateTouchscreen(int& argc,char**& argv);
	virtual ~CalibrateTouchscreen(void);
	
	/* Methods from class Vrui::Application: */
	virtual void prepareMainLoop(void);
	virtual void display(GLContextData& contextData) const;
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	};

/*************************************
Methods of class CalibrateTouchscreen:
*************************************/

void CalibrateTouchscreen::keyCallback(RawHID::EventDevice::KeyFeatureEventCallbackData* cbData)
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	/* Bail out if there is a calibration: */
	if(calibrator!=0)
		return;
	
	if(cbData->featureIndex==penDeviceConfig.touchKeyIndex)
		{
		Realtime::TimePointMonotonic now;
		
		if(cbData->newValue)
			{
			if(now>=measureStart&&currentPoint<numPoints.volume())
				{
				/* Start accumulating data for the current point: */
				RawHID::PenDeviceConfig::PenState ps=penDeviceConfig.getPenState(*penDevice);
				for(int i=0;i<2;++i)
					pointAccum[i]=Scalar(ps.pos[i]);
				if(penDeviceConfig.haveTilt)
					for(int i=0;i<2;++i)
						tiltAccum[i]=Scalar(ps.tilt[i]);
				numAccum=1;
				}
			}
		else
			{
			if(numAccum>=1)
				{
				/* Print the current measurement: */
				std::cout<<currentPoint<<": pos "<<pointAccum[0]/Scalar(numAccum)<<", "<<pointAccum[1]/Scalar(numAccum);
				if(penDeviceConfig.haveTilt)
					std::cout<<", tilt "<<tiltAccum[0]/Scalar(numAccum)<<", "<<tiltAccum[1]/Scalar(numAccum);
				std::cout<<std::endl;
				
				/* Create a new calibration tie point from the accumulated measurements: */
				TiePoint tp;
				unsigned int index[2];
				index[0]=currentPoint%numPoints[0];
				index[1]=currentPoint/numPoints[0];
				if(index[1]%2==1)
					index[0]=numPoints[0]-1-index[0];
				for(int i=0;i<2;++i)
					{
					tp.raw[i]=pointAccum[i]/Scalar(numAccum);
					tp.screen[i]=(Scalar(index[i])+gridGap)/(Scalar(numPoints[i]-1)+Scalar(2)*gridGap);
					}
				tiePoints.push_back(tp);
				
				/* Calculate a calibration if a full set of points has been collected: */
				if(tiePoints.size()>=numPoints.volume())
					calibrate();
				
				/* Move to the next point: */
				++currentPoint;
				}
			
			/* Reset the accumulator: */
			numAccum=0;
			
			/* Wait at least one second before starting the next measurement: */
			measureStart=now+Realtime::TimeVector(1,0);
			}
		}
	else if(cbData->featureIndex==penDeviceConfig.touchKeyIndex)
		{
		/* Update the hovering flag: */
		hovering=cbData->newValue;
		}
	
	Vrui::requestUpdate();
	}

void CalibrateTouchscreen::absAxisCallback(RawHID::EventDevice::AbsAxisFeatureEventCallbackData* cbData)
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	/* Get the current pen state: */
	RawHID::PenDeviceConfig::PenState ps=penDeviceConfig.getPenState(*penDevice);
	if(ps.valid)
		{
		/* Update the pen state with this callback: */
		for(int i=0;i<2;++i)
			{
			if(cbData->featureIndex==penDeviceConfig.posAxisIndices[i])
				ps.pos[i]=cbData->newValue;
			if(cbData->featureIndex==penDeviceConfig.tiltAxisIndices[i])
				ps.tilt[i]=cbData->newValue;
			}
		
		if(calibrator!=0)
			{
			/* Calibrate the new pen position: */
			penPos=calibrator->calibrate(Point2(ps.pos[0],ps.pos[1]));
			for(int i=0;i<2;++i)
				penPos[i]*=screenSize[i];
			
			Vrui::requestUpdate();
			}
		else
			{
			/* Accumulate the new pen position: */
			for(int i=0;i<2;++i)
				pointAccum[i]+=Scalar(ps.pos[i]);
			if(penDeviceConfig.haveTilt)
				for(int i=0;i<2;++i)
					tiltAccum[i]+=Scalar(ps.tilt[i]);
			++numAccum;
			}
		}
	}

void CalibrateTouchscreen::synReportCallback(Misc::CallbackData* cbData)
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	RawHID::PenDeviceConfig::PenState ps=penDeviceConfig.getPenState(*penDevice);
	if(ps.valid)
		{
		if(calibrator!=0)
			{
			/* Calibrate the new pen position: */
			penPos=calibrator->calibrate(Point2(ps.pos[0],ps.pos[1]));
			for(int i=0;i<2;++i)
				penPos[i]*=screenSize[i];
			
			Vrui::requestUpdate();
			}
		else if(numAccum>0)
			{
			/* Accumulate the new pen position: */
			for(int i=0;i<2;++i)
				pointAccum[i]+=Scalar(ps.pos[i]);
			if(penDeviceConfig.haveTilt)
				for(int i=0;i<2;++i)
					tiltAccum[i]+=Scalar(ps.tilt[i]);
			++numAccum;
			}
		}
	}

void CalibrateTouchscreen::calibrate(void)
	{
	{
	/* Save the calibration point set to a CSV file: */
	std::ofstream csv("/home/okreylos/Desktop/CalibrationData.csv");
	for(TiePointList::iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
		csv<<tpIt->raw[0]<<','<<tpIt->raw[1]<<','<<tpIt->screen[0]<<','<<tpIt->screen[1]<<std::endl;
	}
	
	/* Create a configuration file to hold the final calibration: */
	Misc::ConfigurationFile configFile;
	Misc::ConfigurationFileSection root(configFile.getCurrentSection());
	
	#if 1
	
	/* Do all the calibrations to compare them: */
	std::pair<Vrui::Scalar,Vrui::Scalar> residuals;
	Misc::ConfigurationFileSection rectilinear=root.getSection("Rectilinear");
	calibrator=new Vrui::PenPadCalibratorRectilinear(tiePoints,posDomain,rectilinear);
	residuals=calibrator->calcResiduals(tiePoints,screenSize);
	std::cout<<"Rectilinear approximation residuals: "<<residuals.first<<" L^2, "<<residuals.second<<" L^infinity"<<std::endl;
	delete calibrator;
	Misc::ConfigurationFileSection affine=root.getSection("Affine");
	calibrator=new Vrui::PenPadCalibratorAffine(tiePoints,posDomain,affine);
	residuals=calibrator->calcResiduals(tiePoints,screenSize);
	std::cout<<"Affine approximation residuals: "<<residuals.first<<" L^2, "<<residuals.second<<" L^infinity"<<std::endl;
	delete calibrator;
	Misc::ConfigurationFileSection projective=root.getSection("Projective");
	calibrator=new Vrui::PenPadCalibratorProjective(tiePoints,posDomain,projective);
	residuals=calibrator->calcResiduals(tiePoints,screenSize);
	std::cout<<"Projective approximation residuals: "<<residuals.first<<" L^2, "<<residuals.second<<" L^infinity"<<std::endl;
	delete calibrator;
	Misc::ConfigurationFileSection bspline=root.getSection("BSpline");
	calibrator=new Vrui::PenPadCalibratorBSpline(splineDegree,splineSize,tiePoints,posDomain,bspline);
	residuals=calibrator->calcResiduals(tiePoints,screenSize);
	std::cout<<"B-Spline approximation residuals: "<<residuals.first<<" L^2, "<<residuals.second<<" L^infinity"<<std::endl;
	delete calibrator;
	
	#endif
	
	/* Create a calibrator object: */
	switch(calibrationType)
		{
		case 0:
			calibrator=new Vrui::PenPadCalibratorRectilinear(tiePoints,posDomain,root);
			break;
		
		case 1:
			calibrator=new Vrui::PenPadCalibratorAffine(tiePoints,posDomain,root);
			break;
		
		case 2:
			calibrator=new Vrui::PenPadCalibratorProjective(tiePoints,posDomain,root);
			break;
		
		case 3:
			calibrator=new Vrui::PenPadCalibratorBSpline(splineDegree,splineSize,tiePoints,posDomain,root);
			break;
		}
	
	/* Calculate the calibrator's residuals: */
	residuals=calibrator->calcResiduals(tiePoints,screenSize);
	std::cout<<"Selected approximation residuals: "<<residuals.first<<" L^2, "<<residuals.second<<" L^infinity"<<std::endl;
	
	/* Save the configuration file: */
	configFile.saveAs("/home/okreylos/Desktop/TouchscreenCalibration.cfg");
	}

void CalibrateTouchscreen::listPenDevices(void)
	{
	/* Retrieve the list of all event devices: */
	std::vector<std::string> eventDevices=RawHID::EventDevice::getEventDeviceFileNames();
	for(std::vector<std::string>::iterator edIt=eventDevices.begin();edIt!=eventDevices.end();++edIt)
		{
		try
			{
			/* Try opening the event device: */
			RawHID::EventDevice device(edIt->c_str());
			
			/* Query the device's pen device configuration: */
			RawHID::PenDeviceConfig config(device);
			
			/* Print the device's identifier if it is a valid pen device: */
			if(config.valid)
				{
				/* Print information about the pen device: */
				std::cout<<"Pen device ";
				std::cout<<std::hex<<std::setfill('0')<<std::setw(4)<<device.getVendorId()<<':'<<std::setw(4)<<device.getProductId()<<std::dec<<std::setfill(' ');
				std::cout<<", version "<<device.getVersion();
				std::cout<<", "<<device.getDeviceName()<<" (serial no. "<<device.getSerialNumber()<<')'<<std::endl;
				}
			}
		catch(const std::runtime_error& err)
			{
			/* Ignore the error silently not to clog up the output with error messages: */
			// Misc::formattedUserError("Cannot open event device %s due to exception %s",edIt->c_str(),err.what());
			}
		}
	}

CalibrateTouchscreen::CalibrateTouchscreen(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 penDevice(0),
	 screen(0),
	 numPoints(4,3),
	 numAccum(0),
	 calibrationType(0),splineDegree(2,2),splineSize(5,3),
	 calibrator(0),
	 hovering(false)
	{
	/* Parse the command line: */
	bool listDevices=false;
	RawHID::SelectEventDeviceMatcher deviceMatcher;
	gridGap=0.2;
	for(int argi=1;argi<argc;++argi)
		{
		if(argv[argi][0]=='-')
			{
			if(strcasecmp(argv[argi]+1,"listDevices")==0||strcasecmp(argv[argi]+1,"ld")==0)
				listDevices=true;
			else if(strcasecmp(argv[argi]+1,"productVendorId")==0||strcasecmp(argv[argi]+1,"pv")==0)
				{
				if(argi+2<argc)
					{
					deviceMatcher.setVendorId((unsigned short)(strtoul(argv[argi+1],0,16)));
					deviceMatcher.setProductId((unsigned short)(strtoul(argv[argi+2],0,16)));
					}
				
				argi+=2;
				}
			else if(strcasecmp(argv[argi]+1,"deviceName")==0||strcasecmp(argv[argi]+1,"dn")==0)
				{
				if(argi+1<argc)
					deviceMatcher.setDeviceName(argv[argi+1]);
				
				++argi;
				}
			else if(strcasecmp(argv[argi]+1,"index")==0||strcasecmp(argv[argi]+1,"i")==0)
				{
				if(argi+1<argc)
					deviceMatcher.setIndex((unsigned int)(strtoul(argv[argi+1],0,10)));
				
				++argi;
				}
			else if(strcasecmp(argv[argi]+1,"gridGap")==0||strcasecmp(argv[argi]+1,"gg")==0)
				{
				if(argi+1<argc)
					{
					gridGap=strtod(argv[argi+1],0);
					}
				
				argi+=1;
				}
			else if(strcasecmp(argv[argi]+1,"numPoints")==0||strcasecmp(argv[argi]+1,"np")==0)
				{
				if(argi+2<argc)
					{
					for(int i=0;i<2;++i)
						numPoints[i]=(unsigned int)(strtoul(argv[argi+1+i],0,10));
					}
				
				argi+=2;
				}
			else if(strcasecmp(argv[argi]+1,"rectilinear")==0||strcasecmp(argv[argi]+1,"cr")==0)
				calibrationType=0;
			else if(strcasecmp(argv[argi]+1,"affine")==0||strcasecmp(argv[argi]+1,"ca")==0)
				calibrationType=1;
			else if(strcasecmp(argv[argi]+1,"projective")==0||strcasecmp(argv[argi]+1,"cp")==0)
				calibrationType=2;
			else if(strcasecmp(argv[argi]+1,"bspline")==0||strcasecmp(argv[argi]+1,"cb")==0)
				{
				calibrationType=3;
				
				if(argi+4<argc)
					{
					for(int i=0;i<2;++i)
						splineDegree[i]=(unsigned int)(strtoul(argv[argi+1+i],0,10));
					for(int i=0;i<2;++i)
						splineSize[i]=(unsigned int)(strtoul(argv[argi+3+i],0,10));
					}
				
				argi+=4;
				}
			}
		}
	
	if(listDevices)
		{
		/* List all connected pen devices and exit: */
		listPenDevices();
		Vrui::shutdown();
		return;
		}
	
	/* Open the pen device: */
	penDevice=new RawHID::EventDevice(deviceMatcher);
	
	/* Retrieve the pen device's configuration: */
	penDeviceConfig=RawHID::PenDeviceConfig(*penDevice);
	if(!penDeviceConfig.valid)
		{
		delete penDevice;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Requested device is not a pen device");
		}
	
	/* Print information about the pen device: */
	std::cout<<"Calibrating pen device ";
	std::cout<<std::hex<<std::setfill('0')<<std::setw(4)<<penDevice->getVendorId()<<':'<<std::setw(4)<<penDevice->getProductId()<<std::dec<<std::setfill(' ');
	std::cout<<", version "<<penDevice->getVersion();
	std::cout<<", "<<penDevice->getDeviceName()<<" (serial no. "<<penDevice->getSerialNumber()<<')'<<std::endl;
	std::cout<<"Pen device provides "<<penDevice->getNumKeyFeatures()<<" buttons and "<<penDevice->getNumAbsAxisFeatures()<<" absolute axes"<<std::endl;
	
	/* Try grabbing the pen device: */
	if(!penDevice->grabDevice())
		std::cout<<"Unable to grab the pen device!"<<std::endl;
	
	/* Check the device's capabilities: */
	const RawHID::EventDevice::AbsAxisConfig& configX=penDevice->getAbsAxisFeatureConfig(penDeviceConfig.posAxisIndices[0]);
	const RawHID::EventDevice::AbsAxisConfig& configY=penDevice->getAbsAxisFeatureConfig(penDeviceConfig.posAxisIndices[1]);
	std::cout<<"Pen device position axis ranges: ["<<configX.min<<", "<<configX.max<<"], ["<<configY.min<<", "<<configY.max<<"]"<<std::endl;
	posDomain.min[0]=Scalar(configX.min);
	posDomain.max[0]=Scalar(configX.max);
	posDomain.min[1]=Scalar(configY.min);
	posDomain.max[1]=Scalar(configY.max);
	if(penDeviceConfig.haveTilt)
		{
		const RawHID::EventDevice::AbsAxisConfig& configX=penDevice->getAbsAxisFeatureConfig(penDeviceConfig.tiltAxisIndices[0]);
		const RawHID::EventDevice::AbsAxisConfig& configY=penDevice->getAbsAxisFeatureConfig(penDeviceConfig.tiltAxisIndices[1]);
		std::cout<<"Pen device tilt axis ranges: ["<<configX.min<<", "<<configX.max<<"], ["<<configY.min<<", "<<configY.max<<"]"<<std::endl;
		}
	
	/* Install event callbacks with the pen device: */
	penDevice->getKeyFeatureEventCallbacks().add(this,&CalibrateTouchscreen::keyCallback);
	
	if(penDevice->hasSynReport())
		penDevice->getSynReportEventCallbacks().add(this,&CalibrateTouchscreen::synReportCallback);
	else
		penDevice->getAbsAxisFeatureEventCallbacks().add(this,&CalibrateTouchscreen::absAxisCallback);
	
	/* Register the pen device with the event dispatcher: */
	penDevice->registerEventHandler(dispatcher);
	
	/* Register tool classes: */
	addEventTool("Undo Point",0,0);
	
	Vrui::setBackgroundColor(Vrui::Color(1.0f,1.0f,1.0f));
	}

CalibrateTouchscreen::~CalibrateTouchscreen(void)
	{
	delete calibrator;
	
	/* Close the pen device: */
	delete penDevice;
	}

void CalibrateTouchscreen::prepareMainLoop(void)
	{
	/* Query the screen to be calibrated: */
	screen=Vrui::getMainScreen();
	for(int i=0;i<2;++i)
		screenSize[i]=screen->getScreenSize()[i];
	
	/* Start the calibration procedure: */
	currentPoint=0;
	}

void CalibrateTouchscreen::display(GLContextData& contextData) const
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glPointSize(3.0f);
	glLineWidth(1.0f);
	
	/* Go to screen space: */
	Vrui::goToPhysicalSpace(contextData);
	glMultMatrix(screen->getScreenTransformation());
	
	/* Draw the calibration grid: */
	unsigned int currentX=currentPoint%numPoints[0];
	unsigned int currentY=currentPoint/numPoints[0];
	if(currentY%2==1)
		currentX=numPoints[0]-1-currentX;
	glBegin(GL_LINES);
	for(unsigned int y=0;y<numPoints[1];++y)
		{
		if(y==currentY)
			{
			if(numAccum>0)
				glColor3f(0.0f,1.0f,0.0f);
			else if(hovering)
				glColor3f(0.0f,0.333f,0.0f);
			else
				glColor3f(0.0f,0.0f,0.0f);
			}
		else
			glColor3f(0.8f,0.8f,0.8f);
		Scalar sy=(Scalar(y)+gridGap)/(Scalar(numPoints[1]-1)+Scalar(2)*gridGap)*screenSize[1];
		glVertex2d(Scalar(0),sy);
		glVertex2d(screenSize[0],sy);
		}
	for(unsigned int x=0;x<numPoints[0];++x)
		{
		if(x==currentX)
			{
			if(numAccum>0)
				glColor3f(0.0f,1.0f,0.0f);
			else if(hovering)
				glColor3f(0.0f,0.333f,0.0f);
			else
				glColor3f(0.0f,0.0f,0.0f);
			}
		else
			glColor3f(0.8f,0.8f,0.8f);
		Scalar sx=(Scalar(x)+gridGap)/(Scalar(numPoints[0]-1)+Scalar(2)*gridGap)*screenSize[0];
		glVertex2d(sx,Scalar(0));
		glVertex2d(sx,screenSize[1]);
		}
	glEnd();
	
	if(calibrator!=0)
		{
		/* Draw all calibrated measurement points: */
		glBegin(GL_POINTS);
		glColor3f(0.0f,0.5f,0.0f);
		for(TiePointList::const_iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
			{
			Point2 cal=calibrator->calibrate(tpIt->raw);
			glVertex2d(cal[0]*screenSize[0],cal[1]*screenSize[1]);
			}
		glEnd();
		
		if(hovering)
			{
			/* Indicate the current pen position: */
			glBegin(GL_LINES);
			glColor3f(0.0f,0.0f,0.0f);
			glVertex2d(penPos[0],Scalar(0));
			glVertex2d(penPos[0],screenSize[1]);
			glVertex2d(Scalar(0),penPos[1]);
			glVertex2d(screenSize[0],penPos[1]);
			glEnd();
			}
		}
	
	/* Return to original space: */
	glPopMatrix();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void CalibrateTouchscreen::eventCallback(Vrui::Application::EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		switch(eventId)
			{
			case 0: // Undo last calibration point
				if(currentPoint>0)
					{
					tiePoints.pop_back();
					--currentPoint;
					}
				
				break;
			}
		}
	}

VRUI_APPLICATION_RUN(CalibrateTouchscreen)

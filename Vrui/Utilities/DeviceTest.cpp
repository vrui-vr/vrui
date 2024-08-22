/***********************************************************************
DeviceTest - Program to test the connection to a Vrui VR Device Daemon
and to dump device positions/orientations and button states.
Copyright (c) 2002-2024 Oliver Kreylos

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
#include <unistd.h>
#include <termios.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/Timer.h>
#include <Misc/FunctionCalls.h>
#include <Misc/OutputOperators.h>
#include <Misc/Marshaller.h>
#include <Geometry/GeometryMarshallers.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/EventDispatcher.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Realtime/Time.h>
#include <Geometry/AffineCombiner.h>
#include <Geometry/OutputOperators.h>
#include <Vrui/Types.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/BatteryState.h>
#include <Vrui/Internal/HMDConfiguration.h>
#include <Vrui/Internal/VRBaseStation.h>
#include <Vrui/Internal/VRDeviceClient.h>

void printDeviceConfiguration(const Vrui::VRDeviceDescriptor& vd)
	{
	/* Print detailed information about the given virtual input device: */
	std::cout<<"Virtual device "<<vd.name<<":"<<std::endl;
	std::cout<<"  Track type: ";
	if(vd.trackType&Vrui::VRDeviceDescriptor::TRACK_ORIENT)
		std::cout<<"6-DOF";
	else if(vd.trackType&Vrui::VRDeviceDescriptor::TRACK_DIR)
		std::cout<<"Ray-based";
	else if(vd.trackType&Vrui::VRDeviceDescriptor::TRACK_POS)
		std::cout<<"3-DOF";
	else
		std::cout<<"None";
	std::cout<<std::endl;
	
	if(vd.trackType&Vrui::VRDeviceDescriptor::TRACK_DIR)
		std::cout<<"  Device ray direction: "<<vd.rayDirection<<", start: "<<vd.rayStart<<std::endl;
	
	std::cout<<"  Device is "<<(vd.hasBattery?"battery-powered":"connected to power source")<<std::endl;
	
	std::cout<<"  Device can ";
	if(vd.canPowerOff)
		std::cout<<"not ";
	std::cout<<"be powered off on request"<<std::endl;
	
	if(vd.trackType&Vrui::VRDeviceDescriptor::TRACK_POS)
		std::cout<<"  Tracker index: "<<vd.trackerIndex<<std::endl;
	
	if(vd.numButtons>0)
		{
		std::cout<<"  "<<vd.numButtons<<" buttons:";
		for(int i=0;i<vd.numButtons;++i)
			std::cout<<" ("<<vd.buttonNames[i]<<", "<<vd.buttonIndices[i]<<")";
		std::cout<<std::endl;
		}
	
	if(vd.numValuators>0)
		{
		std::cout<<"  "<<vd.numValuators<<" valuators:";
		for(int i=0;i<vd.numValuators;++i)
			std::cout<<" ("<<vd.valuatorNames[i]<<", "<<vd.valuatorIndices[i]<<")";
		std::cout<<std::endl;
		}
	
	if(vd.numHapticFeatures>0)
		{
		std::cout<<"  "<<vd.numHapticFeatures<<" haptic features:";
		for(int i=0;i<vd.numHapticFeatures;++i)
			std::cout<<" ("<<vd.hapticFeatureNames[i]<<", "<<vd.hapticFeatureIndices[i]<<")";
		std::cout<<std::endl;
		}
	
	std::cout<<"  Handle transformation: "<<vd.handleTransform<<std::endl;
	}

/***********************************************************************
Helper classes and functions to analyze an HMD's lens distortion
correction function and estimate visible display resolution.
***********************************************************************/

typedef Vrui::HMDConfiguration::Point2 Point2;

const Point2& getMvColor(int eye,int x,int y,int color,const Vrui::HMDConfiguration& hc)
	{
	/* Get the mesh vertex: */
	const Vrui::HMDConfiguration::DistortionMeshVertex& mv=hc.getDistortionMesh(eye)[y*hc.getDistortionMeshSize()[0]+x];
	
	/* Return the requested component of the source position for the requested color: */
	switch(color)
		{
		case 0: // Red
			return mv.red;
		
		case 1: // Green
			return mv.green;
		
		case 2: // Blue
			return mv.blue;
		
		default:
			return mv.green;
		}
	}

Point2 calcIntermediateImagePos(const Point2& viewportPos,int eye,int color,const Vrui::HMDConfiguration& hc)
	{
	/* Find the distortion mesh cell containing the given point, and the point's cell-relative position: */
	int cell[2];
	Vrui::Scalar cp[2];
	for(int i=0;i<2;++i)
		{
		int ms=hc.getDistortionMeshSize()[i];
		cp[i]=(viewportPos[i]-Vrui::Scalar(hc.getViewport(eye).offset[i]))*Vrui::Scalar(ms-1)/Vrui::Scalar(hc.getViewport(eye).size[i]);
		cell[i]=int(Math::floor(cp[i]));
		if(cell[i]>ms-2)
			cell[i]=ms-2;
		cp[i]-=Vrui::Scalar(cell[i]);
		}
	
	/* Interpolate the distortion-corrected position of the given color component in render framebuffer texture coordinate space: */
	Point2 p0=Geometry::affineCombination(getMvColor(eye,cell[0],cell[1],color,hc),getMvColor(eye,cell[0]+1,cell[1],color,hc),cp[0]);
	Point2 p1=Geometry::affineCombination(getMvColor(eye,cell[0],cell[1]+1,color,hc),getMvColor(eye,cell[0]+1,cell[1]+1,color,hc),cp[0]);
	Point2 result=Geometry::affineCombination(p0,p1,cp[1]);
	
	/* Convert the result to intermediate image pixel space: */
	result[0]=result[0]*hc.getRenderTargetSize()[0];
	result[1]=result[1]*hc.getRenderTargetSize()[1];
	
	return result;
	}

Point2 calcTanSpacePos(const Point2& viewportPos,int eye,int color,const Vrui::HMDConfiguration& hc)
	{
	/* Find the distortion mesh cell containing the given point, and the point's cell-relative position: */
	int cell[2];
	Vrui::Scalar cp[2];
	for(int i=0;i<2;++i)
		{
		int ms=hc.getDistortionMeshSize()[i];
		cp[i]=(viewportPos[i]-Vrui::Scalar(hc.getViewport(eye).offset[i]))*Vrui::Scalar(ms-1)/Vrui::Scalar(hc.getViewport(eye).size[i]);
		cell[i]=int(Math::floor(cp[i]));
		if(cell[i]>ms-2)
			cell[i]=ms-2;
		cp[i]-=Vrui::Scalar(cell[i]);
		}
	
	/* Interpolate the distortion-corrected position of the given color component in render framebuffer texture coordinate space: */
	Point2 p0=Geometry::affineCombination(getMvColor(eye,cell[0],cell[1],color,hc),getMvColor(eye,cell[0]+1,cell[1],color,hc),cp[0]);
	Point2 p1=Geometry::affineCombination(getMvColor(eye,cell[0],cell[1]+1,color,hc),getMvColor(eye,cell[0]+1,cell[1]+1,color,hc),cp[0]);
	Point2 result=Geometry::affineCombination(p0,p1,cp[1]);
	
	/* Convert the result to tangent space: */
	result[0]=result[0]*(hc.getFov(eye)[1]-hc.getFov(eye)[0])+hc.getFov(eye)[0];
	result[1]=result[1]*(hc.getFov(eye)[3]-hc.getFov(eye)[2])+hc.getFov(eye)[2];
	
	return result;
	}

Point2 calcLensCenter(int eye,int color,const Vrui::HMDConfiguration& hc)
	{
	/* Initialize lens center to the position it would have without lens distortion correction: */
	Point2 lensCenter;
	for(int i=0;i<2;++i)
		lensCenter[i]=Vrui::Scalar(hc.getViewport(eye).offset[i])+Vrui::Scalar(hc.getViewport(eye).size[i])*(Vrui::Scalar(0)-hc.getFov(eye)[2*i])/(hc.getFov(eye)[2*i+1]-hc.getFov(eye)[2*i]);
	
	/* Run Newton-Raphson iteration to converge towards the distortion-corrected lens center: */
	for(int iteration=0;iteration<20;++iteration) // 20 should be more than sufficient
		{
		/* Calculate corrected tangent-space position of current estimate and bail out if the estimate is good enough: */
		Point2 lcTan=calcTanSpacePos(lensCenter,eye,color,hc);
		if(Geometry::sqr(lcTan)<Math::sqr(1.0e-6))
			break;
		
		/* Estimate the differential of the distortion correction function at the current lens center estimate: */
		Vrui::Scalar delta=1.0e-3;
		Point2 dxp=calcTanSpacePos(lensCenter+Point2::Vector(delta,0),eye,color,hc);
		Point2 dxn=calcTanSpacePos(lensCenter-Point2::Vector(delta,0),eye,color,hc);
		Point2::Vector dx=(dxp-dxn)/(delta*Vrui::Scalar(2));
		Point2 dyp=calcTanSpacePos(lensCenter+Point2::Vector(0,delta),eye,color,hc);
		Point2 dyn=calcTanSpacePos(lensCenter-Point2::Vector(0,delta),eye,color,hc);
		Point2::Vector dy=(dyp-dyn)/(delta*Vrui::Scalar(2));
		
		/* Calculate a Newton-Raphson step: */
		Vrui::Scalar det=(dx[0]*dy[1]-dx[1]*dy[0]);
		Point2::Vector step((dy[1]*lcTan[0]-dy[0]*lcTan[1])/det,(dx[0]*lcTan[1]-dx[1]*lcTan[0])/det);
		
		/* Adjust the lens center estimate: */
		lensCenter-=step;
		}
	
	/* Return the final lens center estimate: */
	return lensCenter;
	}

void printHmdConfiguration(const Vrui::HMDConfiguration& hc)
	{
	/* Print basic information directly extracted from the given configuration object: */
	std::cout<<"  Tracker index: "<<hc.getTrackerIndex()<<std::endl;
	std::cout<<"  Face detector button index: "<<hc.getFaceDetectorButtonIndex()<<std::endl;
	std::cout<<"  Display latency: "<<hc.getDisplayLatency()<<"ns"<<std::endl;
	std::cout<<"  Recommended per-eye render target size: "<<hc.getRenderTargetSize()<<std::endl;
	std::cout<<"  Per-eye distortion mesh size: "<<hc.getDistortionMeshSize()<<std::endl;
	for(int eye=0;eye<2;++eye)
		{
		if(eye==0)
			std::cout<<"  Left-eye parameters:"<<std::endl;
		else
			std::cout<<"  Right-eye parameters:"<<std::endl;
		
		std::cout<<"    3D eye position : "<<hc.getEyePosition(eye)<<std::endl;
		std::cout<<"    3D eye rotation : "<<hc.getEyeRotation(eye)<<std::endl;
		std::cout<<"    Field-of-view   : "<<hc.getFov(eye)[0]<<", "<<hc.getFov(eye)[1]<<", "<<hc.getFov(eye)[2]<<", "<<hc.getFov(eye)[3]<<std::endl;
		std::cout<<"    Display viewport: "<<hc.getViewport(eye)<<std::endl;
		
		/* Calculate position of lens center in viewport coordinates via bisection, because the value obtained from looking at left and right FoV may be shifted by lens distortion correction: */
		Point2 lensCenter=calcLensCenter(eye,1,hc);
		std::cout<<"    Lens center     : "<<lensCenter<<std::endl;
		
		#if 0
		/* Generate a horizontal cross section of resolution in pixels/degree: */
		Vrui::Scalar delta=0.25;
		for(unsigned int x=0;x<hc.getViewport(eye).size[0];++x)
			{
			/* Get a display pixel: */
			Point2 disp(Vrui::Scalar(x+hc.getViewport(eye).offset[0])+Vrui::Scalar(0.5),lensCenter[1]);
			
			/* Calculate resolution at pixel for the three color channels: */
			std::cout<<x;
			for(int color=0;color<3;++color)
				{
				/* Transform the pixel and a displaced version to tangent space: */
				Point2 tan=calcTanSpacePos(disp,eye,color,hc);
				Point2 tan0=calcTanSpacePos(disp-Point2::Vector(delta,0),eye,color,hc);
				Vrui::Scalar alpha0=Math::atan(tan0[0]);
				Point2 tan1=calcTanSpacePos(disp+Point2::Vector(delta,0),eye,color,hc);
				Vrui::Scalar alpha1=Math::atan(tan1[0]);
				Vrui::Scalar res=(delta*Vrui::Scalar(2))/Math::deg(alpha1-alpha0);
				std::cout<<','<<Math::deg(Math::atan(tan[0]))<<','<<res;
				}
			std::cout<<std::endl;
			}
		#elif 0
		/* Generate a horizontal cross section of intermediate image/display pixel mapping ratio: */
		Scalar delta=0.25;
		for(unsigned int x=0;x<hc.getViewport(eye)[2];++x)
			{
			/* Get a display pixel: */
			Point2 disp(Scalar(x+hc.getViewport(eye)[0])+Scalar(0.5),lensCenter[1]);
			
			/* Calculate resolution at pixel for the three color channels: */
			std::cout<<x;
			for(int color=0;color<3;++color)
				{
				/* Transform the pixel and a displaced version to tangent space: */
				Point2 p=calcIntermediateImagePos(disp,eye,color,hc);
				Point2 p0=calcIntermediateImagePos(disp-Point2::Vector(delta,0),eye,color,hc);
				Point2 p1=calcIntermediateImagePos(disp+Point2::Vector(delta,0),eye,color,hc);
				Scalar factor=(p1[0]-p0[0])/(delta*Scalar(2));
				std::cout<<','<<p[0]<<','<<factor;
				}
			std::cout<<std::endl;
			}
		#elif 0
		/* Generate a vertical cross section of resolution in pixels/degree: */
		Scalar delta=0.25;
		for(unsigned int y=0;y<hc.getViewport(eye)[3];++y)
			{
			/* Get a display pixel: */
			Point2 disp(lensCenter[0],Scalar(y+hc.getViewport(eye)[1])+Scalar(0.5));
			
			/* Calculate resolution at pixel for the three color channels: */
			std::cout<<y;
			for(int color=0;color<3;++color)
				{
				/* Transform the pixel and a displaced version to tangent space: */
				Point2 tan=calcTanSpacePos(disp,eye,color,hc);
				Point2 tan0=calcTanSpacePos(disp-Point2::Vector(0,delta),eye,color,hc);
				Scalar alpha0=Math::atan(tan0[1]);
				Point2 tan1=calcTanSpacePos(disp+Point2::Vector(0,delta),eye,color,hc);
				Scalar alpha1=Math::atan(tan1[1]);
				Scalar res=(delta*Scalar(2))/Math::deg(alpha1-alpha0);
				std::cout<<','<<Math::deg(Math::atan(tan[1]))<<','<<res;
				}
			std::cout<<std::endl;
			}
		#elif 0
		/* Generate a vertical cross section of intermediate image/display pixel mapping ratio: */
		Scalar delta=0.25;
		for(unsigned int y=0;y<hc.getViewport(eye)[3];++y)
			{
			/* Get a display pixel: */
			Point2 disp(lensCenter[0],Scalar(y+hc.getViewport(eye)[1])+Scalar(0.5));
			
			/* Calculate resolution at pixel for the three color channels: */
			std::cout<<y;
			for(int color=0;color<3;++color)
				{
				/* Transform the pixel and a displaced version to tangent space: */
				Point2 p=calcIntermediateImagePos(disp,eye,color,hc);
				Point2 p0=calcIntermediateImagePos(disp-Point2::Vector(0,delta),eye,color,hc);
				Point2 p1=calcIntermediateImagePos(disp+Point2::Vector(0,delta),eye,color,hc);
				Scalar factor=(p1[1]-p0[1])/(delta*Scalar(2));
				std::cout<<','<<p[1]<<','<<factor;
				}
			std::cout<<std::endl;
			}
		#elif 0
		/* Dump entire distortion mesh to file: */
		IO::FilePtr meshFile=IO::openFile("/home/okreylos/Desktop/Stuff/ViveResolution/DistortionMesh.dat",IO::File::WriteOnly);
		meshFile->setEndianness(Misc::LittleEndian);
		meshFile->write<Vrui::HMDConfiguration::UInt>(hc.getViewport(eye),4);
		meshFile->write<Scalar>(hc.getFov(eye),4);
		meshFile->write<Vrui::HMDConfiguration::UInt>(hc.getDistortionMeshSize(),2);
		const Vrui::HMDConfiguration::DistortionMeshVertex* mvPtr=hc.getDistortionMesh(eye);
		for(unsigned int y=0;y<hc.getDistortionMeshSize()[1];++y)
			for(unsigned int x=0;x<hc.getDistortionMeshSize()[0];++x,++mvPtr)
				{
				meshFile->write<Scalar>(mvPtr->red.getComponents(),2);
				meshFile->write<Scalar>(mvPtr->green.getComponents(),2);
				meshFile->write<Scalar>(mvPtr->blue.getComponents(),2);
				}
		#endif
		}
	}

void printBaseStation(const Vrui::VRBaseStation& bs)
	{
	std::cout<<"Serial number: "<<bs.getSerialNumber()<<std::endl;
	std::cout<<"\tField of view : ";
	std::cout<<"horizontal "<<Math::deg(Math::atan(bs.getFov()[0]))<<", "<<Math::deg(Math::atan(bs.getFov()[1]));
	std::cout<<", vertical "<<Math::deg(Math::atan(bs.getFov()[2]))<<", "<<Math::deg(Math::atan(bs.getFov()[3]))<<std::endl;
	std::cout<<"\tTracking range: "<<bs.getRange()[0]<<", "<<bs.getRange()[1]<<std::endl;
	if(bs.getTracking())
		std::cout<<"\tPose          : "<<bs.getPositionOrientation()<<std::endl;
	else
		std::cout<<"\tInactive"<<std::endl;
	}

void printEnvironmentDefinition(const Vrui::EnvironmentDefinition& ed)
	{
	std::cout<<"Coordinate unit  : "<<ed.unit.getFactor()<<' '<<ed.unit.getName()<<std::endl;
	std::cout<<"Up direction     : "<<ed.up<<std::endl;
	std::cout<<"Forward direction: "<<ed.forward<<std::endl;
	std::cout<<"Center point     : "<<ed.center<<std::endl;
	std::cout<<"Radius           : "<<ed.radius<<std::endl;
	std::cout<<"Floor plane      : "<<ed.floor<<std::endl;
	std::cout<<"Boundary polygons ("<<ed.boundary.size()<<"):"<<std::endl;
	for(Vrui::EnvironmentDefinition::PolygonList::const_iterator bIt=ed.boundary.begin();bIt!=ed.boundary.end();++bIt)
		{
		Vrui::EnvironmentDefinition::Polygon::const_iterator pIt=bIt->begin();
		std::cout<<"\t("<<*pIt;
		for(++pIt;pIt!=bIt->end();++pIt)
			std::cout<<", "<<*pIt;
		std::cout<<')'<<std::endl;
		}
	}

typedef Vrui::VRDeviceState::TrackerState TrackerState;
typedef TrackerState::PositionOrientation PositionOrientation;
typedef PositionOrientation::Scalar Scalar;
typedef PositionOrientation::Point Point;
typedef PositionOrientation::Vector Vector;
typedef PositionOrientation::Rotation Rotation;

class LatencyHistogram // Helper class to collect and print tracker data latency histograms
	{
	/* Elements: */
	private:
	unsigned int binSize; // Size of a histogram bin in microseconds
	unsigned int maxBinLatency; // Maximum latency to expect in microseconds
	unsigned int numBins; // Number of bins in the histogram
	unsigned int* bins; // Array of histogram bins
	unsigned int numSamples; // Number of samples in current observation period
	double latencySum; // Sum of all latencies to calculate average latency
	unsigned int minLatency,maxLatency; // Latency range in current observation period in microseconds
	unsigned int maxBinSize; // Maximum number of samples in any bin
	
	/* Constructors and destructors: */
	public:
	LatencyHistogram(unsigned int sBinSize,unsigned int sMaxBinLatency)
		:binSize(sBinSize),maxBinLatency(sMaxBinLatency),
		 numBins(maxBinLatency/binSize+2),bins(new unsigned int[numBins])
		{
		/* Initialize the histogram: */
		reset();
		}
	~LatencyHistogram(void)
		{
		delete[] bins;
		}
	
	/* Methods: */
	void reset(void) // Resets the histogram for the next observation period
		{
		/* Clear the histogram: */
		for(unsigned int i=0;i<numBins;++i)
			bins[i]=0;
		
		/* Reset the latency counter and range: */
		numSamples=0;
		latencySum=0.0;
		minLatency=~0x0U;
		maxLatency=0U;
		maxBinSize=0U;
		}
	void addSample(unsigned int latency) // Adds a latency sample
		{
		/* Update the histogram: */
		unsigned int binIndex=latency/binSize;
		if(binIndex>numBins-1)
			binIndex=numBins-1; // All outliers go into the last bin
		++bins[binIndex];
		if(maxBinSize<bins[binIndex])
			maxBinSize=bins[binIndex];
		
		/* Update sample counter and range: */
		++numSamples;
		latencySum+=double(latency);
		if(minLatency>latency)
			minLatency=latency;
		if(maxLatency<latency)
			maxLatency=latency;
		}
	unsigned int getNumSamples(void) const
		{
		return numSamples;
		}
	void printHistogram(void) const // Prints the histogram
		{
		/* Calculate the range of non-empty bins: */
		unsigned int firstBinIndex=minLatency/binSize;
		if(firstBinIndex>numBins-1)
			firstBinIndex=numBins-1;
		unsigned int lastBinIndex=maxLatency/binSize;
		if(lastBinIndex>numBins-1)
			lastBinIndex=numBins-1;
		
		std::cout<<"Histogram of "<<numSamples<<" latency samples:"<<std::endl;
		for(unsigned int i=firstBinIndex;i<=lastBinIndex;++i)
			{
			if(i<numBins-1)
				std::cout<<std::setw(8)<<i*binSize<<' ';
			else
				std::cout<<"Outliers ";
			unsigned int width=(bins[i]*71+maxBinSize-1)/maxBinSize;
			for(unsigned int j=0;j<width;++j)
				std::cout<<'*';
			std::cout<<std::endl;
			}
		
		std::cout<<"Average latency: "<<latencySum/double(numSamples)<<" us"<<std::endl;
		}
	};

class TrackerPrinter // Helper class to print tracker data
	{
	/* Elements: */
	private:
	Vrui::VRDeviceClient* deviceClient; // Pointer to the device client
	int trackerIndex; // Index of tracker whose state to print, or -1 to print all tracker states
	int printMode; // Tracker printing mode
	bool printButtonStates; // Flag whether to print button states
	bool printNewlines; // Flag to print each tracker state update on a new line
	int vdIndex; // Index of the virtual device to which the printed tracker belongs
	bool hasBattery; // Flag whether the printed tracker is battery-powered
	LatencyHistogram latencyHistogram; // Histogram of tracking data latency
	
	/* Private methods: */
	static void printTrackerPos(const Vrui::VRDeviceState& state,int trackerIndex)
		{
		if(state.getTrackerValid(trackerIndex))
			{
			const TrackerState& ts=state.getTrackerState(trackerIndex);
			Point pos=ts.positionOrientation.getOrigin();
			std::cout.setf(std::ios::fixed);
			std::cout.precision(3);
			std::cout<<"("<<std::setw(9)<<pos[0]<<" "<<std::setw(9)<<pos[1]<<" "<<std::setw(9)<<pos[2]<<")";
			}
		else
			{
			std::cout<<"(-----.--- -----.--- -----.---)";
			}
		}
	static void printTrackerPosQuat(const Vrui::VRDeviceState& state,int trackerIndex)
		{
		if(state.getTrackerValid(trackerIndex))
			{
			const TrackerState& ts=state.getTrackerState(trackerIndex);
			Point pos=ts.positionOrientation.getOrigin();
			Rotation rot=ts.positionOrientation.getRotation();
			const Scalar* quat=rot.getQuaternion();
			std::cout.setf(std::ios::fixed);
			std::cout.precision(3);
			std::cout<<"("<<std::setw(8)<<pos[0]<<" "<<std::setw(8)<<pos[1]<<" "<<std::setw(8)<<pos[2]<<") ";
			std::cout.precision(4);
			std::cout<<"("<<std::setw(7)<<quat[0]<<" "<<std::setw(7)<<quat[1]<<" "<<std::setw(7)<<quat[2]<<" "<<std::setw(7)<<quat[3]<<")";
			}
		else
			{
			std::cout<<"(----.--- ----.--- ----.---) (--.---- --.---- --.---- --.----)";
			}
		}
	static void printTrackerPosOrient(const Vrui::VRDeviceState& state,int trackerIndex)
		{
		if(state.getTrackerValid(trackerIndex))
			{
			const TrackerState& ts=state.getTrackerState(trackerIndex);
			Point pos=ts.positionOrientation.getOrigin();
			Rotation rot=ts.positionOrientation.getRotation();
			Vector axis=rot.getScaledAxis();
			Scalar angle=Math::deg(rot.getAngle());
			std::cout.setf(std::ios::fixed);
			std::cout.precision(3);
			std::cout<<"("<<std::setw(8)<<pos[0]<<" "<<std::setw(8)<<pos[1]<<" "<<std::setw(8)<<pos[2]<<") ";
			std::cout<<"("<<std::setw(8)<<axis[0]<<" "<<std::setw(8)<<axis[1]<<" "<<std::setw(8)<<axis[2]<<") ";
			std::cout<<std::setw(8)<<angle;
			}
		else
			{
			std::cout<<"(----.--- ----.--- ----.---) (----.--- ----.--- ----.---) ----.---";
			}
		}
	static void printTrackerFrame(const Vrui::VRDeviceState& state,int trackerIndex)
		{
		if(state.getTrackerValid(trackerIndex))
			{
			const TrackerState& ts=state.getTrackerState(trackerIndex);
			Point pos=ts.positionOrientation.getOrigin();
			Rotation rot=ts.positionOrientation.getRotation();
			Vector x=rot.getDirection(0);
			Vector y=rot.getDirection(1);
			Vector z=rot.getDirection(2);
			std::cout.setf(std::ios::fixed);
			std::cout.precision(3);
			std::cout<<"("<<std::setw(8)<<pos[0]<<" "<<std::setw(8)<<pos[1]<<" "<<std::setw(8)<<pos[2]<<") ";
			std::cout<<"("<<std::setw(6)<<x[0]<<" "<<std::setw(6)<<x[1]<<" "<<std::setw(6)<<x[2]<<") ";
			std::cout<<"("<<std::setw(6)<<y[0]<<" "<<std::setw(6)<<y[1]<<" "<<std::setw(6)<<y[2]<<") ";
			std::cout<<"("<<std::setw(6)<<z[0]<<" "<<std::setw(6)<<z[1]<<" "<<std::setw(6)<<z[2]<<")";
			}
		else
			{
			std::cout<<"(----.--- ----.--- ----.---) ";
			std::cout<<"(--.--- --.--- --.---) ";
			std::cout<<"(--.--- --.--- --.---) ";
			std::cout<<"(--.--- --.--- --.---)";
			}
		}
	static void printButtons(const Vrui::VRDeviceState& state)
		{
		for(int i=0;i<state.getNumButtons();++i)
			{
			if(i>0)
				std::cout<<" ";
			if(state.getButtonState(i))
				std::cout<<"X";
			else
				std::cout<<'.';
			}
		}
	static void printValuators(const Vrui::VRDeviceState& state)
		{
		std::cout.setf(std::ios::fixed);
		std::cout.precision(3);
		for(int i=0;i<state.getNumValuators();++i)
			{
			if(i>0)
				std::cout<<" ";
			std::cout<<std::setw(6)<<state.getValuatorState(i);
			}
		}
	
	/* Constructors and destructors: */
	public:
	TrackerPrinter(Vrui::VRDeviceClient* sDeviceClient,int sTrackerIndex,int sPrintMode,bool sPrintButtonStates,bool sPrintNewlines)
		:deviceClient(sDeviceClient),
		 trackerIndex(sTrackerIndex),
		 printMode(sPrintMode),printButtonStates(sPrintButtonStates),
		 printNewlines(sPrintNewlines),
		 vdIndex(-1),hasBattery(false),
		 latencyHistogram(10,2000)
		{
		/* Disable printing of tracking information if there are no trackers: */
		deviceClient->lockState();
		if(printMode>=0&&printMode<3&&deviceClient->getState().getNumTrackers()==0)
			printMode=-1;
		deviceClient->unlockState();
		
		/* Find the index of the virtual device to which the selected tracker belongs and check whether it's battery-powered: */
		for(int deviceIndex=0;deviceIndex<deviceClient->getNumVirtualDevices()&&vdIndex<0;++deviceIndex)
			{
			const Vrui::VRDeviceDescriptor& vd=deviceClient->getVirtualDevice(deviceIndex);
			if(vd.trackerIndex==trackerIndex)
				{
				vdIndex=deviceIndex;
				hasBattery=vd.hasBattery;
				}
			}
		
		/* Print output header line: */
		switch(printMode)
			{
			case 0:
				std::cout<<"     Pos X     Pos Y     Pos Z";
				break;
			
			case 1:
				std::cout<<"    Pos X    Pos Y    Pos Z     Axis X   Axis Y   Axis Z     Angle";
				break;
			
			case 2:
				std::cout<<"    Pos X    Pos Y    Pos Z     XA X   XA Y   XA Z     YA X   YA Y   YA Z     ZA X   ZA Y   ZA Z";
				break;
			
			case 4:
				std::cout<<"    Pos X    Pos Y    Pos Z    Quat X  Quat Y  Quat Z  Quat W";
				break;
			}
		if(hasBattery)
			std::cout<<"  Battr.";
		std::cout<<std::endl;
		}
	
	/* Methods: */
	void updateDeviceStates(void)
		{
		deviceClient->updateDeviceStates();
		}
	void print(void)
		{
		/* Get the current timestamp to calculate data latency: */
		Vrui::TimePoint now;
		Vrui::VRDeviceState::TimeStamp nowTs=Vrui::VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+500)/1000);
		
		if(!printNewlines)
			std::cout<<"\r";
		
		/* Grab the current device state: */
		deviceClient->lockState();
		const Vrui::VRDeviceState& state=deviceClient->getState();
		
		/* Collect tracking data latency: */
		if(trackerIndex>=0&&state.getTrackerValid(trackerIndex))
			latencyHistogram.addSample(nowTs-state.getTrackerTimeStamp(trackerIndex));
		
		/* Print tracker data: */
		switch(printMode)
			{
			case 0:
				if(trackerIndex<0)
					{
					printTrackerPos(state,0);
					for(int i=1;i<state.getNumTrackers();++i)
						{
						std::cout<<" ";
						printTrackerPos(state,i);
						}
					}
				else
					printTrackerPos(state,trackerIndex);
				break;

			case 1:
				printTrackerPosOrient(state,trackerIndex);
				break;

			case 2:
				printTrackerFrame(state,trackerIndex);
				break;

			case 3:
				printValuators(state);
				break;

			case 4:
				printTrackerPosQuat(state,trackerIndex);
				break;

			default:
				; // Print nothing; nothing, I say!
			}
		
		/* Print tracker's battery state: */
		if(hasBattery)
			{
			deviceClient->lockBatteryStates();
			const Vrui::BatteryState& bs=deviceClient->getBatteryState(vdIndex);
			std::cout<<' '<<(bs.charging?"C ":"  ")<<std::setw(3)<<bs.batteryLevel<<'%';
			deviceClient->unlockBatteryStates();
			}
		
		/* Print button states: */
		if(printButtonStates)
			{
			std::cout<<" ";
			printButtons(state);
			}
		
		/* Release the device state: */
		deviceClient->unlockState();
		
		if(printNewlines)
			std::cout<<std::endl;
		else
			std::cout<<std::flush;
		}
	unsigned int getNumSamples(void) const
		{
		return latencyHistogram.getNumSamples();
		}
	void printLatency(void) const
		{
		latencyHistogram.printHistogram();
		}
	};

unsigned int numHmdConfigurations=0;
const Vrui::HMDConfiguration** hmdConfigurations=0;
unsigned int* eyePosVersions=0;
unsigned int* eyeRotVersions=0;
unsigned int* eyeVersions=0;
unsigned int* distortionMeshVersions=0;

void hmdConfigurationUpdatedCallback(const Vrui::HMDConfiguration& hmdConfiguration)
	{
	/* Find the updated HMD configuration in the list: */
	unsigned int index;
	for(index=0;index<numHmdConfigurations&&hmdConfigurations[index]!=&hmdConfiguration;++index)
		;
	if(index<numHmdConfigurations)
		{
		std::cout<<"Received configuration update for HMD "<<index<<std::endl;
		if(eyePosVersions[index]!=hmdConfiguration.getEyePosVersion())
			{
			std::cout<<"  Updated left eye position : "<<hmdConfiguration.getEyePosition(0)<<std::endl;
			std::cout<<"  Updated right eye position: "<<hmdConfiguration.getEyePosition(1)<<std::endl;
			
			eyePosVersions[index]=hmdConfiguration.getEyePosVersion();
			}
		if(eyeRotVersions[index]!=hmdConfiguration.getEyeRotVersion())
			{
			std::cout<<"  Updated left eye rotation : "<<hmdConfiguration.getEyeRotation(0)<<std::endl;
			std::cout<<"  Updated right eye rotation: "<<hmdConfiguration.getEyeRotation(1)<<std::endl;
			
			eyeRotVersions[index]=hmdConfiguration.getEyeRotVersion();
			}
		if(eyeVersions[index]!=hmdConfiguration.getEyeVersion())
			{
			std::cout<<"  Updated left eye field-of-view : "<<hmdConfiguration.getFov(0)[0]<<", "<<hmdConfiguration.getFov(0)[1]<<", "<<hmdConfiguration.getFov(0)[2]<<", "<<hmdConfiguration.getFov(0)[3]<<std::endl;
			std::cout<<"  Updated right eye field-of-view: "<<hmdConfiguration.getFov(1)[0]<<", "<<hmdConfiguration.getFov(1)[1]<<", "<<hmdConfiguration.getFov(1)[2]<<", "<<hmdConfiguration.getFov(1)[3]<<std::endl;
			
			eyeVersions[index]=hmdConfiguration.getEyeVersion();
			}
		if(distortionMeshVersions[index]!=hmdConfiguration.getDistortionMeshVersion())
			{
			std::cout<<"  Updated render target size: "<<hmdConfiguration.getRenderTargetSize()<<std::endl;
			std::cout<<"  Updated distortion mesh size: "<<hmdConfiguration.getDistortionMeshSize()<<std::endl;
			std::cout<<"  Updated left eye viewport : "<<hmdConfiguration.getViewport(0)<<std::endl;
			std::cout<<"  Updated right eye viewport: "<<hmdConfiguration.getViewport(1)<<std::endl;
			
			distortionMeshVersions[index]=hmdConfiguration.getDistortionMeshVersion();
			}
		}
	}

void environmentDefinitionUpdatedCallback(const Vrui::EnvironmentDefinition& newEnvironmentDefinition)
	{
	std::cout<<"Server updated environment definition"<<std::endl;
	}

Threads::EventDispatcher dispatcher;

void stdioCallback(Threads::EventDispatcher::IOEvent& event)
	{
	/* Read everything available on stdin: */
	char buffer[1024];
	ssize_t readResult=read(STDIN_FILENO,buffer,sizeof(buffer));
	if(readResult>=0)
		{
		/* Handle all read keypresses: */
		char* bufEnd=buffer+readResult;
		for(char* bufPtr=buffer;bufPtr!=bufEnd;++bufPtr)
			{
			switch(*bufPtr)
				{
				case '\r':
				case '\n':
				case 'Q':
				case 'q':
					/* Shut down the main loop: */
					dispatcher.stop();
					break;
				}
			}
		}
	}

void packetNotificationCallback(Vrui::VRDeviceClient* deviceClient,TrackerPrinter* trackerPrinter)
	{
	/* Print tracker data: */
	trackerPrinter->print();
	}

void updateDevicesCallback(Threads::EventDispatcher::TimerEvent& event)
	{
	/* Update the device client's device state: */
	TrackerPrinter* trackerPrinter=static_cast<TrackerPrinter*>(event.getUserData());
	trackerPrinter->updateDeviceStates();
	
	/* Print tracker data: */
	trackerPrinter->print();
	}

void help(const char* appName)
	{
	std::cout<<"Usage: "<<appName<<" [option]... [ ( -unix <server socket name> ) |  <server host name>[:<server port>] ]"<<std::endl;
	std::cout<<"Server specifiers:"<<std::endl;
	std::cout<<"  -unix <server socket name>"<<std::endl;
	std::cout<<"    Connects to VRDeviceDaemon over a UNIX domain socket of the given name"<<std::endl;
	std::cout<<"  <server host name>[:<server port>]"<<std::endl;
	std::cout<<"    Connects to VRDeviceDaemon over a TCP socket with the given host name and port number (default 8555)"<<std::endl;
	std::cout<<"  Default: connect to VRDeviceDaemon over TCP socket localhost:8555"<<std::endl;
	std::cout<<"Options:"<<std::endl;
	std::cout<<"  -listDevices | -ld"<<std::endl;
	std::cout<<"    Prints detailed information about all tracked devices and exits"<<std::endl;
	std::cout<<"  -listHMDs | -lh"<<std::endl;
	std::cout<<"    Prints detailed information about all head-mounted displays and exits"<<std::endl;
	std::cout<<"  -listBaseStations | -lb"<<std::endl;
	std::cout<<"    Prints detailed information about all tracking base stations and exits"<<std::endl;
	std::cout<<"  -printBatteryStates | -pbs"<<std::endl;
	std::cout<<"    Prints current battery states of all battery-powered devices and exits"<<std::endl;
	std::cout<<"  -haptic <haptic feature index> <duration> <frequency> <amplitude>"<<std::endl;
	std::cout<<"    Triggers a signal on the haptic feature of the given index, with the given duration in ms, frequency in Hz, and amplitude in [0,255], and exits"<<std::endl;
	std::cout<<"  -poweroff <power feature index>"<<std::endl;
	std::cout<<"    Turns off the power feature of the given index and exits"<<std::endl;
	std::cout<<"  -printEnvironmentDefinition | -ped"<<std::endl;
	std::cout<<"    Prints the definition of the current physical-space environment and exits"<<std::endl;
	std::cout<<"  -uploadEnvironmentDefinition | -ued <environment definition file name>"<<std::endl;
	std::cout<<"    Uploads a physical-space environment definition from the given file and exits"<<std::endl;
	std::cout<<"Tracking data printing options:"<<std::endl;
	std::cout<<"  -trackerIndex | -t <tracker index>"<<std::endl;
	std::cout<<"    Prints tracking data from the tracker of the given index"<<std::endl;
	std::cout<<"  -alltrackers"<<std::endl;
	std::cout<<"    Prints positions of all trackers"<<std::endl;
	std::cout<<"  -p"<<std::endl;
	std::cout<<"    Prints tracker positions"<<std::endl;
	std::cout<<"  -o"<<std::endl;
	std::cout<<"    Prints tracker positions and rotations as (axis, angle) pairs"<<std::endl;
	std::cout<<"  -q"<<std::endl;
	std::cout<<"    Prints tracker positions and rotations as unit quaternions"<<std::endl;
	std::cout<<"  -f"<<std::endl;
	std::cout<<"    Prints tracker coordinate frames"<<std::endl;
	std::cout<<"  -v"<<std::endl;
	std::cout<<"    Prints device valuator states instead of tracking data"<<std::endl;
	std::cout<<"  -b"<<std::endl;
	std::cout<<"    Prints device button states ('.' - not pressed, 'X' - pressed) in addition to tracking data"<<std::endl;
	std::cout<<"  -n"<<std::endl;
	std::cout<<"    Separates tracking data records with newlines"<<std::endl;
	}

int main(int argc,char* argv[])
	{
	/* Parse command line: */
	int pipeType=0; // Default to TCP pipe for now
	const char* serverNamePort="localhost:8555";
	const char* serverSocketName="VRDeviceDaemon.socket";
	bool printHelp=false;
	bool serverSocketAbstract=true;
	bool printDevices=false;
	bool printHmdConfigurations=false;
	bool printBaseStations=false;
	bool printBatteryStates=false;
	bool printEnvironment=false;
	bool uploadEnvironmentDefinition=false;
	const char* uploadEnvironmentDefinitionConfigurationFileName=0;
	int trackerIndex=0;
	int printMode=0;
	bool printButtonStates=false;
	bool printNewlines=false;
	bool savePositions=false;
	bool saveTrackerStates=false;
	const char* saveFileName=0;
	int triggerIndex=0;
	bool printLatency=false;
	int powerFeatureIndex=-1;
	int hapticFeatureIndex=-1;
	unsigned int hapticDuration=0;
	unsigned int hapticFrequency=100;
	unsigned int hapticAmplitude=255;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i],"-h")==0)
				printHelp=true;
			else if(strcasecmp(argv[i],"-listDevices")==0||strcasecmp(argv[i],"-ld")==0)
				printDevices=true;
			else if(strcasecmp(argv[i],"-listHMDs")==0||strcasecmp(argv[i],"-lh")==0)
				printHmdConfigurations=true;
			else if(strcasecmp(argv[i],"-listBaseStations")==0||strcasecmp(argv[i],"-lb")==0)
				printBaseStations=true;
			else if(strcasecmp(argv[i],"-printBatteryStates")==0||strcasecmp(argv[i],"-pbs")==0)
				printBatteryStates=true;
			else if(strcasecmp(argv[i],"-printEnvironmentDefinition")==0||strcasecmp(argv[i],"-ped")==0)
				printEnvironment=true;
			else if(strcasecmp(argv[i],"-uploadEnvironmentDefinition")==0||strcasecmp(argv[i],"-ued")==0)
				{
				uploadEnvironmentDefinition=true;
				++i;
				uploadEnvironmentDefinitionConfigurationFileName=argv[i];
				}
			else if(strcasecmp(argv[i],"-t")==0||strcasecmp(argv[i],"--trackerIndex")==0)
				{
				++i;
				trackerIndex=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i],"-alltrackers")==0)
				trackerIndex=-1;
			else if(strcasecmp(argv[i],"-p")==0)
				printMode=0;
			else if(strcasecmp(argv[i],"-o")==0)
				printMode=1;
			else if(strcasecmp(argv[i],"-f")==0)
				printMode=2;
			else if(strcasecmp(argv[i],"-v")==0)
				printMode=3;
			else if(strcasecmp(argv[i],"-q")==0)
				printMode=4;
			else if(strcasecmp(argv[i],"-b")==0)
				printButtonStates=true;
			else if(strcasecmp(argv[i],"-n")==0)
				printNewlines=true;
			else if(strcasecmp(argv[i],"-save")==0)
				{
				savePositions=true;
				++i;
				saveFileName=argv[i];
				}
			else if(strcasecmp(argv[i],"-saveTs")==0)
				{
				saveTrackerStates=true;
				++i;
				saveFileName=argv[i];
				}
			else if(strcasecmp(argv[i],"-trigger")==0)
				{
				++i;
				triggerIndex=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i],"-latency")==0)
				printLatency=true;
			else if(strcasecmp(argv[i],"-poweroff")==0)
				{
				++i;
				powerFeatureIndex=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i],"-haptic")==0)
				{
				++i;
				hapticFeatureIndex=atoi(argv[i]);
				++i;
				hapticDuration=atoi(argv[i]);
				++i;
				hapticFrequency=atoi(argv[i]);
				++i;
				hapticAmplitude=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i],"-unix")==0)
				{
				/* Connect to the VR device server over a UNIX domain socket: */
				pipeType=1;
				}
			}
		else if(pipeType==0)
			serverNamePort=argv[i];
		else
			serverSocketName=argv[i];
		}
	
	if(printHelp)
		{
		help(argv[0]);
		return 0;
		}
	
	Vrui::VRDeviceClient* deviceClient=0;
	try
		{
		if(pipeType==0)
			{
			/* Split the server name into hostname:port: */
			const char* colonPtr=0;
			for(const char* cPtr=serverNamePort;*cPtr!='\0';++cPtr)
				if(*cPtr==':')
					colonPtr=cPtr;
			std::string serverName;
			int portNumber=8555;
			if(colonPtr!=0)
				{
				serverName=std::string(serverNamePort,colonPtr);
				portNumber=atoi(colonPtr+1);
				}
			else
				serverName=serverNamePort;
			
			/* Connect to the VR device server over a TCP socket: */
			deviceClient=new Vrui::VRDeviceClient(dispatcher,serverName.c_str(),portNumber);
			}
		else
			{
			/* Connect to the VR device server over a UNIX domain socket: */
			deviceClient=new Vrui::VRDeviceClient(dispatcher,serverSocketName,serverSocketAbstract);
			}
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"Caught exception "<<err.what()<<" while initializing VR device client"<<std::endl;
		return 1;
		}
	
	/* Print information about the server connection: */
	if(deviceClient->isLocal())
		std::cout<<"VR device server is running on same host"<<std::endl;
	if(deviceClient->hasSharedMemory())
		std::cout<<"VR device server offers shared memory"<<std::endl;
	
	if(printDevices)
		{
		/* Print information about the server's virtual input devices: */
		std::cout<<"Device server defines "<<deviceClient->getNumVirtualDevices()<<" virtual input devices."<<std::endl;
		for(int deviceIndex=0;deviceIndex<deviceClient->getNumVirtualDevices();++deviceIndex)
			printDeviceConfiguration(deviceClient->getVirtualDevice(deviceIndex));
		std::cout<<std::endl;
		
		/* Shut down the device client and exit: */
		delete deviceClient;
		return 0;
		}
	
	if(printHmdConfigurations)
		{
		/* Print information about the server's HMD configurations: */
		std::cout<<"Device server defines "<<deviceClient->getNumHmdConfigurations()<<" head-mounted devices."<<std::endl;
		deviceClient->lockHmdConfigurations();
		for(unsigned int hmdIndex=0;hmdIndex<deviceClient->getNumHmdConfigurations();++hmdIndex)
			{
			std::cout<<"Head-mounted device "<<hmdIndex<<":"<<std::endl;
			printHmdConfiguration(deviceClient->getHmdConfiguration(hmdIndex));
			}
		deviceClient->unlockHmdConfigurations();
		std::cout<<std::endl;
		
		/* Shut down the device client and exit: */
		delete deviceClient;
		return 0;
		}
	
	if(printBaseStations)
		{
		/* Request the list of tracking base stations from the server: */
		std::vector<Vrui::VRBaseStation> baseStations=deviceClient->getBaseStations();
		
		/* Print the current base station states: */
		std::cout<<"Server has "<<baseStations.size()<<" tracking base stations"<<std::endl;
		for(std::vector<Vrui::VRBaseStation>::iterator bsIt=baseStations.begin();bsIt!=baseStations.end();++bsIt)
			printBaseStation(*bsIt);
		std::cout<<std::endl;
		
		/* Shut down the device client and exit: */
		delete deviceClient;
		return 0;
		}
	
	if(printEnvironment)
		{
		/* Request the physical environment definition from the server: */
		Vrui::EnvironmentDefinition environmentDefinition;
		if(deviceClient->getEnvironmentDefinition(environmentDefinition))
			{
			/* Print the environment definition: */
			printEnvironmentDefinition(environmentDefinition);
			}
		else
			std::cout<<"Server does not provide environment definitions"<<std::endl;
		
		/* Shut down the device client and exit: */
		delete deviceClient;
		return 0;
		}
	
	if(uploadEnvironmentDefinition)
		{
		try
			{
			/* Open the environment definition configuration file: */
			Misc::ConfigurationFile configurationFile(uploadEnvironmentDefinitionConfigurationFileName);
			
			/* Read an environment definition from the file's root section: */
			Vrui::EnvironmentDefinition environmentDefinition;
			environmentDefinition.configure(configurationFile.getCurrentSection());
			
			/* Upload the environment definition to the device driver: */
			deviceClient->updateEnvironmentDefinition(environmentDefinition);
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"Unable to upload environment definition from configuration file "<<uploadEnvironmentDefinitionConfigurationFileName<<" due to exception "<<err.what()<<std::endl;
			}
		
		/* Shut down the device client and exit: */
		delete deviceClient;
		return 0;
		}
	
	if(printBatteryStates)
		{
		/* Print the battery states of all virtual devices: */
		std::cout<<"Device battery states:"<<std::endl;
		deviceClient->lockBatteryStates();
		for(int deviceIndex=0;deviceIndex<deviceClient->getNumVirtualDevices();++deviceIndex)
			{
			const Vrui::VRDeviceDescriptor& vd=deviceClient->getVirtualDevice(deviceIndex);
			if(vd.hasBattery)
				{
				const Vrui::BatteryState& bs=deviceClient->getBatteryState(deviceIndex);
				std::cout<<'\t'<<vd.name<<": "<<(bs.charging?"charging":"discharging")<<' '<<bs.batteryLevel<<'%'<<std::endl;
				}
			}
		deviceClient->unlockBatteryStates();
		
		/* Shut down the device client and exit: */
		delete deviceClient;
		return 0;
		}
	
	/* Check whether to trigger a haptic pulse: */
	if(powerFeatureIndex>=0||hapticFeatureIndex>=0)
		{
		/* Request a power off or haptic tick and disconnect from the server: */
		try
			{
			deviceClient->activate();
			if(hapticFeatureIndex>=0)
				deviceClient->hapticTick(hapticFeatureIndex,hapticDuration,hapticFrequency,hapticAmplitude);
			if(powerFeatureIndex>=0)
				deviceClient->powerOff(powerFeatureIndex);
			deviceClient->deactivate();
			}
		catch(const std::runtime_error& err)
			{
			std::cerr<<"Caught exception "<<err.what()<<" while powering off device / triggering haptic pulse"<<std::endl;
			}
		
		/* Shut down the device client and exit: */
		delete deviceClient;
		return 0;
		}
	
	/* Create a tracker printer: */
	TrackerPrinter trackerPrinter(deviceClient,trackerIndex,printMode,printButtonStates,printNewlines);
	
	/* Initialize HMD configuration state arrays: */
	deviceClient->lockHmdConfigurations();
	numHmdConfigurations=deviceClient->getNumHmdConfigurations();
	hmdConfigurations=new const Vrui::HMDConfiguration*[numHmdConfigurations];
	eyePosVersions=new unsigned int[numHmdConfigurations];
	eyeRotVersions=new unsigned int[numHmdConfigurations];
	eyeVersions=new unsigned int[numHmdConfigurations];
	distortionMeshVersions=new unsigned int[numHmdConfigurations];
	for(unsigned int i=0;i<numHmdConfigurations;++i)
		{
		hmdConfigurations[i]=&deviceClient->getHmdConfiguration(i);
		eyePosVersions[i]=hmdConfigurations[i]->getEyePosVersion();
		eyeRotVersions[i]=hmdConfigurations[i]->getEyeRotVersion();
		eyeVersions[i]=hmdConfigurations[i]->getEyeVersion();
		distortionMeshVersions[i]=hmdConfigurations[i]->getDistortionMeshVersion();
		deviceClient->setHmdConfigurationUpdatedCallback(hmdConfigurations[i]->getTrackerIndex(),Misc::createFunctionCall(hmdConfigurationUpdatedCallback));
		}
	deviceClient->unlockHmdConfigurations();
	
	/* Register a callback to be notified when the server's environment definition changes: */
	deviceClient->setEnvironmentDefinitionUpdatedCallback(Misc::createFunctionCall(environmentDefinitionUpdatedCallback));
	
	/* Open the save file: */
	std::ofstream* saveFile=0;
	IO::FilePtr saveTsFile=0;
	Vrui::VRDeviceState::TimeStamp lastTsTs=0;
	if(savePositions)
		{
		saveFile=new std::ofstream(saveFileName);
		saveFile->precision(8);
		}
	else if(saveTrackerStates)
		saveTsFile=IO::openFile(saveFileName,IO::File::WriteOnly);
	
	/* Disable line buffering on stdin: */
	struct termios originalTerm;
	tcgetattr(STDIN_FILENO,&originalTerm);
	struct termios term=originalTerm;
	term.c_lflag&=~ICANON;
	tcsetattr(STDIN_FILENO,TCSANOW,&term);
	
	/* Register a callback for stdin: */
	Threads::EventDispatcher::ListenerKey stdinListener=dispatcher.addIOEventListener(STDIN_FILENO,Threads::EventDispatcher::Read,stdioCallback,0);
	
	/* Activate the device client: */
	deviceClient->activate();
	
	/* Run main loop: */
	Threads::EventDispatcher::ListenerKey updateListener(0);
	if(pipeType==0)
		{
		/* Start streaming device data to the packet notification callback: */
		deviceClient->startStream(Misc::createFunctionCall(packetNotificationCallback,&trackerPrinter));
		}
	else
		{
		/* Register a callback to display device data from the server's shared memory segment at regular intervals: */
		updateListener=dispatcher.addTimerEventListener(Threads::EventDispatcher::Time::now(),Threads::EventDispatcher::Time(0,100000),updateDevicesCallback,&trackerPrinter);
		}
	
	/* Dispatch events: */
	Misc::Timer t;
	dispatcher.dispatchEvents();
	t.elapse();
	std::cout<<std::endl<<"Received "<<trackerPrinter.getNumSamples()<<" device data packets in "<<t.getTime()*1000.0<<" ms ("<<double(trackerPrinter.getNumSamples())/t.getTime()<<" packets/s)"<<std::endl;
	
	if(pipeType==0)
		{
		/* Stop streaming: */
		deviceClient->stopStream();
		}
	else
		{
		/* Unregister the timer callback: */
		dispatcher.removeTimerEventListener(updateListener);
		}
	
	/* Unregister the stdin callback: */
	dispatcher.removeIOEventListener(stdinListener);
	
	deviceClient->deactivate();
	
	/* Print tracking data latency histogram if requested: */
	if(printLatency)
		trackerPrinter.printLatency();
	
	/* Reset stdin to its original state: */
	tcsetattr(STDIN_FILENO,TCSANOW,&originalTerm);
	
	/* Clean up and terminate: */
	delete[] hmdConfigurations;
	delete[] eyePosVersions;
	delete[] eyeRotVersions;
	delete[] eyeVersions;
	delete[] distortionMeshVersions;
	if(savePositions!=0)
		delete saveFile;
	else if(saveTrackerStates)
		saveTsFile=0;
	delete deviceClient;
	return 0;
	}

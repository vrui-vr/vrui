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
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Misc/ValueCoder.h>
#include <Realtime/Time.h>
#include <Threads/Mutex.h>
#include <Threads/EventDispatcherThread.h>
#include <RawHID/EventDevice.h>
#include <Math/Matrix.h>
#include <Math/GaussNewtonMinimizer.h>
#include <Geometry/Point.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/AffineTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <Geometry/PointAlignerATransform.h>
#include <Geometry/PointAlignerPTransform.h>
#include <Geometry/OutputOperators.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/VRScreen.h>

class CalibrateTouchscreen:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	struct PenDeviceConfig // Structure describing the absolute axis / key configuration of a pen device
		{
		/* Elements: */
		public:
		unsigned int posAxisIndices[2]; // Device indices of the device's position axes
		unsigned int tiltAxisIndices[2]; // Device indices of the device's tilt axes
		unsigned int touchKeyIndex; // Index of the device's touch key
		unsigned int pressKeyIndex; // Index of the device's press key
		bool valid; // Flag if the device has the required axes/buttons for a pen device
		bool haveTilt; // Flag if the device has tilt axes
		
		/* Constructors and destructors: */
		PenDeviceConfig(void); // Creates an invalid pen device configuration
		};
	
	typedef Geometry::Point<double,2> Point; // Type for points in screen space
	
	struct CalibrationPoint
		{
		/* Elements: */
		public:
		Point measured; // Measured position in raw pen device coordinates
		Point ideal; // Ideal position in screen coordinates
		};
	
	typedef std::vector<CalibrationPoint> CalibrationPointList; // Type for lists of calibration points
	typedef RawHID::EventDevice::AbsAxisConfig AxisConfig;
	
	class Calibrator // Base class for calibration algorithms
		{
		/* Elements: */
		protected:
		double axisMin[2],axisMax[2]; // Minimum and maximum values for the two absolute axes
		
		/* Protected methods: */
		Point normalize(double rawX,double rawY) const // Normalizes the given raw axis values
			{
			return Point((rawX-axisMin[0])/(axisMax[0]-axisMin[0]),(rawY-axisMin[1])/(axisMax[1]-axisMin[1]));
			}
		Point normalize(const Point& raw) const // Ditto
			{
			return Point((raw[0]-axisMin[0])/(axisMax[0]-axisMin[0]),(raw[1]-axisMin[1])/(axisMax[1]-axisMin[1]));
			}
		
		/* Constructors and destructors: */
		public:
		Calibrator(const AxisConfig& configX,const AxisConfig& configY)
			{
			/* Store the axis value ranges: */
			axisMin[0]=double(configX.min);
			axisMax[0]=double(configX.max);
			axisMin[1]=double(configY.min);
			axisMax[1]=double(configY.max);
			}
		virtual ~Calibrator(void)
			{
			}
		
		/* Methods: */
		virtual Point calibrate(double rawX,double rawY) const =0; // Returns a calibrated point for the given raw absolute axis values
		};
	
	class RectilinearCalibrator:public Calibrator // Class for rectilinear calibration
		{
		/* Elements: */
		private:
		double aX,aY,bX,bY; // Scale factors and offsets for the two axes
		
		/* Constructors and destructors: */
		public:
		RectilinearCalibrator(const AxisConfig& configX,const AxisConfig& configY,const CalibrationPointList& points);
		
		/* Methods from class Calibrator: */
		virtual Point calibrate(double rawX,double rawY) const
			{
			Point r=normalize(rawX,rawY);
			return Point(aX*r[0]+bX,aY*r[1]+bY);
			}
		};
	
	class AffineCalibrator:public Calibrator // Class for affine calibration
		{
		/* Elements: */
		private:
		Geometry::AffineTransformation<double,2> transform; // The calibration transformation
		
		/* Constructors and destructors: */
		public:
		AffineCalibrator(const AxisConfig& configX,const AxisConfig& configY,const CalibrationPointList& points);
		
		/* Methods from class Calibrator: */
		virtual Point calibrate(double rawX,double rawY) const
			{
			return transform.transform(normalize(rawX,rawY));
			}
		};
	
	class ProjectiveCalibrator:public Calibrator // Class for projective calibration
		{
		/* Elements: */
		private:
		Geometry::ProjectiveTransformation<double,2> transform; // The calibration transformation
		
		/* Constructors and destructors: */
		public:
		ProjectiveCalibrator(const AxisConfig& configX,const AxisConfig& configY,const CalibrationPointList& points);
		
		/* Methods from class Calibrator: */
		virtual Point calibrate(double rawX,double rawY) const
			{
			return transform.transform(normalize(rawX,rawY));
			}
		};
	
	class BsplineCalibrator:public Calibrator // Class for B-spline mesh calibration
		{
		/* Elements: */
		private:
		int degrees[2]; // B-spline degrees
		int numPoints[2]; // B-spline mesh size
		Point* mesh; // Array of B-spline mesh control points
		Point* xs; // X-direction B-spline evaluation array
		Point* ys; // Y-direction B-spline evaluation array
		
		/* Private methods: */
		static double bspline(int i,int n,double t) // Returns the value of a B-spline basis function
			{
			/* Use dynamic programming on the Cox-de Boor formulation: */
			double eval[21]; // Only up to degree 20!
			
			/* Initialize the evaluation array: */
			for(int j=0;j<=n;++j)
				eval[j]=t>=double(i+j)&&t<double(i+j+1)?1.0:0.0;
			
			/* Raise the degree: */
			for(int deg=1;deg<=n;++deg)
				for(int j=0;j<=n-deg+1;++j)
					eval[j]=((t-double(i+j))*eval[j]+(double(i+j+deg+1)-t)*eval[j+1])/double(deg);
			
			return eval[0];
			}
			
		/* Constructors and destructors: */
		public:
		BsplineCalibrator(const AxisConfig& configX,const AxisConfig& configY,const int sDegrees[2],const int sNumPoints[2],const CalibrationPointList& points);
		virtual ~BsplineCalibrator(void);
		
		/* Methods from class Calibrator: */
		virtual Point calibrate(double rawX,double rawY) const;
		};
	
	/* Elements: */
	RawHID::EventDevice* penDevice; // The input device associated with the touchscreen's pen device
	Threads::EventDispatcherThread dispatcher; // Event dispatcher for events on the pen device
	unsigned int axisIndices[2]; // Feature indices of the pen's absolute x and y axes
	bool haveTilt; // Flag if the pen provides tilt measurements
	unsigned int tiltIndices[2]; // Feature indices of the pen's x and y tilt axes
	unsigned int hoverButtonIndex; // Feature index of the pen's "in range" button
	unsigned int buttonIndex; // Feature index of the pen's touch button
	Vrui::VRScreen* screen; // The screen associated with the touchscreen
	double screenSize[2]; // Width and height of the touchscreen in physical coordinate units
	double gridGap; // Gap between the outer edges of the screen and the grid in grid tile units
	unsigned int numPoints[2]; // Number of calibration points in x and y
	unsigned int currentPoint; // Current calibration point
	Realtime::TimePointMonotonic measureStart; // Earliest time at which a new measurement can start
	double pointAccum[2]; // Absolute axis accumulators for the current point
	double tiltAccum[2]; // Tilt axis accumulators for the current point
	unsigned int numAccum; // Number of accumulated points
	CalibrationPointList calibrationPoints; // List of already collected calibration points
	int calibrationType; // Type of calibration method
	int splineDegrees[2]; // Degrees for calibration B-spline
	int splineNumPoints[2]; // Numbers of control points for calibration B-spline
	Calibrator* calibrator; // Pointer to a calibration object, or 0 if calibration is incomplete
	mutable Threads::Mutex penStateMutex; // Mutex protecting the current pen state
	bool hovering; // Flag whether the pen is currently close enough to the touch screen for the pen position to be valid
	Point penPos; // Calibration pen position when a calibration has been calculated
	
	/* Private methods: */
	static PenDeviceConfig getPenDeviceConfig(RawHID::EventDevice& device); // Extracts a pen device configuration from the given device
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

/******************************************************
Methods of class CalibrateTouchscreen::PenDeviceConfig:
******************************************************/

CalibrateTouchscreen::PenDeviceConfig::PenDeviceConfig(void)
	:touchKeyIndex(~0x0U),pressKeyIndex(~0x0U),
	 valid(false),haveTilt(false)
	{
	for(int i=0;i<2;++i)
		{
		posAxisIndices[i]=~0x0U;
		tiltAxisIndices[i]=~0x0U;
		}
	}

/************************************************************
Methods of class CalibrateTouchscreen::RectilinearCalibrator:
************************************************************/

CalibrateTouchscreen::RectilinearCalibrator::RectilinearCalibrator(const CalibrateTouchscreen::AxisConfig& configX,const CalibrateTouchscreen::AxisConfig& configY,const CalibrationPointList& points)
	:Calibrator(configX,configY)
	{
	/* Calculate a rectilinear calibration: */
	Math::Matrix xata(2,2,0.0);
	Math::Matrix xatb(2,1,0.0);
	Math::Matrix yata(2,2,0.0);
	Math::Matrix yatb(2,1,0.0);
	for(CalibrationPointList::const_iterator cpIt=points.begin();cpIt!=points.end();++cpIt)
		{
		Point m=normalize(cpIt->measured);
		
		xata(0,0)+=m[0]*m[0];
		xata(0,1)+=m[0];
		xata(1,0)+=m[0];
		xata(1,1)+=1.0;
		xatb(0)+=m[0]*cpIt->ideal[0];
		xatb(1)+=cpIt->ideal[0];
		
		yata(0,0)+=m[1]*m[1];
		yata(0,1)+=m[1];
		yata(1,0)+=m[1];
		yata(1,1)+=1.0;
		yatb(0)+=m[1]*cpIt->ideal[1];
		yatb(1)+=cpIt->ideal[1];
		}
	
	Math::Matrix xc=xatb;
	xc.divideFullPivot(xata);
	Math::Matrix yc=yatb;
	yc.divideFullPivot(yata);
	
	/* Retrieve the calibration coefficients: */
	aX=xc(0);
	bX=xc(1);
	aY=yc(0);
	bY=yc(1);
	
	/* Calculate the calibration residual: */
	double rms2=0.0;
	double linf2=0.0;
	for(CalibrationPointList::const_iterator cpIt=points.begin();cpIt!=points.end();++cpIt)
		{
		Point m=normalize(cpIt->measured);
		Point cal(aX*m[0]+bX,aY*m[1]+bY);
		double dist2=Geometry::sqrDist(cpIt->ideal,cal);
		rms2+=dist2;
		if(linf2<dist2)
			linf2=dist2;
		}
	std::cout<<"Final calibration residual: "<<Math::sqrt(rms2/double(points.size()))<<" RMS, "<<Math::sqrt(linf2)<<" Linf"<<std::endl;
	
	/* Print a calibrator configuration section: */
	std::cout<<std::endl;
	std::cout<<"section Calibrator"<<std::endl;
	std::cout<<"\ttype Rectilinear"<<std::endl;
	std::cout<<"\tscales ("<<aX<<", "<<aY<<")"<<std::endl;
	std::cout<<"\toffsets ("<<bX<<", "<<bY<<")"<<std::endl;
	std::cout<<"endsection"<<std::endl;
	}

/*******************************************************
Methods of class CalibrateTouchscreen::AffineCalibrator:
*******************************************************/

CalibrateTouchscreen::AffineCalibrator::AffineCalibrator(const CalibrateTouchscreen::AxisConfig& configX,const CalibrateTouchscreen::AxisConfig& configY,const CalibrationPointList& points)
	:Calibrator(configX,configY)
	{
	/* Set up a point aligner based on a affine transformation: */
	typedef Geometry::PointAlignerATransform<double,2> Aligner;
	Aligner pointAligner;
	for(CalibrationPointList::const_iterator cpIt=points.begin();cpIt!=points.end();++cpIt)
		pointAligner.addPointPair(normalize(cpIt->measured),cpIt->ideal);
	
	/* Estimate the initial calibration transformation: */
	pointAligner.condition();
	pointAligner.estimateTransform();
	
	/* Improve the calibration transformation with a few steps of Gauss-Newton iteration: */
	Math::GaussNewtonMinimizer<Aligner> minimizer(1000);
	minimizer.minimize(pointAligner);
	
	/* Calculate the residual: */
	std::pair<double,double> res2=pointAligner.calcResidualToSpace(pointAligner.getTransform());
	std::cout<<"Final calibration residual: "<<res2.first<<" RMS, "<<res2.second<<" Linf"<<std::endl;
	
	/* Retrieve the final calibration transformation: */
	transform=pointAligner.getTransform();
	
	/* Print a calibrator configuration section: */
	std::cout<<std::endl;
	std::cout<<"section Calibrator"<<std::endl;
	std::cout<<"\ttype Affine"<<std::endl;
	std::cout<<"\ttransform "<<Misc::ValueCoder<Aligner::Transform>::encode(transform)<<std::endl;
	std::cout<<"endsection"<<std::endl;
	}

/***********************************************************
Methods of class CalibrateTouchscreen::ProjectiveCalibrator:
***********************************************************/

CalibrateTouchscreen::ProjectiveCalibrator::ProjectiveCalibrator(const CalibrateTouchscreen::AxisConfig& configX,const CalibrateTouchscreen::AxisConfig& configY,const CalibrationPointList& points)
	:Calibrator(configX,configY)
	{
	/* Set up a point aligner based on a projective transformation: */
	typedef Geometry::PointAlignerPTransform<double,2> Aligner;
	Aligner pointAligner;
	for(CalibrationPointList::const_iterator cpIt=points.begin();cpIt!=points.end();++cpIt)
		pointAligner.addPointPair(normalize(cpIt->measured),cpIt->ideal);
	
	/* Estimate the initial calibration transformation: */
	pointAligner.condition();
	pointAligner.estimateTransform();
	
	/* Improve the calibration transformation with a few steps of Gauss-Newton iteration: */
	Math::GaussNewtonMinimizer<Aligner> minimizer(1000);
	minimizer.minimize(pointAligner);
	
	/* Calculate the residual: */
	std::pair<double,double> res2=pointAligner.calcResidualToSpace(pointAligner.getTransform());
	std::cout<<"Final calibration residual: "<<res2.first<<" RMS, "<<res2.second<<" Linf"<<std::endl;
	
	/* Retrieve and normalize the final calibration transformation: */
	transform=pointAligner.getTransform();
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			transform.getMatrix()(i,j)/=transform.getMatrix()(2,2);
	
	/* Print a calibrator configuration section: */
	std::cout<<std::endl;
	std::cout<<"section Calibrator"<<std::endl;
	std::cout<<"\ttype Projective"<<std::endl;
	std::cout<<"\ttransform "<<Misc::ValueCoder<Aligner::Transform>::encode(transform)<<std::endl;
	std::cout<<"endsection"<<std::endl;
	}

/********************************************************
Methods of class CalibrateTouchscreen::BsplineCalibrator:
********************************************************/

CalibrateTouchscreen::BsplineCalibrator::BsplineCalibrator(const CalibrateTouchscreen::AxisConfig& configX,const CalibrateTouchscreen::AxisConfig& configY,const int sDegrees[2],const int sNumPoints[2],const CalibrateTouchscreen::CalibrationPointList& points)
	:Calibrator(configX,configY),
	 mesh(0),xs(0),ys(0)
	{
	/* Copy B-spline mesh layout and initialize mesh control point array: */
	for(int i=0;i<2;++i)
		{
		degrees[i]=sDegrees[i];
		numPoints[i]=sNumPoints[i];
		}
	
	/* Create the least-squares calibration system: */
	int eqSize=numPoints[1]*numPoints[0];
	Math::Matrix ata(eqSize,eqSize,0.0);
	Math::Matrix atb(eqSize,2,0.0);
	double* eq=new double[eqSize];
	for(CalibrationPointList::const_iterator cpIt=points.begin();cpIt!=points.end();++cpIt)
		{
		/* Transform the measured point to B-spline mesh space: */
		double px=(cpIt->measured[0]-axisMin[0])*double(numPoints[0]-degrees[0])/(axisMax[0]-axisMin[0])+double(degrees[0]);
		double py=(cpIt->measured[1]-axisMin[1])*double(numPoints[1]-degrees[1])/(axisMax[1]-axisMin[1])+double(degrees[1]);
		
		/* Enter the measured point's B-spline weights into the calibration system: */
		for(int i=0;i<numPoints[1];++i)
			for(int j=0;j<numPoints[0];++j)
				eq[i*numPoints[0]+j]=bspline(i,degrees[1],py)*bspline(j,degrees[0],px);
		for(int i=0;i<eqSize;++i)
			{
			for(int j=0;j<eqSize;++j)
				ata(i,j)+=eq[i]*eq[j];
			for(int j=0;j<2;++j)
				atb(i,j)+=eq[i]*cpIt->ideal[j];
			}
		}
	delete[] eq;
	
	/* Solve the system: */
	Math::Matrix x=atb;
	x.divideFullPivot(ata);
	
	/* Extract the control point mesh: */
	mesh=new Point[numPoints[1]*numPoints[0]];
	for(int i=0;i<numPoints[1];++i)
		for(int j=0;j<numPoints[0];++j)
			for(int k=0;k<2;++k)
				mesh[i*numPoints[0]+j][k]=x(i*numPoints[0]+j,k);
	
	/* Create the B-spline evaluation arrays: */
	xs=new Point[degrees[0]+1];
	ys=new Point[degrees[1]+1];
	
	/* Calculate the calibration residual: */
	double rms2=0.0;
	double linf2=0.0;
	for(CalibrationPointList::const_iterator cpIt=points.begin();cpIt!=points.end();++cpIt)
		{
		/* Calibrate the measured point: */
		Point cal=calibrate(cpIt->measured[0],cpIt->measured[1]);
		
		/* Update the error metrics: */
		double dist2=Geometry::sqrDist(cpIt->ideal,cal);
		rms2+=dist2;
		if(linf2<dist2)
			linf2=dist2;
		}
	std::cout<<"Calibration residual: "<<Math::sqrt(rms2/double(points.size()))<<" RMS, "<<Math::sqrt(linf2)<<" Linf"<<std::endl;
	
	/* Print a calibrator configuration section: */
	std::cout<<std::endl;
	std::cout<<"section Calibrator"<<std::endl;
	std::cout<<"\ttype Bspline"<<std::endl;
	std::cout<<"\tdegrees ("<<degrees[0]<<", "<<degrees[1]<<")"<<std::endl;
	std::cout<<"\tnumPoints ("<<numPoints[0]<<", "<<numPoints[1]<<")"<<std::endl;
	for(int i=0;i<numPoints[1];++i)
		{
		if(i>0)
			std::cout<<"\t        (";
		else
			std::cout<<"\tpoints ((";
		for(int j=0;j<numPoints[0];++j)
			{
			if(j>0)
				std::cout<<", ";
			std::cout<<"("<<mesh[i*numPoints[0]+j][0]<<", "<<mesh[i*numPoints[0]+j][1]<<")";
			}
		if(i<numPoints[1]-1)
			std::cout<<"), \\"<<std::endl;
		else
			std::cout<<"))"<<std::endl;
		}
	std::cout<<"endsection"<<std::endl;
	}

CalibrateTouchscreen::BsplineCalibrator::~BsplineCalibrator(void)
	{
	delete[] mesh;
	delete[] xs;
	delete[] ys;
	}

CalibrateTouchscreen::Point CalibrateTouchscreen::BsplineCalibrator::calibrate(double rawX,double rawY) const
	{
	/* Transform the raw point to B-spline mesh space and find the knot intervals containing it: */
	double px=(rawX-axisMin[0])*double(numPoints[0]-degrees[0])/(axisMax[0]-axisMin[0])+double(degrees[0]);
	int ivx=Math::clamp(int(Math::floor(px)),degrees[0],numPoints[0]-1);
	double py=(rawY-axisMin[1])*double(numPoints[1]-degrees[1])/(axisMax[1]-axisMin[1])+double(degrees[1]);
	int ivy=Math::clamp(int(Math::floor(py)),degrees[1],numPoints[1]-1);
	
	/* Evaluate x-direction B-spline curves: */
	for(int y=0;y<=degrees[1];++y)
		{
		/* Copy the partial control point array: */
		for(int x=0;x<=degrees[0];++x)
			xs[x]=mesh[(ivy-degrees[1]+y)*numPoints[0]+(ivx-degrees[0]+x)];
		
		/* Run Cox-de Boor's algorithm on the partial array: */
		for(int k=0;k<degrees[0];++k)
			{
			int subDeg=degrees[0]-k;
			for(int x=0;x<subDeg;++x)
				xs[x]=Geometry::affineCombination(xs[x],xs[x+1],(px-double(ivx-subDeg+1+x))/double(subDeg));
			}
		
		/* Put the final point into the y-direction B-spline curve evaluation array: */
		ys[y]=xs[0];
		}
	
	/* Evaluate the y-direction B-spline curve: */
	for(int k=0;k<degrees[1];++k)
		{
		int subDeg=degrees[1]-k;
		for(int y=0;y<subDeg;++y)
			ys[y]=Geometry::affineCombination(ys[y],ys[y+1],(py-double(ivy-subDeg+1+y))/double(subDeg));
		}
	
	/* Return the final point: */
	return ys[0];
	}

/*************************************
Methods of class CalibrateTouchscreen:
*************************************/

CalibrateTouchscreen::PenDeviceConfig CalibrateTouchscreen::getPenDeviceConfig(RawHID::EventDevice& device)
	{
	PenDeviceConfig result;
	
	/* Find the absolute axes and keys that define a pen device: */
	unsigned int featureMask=0x0U;
	
	/* Check for absolute axis features: */
	unsigned int numAxes=device.getNumAbsAxisFeatures();
	for(unsigned int i=0;i<numAxes;++i)
		{
		switch(device.getAbsAxisFeatureConfig(i).code)
			{
			case ABS_X:
				result.posAxisIndices[0]=i;
				featureMask|=0x1U;
				break;
			
			case ABS_Y:
				result.posAxisIndices[1]=i;
				featureMask|=0x2U;
				break;
			
			case ABS_TILT_X:
				result.tiltAxisIndices[0]=i;
				featureMask|=0x10U;
				break;
			
			case ABS_TILT_Y:
				result.tiltAxisIndices[1]=i;
				featureMask|=0x20U;
				break;
			}
		}
	
	/* Check for key features: */
	unsigned int numKeys=device.getNumKeyFeatures();
	for(unsigned int i=0;i<numKeys;++i)
		{
		switch(device.getKeyFeatureCode(i))
			{
			case BTN_TOUCH:
				result.touchKeyIndex=i;
				featureMask|=0x04U;
				break;
			
			case BTN_TOOL_PEN:
				result.pressKeyIndex=i;
				featureMask|=0x08U;
				break;
			}
		}
	
	/* Check if all required and optional features are present: */
	result.valid=(featureMask&0x0fU)==0x0fU;
	result.haveTilt=(featureMask&0x30U)==0x30U;
	
	return result;
	}

void CalibrateTouchscreen::keyCallback(RawHID::EventDevice::KeyFeatureEventCallbackData* cbData)
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	/* Bail out if there is a calibration: */
	if(calibrator!=0)
		return;
	
	if(cbData->featureIndex==buttonIndex)
		{
		Realtime::TimePointMonotonic now;
		
		if(cbData->newValue)
			{
			if(now>=measureStart&&currentPoint<numPoints[1]*numPoints[0])
				{
				/* Start accumulating data for the current point: */
				for(int i=0;i<2;++i)
					pointAccum[i]=double(penDevice->getAbsAxisFeatureValue(axisIndices[i]));
				if(haveTilt)
					for(int i=0;i<2;++i)
						tiltAccum[i]=double(penDevice->getAbsAxisFeatureValue(tiltIndices[i]));
				numAccum=1;
				}
			}
		else
			{
			if(numAccum>=1)
				{
				/* Print the current measurement: */
				std::cout<<currentPoint<<": pos "<<pointAccum[0]/double(numAccum)<<", "<<pointAccum[1]/double(numAccum);
				if(haveTilt)
					std::cout<<", tilt "<<tiltAccum[0]/double(numAccum)<<", "<<tiltAccum[1]/double(numAccum);
				std::cout<<std::endl;
				
				/* Create a new calibration point from the accumulated measurements: */
				CalibrationPoint cp;
				unsigned int index[2];
				index[0]=currentPoint%numPoints[0];
				index[1]=currentPoint/numPoints[0];
				if(index[1]%2==1)
					index[0]=numPoints[0]-1-index[0];
				for(int i=0;i<2;++i)
					{
					cp.measured[i]=pointAccum[i]/double(numAccum);
					cp.ideal[i]=(double(index[i])+gridGap)/(double(numPoints[i]-1)+gridGap*2.0)*screenSize[i];
					}
				calibrationPoints.push_back(cp);
				
				/* Calculate a calibration if a full set of points has been collected: */
				if(calibrationPoints.size()>=numPoints[1]*numPoints[0])
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
	else if(cbData->featureIndex==hoverButtonIndex)
		{
		/* Update the hovering flag: */
		hovering=cbData->newValue;
		}
	
	Vrui::requestUpdate();
	}

void CalibrateTouchscreen::absAxisCallback(RawHID::EventDevice::AbsAxisFeatureEventCallbackData* cbData)
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	if(calibrator!=0)
		{
		/* Calculate the current pen position: */
		int raw[2];
		for(int i=0;i<2;++i)
			{
			if(cbData->featureIndex==axisIndices[i])
				raw[i]=cbData->newValue;
			else
				raw[i]=penDevice->getAbsAxisFeatureValue(axisIndices[i]);
			}
		penPos=calibrator->calibrate(raw[0],raw[1]);
		
		Vrui::requestUpdate();
		}
	else if(penDevice->getKeyFeatureValue(buttonIndex))
		{
		/* Accumulate the new axis values: */
		for(int i=0;i<2;++i)
			{
			if(cbData->featureIndex==axisIndices[i])
				pointAccum[i]+=double(cbData->newValue);
			else
				pointAccum[i]+=double(penDevice->getAbsAxisFeatureValue(axisIndices[i]));
			}
		++numAccum;
		}
	}

void CalibrateTouchscreen::synReportCallback(Misc::CallbackData* cbData)
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	if(calibrator!=0)
		{
		/* Calculate the current pen position: */
		penPos=calibrator->calibrate(penDevice->getAbsAxisFeatureValue(axisIndices[0]),penDevice->getAbsAxisFeatureValue(axisIndices[1]));
		
		Vrui::requestUpdate();
		}
	else if(numAccum>0)
		{
		/* Accumulate the current axis values: */
		for(int i=0;i<2;++i)
			pointAccum[i]+=double(penDevice->getAbsAxisFeatureValue(axisIndices[i]));
		if(haveTilt)
			for(int i=0;i<2;++i)
				tiltAccum[i]+=double(penDevice->getAbsAxisFeatureValue(tiltIndices[i]));
		++numAccum;
		}
	}

void CalibrateTouchscreen::calibrate(void)
	{
	Threads::Mutex::Lock penStateLock(penStateMutex);
	
	{
	/* Save the calibration point set to a CSV file: */
	std::ofstream csv("/home/okreylos/Desktop/CalibrationData.csv");
	for(CalibrationPointList::iterator cpIt=calibrationPoints.begin();cpIt!=calibrationPoints.end();++cpIt)
		csv<<cpIt->measured[0]<<','<<cpIt->measured[1]<<','<<cpIt->ideal[0]<<','<<cpIt->ideal[1]<<std::endl;
	}
	
	/* Create a calibrator object: */
	const AxisConfig& configX=penDevice->getAbsAxisFeatureConfig(axisIndices[0]);
	const AxisConfig& configY=penDevice->getAbsAxisFeatureConfig(axisIndices[1]);
	switch(calibrationType)
		{
		case 0:
			calibrator=new RectilinearCalibrator(configX,configY,calibrationPoints);
			break;
		
		case 1:
			calibrator=new AffineCalibrator(configX,configY,calibrationPoints);
			break;
		
		case 2:
			calibrator=new ProjectiveCalibrator(configX,configY,calibrationPoints);
			break;
		
		case 3:
			calibrator=new BsplineCalibrator(configX,configY,splineDegrees,splineNumPoints,calibrationPoints);
			break;
		}
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
			PenDeviceConfig config=getPenDeviceConfig(device);
			
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
			Misc::formattedUserError("Cannot open event device %s due to exception %s",edIt->c_str(),err.what());
			}
		}
	}

CalibrateTouchscreen::CalibrateTouchscreen(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 penDevice(0),
	 haveTilt(false),
	 screen(0),
	 numAccum(0),
	 calibrationType(0),
	 calibrator(0),
	 hovering(false)
	{
	/* Parse the command line: */
	bool listDevices=false;
	int matchMode=0x0;
	unsigned int penDeviceVendorId=0x0U;
	unsigned int penDeviceProductId=0x0U;
	const char* penDeviceName=0;
	unsigned int penDeviceIndex=0;
	axisIndices[0]=0U;
	axisIndices[1]=1U;
	buttonIndex=0U;
	gridGap=0.2;
	numPoints[0]=4;
	numPoints[1]=3;
	splineDegrees[1]=splineDegrees[0]=2;
	splineNumPoints[0]=10;
	splineNumPoints[1]=8;
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
					matchMode|=0x1;
					penDeviceVendorId=(unsigned int)(strtoul(argv[argi+1],0,16));
					penDeviceProductId=(unsigned int)(strtoul(argv[argi+2],0,16));
					}
				
				argi+=2;
				}
			else if(strcasecmp(argv[argi]+1,"deviceName")==0||strcasecmp(argv[argi]+1,"dn")==0)
				{
				if(argi+1<argc)
					{
					matchMode|=0x2;
					penDeviceName=argv[argi+1];
					}
				
				++argi;
				}
			else if(strcasecmp(argv[argi]+1,"index")==0||strcasecmp(argv[argi]+1,"i")==0)
				{
				if(argi+1<argc)
					{
					penDeviceIndex=(unsigned int)(strtoul(argv[argi+1],0,10));
					}
				
				++argi;
				}
			else if(strcasecmp(argv[argi]+1,"axes")==0||strcasecmp(argv[argi]+1,"a")==0)
				{
				if(argi+2<argc)
					{
					for(int i=0;i<2;++i)
						axisIndices[i]=(unsigned int)(strtoul(argv[argi+1+i],0,10));
					}
				
				argi+=2;
				}
			else if(strcasecmp(argv[argi]+1,"button")==0||strcasecmp(argv[argi]+1,"b")==0)
				{
				if(argi+1<argc)
					{
					buttonIndex=(unsigned int)(strtoul(argv[argi+1],0,10));
					}
				
				argi+=1;
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
						splineDegrees[i]=int(strtol(argv[argi+1+i],0,10));
					for(int i=0;i<2;++i)
						splineNumPoints[i]=int(strtol(argv[argi+3+i],0,10));
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
	if(matchMode==0x1)
		penDevice=new RawHID::EventDevice(penDeviceVendorId,penDeviceProductId,penDeviceIndex);
	else if(matchMode==0x2)
		penDevice=new RawHID::EventDevice(penDeviceName,penDeviceIndex);
	else if(matchMode==0x3)
		penDevice=new RawHID::EventDevice(penDeviceVendorId,penDeviceProductId,penDeviceName,penDeviceIndex);
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No pen device specification provided");
	
	/* Try grabbing the pen device: */
	if(!penDevice->grabDevice())
		std::cout<<"Unable to grab the pen device!"<<std::endl;
	
	/* Print information about the pen device: */
	std::cout<<"Calibrating pen device ";
	std::cout<<std::hex<<std::setfill('0')<<std::setw(4)<<penDevice->getVendorId()<<':'<<std::setw(4)<<penDevice->getProductId()<<std::dec<<std::setfill(' ');
	std::cout<<", version "<<penDevice->getVersion();
	std::cout<<", "<<penDevice->getDeviceName()<<" (serial no. "<<penDevice->getSerialNumber()<<')'<<std::endl;
	std::cout<<"Pen device provides "<<penDevice->getNumKeyFeatures()<<" buttons and "<<penDevice->getNumAbsAxisFeatures()<<" absolute axes"<<std::endl;
	
	/* Find the pen device's absolute x and y axes: */
	axisIndices[1]=axisIndices[0]=(unsigned int)(-1);
	tiltIndices[1]=tiltIndices[0]=(unsigned int)(-1);
	unsigned int numAxes=penDevice->getNumAbsAxisFeatures();
	for(unsigned int i=0;i<numAxes;++i)
		{
		if(penDevice->getAbsAxisFeatureConfig(i).code==ABS_X)
			axisIndices[0]=i;
		else if(penDevice->getAbsAxisFeatureConfig(i).code==ABS_Y)
			axisIndices[1]=i;
		else if(penDevice->getAbsAxisFeatureConfig(i).code==ABS_TILT_X)
			tiltIndices[0]=i;
		else if(penDevice->getAbsAxisFeatureConfig(i).code==ABS_TILT_Y)
			tiltIndices[1]=i;
		}
	
	/* Find the pen device's hover and touch button indices: */
	hoverButtonIndex=(unsigned int)(-1);
	buttonIndex=(unsigned int)(-1);
	unsigned int numKeys=penDevice->getNumKeyFeatures();
	for(unsigned int i=0;i<numKeys;++i)
		{
		if(penDevice->getKeyFeatureCode(i)==BTN_TOOL_PEN)
			hoverButtonIndex=i;
		else if(penDevice->getKeyFeatureCode(i)==BTN_TOUCH)
			buttonIndex=i;
		}
	
	/* Check the device's capabilities: */
	if(axisIndices[0]>=numAxes||axisIndices[1]>=numAxes||buttonIndex>=numKeys)
		{
		delete penDevice;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Button or absolute axis indices out of range");
		}
	const RawHID::EventDevice::AbsAxisConfig& configX=penDevice->getAbsAxisFeatureConfig(axisIndices[0]);
	const RawHID::EventDevice::AbsAxisConfig& configY=penDevice->getAbsAxisFeatureConfig(axisIndices[1]);
	std::cout<<"Pen device position axis ranges: ["<<configX.min<<", "<<configX.max<<"], ["<<configY.min<<", "<<configY.max<<"]"<<std::endl;
	haveTilt=tiltIndices[0]<numAxes&&tiltIndices[1]<numAxes;
	if(haveTilt)
		{
		const RawHID::EventDevice::AbsAxisConfig& configX=penDevice->getAbsAxisFeatureConfig(tiltIndices[0]);
		const RawHID::EventDevice::AbsAxisConfig& configY=penDevice->getAbsAxisFeatureConfig(tiltIndices[1]);
		std::cout<<"Pen device tilt axis ranges: ["<<configX.min<<", "<<configX.max<<"], ["<<configY.min<<", "<<configY.max<<"]"<<std::endl;
		}
	
	/* Install event callbacks with the pen device: */
	penDevice->getKeyFeatureEventCallbacks().add(this,&CalibrateTouchscreen::keyCallback);
	
	if(penDevice->haveSynReport())
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
		screenSize[i]=double(screen->getScreenSize()[i]);
	
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
			else if(hoverButtonIndex!=(unsigned int)(-1)&&hovering)
				glColor3f(0.0f,0.333f,0.0f);
			else
				glColor3f(0.0f,0.0f,0.0f);
			}
		else
			glColor3f(0.8f,0.8f,0.8f);
		double sy=(double(y)+gridGap)/(double(numPoints[1]-1)+gridGap*2.0)*screenSize[1];
		glVertex2d(0.0,sy);
		glVertex2d(screenSize[0],sy);
		}
	for(unsigned int x=0;x<numPoints[0];++x)
		{
		if(x==currentX)
			{
			if(numAccum>0)
				glColor3f(0.0f,1.0f,0.0f);
			else if(hoverButtonIndex!=(unsigned int)(-1)&&hovering)
				glColor3f(0.0f,0.333f,0.0f);
			else
				glColor3f(0.0f,0.0f,0.0f);
			}
		else
			glColor3f(0.8f,0.8f,0.8f);
		double sx=(double(x)+gridGap)/(double(numPoints[0]-1)+gridGap*2.0)*screenSize[0];
		glVertex2d(sx,0.0);
		glVertex2d(sx,screenSize[1]);
		}
	glEnd();
	
	if(calibrator!=0)
		{
		/* Draw all calibrated measurement points: */
		glBegin(GL_POINTS);
		glColor3f(0.0f,0.5f,0.0f);
		for(CalibrationPointList::const_iterator cpIt=calibrationPoints.begin();cpIt!=calibrationPoints.end();++cpIt)
			{
			Point cal=calibrator->calibrate(cpIt->measured[0],cpIt->measured[1]);
			glVertex2d(cal[0],cal[1]);
			}
		glEnd();
		
		if(hovering)
			{
			/* Indicate the current pen position: */
			glBegin(GL_LINES);
			glColor3f(0.0f,0.0f,0.0f);
			glVertex2d(penPos[0],0.0);
			glVertex2d(penPos[0],screenSize[1]);
			glVertex2d(0.0,penPos[1]);
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
					calibrationPoints.pop_back();
					--currentPoint;
					}
				
				break;
			}
		}
	}

VRUI_APPLICATION_RUN(CalibrateTouchscreen)

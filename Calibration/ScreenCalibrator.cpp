/***********************************************************************
ScreenCalibrator - Utility to create a calibration transformation
between Vrui's physical coordinate system and a tracking system's
internal coordinate system.
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

#include <stdlib.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <Misc/StdError.h>
#include <IO/TokenSource.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#define GEOMETRY_NONSTANDARD_TEMPLATES
#include <Geometry/Point.h>
#include <Geometry/AffineCombiner.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <Geometry/Box.h>
#include <Geometry/Ray.h>
#include <Geometry/PCACalculator.h>
#include <Geometry/PointPicker.h>
#include <Geometry/RayPicker.h>
#include <Geometry/OutputOperators.h>
#include <Geometry/LevenbergMarquardtMinimizer.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/Application.h>

#include "OGTransformCalculator.h"
#include "OGTransformFitter.h"
#include "ScreenTransformFitter.h"
#include "PTransformFitter.h"

namespace {

/***************
Helper function:
***************/

template <class PointParam>
inline
size_t
cullDuplicates(
	std::vector<PointParam>& points,
	typename PointParam::Scalar tolerance)
	{
	size_t numCulledPoints=0;
	
	/* Traverse the point list and cull all points that have earlier points less than tolerance away: */
	typename PointParam::Scalar t2=Math::sqr(tolerance);
	for(typename std::vector<PointParam>::iterator pIt1=points.begin();pIt1!=points.end();++pIt1)
		{
		/* Check if there's an earlier point that's close: */
		for(typename std::vector<PointParam>::iterator pIt2=points.begin();pIt2!=pIt1;++pIt2)
			if(Geometry::sqrDist(*pIt1,*pIt2)<t2)
				{
				/* Cull the point: */
				points.erase(pIt1);
				--pIt1;
				++numCulledPoints;
				break;
				}
		}
	
	return numCulledPoints;
	}

template <class PointParam>
inline
Geometry::ProjectiveTransformation<double,2>
calcHomography(
	const PointParam corners[4])
	{
	Geometry::Matrix<double,8,8> A=Geometry::Matrix<double,8,8>::zero;
	Geometry::ComponentArray<double,8> b(0.0);
	for(int pointIndex=0;pointIndex<4;++pointIndex)
		{
		/* Calculate the projector corner position: */
		double p[2];
		p[0]=pointIndex&0x1?1.0:-1.0;
		p[1]=pointIndex&0x2?1.0:-1.0;

		A(pointIndex*2+0,0)=p[0];
		A(pointIndex*2+0,1)=p[1];
		A(pointIndex*2+0,2)=1.0;
		A(pointIndex*2+0,6)=-corners[pointIndex][0]*p[0];
		A(pointIndex*2+0,7)=-corners[pointIndex][0]*p[1];
		b[pointIndex*2+0]=corners[pointIndex][0];
		A(pointIndex*2+1,3)=p[0];
		A(pointIndex*2+1,4)=p[1];
		A(pointIndex*2+1,5)=1.0;
		A(pointIndex*2+1,6)=-corners[pointIndex][1]*p[0];
		A(pointIndex*2+1,7)=-corners[pointIndex][1]*p[1];
		b[pointIndex*2+1]=corners[pointIndex][1];
		}
	
	/* Solve for the homography matrix coefficients: */
	Geometry::ComponentArray<double,8> x=b/A;
	Geometry::ProjectiveTransformation<double,2> result;
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			result.getMatrix()(i,j)=(i<2||j<2)?x[i*3+j]:1.0;
	return result;
	}

}

class ScreenCalibrator:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef double Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::Vector<Scalar,3> Vector;
	typedef Geometry::Ray<Scalar,3> Ray;
	typedef Geometry::OrthonormalTransformation<Scalar,3> ONTransform;
	typedef Geometry::OrthogonalTransformation<Scalar,3> OGTransform;
	typedef Geometry::ProjectiveTransformation<Scalar,3> PTransform;
	typedef std::vector<Point> PointList;
	typedef size_t PickResult;
	
	class PointQueryTool;
	typedef Vrui::GenericToolFactory<PointQueryTool> PointQueryToolFactory;
	
	class PointQueryTool:public Vrui::Tool,public Vrui::Application::Tool<ScreenCalibrator>
		{
		friend class Vrui::GenericToolFactory<PointQueryTool>;
		
		/* Elements: */
		private:
		static PointQueryToolFactory* factory;
		
		/* Constructors and destructors: */
		public:
		PointQueryTool(const Vrui::ToolFactory* sFactory,const Vrui::ToolInputAssignment& inputAssignment)
			:Vrui::Tool(sFactory,inputAssignment)
			{
			}
		
		/* Methods from class Vrui::Tool: */
		virtual const Vrui::ToolFactory* getFactory(void) const
			{
			return factory;
			}
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		};
	
	/* Elements: */
	PointList trackingPoints;
	PointList screenPoints;
	PointList floorPoints;
	PointList ballPoints;
	ONTransform screenTransform;
	Scalar screenSize[2];
	PTransform pScreenTransform;
	Vrui::InputDevice* trackingPointsMover;
	Vrui::TrackerState trackingPointsTransform;
	
	/* Private methods: */
	void readOptitrackSampleFile(const char* fileName,bool flipZ);
	PointList readTotalstationSurveyFile(const char* fileName,const char* tag) const;
	
	/* Constructors and destructors: */
	public:
	ScreenCalibrator(int& argc,char**& argv,char**& appDefaults);
	
	/* Methods from Vrui::Application: */
	virtual void display(GLContextData& contextData) const;
	
	/* New methods: */
	PickResult pickPoint(const Point& queryPoint) const;
	PickResult pickPoint(const Ray& queryRay) const;
	};

/*********************************************************
Static elements of class ScreenCalibrator::PointQueryTool:
*********************************************************/

ScreenCalibrator::PointQueryToolFactory* ScreenCalibrator::PointQueryTool::factory=0;

/*************************************************
Methods of class ScreenCalibrator::PointQueryTool:
*************************************************/

void ScreenCalibrator::PointQueryTool::buttonCallback(int,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		/* Get pointer to input device that caused the event: */
		Vrui::InputDevice* device=getButtonDevice(0);
		
		size_t pickResult;
		Vrui::NavTrackerState transform=Vrui::getDeviceTransformation(device);
		if(device->isRayDevice())
			pickResult=application->pickPoint(Ray(transform.getOrigin(),transform.transform(device->getDeviceRayDirection())));
		else
			pickResult=application->pickPoint(transform.getOrigin());
		
		if(pickResult!=~PickResult(0))
			{
			/* Find what type of point this is: */
			if(pickResult<application->trackingPoints.size())
				std::cout<<"Tracking point "<<pickResult<<": "<<application->trackingPoints[pickResult]<<std::endl;
			else
				{
				pickResult-=application->trackingPoints.size();
				if(pickResult<application->floorPoints.size())
					std::cout<<"Floor point "<<pickResult<<": "<<application->floorPoints[pickResult]<<std::endl;
				else
					{
					pickResult-=application->floorPoints.size();
					if(pickResult<application->screenPoints.size())
						std::cout<<"Screen point "<<pickResult<<": "<<application->screenPoints[pickResult]<<std::endl;
					else
						{
						pickResult-=application->screenPoints.size();
						if(pickResult<application->ballPoints.size())
							std::cout<<"Ball point "<<pickResult<<": "<<application->ballPoints[pickResult]<<std::endl;
						}
					}
				}
			}
		}
	}

/*********************************
Methods of class ScreenCalibrator:
*********************************/

void ScreenCalibrator::readOptitrackSampleFile(const char* fileName,bool flipZ)
	{
	/* Open the CSV input file: */
	IO::TokenSource tok(IO::openFile(fileName));
	tok.setPunctuation(",\n");
	tok.setQuotes("\"");
	tok.skipWs();
	
	/* Read all point records from the file: */
	double lastTimeStamp=-Math::Constants<double>::min;
	Point::AffineCombiner pac;
	unsigned int numPoints=0;
	unsigned int line=1;
	while(!tok.eof())
		{
		/* Read a point record: */
		
		/* Read the marker index: */
		int markerIndex=atoi(tok.readNextToken());
		
		if(strcmp(tok.readNextToken(),",")!=0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing comma in line %u",line);
		
		/* Read the sample timestamp: */
		double timeStamp=atof(tok.readNextToken());
		
		/* Read the point position: */
		Point p;
		for(int i=0;i<3;++i)
			{
			if(strcmp(tok.readNextToken(),",")!=0)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing comma in line %u",line);
			
			p[i]=Scalar(atof(tok.readNextToken()));
			}
		
		if(flipZ)
			{
			/* Invert the z component to flip to a right-handed coordinate system: */
			p[2]=-p[2];
			}
		
		if(strcmp(tok.readNextToken(),"\n")!=0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Overlong point record in line %u",line);
		
		/* Check if the point record is valid: */
		if(markerIndex==1)
			{
			/* Check if this record started a new sampling sequence: */
			if(timeStamp>=lastTimeStamp+5.0)
				{
				/* Get the current average point position and reset the accumulator: */
				if(numPoints>0)
					{
					trackingPoints.push_back(pac.getPoint());
					pac.reset();
					numPoints=0;
					}
				}
			
			/* Add the point to the current accumulator: */
			pac.addPoint(p);
			++numPoints;
			
			lastTimeStamp=timeStamp;
			}
		
		++line;
		}
	
	/* Get the last average point position: */
	if(numPoints>0)
		trackingPoints.push_back(pac.getPoint());
	
	/* Cull duplicate points from the point list: */
	size_t numDupes=cullDuplicates(trackingPoints,Scalar(0.05));
	if(numDupes>0)
		std::cout<<"ScreenCalibrator::readOptitrackSampleFile: "<<numDupes<<" duplicate points culled from input file"<<std::endl;
	}

ScreenCalibrator::PointList ScreenCalibrator::readTotalstationSurveyFile(const char* fileName,const char* tag) const
	{
	/* Open the CSV input file: */
	IO::TokenSource tok(IO::openFile(fileName));
	tok.setPunctuation(",\n");
	tok.setQuotes("\"");
	tok.skipWs();
	
	/* Read point records until the end of file: */
	PointList result;
	unsigned int line=2;
	while(!tok.eof())
		{
		/* Read the point coordinates: */
		Point p;
		for(int i=0;i<3;++i)
			{
			if(i>0)
				{
				tok.readNextToken();
				if(!tok.isToken(","))
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Format error in input file %s",fileName);
				}
			p[i]=Scalar(atof(tok.readNextToken()));
			}
			
		tok.readNextToken();
		if(!tok.isToken(","))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Format error in input file %s",fileName);
		
		/* Read the point tag: */
		tok.readNextToken();
		if(tok.isCaseToken(tag))
			{
			/* Store the point: */
			result.push_back(p);
			}
		
		tok.readNextToken();
		if(!tok.isToken("\n"))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Format error in input file %s",fileName);
		
		++line;
		}
	
	/* Cull duplicate points from the point list: */
	size_t numDupes=cullDuplicates(result,Scalar(0.05));
	if(numDupes>0)
		std::cout<<"ScreenCalibrator::readTotalstationSurveyFile: "<<numDupes<<" duplicate points culled from input file"<<std::endl;
	
	return result;
	}

ScreenCalibrator::ScreenCalibrator(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 trackingPointsMover(0)
	{
	/* Create and register the point query tool class: */
	PointQueryToolFactory* pointQueryToolFactory=new PointQueryToolFactory("PointQueryTool","Point Query",0,*Vrui::getToolManager());
	pointQueryToolFactory->setNumButtons(1);
	pointQueryToolFactory->setButtonFunction(0,"Query Point");
	Vrui::getToolManager()->addClass(pointQueryToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Parse the command line: */
	const char* optitrackFileName=0;
	bool optitrackFlipZ=false;
	const char* totalstationFileName=0;
	int screenPixelSize[2]={-1,-1};
	int screenSquareSize=200;
	double unitScale=1.0;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"screenSize")==0)
				{
				for(int j=0;j<2;++j)
					{
					++i;
					screenPixelSize[j]=atoi(argv[i]);
					}
				}
			else if(strcasecmp(argv[i]+1,"squareSize")==0)
				{
				++i;
				screenSquareSize=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"metersToInches")==0)
				unitScale=1000.0/25.4;
			else if(strcasecmp(argv[i]+1,"unitScale")==0)
				{
				++i;
				unitScale=atof(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"flipZ")==0)
				optitrackFlipZ=true;
			else
				{
				}
			}
		else if(totalstationFileName==0)
			totalstationFileName=argv[i];
		else if(optitrackFileName==0)
			optitrackFileName=argv[i];
		else
			{
			}
		}
	
	/* Read the Optitrack sample file: */
	if(optitrackFileName!=0)
		{
		readOptitrackSampleFile(optitrackFileName,optitrackFlipZ);
		std::cout<<"Read "<<trackingPoints.size()<<" ball points from Optitrack sample file"<<std::endl;
		}
	
	/* Read relevant point classes from the Totalstation survey file: */
	if(totalstationFileName!=0)
		{
		screenPoints=readTotalstationSurveyFile(totalstationFileName,"SCREEN");
		floorPoints=readTotalstationSurveyFile(totalstationFileName,"FLOOR");
		ballPoints=readTotalstationSurveyFile(totalstationFileName,"BALLS");
		std::cout<<"Read "<<ballPoints.size()<<" ball points from TotalStation survey file"<<std::endl;
		}
	
	/*********************************************************************
	Establish a normalized coordinate system with the floor at the z=0
	plane, the screen in a plane about orthogonal to the y axis, and the
	screen center above the origin.
	*********************************************************************/
	
	/* Fit a plane to the floor points: */
	Geometry::PCACalculator<3> floorPca;
	for(PointList::const_iterator fpIt=floorPoints.begin();fpIt!=floorPoints.end();++fpIt)
		floorPca.accumulatePoint(*fpIt);
	Point floorCentroid=floorPca.calcCentroid();
	floorPca.calcCovariance();
	double floorEv[3];
	floorPca.calcEigenvalues(floorEv);
	Geometry::PCACalculator<3>::Vector floorNormal=floorPca.calcEigenvector(floorEv[2]);
	std::cout<<"Floor plane fitting residual: "<<floorEv[2]<<std::endl;
	
	/* Fit a plane to the screen points: */
	Geometry::PCACalculator<3> screenPca;
	for(PointList::const_iterator spIt=screenPoints.begin();spIt!=screenPoints.end();++spIt)
		screenPca.accumulatePoint(*spIt);
	Point screenCentroid=screenPca.calcCentroid();
	screenPca.calcCovariance();
	double screenEv[3];
	screenPca.calcEigenvalues(screenEv);
	Geometry::PCACalculator<3>::Vector screenNormal=screenPca.calcEigenvector(screenEv[2]);
	std::cout<<"Screen plane fitting residual: "<<screenEv[2]<<std::endl;
	std::cout<<std::endl;
	
	/* Flip the floor normal such that it points towards the screen points: */
	if((screenCentroid-floorCentroid)*floorNormal<Scalar(0))
		floorNormal=-floorNormal;
	
	/* Flip the screen normal such that it points away from the ball points: */
	Point::AffineCombiner ballC;
	for(PointList::const_iterator bpIt=ballPoints.begin();bpIt!=ballPoints.end();++bpIt)
		ballC.addPoint(*bpIt);
	if((ballC.getPoint()-screenCentroid)*screenNormal>Scalar(0))
		screenNormal=-screenNormal;
	
	/* Project the screen centroid onto the floor plane to get the coordinate system origin: */
	Point origin=screenCentroid-floorNormal*(((screenCentroid-floorCentroid)*floorNormal)/Geometry::sqr(floorNormal));
	
	/* Orthonormalize the screen normal against the floor normal: */
	Vector y=screenNormal-floorNormal*((screenNormal*floorNormal)/Geometry::sqr(floorNormal));
	Vector x=y^floorNormal;
	
	#if 0
	/* Calculate a rotation to align the floor normal with +z and the (horizontal) screen normal with +y: */
	ONTransform::Rotation rot=ONTransform::Rotation::fromBaseVectors(x,y);
	#endif
	
	/*********************************************************************
	Calculate a transformation to move the Totalstation survey points into
	the normalized coordinate system:
	*********************************************************************/
	
	ONTransform transform(origin-Point::origin,ONTransform::Rotation::fromBaseVectors(x,y));
	transform.doInvert();
	
	/* Transform all survey points: */
	for(PointList::iterator spIt=screenPoints.begin();spIt!=screenPoints.end();++spIt)
		*spIt=transform.transform(*spIt);
	for(PointList::iterator fpIt=floorPoints.begin();fpIt!=floorPoints.end();++fpIt)
		*fpIt=transform.transform(*fpIt);
	for(PointList::iterator bpIt=ballPoints.begin();bpIt!=ballPoints.end();++bpIt)
		*bpIt=transform.transform(*bpIt);
	
	if(screenPixelSize[0]>0&&screenPixelSize[1]>0&&screenSquareSize>0)
		{
		/*********************************************************************
		Calculate the optimal projective transformation and screen
		transformation (orthonormal transformation plus non-uniform scaling in
		x and y) from theoretical  screen points to surveyed screen points:
		*********************************************************************/
		
		/* Estimate the screen's width and height based on the surveyed screen points: */
		int numScreenPoints[2];
		for(int i=0;i<2;++i)
			numScreenPoints[i]=(screenPixelSize[i]-1)/screenSquareSize+1;
		if(screenPoints.size()!=size_t(numScreenPoints[0]*numScreenPoints[1]))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Wrong number of screen points, got %d instead of %d",int(screenPoints.size()),numScreenPoints[0]*numScreenPoints[1]);
		double screenWidthSum=0.0;
		for(int y=0;y<numScreenPoints[1];++y)
			screenWidthSum+=Geometry::dist(screenPoints[y*numScreenPoints[0]],screenPoints[y*numScreenPoints[0]+numScreenPoints[0]-1]);
		double screenWidth=screenWidthSum*double(screenPixelSize[0])/double((numScreenPoints[0]-1)*screenSquareSize*numScreenPoints[1]);
		double screenHeightSum=0.0;
		for(int x=0;x<numScreenPoints[0];++x)
			screenHeightSum+=Geometry::dist(screenPoints[x],screenPoints[(numScreenPoints[1]-1)*numScreenPoints[0]+x]);
		double screenHeight=screenHeightSum*double(screenPixelSize[1])/double((numScreenPoints[1]-1)*screenSquareSize*numScreenPoints[0]);
		std::cout<<"Estimated screen size: "<<screenWidth<<" x "<<screenHeight<<std::endl;
		
		/* Create a list of theoretical screen points: */
		int screenPixelOffset[2];
		for(int i=0;i<2;++i)
			screenPixelOffset[i]=((screenPixelSize[i]-1)%screenSquareSize)/2;
		PointList screen;
		for(int y=screenPixelOffset[1];y<screenPixelSize[1];y+=screenSquareSize)
			for(int x=screenPixelOffset[0];x<screenPixelSize[0];x+=screenSquareSize)
				screen.push_back(Point((Scalar(x)+Scalar(0.5))/Scalar(screenPixelSize[0]),Scalar(1)-(Scalar(y)+Scalar(0.5))/Scalar(screenPixelSize[1]),0));
		if(screen.size()!=screenPoints.size())
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Wrong number of screen points, got %d instead of %d",int(screenPoints.size()),int(screen.size()));
		
		/* Calculate an orthogonal pre-alignment transformation for the measured screen points to kickstart non-linear optimization: */
		PointList scaledScreen;
		for(PointList::iterator sIt=screen.begin();sIt!=screen.end();++sIt)
			scaledScreen.push_back(Point((*sIt)[0]*screenWidth,(*sIt)[1]*screenHeight,0));
		std::pair<Geometry::OrthogonalTransformation<double,3>,double> screenInitialFit=calculateOGTransform(scaledScreen,screenPoints);
		std::cout<<"Screen pre-alignment RMS residual: "<<screenInitialFit.second<<std::endl;
		std::cout<<"Screen pre-alignment transformation: "<<screenInitialFit.first<<std::endl;
		std::cout<<std::endl;
		
		/* Find the best-fitting screen transformation for the measured screen points: */
		ScreenTransformFitter stf(screen.size(),&screen[0],&screenPoints[0]);
		
		stf.setTransform(ScreenTransformFitter::Transform(screenInitialFit.first.getTranslation(),screenInitialFit.first.getRotation()));
		stf.setSize(0,screenWidth);
		stf.setSize(1,screenHeight);
		
		Geometry::LevenbergMarquardtMinimizer<ScreenTransformFitter> stMinimizer;
		stMinimizer.maxNumIterations=100000;
		ScreenTransformFitter::Scalar screenResult1=stMinimizer.minimize(stf);
		std::cout<<"Screen transformation RMS residual: "<<Math::sqrt(screenResult1/double(screen.size()))<<std::endl;
		screenTransform=stf.getTransform();
		screenSize[0]=stf.getSize(0);
		screenSize[1]=stf.getSize(1);
		std::cout<<"Optimal screen size: "<<screenSize[0]<<", "<<screenSize[1]<<std::endl;
		std::cout<<"Optimal screen origin: "<<screenTransform.getOrigin()<<std::endl;
		std::cout<<"Optimal horizontal screen axis: "<<screenTransform.getDirection(0)<<std::endl;
		std::cout<<"Optimal vertical screen axis: "<<screenTransform.getDirection(1)<<std::endl;
		std::cout<<std::endl;
		
		/* Find the best-fitting projective transformation for the measured screen points: */
		PTransformFitter ptf(screen.size(),&screen[0],&screenPoints[0]);
		
		ptf.setTransform(PTransformFitter::Transform(screenTransform)*PTransformFitter::Transform::scale(PTransformFitter::Transform::Scale(screenSize[0],screenSize[1],1.0)));
		
		Geometry::LevenbergMarquardtMinimizer<PTransformFitter> pMinimizer;
		pMinimizer.maxNumIterations=100000;
		PTransformFitter::Scalar screenResult2=pMinimizer.minimize(ptf);
		std::cout<<"Projective transformation RMS residual: "<<Math::sqrt(screenResult2/double(screen.size()))<<std::endl;
		pScreenTransform=ptf.getTransform();
		
		/* Print the screen transformation matrix: */
		std::cout<<"Projective transformation matrix:"<<std::endl;
		std::cout<<std::setprecision(6)<<pScreenTransform<<std::endl;
		std::cout<<std::endl;
		
		/*********************************************************************
		Calculate a homography matrix from the optimal screen transformation
		to the optimal projective transformation to correct screen
		misalignments:
		*********************************************************************/
		
		Point sCorners[4];
		Point pCorners[4];
		for(int i=0;i<4;++i)
			{
			sCorners[i][0]=i&0x1?screenSize[0]*unitScale:0.0;
			sCorners[i][1]=i&0x2?screenSize[1]*unitScale:0.0;
			sCorners[i][2]=0.0;
			pCorners[i][0]=i&0x1?1.0:0.0;
			pCorners[i][1]=i&0x2?1.0:0.0;
			pCorners[i][2]=0.0;
			pCorners[i]=screenTransform.inverseTransform(pScreenTransform.transform(pCorners[i]));
			pCorners[i][0]*=unitScale;
			pCorners[i][1]*=unitScale;
			}
		Geometry::ProjectiveTransformation<double,2> sHom=calcHomography(sCorners);
		Geometry::ProjectiveTransformation<double,2> pHom=calcHomography(pCorners);
		Geometry::ProjectiveTransformation<double,2> hom=pHom;
		hom.leftMultiply(Geometry::invert(sHom));
		for(int i=0;i<3;++i)
			for(int j=0;j<3;++j)
				hom.getMatrix()(i,j)/=hom.getMatrix()(2,2);
		
		#if 0
		std::cout<<"Homography matrix for projective transform: "<<pHom<<std::endl;
		std::cout<<"Homography matrix for screen transform: "<<sHom<<std::endl;
		std::cout<<"Screen correction homography matrix: "<<hom<<std::endl;
		std::cout<<std::endl;
		#endif
		
		#if 0
		
		/* Do some experiments: */
		Geometry::ProjectiveTransformation<double,3> hom3=Geometry::ProjectiveTransformation<double,3>::identity;
		for(int i=0;i<3;++i)
			for(int j=0;j<3;++j)
				hom3.getMatrix()(i<2?i:3,j<2?j:3)=hom.getMatrix()(i,j);
		hom3.getMatrix()(2,0)=hom3.getMatrix()(3,0);
		hom3.getMatrix()(2,1)=hom3.getMatrix()(3,1);
		
		std::cout<<hom3<<std::endl;
		std::cout<<Geometry::invert(hom3)<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>(-1.0,-1.0,-1.0,1.0)).toPoint()<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>( 1.0,-1.0,-1.0,1.0)).toPoint()<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>(-1.0, 1.0,-1.0,1.0)).toPoint()<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>( 1.0, 1.0,-1.0,1.0)).toPoint()<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>(-1.0,-1.0, 1.0,1.0)).toPoint()<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>( 1.0,-1.0, 1.0,1.0)).toPoint()<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>(-1.0, 1.0, 1.0,1.0)).toPoint()<<std::endl;
		std::cout<<hom3.transform(Geometry::HVector<double,3>( 1.0, 1.0, 1.0,1.0)).toPoint()<<std::endl;
		std::cout<<std::endl;
		
		#endif
		
		/* Print a configuration file section for the screen: */
		std::cout<<"Configuration settings for screen:"<<std::endl;
		std::cout<<"origin "<<screenTransform.getTranslation()*unitScale<<std::endl;
		std::cout<<"horizontalAxis "<<screenTransform.getDirection(0)<<std::endl;
		std::cout<<"width "<<screenSize[0]*unitScale<<std::endl;
		std::cout<<"verticalAxis "<<screenTransform.getDirection(1)<<std::endl;
		std::cout<<"height "<<screenSize[1]*unitScale<<std::endl;
		std::cout<<"offAxis true"<<std::endl;
		std::cout<<"homography ( ";
		for(int j=0;j<3;++j)
			{
			if(j>0)
				std::cout<<", \\"<<std::endl<<"             ";
			std::cout<<"( ";
			for(int i=0;i<3;++i)
				{
				if(i>0)
					std::cout<<", ";
				std::cout<<pHom.getMatrix()(i,j);
				}
			std::cout<<" )";
			}
		std::cout<<" )"<<std::endl;
		std::cout<<std::endl;
		}
	
	if(optitrackFileName!=0&&totalstationFileName!=0)
		{
		/*********************************************************************
		Calculate the optimal orthogonal transformation from tracking system
		coordinates to the normalized coordinate system by aligning ball
		positions observed by the tracking system with ball positions measured
		using the total station:
		*********************************************************************/
		
		/* Find an orthonormal transformation to align the tracking points with the ball points: */
		std::pair<Geometry::OrthogonalTransformation<double,3>,double> trackingInitialFit=calculateOGTransform(trackingPoints,ballPoints);
		std::cout<<"Tracking pre-alignment RMS residual: "<<trackingInitialFit.second<<std::endl;
		std::cout<<"Tracking pre-alignment transformation: "<<trackingInitialFit.first<<std::endl;
		std::cout<<std::endl;
		
		size_t numPoints=Math::min(trackingPoints.size(),ballPoints.size());
		OGTransformFitter ogtf(numPoints,&trackingPoints[0],&ballPoints[0]);
		ogtf.setTransform(OGTransformFitter::Transform(trackingInitialFit.first));
		
		Geometry::LevenbergMarquardtMinimizer<OGTransformFitter> ogMinimizer;
		ogMinimizer.maxNumIterations=100000;
		OGTransformFitter::Scalar result=ogMinimizer.minimize(ogtf);
		OGTransform tsCal=ogtf.getTransform();
		
		std::cout<<"Tracking system calibration RMS residual: "<<Math::sqrt(result/double(numPoints))<<std::endl;
		std::cout<<"Tracking system calibration transformation: "<<tsCal<<std::endl;
		std::cout<<std::endl;
		
		std::cout<<"Configuration settings for tracking calibrator: "<<std::endl;
		std::cout<<"transformation translate "<<tsCal.getTranslation()*unitScale<<" \\"<<std::endl;
		std::cout<<"               * scale "<<unitScale*tsCal.getScaling()<<" \\"<<std::endl;
		std::cout<<"               * rotate "<<tsCal.getRotation().getAxis()<<", "<<Math::deg(tsCal.getRotation().getAngle())<<std::endl;
		
		/* Transform the tracking points with the result transformation: */
		for(PointList::iterator tpIt=trackingPoints.begin();tpIt!=trackingPoints.end();++tpIt)
			*tpIt=tsCal.transform(*tpIt);
		}
	
	/* Initialize the navigation transformation: */
	Geometry::Box<Scalar,3> bbox=Geometry::Box<Scalar,3>::empty;
	for(PointList::const_iterator tpIt=trackingPoints.begin();tpIt!=trackingPoints.end();++tpIt)
		bbox.addPoint(*tpIt);
	for(PointList::const_iterator spIt=screenPoints.begin();spIt!=screenPoints.end();++spIt)
		bbox.addPoint(*spIt);
	for(PointList::const_iterator fpIt=floorPoints.begin();fpIt!=floorPoints.end();++fpIt)
		bbox.addPoint(*fpIt);
	for(PointList::const_iterator bpIt=ballPoints.begin();bpIt!=ballPoints.end();++bpIt)
		bbox.addPoint(*bpIt);
	
	Vrui::setNavigationTransformation(Geometry::mid(bbox.min,bbox.max),Geometry::dist(bbox.min,bbox.max));
	
	/* Create a virtual input device to move the tracking points interactively: */
	trackingPointsMover=Vrui::addVirtualInputDevice("TrackingPointsMover",0,0);
	// Vrui::getInputGraphManager()->setNavigational(trackingPointsMover,true);
	Vrui::NavTrackerState scaledDeviceT=Vrui::getInverseNavigationTransformation();
	scaledDeviceT*=trackingPointsMover->getTransformation();
	trackingPointsTransform=Vrui::TrackerState(scaledDeviceT.getTranslation(),scaledDeviceT.getRotation());
	trackingPointsTransform.doInvert();
	}

void ScreenCalibrator::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glPointSize(3.0f);
	
	/* Get the tracking point mover's transformation: */
	Vrui::NavTrackerState scaledDeviceT=Vrui::getInverseNavigationTransformation();
	scaledDeviceT*=trackingPointsMover->getTransformation();
	
	/* Calculate the point transformation: */
	Vrui::TrackerState pmt=Vrui::TrackerState(scaledDeviceT.getTranslation(),scaledDeviceT.getRotation());
	pmt*=trackingPointsTransform;
	pmt=Vrui::TrackerState::identity;
	
	/* Draw all tracking and survey points: */
	glBegin(GL_POINTS);
	glColor3f(1.0f,1.0f,0.0f);
	for(PointList::const_iterator tpIt=trackingPoints.begin();tpIt!=trackingPoints.end();++tpIt)
		glVertex(pmt.transform(*tpIt));
	glColor3f(0.0f,1.0f,0.0f);
	for(PointList::const_iterator spIt=screenPoints.begin();spIt!=screenPoints.end();++spIt)
		glVertex(*spIt);
	glColor3f(1.0f,0.0f,0.0f);
	for(PointList::const_iterator fpIt=floorPoints.begin();fpIt!=floorPoints.end();++fpIt)
		glVertex(*fpIt);
	glColor3f(1.0f,0.0f,1.0f);
	for(PointList::const_iterator bpIt=ballPoints.begin();bpIt!=ballPoints.end();++bpIt)
		glVertex(*bpIt);
	glEnd();
	
	/* Draw all tracker calibration pairs: */
	size_t numPoints=trackingPoints.size();
	if(numPoints>ballPoints.size())
		numPoints=ballPoints.size();
	glBegin(GL_LINES);
	for(size_t i=0;i<numPoints;++i)
		{
		glColor3f(1.0f,1.0f,0.0f);
		glVertex(pmt.transform(trackingPoints[i]));
		glColor3f(1.0f,0.0f,1.0f);
		glVertex(ballPoints[i]);
		}
	glEnd();
	
	/* Draw the screen rectangle: */
	glBegin(GL_LINE_LOOP);
	glColor3f(0.0f,1.0f,0.0f);
	glVertex(screenTransform.transform(Point(0,0,0)));
	glVertex(screenTransform.transform(Point(screenSize[0],0,0)));
	glVertex(screenTransform.transform(Point(screenSize[0],screenSize[1],0)));
	glVertex(screenTransform.transform(Point(0,screenSize[1],0)));
	glEnd();
	
	/* Draw the projected screen quadrangle: */
	glBegin(GL_LINE_LOOP);
	glColor3f(0.0f,0.0f,1.0f);
	glVertex(pScreenTransform.transform(Point(0,0,0)));
	glVertex(pScreenTransform.transform(Point(1,0,0)));
	glVertex(pScreenTransform.transform(Point(1,1,0)));
	glVertex(pScreenTransform.transform(Point(0,1,0)));
	glEnd();
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

ScreenCalibrator::PickResult ScreenCalibrator::pickPoint(const Point& queryPoint) const
	{
	/* Create a point picker: */
	Geometry::PointPicker<Scalar,3> picker(queryPoint,Scalar(Vrui::getPointPickDistance()));
	
	/* Process all points: */
	for(PointList::const_iterator pIt=trackingPoints.begin();pIt!=trackingPoints.end();++pIt)
		picker(*pIt);
	for(PointList::const_iterator pIt=floorPoints.begin();pIt!=floorPoints.end();++pIt)
		picker(*pIt);
	for(PointList::const_iterator pIt=screenPoints.begin();pIt!=screenPoints.end();++pIt)
		picker(*pIt);
	for(PointList::const_iterator pIt=ballPoints.begin();pIt!=ballPoints.end();++pIt)
		picker(*pIt);
	
	/* Return the index of the picked point: */
	if(picker.havePickedPoint())
		return picker.getPickIndex();
	else
		return ~PickResult(0);
	}

ScreenCalibrator::PickResult ScreenCalibrator::pickPoint(const Ray& queryRay) const
	{
	/* Create a ray picker: */
	Geometry::RayPicker<Scalar,3> picker(queryRay,Scalar(Vrui::getRayPickCosine()));
	
	/* Process all points: */
	for(PointList::const_iterator pIt=trackingPoints.begin();pIt!=trackingPoints.end();++pIt)
		picker(*pIt);
	for(PointList::const_iterator pIt=floorPoints.begin();pIt!=floorPoints.end();++pIt)
		picker(*pIt);
	for(PointList::const_iterator pIt=screenPoints.begin();pIt!=screenPoints.end();++pIt)
		picker(*pIt);
	for(PointList::const_iterator pIt=ballPoints.begin();pIt!=ballPoints.end();++pIt)
		picker(*pIt);
	
	/* Return the index of the picked point: */
	if(picker.havePickedPoint())
		return picker.getPickIndex();
	else
		return ~PickResult(0);
	}

int main(int argc,char* argv[])
	{
	char** appDefaults=0;
	ScreenCalibrator app(argc,argv,appDefaults);
	app.run();
	
	return 0;
	}

/***********************************************************************
AlignPoints - Utility to align two sets of measurements of the same set
of points using one of several types of transformations.
Copyright (c) 2009-2025 Oliver Kreylos

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

#include <string.h>
#include <vector>
#include <iostream>
#include <Misc/StdError.h>
#include <Misc/FunctionCalls.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CommandLineParser.icpp>
#include <IO/ValueSource.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Math/LevenbergMarquardtMinimizer.h>
#include <Math/RanSaC.h>
#include <Geometry/Point.h>
#include <Geometry/ValuedPoint.h>
#include <Geometry/Box.h>
#include <Geometry/GeometryValueCoders.h>
#include <Geometry/PointAlignerONTransform.h>
#include <Geometry/PointAlignerOGTransform.h>
#include <Geometry/PointAlignerATransform.h>
#include <Geometry/PointAlignerPTransform.h>
#include <Geometry/RanSaCPointAligner.h>
#include <GL/gl.h>
#include <GL/GLNumberRenderer.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/UIManager.h>
#include <Vrui/ObjectSnapperTool.h>

/******************************************
Abstract base class for point set aligners:
******************************************/

class AlignerBase
	{
	/* Embedded classes: */
	public:
	typedef double Scalar; // Scalar type for point spaces
	typedef Geometry::Point<Scalar,3> Point; // Point type
	typedef Geometry::ValuedPoint<Point,bool> VPoint; // Type for points with a "valid" flag
	typedef std::vector<VPoint> PointList; // Type for lists of points
	typedef Geometry::OrthogonalTransformation<Scalar,3> Transformation; // Type for point set transformations
	
	/* Elements: */
	protected:
	PointList froms,tos; // Lists of points to align
	Scalar rms,max; // L-2 and L-infinity alignment residuals in "to" point space
	GLNumberRenderer numberRenderer; // Helper object to label points with indices
	
	/* Private methods: */
	static void readPointFile(const char* fileName,PointList& points); // Reads points from a file of the given name into the given point list
	
	/* Constructors and destructors: */
	public:
	AlignerBase(void); // Creates an empty aligner
	virtual ~AlignerBase(void);
	
	/* Methods: */
	virtual void readPointSets(const char* fromFileName,const char* toFileName); // Loads "from" and "to" point sets from the files of the given names
	virtual void transformPoints(int which,const Transformation& transform); // Transforms one of the point sets (0: froms, 1: tos) with the given transformation
	virtual void align(void) =0; // Calculates an alignment transformation between the two point sets
	Scalar getRms(void) const // Returns the L-2 norm alignment residual
		{
		return rms;
		}
	Scalar getMax(void) const // Returns the L-infinity norm alignment residual
		{
		return max;
		}
	virtual void objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest) =0; // Callback called when an object snapper tool issues a snap request
	virtual void resetNavigation(void) =0; // Resets Vrui's navigation transformation based on current alignment state
	virtual void glRenderAction(GLContextData& contextData) const =0; // Renders the aligned point sets into the given OpenGL context
	};

/****************************
Methods of class AlignerBase:
****************************/

void AlignerBase::readPointFile(const char* fileName,AlignerBase::PointList& points)
	{
	/* Open the input file: */
	IO::ValueSource reader(IO::openFile(fileName));
	reader.setWhitespace(',',true);
	reader.setPunctuation('\n',true);
	
	/* Read points until end of file: */
	reader.skipWs();
	while(!reader.eof())
		{
		/* Read the next point and check its validity: */
		VPoint p;
		p.value=true;
		try
			{
			/* Read the point's coordinate components: */
			for(int i=0;i<3;++i)
				p[i]=Scalar(reader.readNumber());
			}
		catch(const IO::ValueSource::NumberError&)
			{
			/* Invalidate the point: */
			p.value=false;
			}
		points.push_back(p);
		
		/* Skip to the start of the next line: */
		reader.skipLine();
		reader.skipWs();
		}
	}

AlignerBase::AlignerBase(void)
	:rms(0),max(0),
	 numberRenderer(GLfloat(Vrui::getUiSize())*2.0f,true)
	{
	}

AlignerBase::~AlignerBase(void)
	{
	}

void AlignerBase::readPointSets(const char* fromFileName,const char* toFileName)
	{
	/* Read "from" and "to" point sets from the two given files: */
	readPointFile(fromFileName,froms);
	readPointFile(toFileName,tos);
	}

void AlignerBase::transformPoints(int which,const AlignerBase::Transformation& transform)
	{
	/* Transform the "from" or "to" point set: */
	PointList& points=which==0?froms:tos;
	for(PointList::iterator pIt=points.begin();pIt!=points.end();++pIt)
		*pIt=transform.transform(*pIt);
	}

/***********************************************************************
Generic abstract base class for point set aligners using a specific
transformation type:
***********************************************************************/

template <class TransformParam>
class AlignerTransformBase:public AlignerBase
	{
	/* Embedded classes: */
	public:
	using typename AlignerBase::Scalar;
	using typename AlignerBase::Point;
	using typename AlignerBase::VPoint;
	using typename AlignerBase::PointList;
	typedef TransformParam Transform; // Type of transformation
	
	/* Elements: */
	protected:
	using AlignerBase::froms;
	using AlignerBase::tos;
	using AlignerBase::numberRenderer;
	Transform transform; // The current transformation
	
	/* Constructors and destructors: */
	public:
	AlignerTransformBase(void)
		:transform(Transform::identity)
		{
		}
	
	/* Methods from AlignerBase: */
	virtual void objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest);
	virtual void resetNavigation(void);
	virtual void glRenderAction(GLContextData& contextData) const;
	};

/*************************************
Methods of class AlignerTransformBase:
*************************************/

template <class TransformParam>
inline
void
AlignerTransformBase<TransformParam>::objectSnapCallback(
	Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest)
	{
	/* Check all transformed "from" points: */
	for(PointList::const_iterator fromIt=froms.begin();fromIt!=froms.end();++fromIt)
		if(fromIt->value)
			snapRequest.snapPoint(Vrui::Point(transform.transform(*fromIt)));
	
	/* Check all "to" points: */
	for(PointList::const_iterator toIt=tos.begin();toIt!=tos.end();++toIt)
		if(toIt->value)
			snapRequest.snapPoint(Vrui::Point(*toIt));
	}

template <class TransformParam>
inline
void
AlignerTransformBase<TransformParam>::resetNavigation(
	void)
	{
	/* Calculate the joint bounding box of the transformed "from" and the "to" point sets: */
	Geometry::Box<Scalar,3> bbox=Geometry::Box<Scalar,3>::empty;
	for(PointList::const_iterator fromIt=froms.begin();fromIt!=froms.end();++fromIt)
		if(fromIt->value)
			bbox.addPoint(transform.transform(*fromIt));
	for(PointList::const_iterator toIt=tos.begin();toIt!=tos.end();++toIt)
		if(toIt->value)
			bbox.addPoint(*toIt);
	
	/* Calculate the bounding box's center point and size: */
	Vrui::Point center;
	Vrui::Scalar size(0);
	for(int i=0;i<3;++i)
		{
		center[i]=Vrui::Scalar(Math::mid(bbox.min[i],bbox.max[i]));
		size+=Vrui::Scalar(Math::sqr(bbox.max[i]-bbox.min[i]));
		}
	size=Math::sqrt(size);
	
	/* Set the navigation transformation: */
	Vrui::setNavigationTransformation(center,size);
	}

template <class TransformParam>
inline
void
AlignerTransformBase<TransformParam>::glRenderAction(
	GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	glPointSize(3.0f);
	
	/* Draw the transformed "from" points: */
	glBegin(GL_POINTS);
	glColor3f(0.0f,1.0f,0.0f);
	for(typename PointList::const_iterator fromIt=froms.begin();fromIt!=froms.end();++fromIt)
		if(fromIt->value)
			glVertex(transform.transform(*fromIt));
	glEnd();
	
	/* Draw the "to" points: */
	glBegin(GL_POINTS);
	glColor3f(1.0f,0.0f,1.0f);
	for(typename PointList::const_iterator toIt=tos.begin();toIt!=tos.end();++toIt)
		if(toIt->value)
			glVertex(*toIt);
	glEnd();
	
	/* Draw connections between all pairs of valid points: */
	glBegin(GL_LINES);
	for(size_t index=0;index<froms.size()&&index<tos.size();++index)
		if(froms[index].value&&tos[index].value)
			{
			glColor3f(0.0f,1.0f,0.0f);
			glVertex(transform.transform(froms[index]));
			glColor3f(1.0f,0.0f,1.0f);
			glVertex(tos[index]);
			}
	glEnd();
	
	/* Go to physical space to label the point sets: */
	Vrui::goToPhysicalSpace(contextData);
	
	/* Label the transformed "from" points: */
	glColor3f(0.0f,1.0f,0.0f);
	for(size_t index=0;index<froms.size();++index)
		if(froms[index].value)
			{
			glPushMatrix();
			Vrui::Point p(transform.transform(froms[index]));
			p=Vrui::getNavigationTransformation().transform(p);
			glMultMatrix(Vrui::getUiManager()->calcHUDTransform(p));
			GLNumberRenderer::Vector pos(0.0f,GLfloat(Vrui::getUiSize()),0.0f);
			numberRenderer.drawNumber(pos,(unsigned int)index,contextData,0,-1);
			glPopMatrix();
			}
	
	#if 1
	
	/* Label the "to" points: */
	glColor3f(1.0f,0.0f,1.0f);
	for(size_t index=0;index<tos.size();++index)
		if(tos[index].value)
			{
			glPushMatrix();
			Vrui::Point p(tos[index]);
			p=Vrui::getNavigationTransformation().transform(p);
			glMultMatrix(Vrui::getUiManager()->calcHUDTransform(p));
			GLNumberRenderer::Vector pos(0.0f,GLfloat(Vrui::getUiSize()),0.0f);
			numberRenderer.drawNumber(pos,(unsigned int)index,contextData,0,-1);
			glPopMatrix();
			}
	
	#endif
	
	/* Return to navigational space: */
	glPopMatrix();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

/*********************************************************************
Generic class for point set aligners using a point set aligner kernel:
*********************************************************************/

template <class PointAlignerParam>
class Aligner:public AlignerTransformBase<typename PointAlignerParam::Transform>
	{
	/* Embedded classes: */
	public:
	using typename AlignerBase::Scalar;
	using typename AlignerBase::Point;
	using typename AlignerBase::VPoint;
	using typename AlignerBase::PointList;
	using typename AlignerTransformBase<typename PointAlignerParam::Transform>::Transform;
	typedef PointAlignerParam PointAligner; // Type of aligner
	typedef typename PointAligner::PointPairList PointPairList; // Type for lists of pairs of points
	
	/* Elements: */
	private:
	using AlignerBase::froms;
	using AlignerBase::tos;
	using AlignerBase::rms;
	using AlignerBase::max;
	using AlignerTransformBase<typename PointAlignerParam::Transform>::transform;
	PointAligner aligner; // A point set alignment object
	
	/* Constructors and destructors: */
	public:
	Aligner(void)
		{
		}
	
	/* Methods from AlignerBase: */
	virtual void align(void);
	};

/**************************************
Methods of class Aligner<AlignerParam>:
**************************************/

template <class PointAlignerParam>
inline
void
Aligner<PointAlignerParam>::align(
	void)
	{
	/* Stuff all valid pairs of points into the aligner: */
	const PointList& fs=froms;
	const PointList& ts=tos;
	for(size_t pi=0;pi<fs.size()&&pi<ts.size();++pi)
		if(fs[pi].value&&ts[pi].value)
			aligner.addPointPair(fs[pi],ts[pi]);
	
	/* Condition the point sets to increase numerical stability: */
	aligner.condition();
	
	/* Estimate an initial alignment transformation: */
	aligner.estimateTransform();
	
	/* Refine the transformation through iterative optimization: */
	Math::LevenbergMarquardtMinimizer<PointAligner> minimizer;
	minimizer.maxNumIterations=10000;
	minimizer.minimize(aligner);
	
	/* Retrieve the alignment transformation: */
	transform=aligner.getTransform();
	std::cout<<"Alignment transformation: "<<Misc::ValueCoder<Transform>::encode(transform)<<std::endl;
	
	/* Calculate the alignment residual norms: */
	std::pair<Scalar,Scalar> residuals=aligner.calcResidualToSpace(transform);
	rms=residuals.first;
	max=residuals.second;
	std::cout<<"Alignment residuals: "<<rms<<" RMS, "<<max<<" max"<<std::endl;
	}

/***********************************************************************
Generic class for point set aligners using a point set aligner kernel
and RanSaC iteration to ignore outliers:
***********************************************************************/

template <class PointAlignerParam>
class RanSaCAligner:public AlignerTransformBase<typename PointAlignerParam::Transform>
	{
	/* Embedded classes: */
	public:
	using typename AlignerBase::Scalar;
	using typename AlignerBase::Point;
	using typename AlignerBase::VPoint;
	using typename AlignerBase::PointList;
	using typename AlignerTransformBase<typename PointAlignerParam::Transform>::Transform;
	typedef PointAlignerParam PointAligner; // Type of aligner
	typedef typename PointAligner::PointPair PointPair; // Type for pairs of points
	typedef typename PointAligner::PointPairList PointPairList; // Type for lists of pairs of points
	typedef Geometry::RanSaCPointAligner<PointAligner,Math::LevenbergMarquardtMinimizer> RPointAligner; // Adapter type to align points via RanSaC
	typedef Math::RanSaC<RPointAligner> RanSaCer; // Type for RanSaC algorithm
	
	/* Elements: */
	private:
	using AlignerBase::froms;
	using AlignerBase::tos;
	using AlignerBase::rms;
	using AlignerBase::max;
	using AlignerTransformBase<typename PointAlignerParam::Transform>::transform;
	RPointAligner aligner; // A point set alignment object using RanSaC
	RanSaCer ransacer; // A RanSaC algorithm
	
	/* Constructors and destructors: */
	public:
	RanSaCAligner(size_t sMaxNumIterations,Scalar sMaxInlierDist)
		:ransacer(sMaxNumIterations,Math::sqr(sMaxInlierDist),0.0)
		{
		}
	
	/* Methods from AlignerBase: */
	virtual void align(void);
	};

/******************************
Methods of class RanSaCAligner:
******************************/

template <class PointAlignerParam>
inline void
RanSaCAligner<PointAlignerParam>::align(
	void)
	{
	/* Stuff all valid pairs of points into the RanSaC aligner: */
	const PointList& fs=froms;
	const PointList& ts=tos;
	for(size_t pi=0;pi<fs.size()&&pi<ts.size();++pi)
		if(fs[pi].value&&ts[pi].value)
			ransacer.addDataPoint(PointPair(fs[pi],ts[pi]));
	
	/* Fit a model via RanSaC: */
	ransacer.fitModel(aligner);
	
	/* Retrieve the alignment transformation: */
	transform=ransacer.getModel();
	std::cout<<"Alignment transformation: "<<Misc::ValueCoder<Transform>::encode(transform)<<std::endl;
	std::cout<<"Number of inlier points: "<<ransacer.getNumInliers()<<" ("<<Scalar(ransacer.getNumInliers())*Scalar(100)/Scalar(ransacer.getDataPoints().size())<<"%)"<<std::endl;
	
	/* Calculate the alignment residual norms: */
	rms=Math::sqrt(ransacer.getSqrResidual()/Scalar(ransacer.getNumInliers()));
	max=Scalar(0);
	std::cout<<"Alignment residual: "<<rms<<" RMS"<<std::endl;
	}

/**********************
Main application class:
**********************/

class AlignPoints:public Vrui::Application
	{
	/* Elements: */
	AlignerBase* aligner; // Pointer to the generic point set aligner
	
	/* Constructors and destructors: */
	public:
	AlignPoints(int& argc,char**& argv);
	virtual ~AlignPoints(void);
	
	/* Methods from Vrui::Application: */
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

AlignPoints::AlignPoints(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 aligner(0)
	{
	/* Parse the command line: */
	Misc::CommandLineParser cmdLine;
	cmdLine.setDescription("Utility to calculate an alignment transformation between two sets of matching points.");
	cmdLine.setArguments("<from point set file name> <to point set file name>","Names of the files from which to read the \"from\" point set and the \"to\" point set, respectively. The resulting transformation will transform the \"from\" set to the \"to\" set.");
	const char* transformModeNames[]=
		{
		"ON","OG","A","P"
		};
	unsigned int transformMode=0;
	cmdLine.addCategoryOption("transformMode","tm",4,transformModeNames,transformMode,"Selects the type of alignment transformation between orthonormal, orthogonal, affine, or projective.");
	cmdLine.addFixedValueOption(0,"on",0U,transformMode,"Selects an orthonormal alignment transformation.");
	cmdLine.addFixedValueOption(0,"og",1U,transformMode,"Selects an orthogonal alignment transformation.");
	cmdLine.addFixedValueOption(0,"a",2U,transformMode,"Selects an affine alignment transformation.");
	cmdLine.addFixedValueOption(0,"p",3U,transformMode,"Selects a projective alignment transformation.");
	AlignerBase::Transformation fromTransform,toTransform;
	cmdLine.addValueOption("fromTransform","fromt",fromTransform,"<transformation string>","Sets a pre-alignment transformation to apply to the \"from\" point set.");
	cmdLine.addValueOption("toTransform","tot",toTransform,"<transformation string>","Sets a pre-alignment transformation to apply to the \"to\" point set.");
	double ransacParams[2]={0.0,0.0};
	cmdLine.addArrayOption("ransac","ransac",2,ransacParams,"<num iterations> <max inlier dist>","Selects RANSAC optimization with the given number of iterators and maximum inlier distance.");
	std::vector<std::string> fileNames;
	cmdLine.addArgumentsToList(fileNames);
	cmdLine.parse(argv,argv+argc);
	if(cmdLine.hadHelp())
		{
		Vrui::shutdown();
		return;
		}
	
	/* Check if exactly two point set file names were provided: */
	if(fileNames.size()==0)
		throw Misc::makeStdErr(0,"No point set file names provided");
	else if(fileNames.size()==1)
		throw Misc::makeStdErr(0,"No \"to\" point set file name provided");
	else if(fileNames.size()>2)
		throw Misc::makeStdErr(0,"Too many point set file names provided");
	
	/* Create a point set aligner of the requested type: */
	unsigned int ransacNumIterations=Math::floor(ransacParams[0]+0.5);
	if(ransacNumIterations>0)
		{
		switch(transformMode)
			{
			case 0:
				aligner=new RanSaCAligner<Geometry::PointAlignerONTransform<double,3> >(ransacNumIterations,ransacParams[1]);
				break;
			
			case 1:
				aligner=new RanSaCAligner<Geometry::PointAlignerOGTransform<double,3> >(ransacNumIterations,ransacParams[1]);
				break;
			
			case 2:
				aligner=new RanSaCAligner<Geometry::PointAlignerATransform<double,3> >(ransacNumIterations,ransacParams[1]);
				break;
			
			case 3:
				aligner=new RanSaCAligner<Geometry::PointAlignerPTransform<double,3> >(ransacNumIterations,ransacParams[1]);
				break;
			}
		}
	else
		{
		switch(transformMode)
			{
			case 0:
				aligner=new Aligner<Geometry::PointAlignerONTransform<double,3> >;
				break;
			
			case 1:
				aligner=new Aligner<Geometry::PointAlignerOGTransform<double,3> >;
				break;
			
			case 2:
				aligner=new Aligner<Geometry::PointAlignerATransform<double,3> >;
				break;
			
			case 3:
				aligner=new Aligner<Geometry::PointAlignerPTransform<double,3> >;
				break;
			}
		}
	
	/* Load the point set files: */
	aligner->readPointSets(fileNames[0].c_str(),fileNames[1].c_str());
	
	/* Pre-transform the point sets if requested: */
	if(fromTransform!=AlignerBase::Transformation::identity)
		aligner->transformPoints(0,fromTransform);
	if(toTransform!=AlignerBase::Transformation::identity)
		aligner->transformPoints(1,toTransform);
	
	/* Align the point sets: */
	aligner->align();
	
	/* Register a callback with the object snapper tool class: */
	Vrui::ObjectSnapperTool::addSnapCallback(Misc::createFunctionCall(aligner,&AlignerBase::objectSnapCallback));
	}

AlignPoints::~AlignPoints(void)
	{
	/* Destroy the point set aligner: */
	delete aligner;
	}

void AlignPoints::display(GLContextData& contextData) const
	{
	/* Let the point set aligner render its state: */
	aligner->glRenderAction(contextData);
	}


void AlignPoints::resetNavigation(void)
	{
	/* Let the point set aligner handle it: */
	aligner->resetNavigation();
	}

VRUI_APPLICATION_RUN(AlignPoints)

/***********************************************************************
PenPadCalibratorBSpline - Class to calibrate a pen pad's position
using a tensor-product B-Spline.
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

#include <Vrui/Internal/PenPadCalibratorBSpline.h>

#include <vector>
#include <Misc/Utility.h>
#include <Misc/StdError.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Matrix.h>
#include <Geometry/GeometryValueCoders.h>

namespace Vrui {

/****************************************
Methods of class PenPadCalibratorBSpline:
****************************************/

Scalar PenPadCalibratorBSpline::bspline(unsigned int i,unsigned int n,Scalar t) const
	{
	/* Initialize the evaluation array: */
	for(unsigned int j=0;j<=n;++j)
		coxDeBoor[j]=t>=Scalar(i+j)&&t<Scalar(i+j+1)?1.0:0.0;
	
	/* Use dynamic programming on the Cox-de Boor formulation: */
	for(unsigned int deg=1;deg<=n;++deg)
		{
		/* Raise the degree: */
		for(unsigned int j=0;j<=n-deg+1;++j)
			coxDeBoor[j]=((t-Scalar(i+j))*coxDeBoor[j]+(Scalar(i+j+deg+1)-t)*coxDeBoor[j+1])/Scalar(deg);
		}
	
	/* Return the final result: */
	return coxDeBoor[0];
	}

PenPadCalibratorBSpline::PenPadCalibratorBSpline(const PenPadCalibratorBSpline::Size& sDegree,const PenPadCalibratorBSpline::Size& sSize,const PenPadCalibrator::TiePointList& tiePoints,const PenPadCalibrator::Box2& rawDomain,Misc::ConfigurationFileSection& configFileSection)
	:degree(sDegree),size(sSize),
	 mesh(0),coxDeBoor(0),xs(0),ys(0)
	{
	/* Check the degree and mesh size for validity: */
	if(degree[0]<1||degree[1]<1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid B-Spline degree (%u, %u)",degree[0],degree[1]);
	if(size[0]<degree[0]+1||size[1]<degree[1]+1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid B-Spline mesh size (%u, %u) for degree (%u, %u)",size[0],size[1],degree[0],degree[1]);
	
	/* Calculate the raw measurement transformation coefficients: */
	for(int i=0;i<2;++i)
		{
		rawScale[i]=Scalar(size[i]-degree[i])/Scalar(rawDomain.max[i]-rawDomain.min[i]);
		rawOffset[i]=Scalar(degree[i])-Scalar(rawDomain.min[i])*rawScale[i];
		}
	
	/* Allocate the mesh and evaluation buffers: */
	mesh=new Point2[size.volume()];
	coxDeBoor=new Scalar[Misc::max(degree[0],degree[1])+1];
	
	/* Create the least-squares calibration system: */
	unsigned int eqSize(size.volume());
	Math::Matrix ata(eqSize,eqSize,0.0);
	Math::Matrix atb(eqSize,2,0.0);
	double* eq=new double[eqSize];
	for(TiePointList::const_iterator tpIt=tiePoints.begin();tpIt!=tiePoints.end();++tpIt)
		{
		/* Transform the measured point to B-Spline mesh space: */
		Scalar mx=Scalar(tpIt->raw[0])*rawScale[0]+rawOffset[0];
		Scalar my=Scalar(tpIt->raw[1])*rawScale[1]+rawOffset[1];
		
		/* Enter the measured point's B-spline weights into the calibration system: */
		for(unsigned int i=0;i<size[1];++i)
			for(unsigned int j=0;j<size[0];++j)
				eq[i*size[0]+j]=bspline(i,degree[1],my)*bspline(j,degree[0],mx);
		for(unsigned int i=0;i<eqSize;++i)
			{
			for(unsigned int j=0;j<eqSize;++j)
				ata(i,j)+=eq[i]*eq[j];
			for(int j=0;j<2;++j)
				atb(i,j)+=eq[i]*tpIt->screen[j];
			}
		}
	delete[] eq;
	
	/* Solve the system: */
	Math::Matrix x=atb;
	x.divideFullPivot(ata);
	
	/* Extract the control point mesh: */
	for(unsigned int i=0;i<size[1];++i)
		for(unsigned int j=0;j<size[0];++j)
			for(int k=0;k<2;++k)
				mesh[i*size[0]+j][k]=Scalar(x(i*size[0]+j,k));
	try
		{
		/* Write the type of this calibrator: */
		configFileSection.storeString("./calibratorType","BSpline");
		
		/* Write the B-Spline degree and size: */
		configFileSection.storeValue<Size>("./degree",degree);
		configFileSection.storeValue<Size>("./size",size);
		
		/* Write the B-Spline control point mesh: */
		typedef std::vector<Point2> MeshRow;
		typedef std::vector<MeshRow> Mesh;
		Mesh tempMesh;
		tempMesh.reserve(size[1]);
		for(unsigned int y=0;y<size[1];++y)
			{
			MeshRow tempRow;
			tempRow.reserve(size[0]);
			for(unsigned int x=0;x<size[0];++x)
				tempRow.push_back(mesh[y*size[0]+x]);
			tempMesh.push_back(tempRow);
			}
		configFileSection.storeValue<Mesh>("./mesh",tempMesh);
		}
	catch(const std::runtime_error& err)
		{
		/* Release allocated resources and throw an exception: */
		delete[] mesh;
		delete[] coxDeBoor;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Can not write configuration due to exception %s",err.what());
		}
	
	/* Allocate the x and y evaluation arrays: */
	xs=new Point2[degree[0]+1];
	ys=new Point2[degree[1]+1];
	}

PenPadCalibratorBSpline::PenPadCalibratorBSpline(const Misc::ConfigurationFileSection& configFileSection,const Box2& rawDomain)
	:mesh(0),coxDeBoor(0),xs(0),ys(0)
	{
	try
		{
		/* Read the B-Spline degree and size and check them for validity: */
		degree=configFileSection.retrieveValue<Size>("./degree");
		if(degree[0]<1||degree[1]<1)
			throw Misc::makeStdErr(0,"Invalid B-Spline degree (%u, %u)",degree[0],degree[1]);
		size=configFileSection.retrieveValue<Size>("./size");
		if(size[0]<degree[0]+1||size[1]<degree[1]+1)
			throw Misc::makeStdErr(0,"Invalid B-Spline mesh size (%u, %u) for degree (%u, %u)",size[0],size[1],degree[0],degree[1]);
		
		/* Calculate the raw measurement transformation coefficients: */
		for(int i=0;i<2;++i)
			{
			rawScale[i]=Scalar(size[i]-degree[i])/Scalar(rawDomain.max[i]-rawDomain.min[i]);
			rawOffset[i]=Scalar(degree[i])-Scalar(rawDomain.min[i])*rawScale[i];
			}
		
		/* Read the B-Spline control point mesh: */
		typedef std::vector<Point2> MeshRow;
		typedef std::vector<MeshRow> Mesh;
		Mesh tempMesh=configFileSection.retrieveValue<Mesh>("./mesh");
		
		/* Check the mesh for validity: */
		bool valid=tempMesh.size()==size[1];
		for(unsigned int y=0;y<size[1];++y)
			valid=valid&&tempMesh[y].size()==size[0];
		if(!valid)
			throw Misc::makeStdErr(0,"Invalid B-Spline mesh for mesh size (%u, %u)",size[0],size[1]);
		
		/* Extract the mesh: */
		mesh=new Point2[size.volume()];
		for(unsigned int y=0;y<size[1];++y)
			for(unsigned int x=0;x<size[0];++x)
				mesh[y*size[0]+x]=tempMesh[y][x];
		
		/* Allocate the Cox-de Boor evaluation array: */
		coxDeBoor=new Scalar[Misc::max(degree[0],degree[1])+1];
		
		/* Allocate the x and y evaluation arrays: */
		xs=new Point2[degree[0]+1];
		ys=new Point2[degree[1]+1];
		}
	catch(const std::runtime_error& err)
		{
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Can not initialize calibrator due to exception %s",err.what());
		}
	}

PenPadCalibratorBSpline::~PenPadCalibratorBSpline(void)
	{
	/* Release allocated resources: */
	delete[] mesh;
	delete[] coxDeBoor;
	delete[] xs;
	delete[] ys;
	}

PenPadCalibrator::Point2 PenPadCalibratorBSpline::calibrate(const PenPadCalibrator::Point2& raw) const
	{
	/* Transform the raw point to B-Spline mesh space and find the knot intervals containing it: */
	Scalar mx=Scalar(raw[0])*rawScale[0]+rawOffset[0];
	unsigned int ivx(Math::clamp(int(Math::floor(mx)),int(degree[0]),int(size[0]-1)));
	Scalar my=Scalar(raw[1])*rawScale[1]+rawOffset[1];
	unsigned int ivy(Math::clamp(int(Math::floor(my)),int(degree[1]),int(size[1]-1)));
	
	/* Evaluate x-direction B-Spline curves: */
	for(unsigned int y=0;y<=degree[1];++y)
		{
		/* Copy the partial control point array: */
		for(unsigned int x=0;x<=degree[0];++x)
			xs[x]=mesh[(ivy-degree[1]+y)*size[0]+(ivx-degree[0]+x)];
		
		/* Run Cox-de Boor's algorithm on the partial array: */
		for(unsigned int k=0;k<degree[0];++k)
			{
			unsigned int subDeg=degree[0]-k;
			for(unsigned int x=0;x<subDeg;++x)
				xs[x]=Geometry::affineCombination(xs[x],xs[x+1],(mx-Scalar(ivx-subDeg+1+x))/Scalar(subDeg));
			}
		
		/* Put the final point into the y-direction B-spline curve evaluation array: */
		ys[y]=xs[0];
		}
	
	/* Evaluate the y-direction B-spline curve: */
	for(unsigned int k=0;k<degree[1];++k)
		{
		unsigned int subDeg=degree[1]-k;
		for(unsigned int y=0;y<subDeg;++y)
			ys[y]=Geometry::affineCombination(ys[y],ys[y+1],(my-Scalar(ivy-subDeg+1+y))/Scalar(subDeg));
		}
	
	/* Return the final point: */
	return ys[0];
	}

}

/***********************************************************************
GeodeticToCartesianTransformNode - Point transformation class to convert
geodetic coordinates (longitude/latitude/altitude on a reference
ellipsoid) to Cartesian coordinates.
Copyright (c) 2009-2023 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/GeodeticToCartesianTransformNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <AL/ALContextData.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/SphereCollisionQuery.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/ALRenderState.h>

namespace SceneGraph {

/*********************************************************
Static elements of class GeodeticToCartesianTransformNode:
*********************************************************/

const char* GeodeticToCartesianTransformNode::className="GeodeticToCartesianTransform";

/*************************************************
Methods of class GeodeticToCartesianTransformNode:
*************************************************/

GeodeticToCartesianTransformNode::GeodeticToCartesianTransformNode(void)
	:longitudeFirst(true),
	 degrees(false),
	 colatitude(false),
	 geodetic(Point::origin),
	 translateOnly(false)
	{
	}

const char* GeodeticToCartesianTransformNode::getClassName(void) const
	{
	return className;
	}

void GeodeticToCartesianTransformNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"referenceEllipsoid")==0)
		{
		vrmlFile.parseSFNode(referenceEllipsoid);
		}
	else if(strcmp(fieldName,"longitudeFirst")==0)
		{
		vrmlFile.parseField(longitudeFirst);
		}
	else if(strcmp(fieldName,"degrees")==0)
		{
		vrmlFile.parseField(degrees);
		}
	else if(strcmp(fieldName,"colatitude")==0)
		{
		vrmlFile.parseField(colatitude);
		}
	else if(strcmp(fieldName,"geodetic")==0)
		{
		vrmlFile.parseField(geodetic);
		}
	else if(strcmp(fieldName,"translateOnly")==0)
		{
		vrmlFile.parseField(translateOnly);
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

void GeodeticToCartesianTransformNode::update(void)
	{
	typedef ReferenceEllipsoidNode::Geoid::Scalar GScalar;
	typedef ReferenceEllipsoidNode::Geoid::Point GPoint;
	typedef ReferenceEllipsoidNode::Geoid::Frame GFrame;
	
	/* Create a default reference ellipsoid if none was given: */
	if(referenceEllipsoid.getValue()==0)
		{
		referenceEllipsoid.setValue(new ReferenceEllipsoidNode);
		referenceEllipsoid.getValue()->update();
		}
	
	/* Convert the geodetic point to longitude and latitude in radians and height in meters: */
	GPoint g;
	g[0]=GScalar(longitudeFirst.getValue()?geodetic.getValue()[0]:geodetic.getValue()[1]);
	g[1]=GScalar(longitudeFirst.getValue()?geodetic.getValue()[1]:geodetic.getValue()[0]);
	if(degrees.getValue())
		{
		g[0]=Math::rad(g[0]);
		g[1]=Math::rad(g[1]);
		}
	if(colatitude.getValue())
		g[1]=Math::div2(Math::Constants<GScalar>::pi)-g[1];
	g[2]=GScalar(geodetic.getValue()[2]);
	
	/* Calculate the current transformation: */
	if(translateOnly.getValue())
		{
		/* Calculate a translation from the origin to the given point in geodetic coordinates: */
		transform=DOGTransform::translateFromOriginTo(referenceEllipsoid.getValue()->getRE().geodeticToCartesian(g));
		}
	else
		{
		/* Calculate the full reference frame transformation: */
		GFrame frame=referenceEllipsoid.getValue()->getRE().geodeticToCartesianFrame(g);
		transform=DOGTransform(frame.getTranslation(),frame.getRotation(),referenceEllipsoid.getValue()->scale.getValue());
		}
	
	/* Call the base class method: */
	GroupNode::update();
	}

void GeodeticToCartesianTransformNode::read(SceneGraphReader& reader)
	{
	/* Call the base class method: */
	GroupNode::read(reader);
	
	/* Read all fields: */
	reader.readSFNode(referenceEllipsoid);
	reader.readField(longitudeFirst);
	reader.readField(degrees);
	reader.readField(colatitude);
	reader.readField(geodetic);
	reader.readField(translateOnly);
	}

void GeodeticToCartesianTransformNode::write(SceneGraphWriter& writer) const
	{
	/* Call the base class method: */
	GroupNode::write(writer);
	
	/* Write all fields: */
	writer.writeSFNode(referenceEllipsoid);
	writer.writeField(longitudeFirst);
	writer.writeField(degrees);
	writer.writeField(colatitude);
	writer.writeField(geodetic);
	writer.writeField(translateOnly);
	}

Box GeodeticToCartesianTransformNode::calcBoundingBox(void) const
	{
	/* Return the explicit bounding box if there is one: */
	if(explicitBoundingBox!=0)
		return *explicitBoundingBox;
	else
		{
		/* Calculate the group's bounding box as the union of the transformed children's boxes: */
		Box result=Box::empty;
		for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			{
			Box childBox=(*chIt)->calcBoundingBox();
			childBox.transform(transform);
			result.addBox(childBox);
			}
		return result;
		}
	}

void GeodeticToCartesianTransformNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Transform the collision query: */
	OGTransform ogTransform(transform);
	SphereCollisionQuery transformedCollisionQuery=collisionQuery.transform(ogTransform);
	
	/* Delegate to the base class: */
	GroupNode::testCollision(transformedCollisionQuery);
	
	/* Update the original collision query if the transformed copy was updated: */
	if(transformedCollisionQuery.getHitLambda()<collisionQuery.getHitLambda())
		collisionQuery.updateFromTransform(ogTransform,transformedCollisionQuery);
	}

void GeodeticToCartesianTransformNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform);
	
	/* Delegate to the base class: */
	GroupNode::glRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

void GeodeticToCartesianTransformNode::alRenderAction(ALRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	DOGTransform previousTransform=renderState.pushTransform(transform);
	
	/* Delegate to the base class: */
	GroupNode::alRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

}

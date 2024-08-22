/***********************************************************************
ShapeNode - Class for shapes represented as a combination of a geometry
node and an attribute node defining the geometry's appearance.
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

#include <SceneGraph/ShapeNode.h>

#include <string.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/**********************************
Static elements of class ShapeNode:
**********************************/

const char* ShapeNode::className="Shape";

/**************************
Methods of class ShapeNode:
**************************/

void ShapeNode::updateRequirements(void)
	{
	/* Compose the current appearance node's requirement mask: */
	appearanceRequirementMask=0x0;
	if(appearance.getValue()!=0)
		appearanceRequirementMask=appearance.getValue()->getAppearanceRequirementMask();
	
	/* Check if the geometry node changed since the last update() call or needs to be updated: */
	if(geometry.getValue()!=previousGeometry)
		{
		/* Remove the previous appearance node's requirements from the previous geometry node: */
		if(previousGeometry!=0)
			previousGeometry->removeAppearanceRequirement(previousAppearanceRequirementMask);
		
		/* Add the current appearance node's requirements to the current geometry node: */
		if(geometry.getValue()!=0)
			geometry.getValue()->addAppearanceRequirement(appearanceRequirementMask);
		}
	else if(geometry.getValue()!=0&&appearanceRequirementMask!=previousAppearanceRequirementMask)
		{
		/* Remove the previous appearance node's requirements from the current geometry node: */
		geometry.getValue()->removeAppearanceRequirement(previousAppearanceRequirementMask);
		
		/* Add the current appearance node's requirements to the current geometry node: */
		geometry.getValue()->addAppearanceRequirement(appearanceRequirementMask);
		}
	
	/* Compose the current geometry node's requirement mask: */
	geometryRequirementMask=0x0;
	if(geometry.getValue()!=0)
		geometryRequirementMask=geometry.getValue()->getGeometryRequirementMask();
	
	/* Check if the appearance node changed since the last update() call or needs to be updated: */
	if(appearance.getValue()!=previousAppearance)
		{
		/* Remove the previous geometry node's requirements from the previous appearance node: */
		if(previousAppearance!=0)
			previousAppearance->removeGeometryRequirement(previousGeometryRequirementMask);
		
		/* Add the current geometry node's requirements to the current appearance node: */
		if(appearance.getValue()!=0)
			appearance.getValue()->addGeometryRequirement(geometryRequirementMask);
		}
	else if(appearance.getValue()!=0&&geometryRequirementMask!=previousGeometryRequirementMask)
		{
		/* Remove the previous geometry node's requirements from the current appearance node: */
		appearance.getValue()->removeGeometryRequirement(previousGeometryRequirementMask);
		
		/* Add the new current geometry node's requirements to the current appearance node: */
		appearance.getValue()->addGeometryRequirement(geometryRequirementMask);
		}
	
	/* Update appearance and geometry node state: */
	previousAppearance=appearance.getValue();
	previousAppearanceRequirementMask=appearanceRequirementMask;
	previousGeometry=geometry.getValue();
	previousGeometryRequirementMask=geometryRequirementMask;
	}

ShapeNode::ShapeNode(void)
	:appearanceRequirementMask(0x0),previousAppearanceRequirementMask(0x0),
	 geometryRequirementMask(0x0),previousGeometryRequirementMask(0x0)
	{
	}

const char* ShapeNode::getClassName(void) const
	{
	return className;
	}

void ShapeNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"appearance")==0)
		{
		vrmlFile.parseSFNode(appearance);
		}
	else if(strcmp(fieldName,"geometry")==0)
		{
		vrmlFile.parseSFNode(geometry);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

void ShapeNode::update(void)
	{
	/* Update appearance and geometry nodes' requirements: */
	updateRequirements();
	
	/* Calculate the new pass mask: */
	PassMask newPassMask=0x0U;
	
	/* Check if there is a geometry node, which is required for all passes: */
	if(geometry.getValue()!=0)
		{
		/* Do collision detection if the geometry node supports it: */
		if(geometry.getValue()->canCollide())
			newPassMask|=CollisionPass;
		
		/* Check if there is an appearance node, and whether it requires transparency: */
		if(appearance.getValue()!=0&&appearance.getValue()->isTransparent())
			newPassMask|=GLTransparentRenderPass;
		else
			newPassMask|=GLRenderPass;
		}
	
	setPassMask(newPassMask);
	}

void ShapeNode::read(SceneGraphReader& reader)
	{
	/* Read all fields: */
	reader.readSFNode(appearance);
	reader.readSFNode(geometry);
	}

void ShapeNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeSFNode(appearance);
	writer.writeSFNode(geometry);
	}

Box ShapeNode::calcBoundingBox(void) const
	{
	/* Return the geometry node's bounding box or an empty box if there is no geometry node: */
	if(geometry.getValue()!=0)
		return geometry.getValue()->calcBoundingBox();
	else
		return Box::empty;
	}

void ShapeNode::testCollision(SphereCollisionQuery& collisionQuery) const
	{
	/* Apply the collision query to the geometry node (this method is never called if the geometry node does not support collisions): */
	geometry.getValue()->testCollision(collisionQuery);
	}

void ShapeNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set the appearance node's OpenGL state: */
	if(appearance.getValue()!=0)
		appearance.getValue()->setGLState(geometryRequirementMask,renderState);
	else
		{
		/* Turn off all appearance aspects: */
		renderState.disableMaterials();
		renderState.setEmissiveColor(GLRenderState::Color(1.0f,1.0f,1.0f));
		renderState.disableTextures();
		}
	
	/* Render the geometry node: */
	geometry.getValue()->glRenderAction(appearanceRequirementMask,renderState);
	
	/* Reset the appearance node's OpenGL state: */
	if(appearance.getValue()!=0)
		appearance.getValue()->resetGLState(geometryRequirementMask,renderState);
	}

}

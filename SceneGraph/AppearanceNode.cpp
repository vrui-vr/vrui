/***********************************************************************
AppearanceNode - Class defining the appearance (material properties,
textures) of a shape node.
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

#include <SceneGraph/AppearanceNode.h>

#include <string.h>
#include <GL/gl.h>
#include <GL/GLTransformationWrappers.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/SceneGraphReader.h>
#include <SceneGraph/SceneGraphWriter.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***************************************
Static elements of class AppearanceNode:
***************************************/

const char* AppearanceNode::className="Appearance";

/*******************************
Methods of class AppearanceNode:
*******************************/

AppearanceNode::AppearanceNode(void)
	:transparent(false)
	{
	}

const char* AppearanceNode::getClassName(void) const
	{
	return className;
	}

EventOut* AppearanceNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"material")==0)
		return makeEventOut(this,material);
	else if(strcmp(fieldName,"texture")==0)
		return makeEventOut(this,texture);
	else if(strcmp(fieldName,"textureTransform")==0)
		return makeEventOut(this,textureTransform);
	else
		return AttributeNode::getEventOut(fieldName);
	}

EventIn* AppearanceNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"material")==0)
		return makeEventIn(this,material);
	else if(strcmp(fieldName,"texture")==0)
		return makeEventIn(this,texture);
	else if(strcmp(fieldName,"textureTransform")==0)
		return makeEventIn(this,textureTransform);
	else
		return AttributeNode::getEventIn(fieldName);
	}

void AppearanceNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"material")==0)
		{
		vrmlFile.parseSFNode(material);
		}
	else if(strcmp(fieldName,"texture")==0)
		{
		vrmlFile.parseSFNode(texture);
		}
	else if(strcmp(fieldName,"textureTransform")==0)
		{
		vrmlFile.parseSFNode(textureTransform);
		}
	else
		AttributeNode::parseField(fieldName,vrmlFile);
	}

void AppearanceNode::update(void)
	{
	/* Require transparent rendering pass if there is a material node, and it has non-zero transparency: */
	bool newTransparent=material.getValue()!=0&&material.getValue()->transparency.getValue()!=Scalar(0);
	
	/* Update the transparency status and notify caller of changes: */
	if(transparent!=newTransparent)
		; // CALL SOMETHING!
	transparent=newTransparent;
	}

void AppearanceNode::read(SceneGraphReader& reader)
	{
	/* Read all fields: */
	reader.readSFNode(material);
	reader.readSFNode(texture);
	reader.readSFNode(textureTransform);
	}

void AppearanceNode::write(SceneGraphWriter& writer) const
	{
	/* Write all fields: */
	writer.writeSFNode(material);
	writer.writeSFNode(texture);
	writer.writeSFNode(textureTransform);
	}

int AppearanceNode::getAppearanceRequirementMask(void) const
	{
	int result=0x0;
	
	/* Require texture coordinates if there is a texture node: */
	if(texture.getValue()!=0)
		result|=GeometryNode::NeedsTexCoords;
	
	/* Require normals if there is a material node, and it requires normals: */
	if(material.getValue()!=0&&material.getValue()->requiresNormals())
		result|=GeometryNode::NeedsNormals;
	
	return result;
	}

bool AppearanceNode::isTransparent(void) const
	{
	/* Return the transparency flag: */
	return transparent;
	}

int AppearanceNode::setGLState(int geometryRequirementMask,GLRenderState& renderState) const
	{
	int appearanceRequirementsMask=0x0;
	
	if((geometryRequirementMask&HasSurfaces)!=0x0)
		{
		if(material.getValue()!=0)
			{
			/* Apply the material: */
			material.getValue()->setGLState(renderState);
			if(material.getValue()->requiresNormals())
				appearanceRequirementsMask|=GeometryNode::NeedsNormals;
			
			/* Set other lighting state: */
			renderState.setTwoSidedLighting((geometryRequirementMask&HasTwoSidedSurfaces)!=0x0);
			renderState.setColorMaterial((geometryRequirementMask&HasColors)!=0x0);
			}
		else
			{
			/* Disable lighting: */
			renderState.disableMaterials();
			renderState.setEmissiveColor(GLRenderState::Color(0.0f,0.0f,0.0f));
			}
		
		if(texture.getValue()!=0)
			{
			/* Apply the texture: */
			texture.getValue()->setGLState(renderState);
			appearanceRequirementsMask|=GeometryNode::NeedsTexCoords;
			
			if(textureTransform.getValue()!=0)
				{
				/* Apply the texture transformation: */
				textureTransform.getValue()->setGLState(renderState);
				}
			}
		else
			renderState.disableTextures();
		}
	else
		{
		/* Disable lighting and texture mapping: */
		renderState.disableMaterials();
		if(material.getValue()!=0)
			{
			/* Set the emissive color: */
			const GLMaterial& m=material.getValue()->getMaterial();
			renderState.setEmissiveColor(m.emission);
			
			/* Set the blending function if transparency is required: */
			if(m.emission[3]!=1.0f)
				renderState.blendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			}
		else
			renderState.setEmissiveColor(GLRenderState::Color(0.0f,0.0f,0.0f));
		renderState.disableTextures();
		}
	
	return appearanceRequirementsMask;
	}

void AppearanceNode::resetGLState(int geometryRequirementMask,GLRenderState& renderState) const
	{
	if((geometryRequirementMask&HasSurfaces)!=0x0)
		{
		if(material.getValue()!=0)
			material.getValue()->resetGLState(renderState);
		
		if(texture.getValue()!=0)
			{
			if(textureTransform.getValue()!=0)
				{
				/* Reset the texture transformation: */
				textureTransform.getValue()->resetGLState(renderState);
				}
			
			/* Disable the texture: */
			texture.getValue()->resetGLState(renderState);
			}
		}
	else
		{
		/* No need to do anything; next guy cleans up */
		}
	}

AppearanceNodePointer createEmissiveAppearance(const Color& emissiveColor)
	{
	/* Create an appearance node: */
	AppearanceNodePointer result=new AppearanceNode;
	
	/* Create a material node: */
	result->material.setValue(new MaterialNode);
	
	/* Set the material node's fields: */
	result->material.getValue()->ambientIntensity.setValue(0.0f);
	result->material.getValue()->diffuseColor.setValue(Color(0,0,0));
	result->material.getValue()->specularColor.setValue(Color(0,0,0));
	result->material.getValue()->shininess.setValue(0.0f);
	result->material.getValue()->emissiveColor.setValue(emissiveColor);
	result->material.getValue()->update();
	
	/* Finalize and return the appearance node: */
	result->update();
	return result;
	}

AppearanceNodePointer createDiffuseAppearance(const Color& diffuseColor)
	{
	/* Create an appearance node: */
	AppearanceNodePointer result=new AppearanceNode;
	
	/* Create a material node: */
	result->material.setValue(new MaterialNode);
	
	/* Set the material node's fields: */
	result->material.getValue()->diffuseColor.setValue(diffuseColor);
	result->material.getValue()->specularColor.setValue(Color(0,0,0));
	result->material.getValue()->shininess.setValue(0.0f);
	result->material.getValue()->update();
	
	/* Finalize and return the appearance node: */
	result->update();
	return result;
	}

AppearanceNodePointer createPhongAppearance(const Color& diffuseColor,const Color& specularColor,Scalar shininess)
	{
	/* Create an appearance node: */
	AppearanceNodePointer result=new AppearanceNode;
	
	/* Create a material node: */
	result->material.setValue(new MaterialNode);
	
	/* Set the material node's fields: */
	result->material.getValue()->diffuseColor.setValue(diffuseColor);
	result->material.getValue()->specularColor.setValue(specularColor);
	result->material.getValue()->shininess.setValue(shininess);
	result->material.getValue()->update();
	
	/* Finalize and return the appearance node: */
	result->update();
	return result;
	}

}

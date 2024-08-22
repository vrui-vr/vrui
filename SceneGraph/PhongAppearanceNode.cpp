/***********************************************************************
PhongAppearanceNode - Class defining the appearance (material
properties, textures) of a shape node that uses Phong shading for
rendering.
Copyright (c) 2022-2024 Oliver Kreylos

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

#include <SceneGraph/PhongAppearanceNode.h>

#include <Misc/StdError.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLLightTracker.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <SceneGraph/Config.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***********************************************
Methods of struct PhongAppearanceNode::DataItem:
***********************************************/

PhongAppearanceNode::DataItem::DataItem(GLShaderManager::Namespace& sShaderNamespace)
	:shaderNamespace(sShaderNamespace)
	{
	/* Initialize the required OpenGL extensions: */
	GLARBMultitexture::initExtension();
	GLARBShaderObjects::initExtension();
	GLARBVertexShader::initExtension();
	GLARBFragmentShader::initExtension();
	}

/********************************************
Static elements of class PhongAppearanceNode:
********************************************/

const char* PhongAppearanceNode::className="PhongAppearance";

/************************************
Methods of class PhongAppearanceNode:
************************************/

PhongAppearanceNode::PhongAppearanceNode(void)
	{
	}

const char* PhongAppearanceNode::getClassName(void) const
	{
	return className;
	}

void PhongAppearanceNode::update(void)
	{
	/* Throw an error if there is no material node: */
	if(material.getValue()==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Phong appearance node requires a material node");
	
	/* Call the base class method: */
	return AppearanceNode::update();
	}

int PhongAppearanceNode::setGLState(int geometryRequirementMask,GLRenderState& renderState) const
	{
	int appearanceRequirementsMask=0x0;
	
	if((geometryRequirementMask&HasSurfaces)!=0x0)
		{
		/* Retrieve the context data item: */
		DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
		
		/* Apply the material: */
		material.getValue()->setGLState(renderState);
		if(material.getValue()->requiresNormals())
			appearanceRequirementsMask|=GeometryNode::NeedsNormals;
		
		/* Check if there is a texture: */
		GLint activeTexture=0;
		if(texture.getValue()!=0)
			{
			/* Apply the texture: */
			glGetIntegerv(GL_ACTIVE_TEXTURE_ARB,&activeTexture);
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
		
		/* Determine which shader to use: */
		unsigned int shaderIndex=0;
		if(texture.getValue()!=0)
			shaderIndex+=2;
		if((geometryRequirementMask&HasColors)!=0x0)
			shaderIndex+=1;
		
		/* Check if the shader required for this rendering still needs to be created: */
		GLShaderManager::Namespace& sns=dataItem->shaderNamespace;
		GLhandleARB shader=sns.getShader(shaderIndex);
		if(shader==GLhandleARB(0))
			{
			/* Compile the appropriate vertex and fragment shaders: */
			static const char* shaderNames[4]=
				{
				"PhongAppearanceShader","PhongAppearanceShaderColor",
				"PhongAppearanceShaderTex2D","PhongAppearanceShaderColorTex2D"
				};
			
			/* Compile the vertex shader: */
			std::string vertexShaderName=SCENEGRAPH_CONFIG_SHADERDIR;
			vertexShaderName.push_back('/');
			vertexShaderName.append(shaderNames[shaderIndex]);
			vertexShaderName.append(".vs");
			GLhandleARB vertexShader=glCompileVertexShaderFromFile(vertexShaderName.c_str());
			
			/* Compile the fragment shader: */
			std::string fragmentShaderName=SCENEGRAPH_CONFIG_SHADERDIR;
			fragmentShaderName.push_back('/');
			fragmentShaderName.append(shaderNames[shaderIndex]);
			fragmentShaderName.append(".fs");
			GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(fragmentShaderName.c_str());
			
			/* Link the shader program: */
			shader=glCreateProgramObjectARB();
			glAttachObjectARB(shader,vertexShader);
			glAttachObjectARB(shader,fragmentShader);
			glLinkAndTestShader(shader);
			
			/* Release extra references for the vertex and fragment shaders: */
			glDeleteObjectARB(vertexShader);
			glDeleteObjectARB(fragmentShader);
			
			/* Store the shader program in the namespace: */
			sns.setShader(shaderIndex,shader);
			
			/* Query the locations of the shader's uniform variables: */
			sns.setUniformLocation(shaderIndex,0,"clipPlaneEnableds");
			sns.setUniformLocation(shaderIndex,1,"lightEnableds");
			if(shaderIndex>=2)
				sns.setUniformLocation(shaderIndex,2,"texture");
			}
		
		/* Bind the shader program: */
		renderState.bindShader(shader);
		
		/* Upload the arrays of enabled clipping planes and light sources: */
		renderState.contextData.getClipPlaneTracker()->uploadClipPlaneEnableds(sns.getUniformLocation(shaderIndex,0));
		renderState.contextData.getLightTracker()->uploadLightEnableds(sns.getUniformLocation(shaderIndex,1));
		
		/* Set the texture image's texture unit: */
		if(shaderIndex>=2)
			glUniform1iARB(sns.getUniformLocation(shaderIndex,2),activeTexture-GL_TEXTURE0_ARB);
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

void PhongAppearanceNode::resetGLState(int geometryRequirementMask,GLRenderState& renderState) const
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

void PhongAppearanceNode::initContext(GLContextData& contextData) const
	{
	/* Create a namespace to hold the GLSL shaders: */
	static const unsigned int numShaderUniforms[4]=
		{
		2,2,3,3
		};
	std::pair<GLShaderManager::Namespace&,bool> cnsr=contextData.getShaderManager()->createNamespace("SceneGraph/PhongAppearanceNode",4,numShaderUniforms);
	
	/* Create a new data item and store it in the OpenGL context: */
	DataItem* dataItem=new DataItem(cnsr.first);
	contextData.addDataItem(this,dataItem);
	}

}

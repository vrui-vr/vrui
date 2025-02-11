/***********************************************************************
GLLabel - Class to render 3D text strings using texture-based fonts.
Copyright (c) 2010-2025 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/GLLabel.h>

#include <GL/gl.h>
#include <GL/Config.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLTexCoordTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLTexEnvTemplates.h>
#include <GL/GLVector.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLLightTracker.h>
#include <GL/GLContextData.h>
#include <GL/GLShaderManager.h>
#include <GL/GLFont.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBFragmentShader.h>

/***************************************
Declaration of struct GLLabel::DataItem:
***************************************/

struct GLLabel::DataItem:public GLObject::DataItem
	{
	/* Elements: */
	public:
	const GLShaderManager::Namespace& shaderNamespace; // Namespace containing the GLSL shaders
	GLuint textureObjectId;
	unsigned int version; // Version number of string currently in texture object

	/* Constructors and destructors: */
	DataItem(const GLShaderManager::Namespace& sShaderNamespace);
	~DataItem(void);
	};

/***********************************
Methods of struct GLLabel::DataItem:
***********************************/

GLLabel::DataItem::DataItem(const GLShaderManager::Namespace& sShaderNamespace)
	:shaderNamespace(sShaderNamespace),
	 version(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBMultitexture::initExtension();
	GLARBShaderObjects::initExtension();
	GLARBVertexShader::initExtension();
	GLARBFragmentShader::initExtension();
	
	glGenTextures(1,&textureObjectId);
	}

GLLabel::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&textureObjectId);
	}

/**************************************************
Static elements of class GLLabel::DeferredRenderer:
**************************************************/

GL_THREAD_LOCAL(GLLabel::DeferredRenderer*) GLLabel::DeferredRenderer::currentDeferredRenderer=0;

/******************************************
Methods of class GLLabel::DeferredRenderer:
******************************************/

GLLabel::DeferredRenderer::DeferredRenderer(GLContextData& sContextData)
	:contextData(sContextData),
	 previousDeferredRenderer(currentDeferredRenderer)
	{
	/* Install the deferred renderer: */
	currentDeferredRenderer=this;
	}

GLLabel::DeferredRenderer::~DeferredRenderer(void)
	{
	/* Draw all undrawn labels: */
	draw();
	
	/* Uninstall the deferred renderer: */
	currentDeferredRenderer=previousDeferredRenderer;
	}

void GLLabel::DeferredRenderer::draw(void)
	{
	/* Bail out if no labels were gathered: */
	if(gatheredLabels.empty())
		return;
	
	/* Retrieve the first gathered label's context data item: */
	std::vector<const GLLabel*>::iterator lIt=gatheredLabels.begin();
	GLLabel::DataItem* dataItem=contextData.retrieveDataItem<GLLabel::DataItem>(*lIt);
	
	/* Retrieve the shader namespace: */
	const GLShaderManager::Namespace& sns=dataItem->shaderNamespace;
	
	/* Determine which shader to use based on whether lighting is enabled: */
	unsigned int shaderIndex=contextData.getLightTracker()->isLightingEnabled()?1:0;
	
	/* Bind the shader program: */
	glUseProgramObjectARB(sns.getShader(shaderIndex));
	
	/* Prepare the first texture unit: */
	glActiveTextureARB(GL_TEXTURE0_ARB);
	sns.uniform(shaderIndex,0,0);
	
	/* Upload the array of enabled clipping planes: */
	contextData.getClipPlaneTracker()->uploadClipPlaneEnableds(sns.getUniformLocation(shaderIndex,2));
	
	/* Upload the array of enabled light sources if lighting is enabled: */
	if(shaderIndex==1U)
		contextData.getLightTracker()->uploadLightEnableds(sns.getUniformLocation(shaderIndex,3));
	
	/* Draw each gathered label: */
	for(;lIt!=gatheredLabels.end();++lIt)
		{
		const GLLabel* l=*lIt;
		
		/* Retrieve the label's context data item: */
		dataItem=contextData.retrieveDataItem<GLLabel::DataItem>(l);
		
		/* Upload the label's foreground color: */
		glUniform4fvARB(sns.getUniformLocation(shaderIndex,1),1,l->foreground.getRgba());
		
		/* Bind the label texture: */
		glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
		
		/* Check if the texture object needs to be updated: */
		if(dataItem->version!=l->version)
			{
			/* Upload the string's texture image: */
			l->font->uploadStringTexture(*l);
			
			/* Update the texture version number: */
			dataItem->version=l->version;
			}
		
		/* Draw a textured quad: */
		glColor(l->background);
		glBegin(GL_QUADS);
		glNormal3f(0.0f,0.0f,1.0f);
		glTexCoord(l->textureBox.getCorner(0));
		glVertex(l->labelBox.getCorner(0));
		glTexCoord(l->textureBox.getCorner(1));
		glVertex(l->labelBox.getCorner(1));
		glTexCoord(l->textureBox.getCorner(3));
		glVertex(l->labelBox.getCorner(3));
		glTexCoord(l->textureBox.getCorner(2));
		glVertex(l->labelBox.getCorner(2));
		glEnd();
		}
	
	/* Reset OpenGL state: */
	glBindTexture(GL_TEXTURE_2D,0);
	glUseProgramObjectARB(0);
	
	/* Clear the list of labels: */
	gatheredLabels.clear();
	}

bool GLLabel::DeferredRenderer::addLabel(const GLLabel* label)
	{
	/* Deny request if there is no current deferred renderer: */
	if(currentDeferredRenderer==0)
		return false;
	
	/* Add the label to the current deferred renderer's list: */
	currentDeferredRenderer->gatheredLabels.push_back(label);
	
	return true;
	}

/************************
Methods of class GLLabel:
************************/

GLLabel::GLLabel(const char* sString,const GLFont& sFont)
	:GLString(sString,sFont),font(&sFont),
	 background(font->getBackgroundColor()),foreground(font->getForegroundColor()),
	 version(1),
	 labelBox(Box::Vector(0.0f,0.0f,0.0f),font->calcStringSize(texelWidth))
	{
	}

GLLabel::GLLabel(const char* sStringBegin,const char* sStringEnd,const GLFont& sFont)
	:GLString(sStringBegin,sStringEnd,sFont),font(&sFont),
	 background(font->getBackgroundColor()),foreground(font->getForegroundColor()),
	 version(1),
	 labelBox(Box::Vector(0.0f,0.0f,0.0f),font->calcStringSize(texelWidth))
	{
	}

GLLabel::GLLabel(const GLString& sString,const GLFont& sFont)
	:GLString(sString),font(&sFont),
	 background(font->getBackgroundColor()),foreground(font->getForegroundColor()),
	 version(1),
	 labelBox(Box::Vector(0.0f,0.0f,0.0f),font->calcStringSize(texelWidth))
	{
	}

GLLabel::GLLabel(const GLLabel& source)
	:GLString(source),font(source.font),
	 background(source.background),foreground(source.foreground),
	 version(1),
	 labelBox(Box::Vector(0.0f,0.0f,0.0f),font->calcStringSize(texelWidth))
	{
	}

GLLabel& GLLabel::operator=(const GLLabel& source)
	{
	if(this!=&source)
		{
		/* Copy the source label: */
		GLString::operator=(source);
		font=source.font;
		
		/* Copy the source colors: */
		background=source.background;
		foreground=source.foreground;
		
		/* Increment the version number: */
		++version;
		
		/* Copy the label box: */
		labelBox=source.labelBox;
		}
	
	return *this;
	}

void GLLabel::setString(const char* newString,const GLFont& newFont)
	{
	/* Replace the string and font: */
	GLString::setString(newString,newFont);
	font=&newFont;
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::setString(const char* newStringBegin,const char* newStringEnd,const GLFont& newFont)
	{
	/* Replace the string and font: */
	GLString::setString(newStringBegin,newStringEnd,newFont);
	font=&newFont;
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::adoptString(char* newString,const GLFont& newFont)
	{
	/* Replace the string and font: */
	GLString::adoptString(newString,newFont);
	font=&newFont;
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::adoptString(GLsizei newLength,char* newString,const GLFont& newFont)
	{
	/* Replace the string and font: */
	GLString::adoptString(newLength,newString,newFont);
	font=&newFont;
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::setFont(const GLFont& newFont)
	{
	/* Update the string: */
	GLString::setFont(newFont);
	
	/* Update the font: */
	font=&newFont;
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::initContext(GLContextData& contextData) const
	{
	/* Create a namespace to hold the GLSL shaders: */
	static const unsigned int numShaderUniforms[2]=
		{
		3,4
		};
	std::pair<GLShaderManager::Namespace&,bool> cnsr=contextData.getShaderManager()->createNamespace("GLSupport/GLLabel",2,numShaderUniforms);
	
	/* Create a data item: */
	DataItem* dataItem=new DataItem(cnsr.first);
	contextData.addDataItem(this,dataItem);
	
	/* Check if the shaders need to be created: */
	if(cnsr.second)
		{
		/*******************************************************************
		Build the shader for non-illuminated GLLabels:
		*******************************************************************/
		
		{
		/* Compile the vertex shader: */
		std::string vertexShaderName=GLSUPPORT_CONFIG_SHADERDIR;
		vertexShaderName.append("/GLLabelNonIlluminated.vs");
		GLhandleARB vertexShader=glCompileVertexShaderFromFile(vertexShaderName.c_str());
		
		/* Compile the fragment shader: */
		std::string fragmentShaderName=GLSUPPORT_CONFIG_SHADERDIR;
		fragmentShaderName.append("/GLLabelNonIlluminated.fs");
		GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(fragmentShaderName.c_str());
		
		/* Link the shader program: */
		GLhandleARB shader=glCreateProgramObjectARB();
		glAttachObjectARB(shader,vertexShader);
		glAttachObjectARB(shader,fragmentShader);
		glLinkAndTestShader(shader);
		
		/* Release extra references for the vertex and fragment shaders: */
		glDeleteObjectARB(vertexShader);
		glDeleteObjectARB(fragmentShader);
		
		/* Store the shader program in the namespace: */
		cnsr.first.setShader(0,shader);
		
		/* Query the locations of the shader's uniform variables: */
		cnsr.first.setUniformLocation(0,0,"stringTexture");
		cnsr.first.setUniformLocation(0,1,"foregroundColor");
		cnsr.first.setUniformLocation(0,2,"clipPlaneEnableds");
		}
		
		/*******************************************************************
		Build the shader for illuminated GLLabels:
		*******************************************************************/
		
		{
		/* Compile the vertex shader: */
		std::string vertexShaderName=GLSUPPORT_CONFIG_SHADERDIR;
		vertexShaderName.append("/GLLabelIlluminated.vs");
		GLhandleARB vertexShader=glCompileVertexShaderFromFile(vertexShaderName.c_str());
		
		/* Compile the fragment shader: */
		std::string fragmentShaderName=GLSUPPORT_CONFIG_SHADERDIR;
		fragmentShaderName.append("/GLLabelIlluminated.fs");
		GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(fragmentShaderName.c_str());
		
		/* Link the shader program: */
		GLhandleARB shader=glCreateProgramObjectARB();
		glAttachObjectARB(shader,vertexShader);
		glAttachObjectARB(shader,fragmentShader);
		glLinkAndTestShader(shader);
		
		/* Release extra references for the vertex and fragment shaders: */
		glDeleteObjectARB(vertexShader);
		glDeleteObjectARB(fragmentShader);
		
		/* Store the shader program in the namespace: */
		cnsr.first.setShader(1,shader);
		
		/* Query the locations of the shader's uniform variables: */
		cnsr.first.setUniformLocation(1,0,"stringTexture");
		cnsr.first.setUniformLocation(1,1,"foregroundColor");
		cnsr.first.setUniformLocation(1,2,"clipPlaneEnableds");
		cnsr.first.setUniformLocation(1,3,"lightEnableds");
		}
		}
	}

GLLabel::Box::Vector GLLabel::calcNaturalSize(void) const
	{
	/* Return the string size as determined by the font: */
	return font->calcStringSize(texelWidth);
	}

void GLLabel::setString(const char* newString)
	{
	/* Replace the string: */
	GLString::setString(newString,*font);
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::setString(const char* newStringBegin,const char* newStringEnd)
	{
	/* Replace the string: */
	GLString::setString(newStringBegin,newStringEnd,*font);
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::setString(const GLString& newString)
	{
	/* Replace the string, but use the current font: */
	GLString::setString(newString.getString(),newString.getString()+newString.getLength(),*font);
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::adoptString(char* newString)
	{
	/* Replace the string: */
	GLString::adoptString(newString,*font);
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::adoptString(GLsizei newLength,char* newString)
	{
	/* Replace the string: */
	GLString::adoptString(newLength,newString,*font);
	
	/* Increment the version number: */
	++version;
	
	/* Update the label box size: */
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::resetBox(void)
	{
	/* Re-calculate the texture coordinate box: */
	textureBox=font->calcStringTexCoords(texelWidth,textureWidth);
	
	/* Re-calculate the label box: */
	labelBox.origin=Box::Vector(0.0f,0.0f,0.0f);
	labelBox.size=font->calcStringSize(texelWidth);
	}

void GLLabel::setOrigin(const GLLabel::Box::Vector& newOrigin)
	{
	labelBox.origin=newOrigin;
	}

void GLLabel::clipBox(const GLLabel::Box& clipBox)
	{
	/* Update the texture coordinate box and the label box: */
	for(int i=0;i<2;++i)
		{
		GLfloat dMin=clipBox.origin[i]-labelBox.origin[i];
		if(dMin>0.0f)
			{
			GLfloat texDMin=dMin*textureBox.size[i]/labelBox.size[i];
			textureBox.origin[i]+=texDMin;
			textureBox.size[i]-=texDMin;
			labelBox.origin[i]+=dMin;
			labelBox.size[i]-=dMin;
			}
		GLfloat dMax=(labelBox.origin[i]+labelBox.size[i])-(clipBox.origin[i]+clipBox.size[i]);
		if(dMax>0.0f)
			{
			GLfloat texDMax=dMax*textureBox.size[i]/labelBox.size[i];
			textureBox.size[i]-=texDMax;
			labelBox.size[i]-=dMax;
			}
		}
	}

GLint GLLabel::calcCharacterIndex(GLfloat modelPos) const
	{
	/* Convert the model-space position to texture space: */
	GLfloat texPos=(modelPos-labelBox.origin[0])*textureBox.size[0]/labelBox.size[0]+textureBox.origin[0];
	
	/* Return the result: */
	return font->calcCharacterPos(getString(),textureWidth,texPos);
	}

GLfloat GLLabel::calcCharacterPos(GLint characterPos) const
	{
	/* Calculate the character's texture coordinate: */
	GLfloat texPos=font->calcCharacterTexCoord(getString(),textureWidth,characterPos);
	
	/* Convert the texture coordinate to model space: */
	return (texPos-textureBox.origin[0])*labelBox.size[0]/textureBox.size[0]+labelBox.origin[0];
	}

void GLLabel::draw(GLContextData& contextData) const
	{
	/* Try listing the label with a deferred renderer: */
	if(DeferredRenderer::addLabel(this))
		return;
	
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Retrieve the shader manager: */
	const GLShaderManager::Namespace& sns=dataItem->shaderNamespace;
	
	/* Determine which shader to use based on whether lighting is enabled: */
	unsigned int shaderIndex=contextData.getLightTracker()->isLightingEnabled()?1:0;
	
	/* Bind the shader program: */
	glUseProgramObjectARB(sns.getShader(shaderIndex));
	
	/* Bind the string texture: */
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	sns.uniform(shaderIndex,0,0);
	
	/* Check if the texture object needs to be updated: */
	if(dataItem->version!=version)
		{
		/* Upload the string's texture image: */
		font->uploadStringTexture(*this);
		
		/* Update the texture version number: */
		dataItem->version=version;
		}
	
	/* Upload the label's foreground color: */
	glUniform4fvARB(sns.getUniformLocation(shaderIndex,1),1,foreground.getRgba());
	
	/* Upload the array of enabled clipping planes: */
	contextData.getClipPlaneTracker()->uploadClipPlaneEnableds(sns.getUniformLocation(shaderIndex,2));
	
	/* Upload the array of enabled light sources if lighting is enabled: */
	if(shaderIndex==1U)
		contextData.getLightTracker()->uploadLightEnableds(sns.getUniformLocation(shaderIndex,3));
	
	/* Draw a textured quad: */
	glColor(background);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glTexCoord(textureBox.getCorner(0));
	glVertex(labelBox.getCorner(0));
	glTexCoord(textureBox.getCorner(1));
	glVertex(labelBox.getCorner(1));
	glTexCoord(textureBox.getCorner(3));
	glVertex(labelBox.getCorner(3));
	glTexCoord(textureBox.getCorner(2));
	glVertex(labelBox.getCorner(2));
	glEnd();
	
	/* Reset OpenGL state: */
	glBindTexture(GL_TEXTURE_2D,0);
	glUseProgramObjectARB(0);
	}

void GLLabel::draw(GLsizei selectionStart,GLsizei selectionEnd,const GLLabel::Color& selectionBackgroundColor,const GLLabel::Color& selectionForegroundColor,GLContextData& contextData) const
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Retrieve the shader manager: */
	const GLShaderManager::Namespace& sns=dataItem->shaderNamespace;
	
	/* Determine which shader to use based on whether lighting is enabled: */
	unsigned int shaderIndex=contextData.getLightTracker()->isLightingEnabled()?1:0;
	
	/* Bind the shader program: */
	glUseProgramObjectARB(sns.getShader(shaderIndex));
	
	/* Bind the string texture: */
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	sns.uniform(shaderIndex,0,0);
	
	/* Check if the texture object needs to be updated: */
	if(dataItem->version!=version)
		{
		/* Upload the string's texture image: */
		font->uploadStringTexture(*this,selectionStart,selectionEnd);
		
		/* Update the texture version number: */
		dataItem->version=version;
		}
	
	/* Upload the label's foreground color: */
	glUniform4fvARB(sns.getUniformLocation(shaderIndex,1),1,foreground.getRgba());
	
	/* Upload the array of enabled clipping planes: */
	contextData.getClipPlaneTracker()->uploadClipPlaneEnableds(sns.getUniformLocation(shaderIndex,2));
	
	/* Upload the array of enabled light sources if lighting is enabled: */
	if(shaderIndex==1U)
		contextData.getLightTracker()->uploadLightEnableds(sns.getUniformLocation(shaderIndex,3));
	
	/* Draw a textured quad: */
	glColor(background);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glTexCoord(textureBox.getCorner(0));
	glVertex(labelBox.getCorner(0));
	glTexCoord(textureBox.getCorner(1));
	glVertex(labelBox.getCorner(1));
	glTexCoord(textureBox.getCorner(3));
	glVertex(labelBox.getCorner(3));
	glTexCoord(textureBox.getCorner(2));
	glVertex(labelBox.getCorner(2));
	glEnd();
	
	/* Reset OpenGL state: */
	glBindTexture(GL_TEXTURE_2D,0);
	glUseProgramObjectARB(0);
	}

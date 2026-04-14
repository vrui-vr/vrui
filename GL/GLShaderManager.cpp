/***********************************************************************
GLShaderManager - Class to manage OpenGL shader programs used by
multiple entities.
Copyright (c) 2022-2026 Oliver Kreylos

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

#include <GL/GLShaderManager.h>

#include <Misc/StdError.h>

/*******************************************
Methods of class GLShaderManager::Namespace:
*******************************************/

GLShaderManager::Namespace::Namespace(unsigned int sNumShaders,const unsigned int sNumShaderUniforms[])
	:numShaders(sNumShaders),shaders(new Shader[numShaders]),
	 numUniforms(0),uniformLocations(0),
	 numVersionNumbers(0),versionNumbers(0)
	{
	/* Calculate the total number of uniform variables: */
	for(unsigned int i=0;i<numShaders;++i)
		numUniforms+=sNumShaderUniforms[i];
	if(numUniforms>0)
		uniformLocations=new GLint[numUniforms];
	
	/* Initialize all uniform locations to invalid: */
	for(unsigned int i=0;i<numUniforms;++i)
		uniformLocations[i]=GLint(-1);
	
	/* Initialize all shader states to invalid: */
	GLint* ulPtr=uniformLocations;
	for(unsigned int i=0;i<numShaders;++i)
		{
		shaders[i].handle=GLhandleARB(0);
		shaders[i].numUniforms=sNumShaderUniforms[i];
		shaders[i].uniformLocations=ulPtr;
		shaders[i].numVersionNumbers=0;
		shaders[i].versionNumbers=0;
		
		ulPtr+=sNumShaderUniforms[i];
		}
	}

GLShaderManager::Namespace::Namespace(unsigned int sNumShaders,const unsigned int sNumShaderUniforms[],const unsigned int sNumVersionNumbers[])
	:numShaders(sNumShaders),shaders(new Shader[numShaders]),
	 numUniforms(0),uniformLocations(0),
	 numVersionNumbers(0),versionNumbers(0)
	{
	/* Calculate the total number of uniform variables: */
	for(unsigned int i=0;i<numShaders;++i)
		numUniforms+=sNumShaderUniforms[i];
	if(numUniforms>0)
		uniformLocations=new GLint[numUniforms];
	
	/* Initialize all uniform locations to invalid: */
	for(unsigned int i=0;i<numUniforms;++i)
		uniformLocations[i]=GLint(-1);
	
	/* Calculate the total number of version numbers: */
	for(unsigned int i=0;i<numShaders;++i)
		numVersionNumbers+=sNumVersionNumbers[i];
	if(numVersionNumbers>0)
		versionNumbers=new unsigned int[numVersionNumbers];
	
	/* Initialize all version numbers to zero: */
	for(unsigned int i=0;i<numVersionNumbers;++i)
		versionNumbers[i]=0U;
	
	/* Initialize all shader states to invalid: */
	GLint* ulPtr=uniformLocations;
	unsigned int* vnPtr=versionNumbers;
	for(unsigned int i=0;i<numShaders;++i)
		{
		shaders[i].handle=GLhandleARB(0);
		shaders[i].numUniforms=sNumShaderUniforms[i];
		shaders[i].uniformLocations=ulPtr;
		shaders[i].numVersionNumbers=sNumVersionNumbers[i];
		shaders[i].versionNumbers=vnPtr;
		
		ulPtr+=sNumShaderUniforms[i];
		vnPtr+=sNumVersionNumbers[i];
		}
	}

GLShaderManager::Namespace::Namespace(GLShaderManager::Namespace&& source)
	:numShaders(source.numShaders),shaders(source.shaders),
	 numUniforms(source.numUniforms),uniformLocations(source.uniformLocations),
	 numVersionNumbers(source.numVersionNumbers),versionNumbers(source.versionNumbers)
	{
	/* Invalidate the source: */
	source.numShaders=0;
	source.shaders=0;
	source.numUniforms=0;
	source.uniformLocations=0;
	source.numVersionNumbers=0;
	source.versionNumbers=0;
	}

GLShaderManager::Namespace& GLShaderManager::Namespace::operator=(GLShaderManager::Namespace&& source)
	{
	if(this!=&source)
		{
		/* Copy the source's state: */
		numShaders=source.numShaders;
		shaders=source.shaders;
		numUniforms=source.numUniforms;
		uniformLocations=source.uniformLocations;
		numVersionNumbers=source.numVersionNumbers;
		versionNumbers=source.versionNumbers;
		
		/* Invalidate the source: */
		source.numShaders=0;
		source.shaders=0;
		source.numUniforms=0;
		source.uniformLocations=0;
		source.numVersionNumbers=0;
		source.versionNumbers=0;
		}
	
	return *this;
	}

GLShaderManager::Namespace::~Namespace(void)
	{
	/* Destroy all shader programs: */
	for(unsigned int i=0;i<numShaders;++i)
		if(shaders[i].handle!=GLhandleARB(0))
			glDeleteObjectARB(shaders[i].handle);
	
	/* Release resources: */
	delete[] shaders;
	delete[] uniformLocations;
	delete[] versionNumbers;
	}

void GLShaderManager::Namespace::setShader(unsigned int shaderIndex,GLhandleARB shader)
	{
	/* Destroy the current shader program: */
	if(shaders[shaderIndex].handle!=GLhandleARB(0))
		glDeleteObjectARB(shaders[shaderIndex].handle);
	
	/* Set the new shader program: */
	shaders[shaderIndex].handle=shader;
	}

void GLShaderManager::Namespace::setUniformLocation(unsigned int shaderIndex,unsigned int variableIndex,GLint uniformLocation)
	{
	shaders[shaderIndex].uniformLocations[variableIndex]=uniformLocation;
	}

void GLShaderManager::Namespace::setUniformLocation(unsigned int shaderIndex,unsigned int variableIndex,const char* uniformName)
	{
	shaders[shaderIndex].uniformLocations[variableIndex]=glGetUniformLocationARB(shaders[shaderIndex].handle,uniformName);
	}

void GLShaderManager::Namespace::markUpToDate(unsigned int shaderIndex,const unsigned int versionNumbers[])
	{
	/* Update all version numbers: */
	Shader& shader=shaders[shaderIndex];
	for(unsigned int i=0;i<shader.numVersionNumbers;++i)
		shader.versionNumbers[i]=versionNumbers[i];
	}

void GLShaderManager::Namespace::useShader(unsigned int shaderIndex)
	{
	glUseProgramObjectARB(shaders[shaderIndex].handle);
	}

void GLShaderManager::Namespace::disableShaders(void)
	{
	glUseProgramObjectARB(0);
	}

/********************************
Methods of class GLShaderManager:
********************************/

GLShaderManager::GLShaderManager(void)
	:namespaceMap(17)
	{
	/* Initialize required OpenGL extensions: */
	GLARBShaderObjects::initExtension();
	}

GLShaderManager::~GLShaderManager(void)
	{
	/* No need to actually do anything... */
	}

std::pair<GLShaderManager::Namespace&,bool> GLShaderManager::createNamespace(const std::string& namespaceName,unsigned int numShaders,const unsigned int numShaderUniforms[])
	{
	bool isNew=false;
	
	/* Look for an existing namespace of the given name: */
	NamespaceMap::Iterator nmIt=namespaceMap.findEntry(namespaceName);
	
	/* Check if there is no namespace of the given name yet: */
	if(nmIt.isFinished())
		{
		/* Create a new namespace: */
		nmIt=namespaceMap.setAndFindEntry(NamespaceMap::Entry(namespaceName,Namespace(numShaders,numShaderUniforms)));
		isNew=true;
		}
	else
		{
		/* Check that the existing namespace has the requested number of shaders, uniform variables, and version numbers: */
		if(numShaders!=nmIt->getDest().numShaders)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Existing namespace has mismatching number of shaders");
		for(unsigned int i=0;i<numShaders;++i)
			{
			if(nmIt->getDest().shaders[i].numUniforms!=numShaderUniforms[i])
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Existing namespace has mismatching number of uniform variables per shader");
			if(nmIt->getDest().shaders[i].numVersionNumbers!=0)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Existing namespace has mismatching number of version numbers per shader");
			}
		}
	
	/* Return the namespace: */
	return std::pair<Namespace&,bool>(nmIt->getDest(),isNew);
	}

std::pair<GLShaderManager::Namespace&,bool> GLShaderManager::createNamespace(const std::string& namespaceName,unsigned int numShaders,const unsigned int numShaderUniforms[],const unsigned int numShaderVersionNumbers[])
	{
	bool isNew=false;
	
	/* Look for an existing namespace of the given name: */
	NamespaceMap::Iterator nmIt=namespaceMap.findEntry(namespaceName);
	
	/* Check if there is no namespace of the given name yet: */
	if(nmIt.isFinished())
		{
		/* Create a new namespace: */
		nmIt=namespaceMap.setAndFindEntry(NamespaceMap::Entry(namespaceName,Namespace(numShaders,numShaderUniforms,numShaderVersionNumbers)));
		isNew=true;
		}
	else
		{
		/* Check that the existing namespace has the requested number of shaders, uniform variables, and version numbers: */
		if(numShaders!=nmIt->getDest().numShaders)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Existing namespace has mismatching number of shaders");
		for(unsigned int i=0;i<numShaders;++i)
			{
			if(nmIt->getDest().shaders[i].numUniforms!=numShaderUniforms[i])
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Existing namespace has mismatching number of uniform variables per shader");
			if(nmIt->getDest().shaders[i].numVersionNumbers!=numShaderVersionNumbers[i])
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Existing namespace has mismatching number of version numbers per shader");
			}
		}
	
	/* Return the namespace: */
	return std::pair<Namespace&,bool>(nmIt->getDest(),isNew);
	}

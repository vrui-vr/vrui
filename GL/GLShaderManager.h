/***********************************************************************
GLShaderManager - Class to manage OpenGL shader programs used by
multiple entities.
Copyright (c) 2022-2024 Oliver Kreylos

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

#ifndef GLSHADERMANAGER_INCLUDED
#define GLSHADERMANAGER_INCLUDED

#include <utility>
#include <string>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <GL/gl.h>
#include <GL/Extensions/GLARBShaderObjects.h>

class GLShaderManager
	{
	/* Embedded classes: */
	public:
	class Namespace // Class to hold a set of shader programs used by an entity
		{
		friend class GLShaderManager;
		
		/* Embedded classes: */
		private:
		struct Shader // Structure representing a single shader
			{
			/* Elements: */
			public:
			GLhandleARB handle; // Shader program's handle
			unsigned int numUniforms; // Number of uniform variables used by the shader program
			GLint* uniformLocations; // Pointer to sub-array containing this shader program's uniform variable locations
			};
		
		/* Elements: */
		private:
		unsigned int numShaders; // Number of shaders defined in this namespace
		Shader* shaders; // Array of shader program states
		unsigned int numUniforms; // Total number of uniform variables used by shaders defined in this namespace
		GLint* uniformLocations; // Array of shader uniform variable locations
		
		/* Constructors and destructors: */
		public:
		Namespace(unsigned int sNumShaders,const unsigned int sNumShaderUniforms[]); // Creates a namespace for the given number of shaders and array of number of uniform variables per shader
		Namespace(Namespace&& source); // Move constructor
		Namespace& operator=(Namespace&& source); // Move assignment operator
		~Namespace(void); // Destroys the namespace and all its shaders
		
		/* Methods: */
		GLhandleARB getShader(unsigned int shaderIndex) const // Returns the shader of the given index
			{
			return shaders[shaderIndex].handle;
			}
		GLint getUniformLocation(unsigned int shaderIndex,unsigned int variableIndex) const // Returns the location of the uniform variable of the given index for the shader of the given index
			{
			return shaders[shaderIndex].uniformLocations[variableIndex];
			}
		void setShader(unsigned int shaderIndex,GLhandleARB shader); // Sets the shader program handle for the given namespace slot; assumes that slot is defined and currently empty
		void setUniformLocation(unsigned int shaderIndex,unsigned int variableIndex,GLint uniformLocation); // Sets the location of the uniform variable of the given index for the shader of the given index
		void setUniformLocation(unsigned int shaderIndex,unsigned int variableIndex,const char* uniformName); // Ditto, using glGetUniformLocation to look up the location of the uniform variable of the given name
		
		/* Wrappers for glUniform*ARB functions: */
		void uniform1f(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0) const
			{
			glUniform1fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0);
			}
		void uniform2f(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0,GLfloat v1) const
			{
			glUniform2fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1);
			}
		void uniform3f(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0,GLfloat v1,GLfloat v2) const
			{
			glUniform3fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2);
			}
		void uniform4f(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0,GLfloat v1,GLfloat v2,GLfloat v3) const
			{
			glUniform4fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2,v3);
			}
		void uniform1i(unsigned int shaderIndex,unsigned int variableIndex,GLint v0) const
			{
			glUniform1iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0);
			}
		void uniform2i(unsigned int shaderIndex,unsigned int variableIndex,GLint v0,GLint v1) const
			{
			glUniform2iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1);
			}
		void uniform3i(unsigned int shaderIndex,unsigned int variableIndex,GLint v0,GLint v1,GLint v2) const
			{
			glUniform3iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2);
			}
		void uniform4i(unsigned int shaderIndex,unsigned int variableIndex,GLint v0,GLint v1,GLint v2,GLint v3) const
			{
			glUniform4iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2,v3);
			}
		void uniform1fv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLfloat* value) const
			{
			glUniform1fvARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniform2fv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLfloat* value) const
			{
			glUniform2fvARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniform3fv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLfloat* value) const
			{
			glUniform3fvARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniform4fv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLfloat* value) const
			{
			glUniform4fvARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniform1iv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLint* value) const
			{
			glUniform1ivARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniform2iv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLint* value) const
			{
			glUniform2ivARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniform3iv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLint* value) const
			{
			glUniform3ivARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniform4iv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,const GLint* value) const
			{
			glUniform4ivARB(shaders[shaderIndex].uniformLocations[variableIndex],count,value);
			}
		void uniformMatrix2fv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,GLboolean transpose,const GLfloat* value) const
			{
			glUniformMatrix2fvARB(shaders[shaderIndex].uniformLocations[variableIndex],count,transpose,value);
			}
		void uniformMatrix3fv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,GLboolean transpose,const GLfloat* value) const
			{
			glUniformMatrix3fvARB(shaders[shaderIndex].uniformLocations[variableIndex],count,transpose,value);
			}
		void uniformMatrix4fv(unsigned int shaderIndex,unsigned int variableIndex,GLsizei count,GLboolean transpose,const GLfloat* value) const
			{
			glUniformMatrix4fvARB(shaders[shaderIndex].uniformLocations[variableIndex],count,transpose,value);
			}
		
		/* Overloaded versions of component-based glUniform calls: */
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLint v0) const
			{
			glUniform1iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0);
			}
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0) const
			{
			glUniform1fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0);
			}
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLint v0,GLint v1) const
			{
			glUniform2iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1);
			}
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0,GLfloat v1) const
			{
			glUniform2fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1);
			}
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLint v0,GLint v1,GLint v2) const
			{
			glUniform3iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2);
			}
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0,GLfloat v1,GLfloat v2) const
			{
			glUniform3fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2);
			}
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLint v0,GLint v1,GLint v2,GLint v3) const
			{
			glUniform4iARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2,v3);
			}
		void uniform(unsigned int shaderIndex,unsigned int variableIndex,GLfloat v0,GLfloat v1,GLfloat v2,GLfloat v3) const
			{
			glUniform4fARB(shaders[shaderIndex].uniformLocations[variableIndex],v0,v1,v2,v3);
			}
		};
	
	private:
	typedef Misc::HashTable<std::string,Namespace> NamespaceMap; // Type for hash tables mapping from namespace names to namespaces
	
	/* Elements: */
	NamespaceMap namespaceMap; // Map of defined namespaces
	
	/* Constructors and destructors: */
	public:
	GLShaderManager(void); // Creates an empty shader manager
	~GLShaderManager(void); // Destroys the shader manager and all managed shaders
	
	/* Methods: */
	const Namespace& getNamespace(const std::string& namespaceName) const // Returns the namespace of the given name; throws an exception if requested namespace does not exist
		{
		return namespaceMap.getEntry(namespaceName).getDest();
		}
	std::pair<Namespace&,bool> createNamespace(const std::string& namespaceName,unsigned int numShaders,const unsigned int numShaderUniforms[]); // Returns a new or existing namespace with the given name and number of shaders and array of numbers of uniform variables per shader; second element of result is true if returned namespace is newly created
	};

#endif

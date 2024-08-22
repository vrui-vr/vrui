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

#ifndef SCENEGRAPH_PHONGAPPEARANCENODE_INCLUDED
#define SCENEGRAPH_PHONGAPPEARANCENODE_INCLUDED

#include <Misc/Autopointer.h>
#include <GL/gl.h>
#include <GL/GLShaderManager.h>
#include <GL/GLObject.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <SceneGraph/AppearanceNode.h>

namespace SceneGraph {

class PhongAppearanceNode:public AppearanceNode,public GLObject
	{
	/* Embedded classes: */
	protected:
	struct DataItem:public GLObject::DataItem	
		{
		/* Elements: */
		public:
		GLShaderManager::Namespace& shaderNamespace; // Namespace containing the GLSL shaders
		
		/* Constructors and destructors: */
		DataItem(GLShaderManager::Namespace& sShaderNamespace);
		};
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Constructors and destructors: */
	PhongAppearanceNode(void); // Creates a default Phong-shaded appearance node
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void update(void);
	
	/* Methods from class BaseAppearanceNode: */
	virtual int setGLState(int geometryRequirementMask,GLRenderState& renderState) const;
	virtual void resetGLState(int geometryRequirementMask,GLRenderState& renderState) const;
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

typedef Misc::Autopointer<PhongAppearanceNode> PhongAppearanceNodePointer;

}

#endif

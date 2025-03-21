/***********************************************************************
MaterialNode - Class for attribute nodes defining Phong material
properties.
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

#ifndef SCENEGRAPH_MATERIALNODE_INCLUDED
#define SCENEGRAPH_MATERIALNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <GL/GLMaterial.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/AttributeNode.h>

namespace SceneGraph {

class MaterialNode:public AttributeNode
	{
	/* Embedded classes: */
	protected:
	typedef GLMaterial Material; // Type for material properties
	typedef Material::Color MColor; // Type for material colors
	
	/* Elements: */
	public:
	static const char* className; // The class's name
	
	/* Fields: */
	SFFloat ambientIntensity;
	SFColor diffuseColor;
	SFColor specularColor;
	SFFloat shininess;
	SFColor emissiveColor;
	SFFloat transparency;
	
	protected:
	Material material; // The material properties
	bool needsNormals; // Flag whether the material defined by this node requires normal vectors to be rendered
	
	/* Constructors and destructors: */
	public:
	MaterialNode(void); // Creates a material node with default material properties
	
	/* Methods from class Node: */
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	virtual void read(SceneGraphReader& reader);
	virtual void write(SceneGraphWriter& writer) const;
	
	/* Methods from class AttributeNode: */
	virtual void setGLState(GLRenderState& renderState) const;
	virtual void resetGLState(GLRenderState& renderState) const;
	
	/* New methods: */
	bool requiresNormals(void) const // Returns true if the material defined in this node requires per-vertex normal vectors for rendering
		{
		return needsNormals;
		}
	const GLMaterial& getMaterial(void) const // Returns the current derived material properties
		{
		return material;
		}
	};

typedef Misc::Autopointer<MaterialNode> MaterialNodePointer;

}

#endif

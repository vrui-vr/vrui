/***********************************************************************
BaseAppearanceNode - Base class for nodes defining the appearance
(material properties, textures, etc.) of shape nodes.
Copyright (c) 2019-2022 Oliver Kreylos

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

#ifndef SCENEGRAPH_BASEAPPEARANCENODE_INCLUDED
#define SCENEGRAPH_BASEAPPEARANCENODE_INCLUDED

#include <Misc/Autopointer.h>
#include <SceneGraph/AttributeNode.h>

namespace SceneGraph {

class BaseAppearanceNode:public AttributeNode
	{
	/* Embedded classes: */
	public:
	enum GeometryRequirementFlags // Enumerated type for appearance components required by a geometry node
		{
		HasPoints=0x1, // The geometry defines some point primitives
		HasLines=0x2, // The geometry defines some line primitives
		HasSurfaces=0x4, // The geometry defines some surface primitives
		HasTwoSidedSurfaces=0x8, // The geometry defines some two-sided surfaces
		HasColors=0x10 // The geometry defines per-part colors
		};
	
	/* Elements: */
	protected:
	unsigned int numHasPoints; // Number of geometry nodes that currently require point rendering
	unsigned int numHasLines; // Number of geometry nodes that currently require line rendering
	unsigned int numHasSurfaces; // Number of geometry nodes that currently require surface rendering
	unsigned int numHasTwoSidedSurfaces; // Number of geometry nodes that currently require two-sided lighting
	unsigned int numHasColors; // Number of geometry nodes that currently require color support
	
	/* Constructors and destructors: */
	public:
	BaseAppearanceNode(void); // Creates an unattached base appearance node
	
	/* Methods from class AttributeNode: */
	virtual void setGLState(GLRenderState& renderState) const;
	virtual void resetGLState(GLRenderState& renderState) const;
	
	/* New methods: */
	virtual int getAppearanceRequirementMask(void) const =0; // Returns the mask of requirements this appearance node has of geometry nodes
	virtual bool isTransparent(void) const =0; // Returns true if the appearance in this node means that the geometry using it will be rendered during the transparent rendering pass
	virtual void addGeometryRequirement(int geometryRequirementMask); // Adds a mask of geometry requirement flags
	virtual void removeGeometryRequirement(int geometryRequirementMask); // Removes a mask of geometry requirement flags
	virtual int setGLState(int geometryRequirementMask,GLRenderState& renderState) const =0; // Sets OpenGL state for rendering based on the requirements of the geometry to be rendered; returns mask of appearance requirements for the geometry to be rendered
	virtual void resetGLState(int geometryRequirementMask,GLRenderState& renderState) const =0; // Resets OpenGL state after rendering based on the requirements of the geometry to be rendered
	};

typedef Misc::Autopointer<BaseAppearanceNode> BaseAppearanceNodePointer;

}

#endif

/***********************************************************************
BaseAppearanceNode - Base class for nodes defining the appearance
(material properties, textures, etc.) of shape nodes.
Copyright (c) 2019-2024 Oliver Kreylos

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

#include <SceneGraph/BaseAppearanceNode.h>

#include <Misc/StdError.h>

namespace SceneGraph {

/***********************************
Methods of class BaseAppearanceNode:
***********************************/

BaseAppearanceNode::BaseAppearanceNode(void)
	:numHasPoints(0),numHasLines(0),numHasSurfaces(0),numHasTwoSidedSurfaces(0),numHasColors(0)
	{
	}

void BaseAppearanceNode::setGLState(GLRenderState& renderState) const
	{
	/* This is not allowed: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot call without geometryRequirementMask parameter");
	}

void BaseAppearanceNode::resetGLState(GLRenderState& renderState) const
	{
	/* This is not allowed: */
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot call without geometryRequirementMask parameter");
	}

void BaseAppearanceNode::addGeometryRequirement(int geometryRequirementMask)
	{
	/* Add an instance to each requirement in the mask: */
	if(geometryRequirementMask&HasPoints)
		++numHasPoints;
	if(geometryRequirementMask&HasLines)
		++numHasLines;
	if(geometryRequirementMask&HasSurfaces)
		++numHasSurfaces;
	if(geometryRequirementMask&HasTwoSidedSurfaces)
		++numHasTwoSidedSurfaces;
	if(geometryRequirementMask&HasColors)
		++numHasColors;
	}

void BaseAppearanceNode::removeGeometryRequirement(int geometryRequirementMask)
	{
	/* Remove an instance from each requirement in the mask: */
	if(geometryRequirementMask&HasPoints)
		{
		if(numHasPoints==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unbalanced HasPoints field");
		--numHasPoints;
		}
	if(geometryRequirementMask&HasLines)
		{
		if(numHasLines==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unbalanced HasLines field");
		--numHasLines;
		}
	if(geometryRequirementMask&HasSurfaces)
		{
		if(numHasSurfaces==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unbalanced HasSurfaces field");
		--numHasSurfaces;
		}
	if(geometryRequirementMask&HasTwoSidedSurfaces)
		{
		if(numHasTwoSidedSurfaces==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unbalanced HasTwoSidedSurfaces field");
		--numHasTwoSidedSurfaces;
		}
	if(geometryRequirementMask&HasColors)
		{
		if(numHasColors==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unbalanced HasColors field");
		--numHasColors;
		}
	}

}

/***********************************************************************
GLLabelNonIlluminated.vs - Vertex shader to render GLLabel objects
without illumination.
Copyright (c) 2023-2025 Oliver Kreylos

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

#version 130

uniform bool clipPlaneEnableds[gl_MaxClipPlanes];

varying float gl_ClipDistance[gl_MaxClipDistances];
varying vec2 texCoord;
varying vec4 backgroundColor;

void main()
	{
	/* Pass through vertex texture coordinates: */
	texCoord=gl_MultiTexCoord0.xy;
	
	/* Pass through the vertex color: */
	backgroundColor=gl_Color;
	
	/* Transform vertex position to eye space: */
	vec4 eyeVertex=gl_ModelViewMatrix*gl_Vertex;
	
	/* Calculate clipping plane distances for all enabled clipping planes: */
	for(int clipPlaneIndex=0;clipPlaneIndex<gl_MaxClipPlanes;++clipPlaneIndex)
		if(clipPlaneEnableds[clipPlaneIndex])
			gl_ClipDistance[clipPlaneIndex]=dot(gl_ClipPlane[clipPlaneIndex],eyeVertex);
	
	/* Use standard vertex position: */
	gl_Position=ftransform();
	}

/***********************************************************************
PhongAppearanceShaderColorTex2D.vs - Vertex shader for Phong shading
with vertex colors and 2D texture mapping.
Copyright (c) 2022-2025 Oliver Kreylos

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

varying vec4 position,color;
varying vec3 normal;
varying vec3 texCoord;
varying float gl_ClipDistance[gl_MaxClipDistances];

void main()
	{
	/* Pass through vertex color: */
	color=gl_Color;
	
	/* Pass through vertex texture coordinates: */
	vec4 tc=gl_TextureMatrix[0]*gl_MultiTexCoord0;
	texCoord=vec3(tc.xy,tc.w);
	
	/* Pass through vertex normal and position in eye coordinates: */
	normal=gl_NormalMatrix*gl_Normal;
	position=gl_ModelViewMatrix*gl_Vertex;
	
	/* Calculate clipping plane distances for all enabled clipping planes: */
	for(int clipPlaneIndex=0;clipPlaneIndex<gl_MaxClipPlanes;++clipPlaneIndex)
		if(clipPlaneEnableds[clipPlaneIndex])
			gl_ClipDistance[clipPlaneIndex]=dot(gl_ClipPlane[clipPlaneIndex],position);
	
	/* Use standard vertex position: */
	gl_Position=ftransform();
	}

/***********************************************************************
GLLabelIlluminated.fs - Fragment shader to render GLLabel objects with
illumination.
Copyright (c) 2023 Oliver Kreylos

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

uniform vec4 foregroundColor;
uniform sampler2D stringTexture;

varying vec2 texCoord;
varying vec4 ambientDiffuseColor;
varying vec4 specularColor;

void main()
	{
	/* Assign the final fragment color: */
	gl_FragColor=mix(foregroundColor,ambientDiffuseColor+specularColor,texture2D(stringTexture,texCoord).r);
	}

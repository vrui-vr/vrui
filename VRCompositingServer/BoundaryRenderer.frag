/***********************************************************************
BoundaryRenderer.frag - Fragment shader for the VR boundary renderer.
Copyright (c) 2023 Oliver Kreylos
***********************************************************************/

#version 450

layout(location=0) in vec4 inColor;

layout(location=0) out vec4 outColor;

void main()
	{
	/* Copy the incoming color into the fragment color: */
	outColor=inColor;
	}

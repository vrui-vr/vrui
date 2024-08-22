/***********************************************************************
BoundaryRenderer.vert - Vertex shader for the VR boundary renderer.
Copyright (c) 2023 Oliver Kreylos
***********************************************************************/

#version 450

layout(location=0) in vec3 pos;

layout(push_constant) uniform RenderState
	{
	mat4 pmv; // Combined projection-modelview matrix for physical space
	vec4 color; // Color to render
	} renderState;

layout(location=0) out vec4 color;

void main()
	{
	/* Transform the vertex position from physical space to clip space: */
	gl_Position=renderState.pmv*vec4(pos,1.0);
	
	/* Set the vertex color: */
	color=renderState.color;
	}

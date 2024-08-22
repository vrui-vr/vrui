/***********************************************************************
DistortionCorrection.vert - Vertex shader for the VR compositing shader.
Copyright (c) 2022 Oliver Kreylos
***********************************************************************/

#version 450

layout(location=0) in vec2 redTexCoord;
layout(location=1) in vec2 greenTexCoord;
layout(location=2) in vec2 blueTexCoord;
layout(location=3) in vec2 pos;

layout(push_constant) uniform ReprojectionState
	{
	mat3 rotation;
	vec2 viewportOffset;
	vec2 viewportScale;
	} reprojection;

layout(location=0) out vec2 fragRedTexCoord;
layout(location=1) out vec2 fragGreenTexCoord;
layout(location=2) out vec2 fragBlueTexCoord;

vec2 reproject(in vec2 tex)
	{
	/* Convert the texture coordinate from viewport space to tangent space: */
	vec3 tan=vec3(reprojection.viewportOffset+reprojection.viewportScale*tex,-1.0);
	
	/* Rotate the tangent space vector: */
	vec3 rtan=reprojection.rotation*tan;
	
	/* Convert the rotated tangent space vector back to viewport space: */
	vec2 rftex=(vec2(-rtan.x/rtan.z,-rtan.y/rtan.z)-reprojection.viewportOffset)/reprojection.viewportScale;
	
	return rftex;
	
	/* Flip the final texture coordinate's y coordinate to account for Vulkan having y go down: */
	return vec2(rftex.x,1.0-rftex.y);
	}

void main()
	{
	/* Copy the per-vertex per-primary color texture coordinates: */
	fragRedTexCoord=reproject(redTexCoord);
	fragGreenTexCoord=reproject(greenTexCoord);
	fragBlueTexCoord=reproject(blueTexCoord);
	
	/* Set the vertex position: */
	gl_Position=vec4(pos,0.0,1.0);
	}

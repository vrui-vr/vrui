/***********************************************************************
DistortionCorrection.frag - Fragment shader for the VR compositing
shader.
Copyright (c) 2022 Oliver Kreylos
***********************************************************************/

#version 450

layout(location=0) in vec2 redTexCoord;
layout(location=1) in vec2 greenTexCoord;
layout(location=2) in vec2 blueTexCoord;

layout(binding=0) uniform sampler2D inputImageSampler;

layout(location=0) out vec4 outColor;

void main()
	{
	/* Look up the three primary color components individually: */
	float red=texture(inputImageSampler,redTexCoord).r;
	float green=texture(inputImageSampler,greenTexCoord).g;
	float blue=texture(inputImageSampler,blueTexCoord).b;
	
	/* Combine the three color components into the output color value: */
	outColor=vec4(red,green,blue,1.0);
	}

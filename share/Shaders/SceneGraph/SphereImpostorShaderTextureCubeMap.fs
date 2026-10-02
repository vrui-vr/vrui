/***********************************************************************
SphereImpostorShaderCubeMap.fs - Fragment shader for sphere impostor
rendering with cube map texture mapping.
Copyright (c) 2026 Oliver Kreylos

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

uniform vec3 sphereCenter; // Vector from eye position to sphere center in eye coordinates
uniform float sphereRadius; // Sphere radius in eye coordinate units
uniform float c; // Constant coefficient of the sphere intersection formula; does not depend on varying rayDirection
uniform samplerCube texture; // Sampler for the cube map texture
uniform mat3 textureMatrix; // Matrix to fix texture coordinates in model space

varying vec3 rayDirection;

void accumulateDirectionalLight(in int lightIndex,in vec4 vertexEc,in vec3 normalEc,inout vec4 ambientDiffuseAccum,inout vec4 specularAccum)
	{
	/* Calculate the light direction: */
	vec3 lightDirEc=normalize(gl_LightSource[lightIndex].position.xyz);
	
	/* Calculate the per-light ambient light term: */
	ambientDiffuseAccum+=gl_FrontLightProduct[lightIndex].ambient;
	
	/* Compute the diffuse lighting angle: */
	float nl=dot(normalEc,lightDirEc);
	if(nl>0.0)
		{
		/* Calculate the per-light diffuse light term: */
		ambientDiffuseAccum+=gl_FrontLightProduct[lightIndex].diffuse*nl;
		
		/* Calculate the eye direction: */
		vec3 eyeDirEc=normalize(-vertexEc.xyz);
		
		/* Calculate the specular lighting angle: */
		float nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);
		
		/* Calculate the per-light specular lighting term: */
		specularAccum+=gl_FrontLightProduct[lightIndex].specular*pow(nhv,gl_FrontMaterial.shininess);
		}
	}

void accumulatePointLight(in int lightIndex,in vec4 vertexEc,in vec3 normalEc,inout vec4 ambientDiffuseAccum,inout vec4 specularAccum)
	{
	/* Calculate the light direction (works for arbitrary homogeneous weights): */
	vec3 lightDirEc=gl_LightSource[lightIndex].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[lightIndex].position.w;
	float lightDist=length(lightDirEc)/(gl_LightSource[lightIndex].position.w*vertexEc.w);
	lightDirEc=normalize(lightDirEc);
	
	/* Calculate the light attenuation factor: */
	float att=1.0/((gl_LightSource[lightIndex].quadraticAttenuation*lightDist+gl_LightSource[lightIndex].linearAttenuation)*lightDist+gl_LightSource[lightIndex].constantAttenuation);
	
	/* Calculate the per-light ambient light term: */
	ambientDiffuseAccum+=gl_FrontLightProduct[lightIndex].ambient*att;
	
	/* Calculate the diffuse lighting angle: */
	float nl=dot(normalEc,lightDirEc);
	if(nl>0.0)
		{
		/* Calculate per-source diffuse light term: */
		ambientDiffuseAccum+=gl_FrontLightProduct[lightIndex].diffuse*(nl*att);
		
		/* Calculate the eye direction: */
		vec3 eyeDirEc=normalize(-vertexEc.xyz);
		
		/* Calculate the specular lighting angle: */
		float nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);
		
		/* Calculate the per-light specular lighting term: */
		specularAccum+=gl_FrontLightProduct[lightIndex].specular*(pow(nhv,gl_FrontMaterial.shininess)*att);
		}
	}

void accumulateSpotLight(in int lightIndex,in vec4 vertexEc,in vec3 normalEc,inout vec4 ambientDiffuseAccum,inout vec4 specularAccum)
	{
	/* Calculate the light direction (works for arbitrary homogeneous weights): */
	vec3 lightDirEc=gl_LightSource[lightIndex].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[lightIndex].position.w;
	float lightDist=length(lightDirEc)/(gl_LightSource[lightIndex].position.w*vertexEc.w);
	lightDirEc=normalize(lightDirEc);
	
	/* Calculate the spot light angle: */
	float sl=-dot(lightDirEc,normalize(gl_LightSource[lightIndex].spotDirection));
	
	/* Check if the point is inside the spot light's cone: */
	if(sl>=gl_LightSource[lightIndex].spotCosCutoff)
		{
		/* Calculate the light attenuation factor: */
		float att=1.0/((gl_LightSource[lightIndex].quadraticAttenuation*lightDist+gl_LightSource[lightIndex].linearAttenuation)*lightDist+gl_LightSource[lightIndex].constantAttenuation);
		
		/* Calculate the spot light attenuation factor: */
		att*=pow(sl,gl_LightSource[lightIndex].spotExponent);
		
		/* Calculate the per-light ambient light term: */
		ambientDiffuseAccum+=gl_FrontLightProduct[lightIndex].ambient*att;
		
		/* Calculate the diffuse lighting angle: */
		float nl=dot(normalEc,lightDirEc);
		if(nl>0.0)
			{
			/* Calculate the per-light diffuse light term: */
			ambientDiffuseAccum+=gl_FrontLightProduct[lightIndex].diffuse*(nl*att);
			
			/* Calculate the eye direction: */
			vec3 eyeDirEc=normalize(-vertexEc.xyz);
			
			/* Calculate the specular lighting angle: */
			float nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);
			
			/* Calculate the per-light specular lighting term: */
			specularAccum+=gl_FrontLightProduct[lightIndex].specular*(pow(nhv,gl_FrontMaterial.shininess)*att);
			}
		}
	}

void main()
	{
	/* Intersect the ray from the eye position to the pixel position with the sphere: */
	float a=dot(rayDirection,rayDirection);
	float b=-2.0*dot(sphereCenter,rayDirection);
	float disc=b*b-4.0*a*c;
	if(disc<0.0)
		discard;
	float lambda=(2.0*c)/(sqrt(disc)-b);
	vec3 position=rayDirection*lambda;
	vec3 normal=(position-sphereCenter)/sphereRadius;
	vec4 vertexEc=vec4(position,1.0);
	
	/* Calculate the intersection point's depth buffer value: */
	vec4 vertexC=gl_ProjectionMatrix*vertexEc;
	gl_FragDepth=0.5*(vertexC.z*gl_DepthRange.diff/vertexC.w+gl_DepthRange.near+gl_DepthRange.far);
	
	/* Calculate texture coordinates: */
	vec3 texPosition=(textureMatrix*normal).xzy;
	
	/* Start with the global ambient light term: */
	vec4 ambientDiffuseColor=gl_FrontLightModelProduct.sceneColor;
	vec4 specularColor=vec4(0.0,0.0,0.0,1.0);
	
	/* Accumulate per-lightsource contributions: */
	// for(int lightIndex=0;lightIndex<gl_MaxLights;++lightIndex)
	for(int lightIndex=0;lightIndex<1;++lightIndex)
		// if(lightEnableds[lightIndex])
			{
			if(gl_LightSource[lightIndex].position.w==0.0)
				accumulateDirectionalLight(lightIndex,vertexEc,normal,ambientDiffuseColor,specularColor);
			else if(gl_LightSource[lightIndex].spotCosCutoff<-0.001)
				accumulatePointLight(lightIndex,vertexEc,normal,ambientDiffuseColor,specularColor);
			else
				accumulateSpotLight(lightIndex,vertexEc,normal,ambientDiffuseColor,specularColor);
			}
	
	/* Assign the final fragment color: */
	gl_FragColor=ambientDiffuseColor*textureCube(texture,texPosition)+specularColor+gl_FrontMaterial.emission;
	}

/***********************************************************************
PhongAppearanceShaderTex2D.fs - Fragment shader for Phong shading
without vertex colors and with 2D texture mapping.
Copyright (c) 2022-2024 Oliver Kreylos

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

uniform bool lightEnableds[gl_MaxLights];
uniform sampler2D texture;

varying vec4 position;
varying vec3 normal;
varying vec3 texCoord;

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
		ambientDiffuseAccum+=(gl_FrontLightProduct[lightIndex].ambient)*att;
		
		/* Calculate the diffuse lighting angle: */
		float nl=dot(normalEc,lightDirEc);
		if(nl>0.0)
			{
			/* Calculate the per-light diffuse light term: */
			ambientDiffuseAccum+=(gl_FrontLightProduct[lightIndex].diffuse)*(nl*att);
			
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
	/* Start with the global ambient light term: */
	vec4 ambientDiffuseColor=gl_FrontLightModelProduct.sceneColor;
	vec4 specularColor=vec4(0.0,0.0,0.0,1.0);
	
	/* Accumulate per-lightsource contributions: */
	vec3 fragNormal=normalize(normal);
	for(int lightIndex=0;lightIndex<gl_MaxLights;++lightIndex)
		if(lightEnableds[lightIndex])
			{
			if(gl_LightSource[lightIndex].position.w==0.0)
				accumulateDirectionalLight(lightIndex,position,fragNormal,ambientDiffuseColor,specularColor);
			else if(gl_LightSource[lightIndex].spotCosCutoff<-0.001)
				accumulatePointLight(lightIndex,position,fragNormal,ambientDiffuseColor,specularColor);
			else
				accumulateSpotLight(lightIndex,position,fragNormal,ambientDiffuseColor,specularColor);
			}
	
	/* Assign the final fragment color: */
	gl_FragColor=ambientDiffuseColor*texture2D(texture,texCoord.xy)+specularColor;
	}

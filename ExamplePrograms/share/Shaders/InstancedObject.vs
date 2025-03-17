// Vertex shader for per-vertex lighting using the same formulae as fixed-functionality OpenGL

attribute vec3 v_normal;
attribute vec4 v_position;
attribute vec4 i_color;
attribute vec3 i_translation;
attribute vec4 i_rotation;

vec3 rotate(in vec4 rotation,in vec3 vec)
	{
	float wxvx=rotation.y*vec.z-rotation.z*vec.y+rotation.w*vec.x;
	float wxvy=rotation.z*vec.x-rotation.x*vec.z+rotation.w*vec.y;
	float wxvz=rotation.x*vec.y-rotation.y*vec.x+rotation.w*vec.z;
	return vec3(vec.x+2.0*(rotation.y*wxvz-rotation.z*wxvy),
	            vec.y+2.0*(rotation.z*wxvx-rotation.x*wxvz),
	            vec.z+2.0*(rotation.x*wxvy-rotation.y*wxvx));
	}

vec4 rotate(in vec4 rotation,in vec4 vec)
	{
	float wxvx=rotation.y*vec.z-rotation.z*vec.y+rotation.w*vec.x;
	float wxvy=rotation.z*vec.x-rotation.x*vec.z+rotation.w*vec.y;
	float wxvz=rotation.x*vec.y-rotation.y*vec.x+rotation.w*vec.z;
	return vec4(vec.x+2.0*(rotation.y*wxvz-rotation.z*wxvy),
	            vec.y+2.0*(rotation.z*wxvx-rotation.x*wxvz),
	            vec.z+2.0*(rotation.x*wxvy-rotation.y*wxvx),
	            vec.w);
	}

void main()
	{
	/* Calculate per-source ambient light term: */
	vec4 color=gl_LightSource[0].ambient*i_color;
	
	/* Compute the normal vector: */
	vec3 normal=normalize(gl_NormalMatrix*rotate(i_rotation,v_normal));
	
	/* Compute the light direction (works both for directional and point lights): */
	vec4 vertexEc=gl_ModelViewMatrix*(rotate(i_rotation,v_position)+vec4(i_translation,0.0));
	vec3 lightDir=gl_LightSource[0].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[0].position.w;
	float lightDist=length(lightDir);
	lightDir=normalize(lightDir);
	
	/* Compute the diffuse lighting angle: */
	float nl=dot(normal,lightDir);
	if(nl>0.0)
		{
		/* Calculate per-source diffuse light term: */
		color+=(gl_LightSource[0].diffuse*i_color)*nl;
		
		/* Compute the eye direction: */
		vec3 eyeDir=normalize(-vertexEc.xyz);
		
		/* Compute the specular lighting angle: */
		float nhv=max(dot(normal,normalize(eyeDir+lightDir)),0.0);
		
		/* Calculate per-source specular lighting term: */
		color+=(gl_LightSource[0].specular*gl_FrontMaterial.specular)*pow(nhv,gl_FrontMaterial.shininess);
		}
	
	/* Attenuate the per-source light terms: */
	float att=(gl_LightSource[0].quadraticAttenuation*lightDist+gl_LightSource[0].linearAttenuation)*lightDist+gl_LightSource[0].constantAttenuation;
	color=color*(1.0/att);
	
	/* Calculate global ambient light term: */
	color+=gl_LightModel.ambient*gl_FrontMaterial.ambient;
	
	/* Compute final vertex color: */
	gl_FrontColor=color;
	
	/* Compute vertex position: */
	gl_Position=gl_ProjectionMatrix*vertexEc;
	}

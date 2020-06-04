#vertex
#version 330 core

in vec3 in_position;
in vec2 in_uv;
in vec3 in_normal;

out vec2 pass_uv;
out vec3 pass_normal;
out vec3 pass_worldPos;
out vec3 pass_cameraPos;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_transMat;
uniform mat3 u_normMat;
uniform float u_time;
uniform float u_tiling = 1;

void main(void) {
	vec4 worldPos = u_transMat * vec4(in_position, 1.0);
	gl_Position = u_projMat * u_viewMat * worldPos;
	
	pass_normal = u_normMat * in_normal;
	pass_worldPos = worldPos.xyz;
	pass_cameraPos = (inverse(u_viewMat) * vec4(0, 0, 0, 1)).xyz;
	pass_uv = in_uv * u_tiling;
}

#fragment
#version 330 core
#include "res/shaders/3d_shader.glh"

const float pi = 3.14159;

out vec4 out_colour;

uniform sampler2D t_diffuse;
uniform float shineDamper = 50;
uniform float reflectivity = 0.1;
uniform vec3 u_cameraPos;
uniform vec4 u_emission;

void main(void) {
	// simple phong shader
	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	vec3 diffuseColour = texture(t_diffuse, pass_uv).rgb;
	vec3 unitNormal = normalize(pass_normal);
	vec3 unitView = normalize(pass_cameraPos - pass_worldPos);
	
	for(int i = 0; i < numLights; ++i) {
		vec4 lightDirection = getLightVector(u_lights[i]);
		
		if(lightDirection == vec4(0))
			// fragment is not affected by this light
			continue; 
		   
		vec3 lightColour = u_lights[i].colour.rgb * u_lights[i].colour.w;
																		   // This works for now, for whatever reason LOL
		float brightness = max(dot(lightDirection.xyz, unitNormal), 0.0) * 0.2;
		totalDiffuse  += lightDirection.w * lightColour * brightness;
		
		
		vec3 reflectedLightDir = reflect(-lightDirection.xyz, unitNormal);
		float specularFactor = dot(reflectedLightDir, unitView);
		specularFactor = max(specularFactor, 0.0);

		float dampedFactor = pow(specularFactor, shineDamper);
		dampedFactor = max(dampedFactor, 0.0);
		vec3 finalSpecular = dampedFactor * reflectivity * lightColour;

	    totalSpecular += finalSpecular * lightDirection.w;
	}
	
	out_colour = vec4(totalDiffuse * diffuseColour + totalSpecular + u_emission.rgb, 1.0);
}
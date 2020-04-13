#version 330 core

const float pi = 3.14159;
const int lightCount = 1;

struct Light {
	vec3 vector;
	vec4 colour;
	vec4 data1;
	vec4 data2;
	int type;
};

in vec2 pass_uv;
in vec3 pass_normal;
in vec3 pass_worldPos;
in vec3 pass_cameraPos;

out vec4 out_colour;

uniform sampler2D t_diffuse;
uniform float shineDamper = 50;
uniform float reflectivity = 0.1;
uniform vec3 u_cameraPos;
uniform Light u_lights[lightCount];

vec4 getLightVector(Light light) {
	if(light.type == 1) {
		// point light
		vec3 dir = normalize(light.vector - pass_worldPos);
		float dist = length(dir);
		float attenuation = 1.0 / (light.data1.x + light.data1.y * dist + light.data1.z * (dist * dist));    
		return vec4(dir, attenuation);		
	}
	
	if(light.type == 2)
		// directional light
		return vec4(normalize(-light.vector), 1.0);

	if(light.type == 3) {
		// spot light
		vec3 dir = normalize(light.vector - pass_worldPos);
		vec3 lightDir = vec3(-light.data1.xyz);
		float theta = dot(dir, lightDir);
		if(theta > light.data2.x) {
			float epsilon   = light.data2.x - light.data1.w;
			float intensity = clamp((theta - light.data2.x) / epsilon, 0.0, 1.0);
			float dist = length(dir);
			float attenuation = 1.0 / (light.data2.y + light.data2.z * dist + light.data2.w * (dist * dist));   
			return vec4(dir, intensity * attenuation);
		}
	
		return vec4(0.0);
	}
	
	return vec4(1, 0, 0, 1);
}

void main(void) {
	// simple phong shader
	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	vec3 diffuseColour = texture(t_diffuse, pass_uv).rgb;
	vec3 unitNormal = normalize(pass_normal);
	vec3 unitView = normalize(pass_cameraPos - pass_worldPos);
	
	for(int i = 0; i < lightCount; ++i) {
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
	
	out_colour = vec4(totalDiffuse * diffuseColour + totalSpecular + vec3(0.00) * diffuseColour, 1.0);
}
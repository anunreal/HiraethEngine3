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
in mat3 pass_tangSpace;
in vec3 pass_tangent;

out vec4 out_colour;

uniform sampler2D t_diffuse;
uniform sampler2D t_normal;
uniform sampler2D t_arm;
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
		return vec4(normalize(light.vector), 1.0);

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

float NDF(vec3 normal, vec3 halfway, float roughness) {
	float r2 = roughness * roughness;
	float ndoth = max(dot(normal, halfway), 0.0);
	float ndoth2 = ndoth * ndoth;
	float nom = r2;
	float denom = (ndoth2 * (r2 - 1.0) + 1.0);
	denom = pi * denom * denom;
	return nom / denom;
}

float GGX(float ndotv, float k) {
	float nom = ndotv;
	float denom = ndotv * (1.0 - k) + k;
	return nom / denom;
}

float GS(vec3 normal, vec3 viewdir, vec3 L, float k) {
	float ndotv = max(dot(normal, viewdir), 0.0);
	float ndotl = max(dot(normal, L), 0.0);
	float ggx1 = GGX(ndotv, k);
	float ggx2 = GGX(ndotl, k);
	return ggx1 * ggx2;
}

vec3 fresnel(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 getLightColour(vec3 normal, vec3 lightDirection, vec3 viewDirection, vec3 halfway, vec3 F0, vec3 diffuse, vec3 lightColour, float roughness, float metallic) {
	float d = NDF(normal, halfway, roughness);
	float g = GS(normal, viewDirection, lightDirection, roughness);
	vec3 f = fresnel(max(dot(halfway, viewDirection), 0.0), F0);
	vec3 ks = f;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - metallic;
	
	vec3 numerator = d * g * f;
	float denominator = 4.0 * max(dot(normal, viewDirection), 0.0) * max(dot(normal, lightDirection), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);
	float ndotl = max(dot(normal, lightDirection), 0.0);
	return (kd * diffuse / pi + specular) * lightColour * ndotl;
}

void main(void) {
	
	vec3 normalRgb = texture(t_normal, pass_uv).rgb;
	if(normalRgb == vec3(0))
		// probably no normal map found, use "default" normal
		normalRgb = vec3(0.5, 0.5, 1.0);

	vec3 normal = normalize(pass_tangSpace * normalize(2.0 * normalRgb - 1.0));
	
	vec4 diffuse = texture(t_diffuse, pass_uv);
	float metallic = texture(t_arm, pass_uv).b;
	float roughness = texture(t_arm, pass_uv).g;
	float ao = texture(t_arm, pass_uv).r;

	vec3 F0 = vec3(0.0);
	F0 = mix(F0, diffuse.rgb, metallic);

	vec3 viewDirection = normalize(pass_cameraPos - pass_worldPos);
	vec3 totalLight = vec3(0.0);
	
	for(int i = 0; i < lightCount; ++i) { 
		vec4 lightDirection = getLightVector(u_lights[i]);
		  
		if(lightDirection == vec4(0))
			// fragment is not affected by this light
			continue; 
		   
		vec3 halfwayDirection = normalize(lightDirection.xyz + viewDirection);
		
		vec3 lightColour = getLightColour(normal, lightDirection.xyz, viewDirection, halfwayDirection, F0, diffuse.rgb, (u_lights[i].colour.rgb * u_lights[i].colour.w) * lightDirection.w, roughness, metallic);
		totalLight += lightColour; 
	}  
	 
	vec3 ambient = vec3(0.03) * diffuse.rgb * ao;
	out_colour = vec4(ambient + totalLight, 1.0);
}
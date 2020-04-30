#vertex
#version 140
in vec2 in_position;

out vec2 pass_uv;

void main(void) {
	gl_Position = vec4(in_position, 0.0, 1.0);
	pass_uv = in_position * 0.5 + 0.5;
}

#fragment
#version 330 core

const float pi = 3.14159;
const int lightCount = 1;

in vec2 pass_uv;

out vec4 out_colour;

struct Light {
	vec3 vector;
	vec4 colour;
	vec4 data1;
	vec4 data2;
	int type;
};

uniform sampler2D t_worldSpace;
uniform sampler2D t_normals;
uniform sampler2D t_diffuse;
uniform sampler2D t_arm;
uniform sampler2D t_brdf;
uniform samplerCube t_irradiance;
uniform samplerCube t_environment;
uniform vec3 u_cameraPos;
uniform Light u_lights[lightCount];

vec4 getLightVector(Light light, vec3 worldPos) {
	if(light.type == 1) {
		// point light
		vec3 dir = normalize(light.vector - worldPos);
		float dist = length(dir);
		float attenuation = 1.0 / (light.data1.x + light.data1.y * dist + light.data1.z * (dist * dist));    
		return vec4(dir, attenuation);		
	}
	
	if(light.type == 2)
		// directional light
		return vec4(normalize(-light.vector), 1.0);

	if(light.type == 3) {
		// spot light
		vec3 dir = normalize(light.vector - worldPos);
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


float m1_NDF(vec3 normal, vec3 halfway, float roughness) {
	float r2 = roughness * roughness;
	float ndoth = max(dot(normal, halfway), 0.0);
	float ndoth2 = ndoth * ndoth;
	float nom = r2;
	float denom = (ndoth2 * (r2 - 1.0) + 1.0);
	denom = pi * denom * denom;
	return nom / denom;
}

float m1_GGX(float ndotv, float k) {
	float nom = ndotv;
	float denom = ndotv * (1.0 - k) + k;
	return nom / denom;
}

float m1_GS(vec3 normal, vec3 viewdir, vec3 L, float k) {
	float ndotv = max(dot(normal, viewdir), 0.0);
	float ndotl = max(dot(normal, L), 0.0);
	float ggx1 = m1_GGX(ndotv, k);
	float ggx2 = m1_GGX(ndotl, k);
	return ggx1 * ggx2;
}

vec3 m1_fresnel(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}  

vec3 m1_getLightColour(vec3 lightColour, vec3 lightDirection, vec3 normal, vec3 viewDirection, vec3 diffuse) {
	vec4 arm = texture(t_arm, pass_uv);
	float metallic = arm.b;
	float roughness = arm.g;
	
	vec3 F0 = vec3(0.0);
	F0 = mix(F0, diffuse.rgb, metallic);
	
	vec3 halfwayDirection = normalize(lightDirection.xyz + viewDirection);
		
	float d = m1_NDF(normal, halfwayDirection, roughness);
	float g = m1_GS(normal, viewDirection, lightDirection, roughness);
	vec3 f = m1_fresnel(max(dot(halfwayDirection, viewDirection), 0.0), F0);
	vec3 ks = f;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - metallic;
	
	vec3 numerator = d * g * f;
	float denominator = 4.0 * max(dot(normal, viewDirection), 0.0) * max(dot(normal, lightDirection), 0.0);
	vec3 specular = numerator / max(denominator, 0.001);
	float ndotl = max(dot(normal, lightDirection), 0.0);
	return (kd * diffuse / pi + specular) * lightColour * ndotl;
}

float m1_getAo() {
	return texture(t_arm, pass_uv).r;
}

vec3 getLight(int materialType, vec3 lightColour, vec3 lightDirection, vec3 normal, vec3 viewDirection, vec3 diffuse) {
	vec3 light = vec3(0);
	
	switch(materialType) {
		case 1:
		light = m1_getLightColour(lightColour, lightDirection, normal, viewDirection, diffuse);
		break;
		
		case 2:
		float brightness = max(dot(lightDirection.xyz, normal), 0.0) * 0.2;
		light =  lightColour * brightness * diffuse;
		break;
	}
	
	return light;
}

float getAo(int materialType) {
	float ao = 1.0;
	
	switch(materialType) {
		case 1:
		ao = m1_getAo();
		break;
	}
	
	return ao;
}

void main(void) {
	vec4 worldPosTex = texture(t_worldSpace, pass_uv);
	int material = int(worldPosTex.a);
	if(material == 0)
		discard;
	
	vec3 worldPos = worldPosTex.xyz;
	vec3 normal   = normalize(texture(t_normals, pass_uv).rgb);
	vec4 albedo   = texture(t_diffuse, pass_uv);
	vec3 viewDir  = normalize(u_cameraPos - worldPos);
	vec3 totalLight = vec3(0.0);
	
	for(int i = 0; i < lightCount; ++i) {
		Light l = u_lights[i];
		vec4 lightDir = getLightVector(l, normal);
		if(lightDir.w > 0.0)
			totalLight += getLight(material, (l.colour.rgb * l.colour.a * lightDir.w), lightDir.xyz, normal, viewDir, albedo.rgb); 
	}
	
	vec4 arm = texture(t_arm, pass_uv);
	float metallic = arm.b;
	float roughness = arm.g;
	float ao = getAo(material);
	vec3 F0 = vec3(0.0);
	F0 = mix(F0, albedo.rgb, metallic);
	vec3 kS = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness); 
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;
	vec3 irradiance = texture(t_irradiance, normal).rgb;
	vec3 diffuse    = irradiance * albedo.rgb;
	
	float lod = 0.0;
	vec3 prefilteredColour = textureLod(t_environment, reflect(-viewDir, normal), roughness * 4).rgb;
	vec2 envBRDF = texture(t_brdf, vec2(dot(normal, viewDir), arm.g)).xy;
	vec3 specular = prefilteredColour * (kS * envBRDF.x + envBRDF.y);
	
	vec3 ambient = (kD * diffuse + specular) * ao; 
	out_colour = vec4(ambient + totalLight, 1.0);
	out_colour.rgb = out_colour.rgb / (out_colour.rgb + vec3(1.0)); 
}

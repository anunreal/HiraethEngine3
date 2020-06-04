#vertex
#version 140

in vec3 in_position;
in vec2 in_uv;
in vec3 in_normal;
in vec3 in_tangent;

out vec2 pass_uv;
out vec3 pass_normal;
out vec3 pass_worldPos;
out vec3 pass_cameraPos;
out mat3 pass_tangSpace;
out vec3 pass_tangent;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_transMat;
uniform mat3 u_normMat;
uniform vec3 u_cameraPos;
uniform float u_time;
uniform float u_tiling = 1;

vec3 getScale(mat4 transMat) {
	vec3 s;
	s.x = length(vec3(transMat[0][0], transMat[0][1], transMat[0][2]));
	s.y = length(vec3(transMat[1][0], transMat[1][1], transMat[1][2]));
	s.z = length(vec3(transMat[2][0], transMat[2][1], transMat[2][2]));
	return s;
}

mat3 getTangentMatrix(mat4 tMat, mat3 nMat) {
	vec3 norm = normalize(nMat * in_normal);
	vec3 tang = normalize(tMat * vec4(in_tangent, 0.0)).xyz; 
	tang = (tang - dot(tang, norm) * norm);
	vec3 bitang = normalize(cross(norm, tang));
	return mat3(tang, bitang, norm);
}  

void main(void) {
	vec4 worldPos = u_transMat * vec4(in_position, 1.0);
	gl_Position = u_projMat * u_viewMat * worldPos;
	
	pass_normal = u_normMat * in_normal;
	//pass_normal = (u_transMat * vec4(in_normal, 0.0)).xyz;
	pass_worldPos = worldPos.xyz;
	//pass_cameraPos = (inverse(u_viewMat) * vec4(0, 0, 0, 1)).xyz;
	pass_cameraPos = u_cameraPos;
	pass_tangSpace = getTangentMatrix(u_transMat, u_normMat);
	//pass_uv = in_uv * getScale(u_transMat).xy * u_tiling;
	pass_uv = in_uv * u_tiling;
	pass_tangent = in_tangent;
}

#fragment
#version 430
#include "res/shaders/3d_shader.glh"

const float pi = 3.14159;
 
in mat3 pass_tangSpace;
in vec3 pass_tangent;

out vec4 out_colour;

uniform samplerCube t_irradiance;
uniform samplerCube t_specular;
uniform sampler2D t_diffuse;
uniform sampler2D t_normal;
uniform sampler2D t_arm;
uniform sampler2D t_brdf;
uniform vec3 u_cameraPos;
uniform vec4 u_emission;

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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
	
	vec4 albedo = texture(t_diffuse, pass_uv);
	float metallic = texture(t_arm, pass_uv).b;
	float roughness = texture(t_arm, pass_uv).g;
	float ao = texture(t_arm, pass_uv).r;

	vec3 F0 = vec3(0.0);
	F0 = mix(F0, albedo.rgb, metallic);

	vec3 viewDirection = normalize(pass_cameraPos - pass_worldPos);
	vec3 totalLight = vec3(0.0);
	
	for(int i = 0; i < numLights; ++i) { 
		vec4 lightDirection = getLightVector(u_lights[i]);
		  
		if(lightDirection == vec4(0))
			// fragment is not affected by this light
			continue; 
		   
		vec3 halfwayDirection = normalize(lightDirection.xyz + viewDirection);
		
		vec3 lightColour = getLightColour(normal, lightDirection.xyz, viewDirection, halfwayDirection, F0, albedo.rgb, (u_lights[i].colour.rgb * u_lights[i].colour.w) * lightDirection.w, roughness, metallic);
		totalLight += lightColour; 
	}   
	 
	vec3 kS = fresnelSchlickRoughness(max(dot(normal, viewDirection), 0.0), F0, roughness); 
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;
	vec3 irradiance = texture(t_irradiance, normal).rgb;
	vec3 diffuse    = irradiance * albedo.rgb;
	
	vec3 prefilteredColour = textureLod(t_specular, reflect(-viewDirection, normal), roughness * 4).rgb;
	vec2 envBRDF = texture(t_brdf, vec2(dot(normal, viewDirection), roughness)).xy;
	vec3 specular = prefilteredColour * (kS * envBRDF.x + envBRDF.y);
	
	vec3 ambient = (kD * diffuse + specular) * ao;
	 
	out_colour = vec4(ambient + totalLight + u_emission.rgb, 1.0);
}
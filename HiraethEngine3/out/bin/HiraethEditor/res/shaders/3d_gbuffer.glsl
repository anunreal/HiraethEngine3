#vertex
#version 140

in vec3 in_position;
in vec2 in_uv;
in vec3 in_normal;
in vec3 in_tangent;

out vec3 pass_position;
out vec3 pass_normal;
out vec2 pass_uv;
out mat3 pass_tangentSpace;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_transMat;
uniform mat3 u_normMat;

mat3 getTangentMatrix(mat4 tMat) {
	vec3 norm = normalize(u_normMat * in_normal);
	vec3 tang = normalize(tMat      * vec4(in_tangent, 0.0)).xyz; 
	tang = (tang - dot(tang, norm)  * norm);
	vec3 bitang = normalize(cross(norm, tang));
	return mat3(tang, bitang, norm);
}  

void main(void) {
	vec4 worldPos = u_transMat * vec4(in_position, 1.0);
	gl_Position = u_projMat * u_viewMat * worldPos;
	
	pass_position     = worldPos.xyz;
	pass_normal       = u_normMat * in_normal;
	pass_uv           = in_uv;
	pass_tangentSpace = getTangentMatrix(u_transMat);
	
}

#fragment
#version 430

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_diffuse;
layout(location = 3) out vec4 out_arm;
layout(location = 4) out vec4 out_emission;

in vec3 pass_position;
in vec3 pass_normal;
in vec2 pass_uv;
in mat3 pass_tangentSpace;

uniform sampler2D t_diffuse;
uniform sampler2D t_normal;
uniform sampler2D t_arm;
uniform vec4 	  u_emission;
uniform int 	  u_materialType;

void main(void) {
	vec3 normalRgb = texture(t_normal, pass_uv).rgb;
	if(normalRgb == vec3(0))
		// probably no normal map found, use "default" normal
		out_normal = vec4(pass_normal, 1);
	else
		out_normal = vec4(normalize(pass_tangentSpace * (normalRgb * 2.0 - 1.0)), 1.0);

	out_position = vec4(pass_position, float(u_materialType));
	out_diffuse  = texture(t_diffuse, pass_uv);
	out_arm      = texture(t_arm, pass_uv);
	out_emission = u_emission;
}
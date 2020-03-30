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
uniform float u_time;
uniform float u_tiling = 1;

vec3 getScale(mat4 transMat) {
	vec3 s;
	s.x = length(vec3(transMat[0][0], transMat[0][1], transMat[0][2]));
	s.y = length(vec3(transMat[1][0], transMat[1][1], transMat[1][2]));
	s.z = length(vec3(transMat[2][0], transMat[2][1], transMat[2][2]));
	return s;
}

mat3 getTangentMatrix(mat4 tMat) {
	vec3 norm = normalize(tMat * vec4(in_normal, 0.0)).xyz;
	vec3 tang = normalize(tMat * vec4(in_tangent, 0.0)).xyz; 
	tang = (tang - dot(tang, norm) * norm);
	vec3 bitang = normalize(cross(norm, tang));
	return mat3(tang, bitang, norm);
}  

void main(void) {
	vec4 worldPos = u_transMat * vec4(in_position, 1.0);
	gl_Position = u_projMat * u_viewMat * worldPos;
	
	pass_normal = normalize(u_transMat * vec4(in_normal, 0.0)).xyz;
	pass_worldPos = worldPos.xyz;
	pass_cameraPos = (inverse(u_viewMat) * vec4(0, 0, 0, 1)).xyz;
	pass_tangSpace = getTangentMatrix(u_transMat);
	//pass_uv = in_uv * getScale(u_transMat).xy * u_tiling;
	pass_uv = in_uv * u_tiling;
	pass_tangent = in_tangent;
}
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
uniform float u_time;
uniform float u_tiling = 1;

void main(void) {
	vec4 worldPos = u_transMat * vec4(in_position, 1.0);
	gl_Position = u_projMat * u_viewMat * worldPos;
	
	pass_normal = normalize(u_transMat * vec4(in_normal, 0.0)).xyz;
	pass_worldPos = worldPos.xyz;
	pass_cameraPos = (inverse(u_viewMat) * vec4(0, 0, 0, 1)).xyz;
	pass_uv = in_uv * u_tiling;
}
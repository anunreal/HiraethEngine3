#version 140

in vec3 in_position;

out vec2 pass_uv;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_transMat;

void main(void) {
	gl_Position = u_projMat * u_viewMat * u_transMat * vec4(in_position, 1.0);
	pass_uv = in_position.xz * 0.5 + 0.5;
}
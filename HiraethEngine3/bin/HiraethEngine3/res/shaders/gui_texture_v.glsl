#version 140

in vec2 in_position;

out vec2 pass_uv;

uniform mat4 u_transMat;

void main(void) {
	gl_Position = u_transMat * vec4(in_position, 0.0, 1.0);
	pass_uv = in_position * 0.5 + 0.5;
}	
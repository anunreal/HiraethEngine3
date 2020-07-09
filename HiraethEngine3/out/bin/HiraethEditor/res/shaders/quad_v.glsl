#version 140

in vec2 in_position;

out vec2 pass_uv;

void main(void) {
	gl_Position = vec4(in_position, 0.0, 1.0);
	pass_uv = in_position * 0.5 + 0.5;
}
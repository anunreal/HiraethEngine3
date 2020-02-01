#version 140

in vec2 in_position;

void main(void) {
	gl_Position = vec4(in_position, 0.0, 1.0);
}
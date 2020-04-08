#version 330 core

precision highp int;

out vec4 out_colour;

in vec4 fs_colour;

void main(void) {
	out_colour = fs_colour;
}
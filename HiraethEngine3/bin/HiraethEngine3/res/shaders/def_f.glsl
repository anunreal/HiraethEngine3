#version 140

in vec2 pass_uv;

out vec4 out_colour;

uniform sampler2D u_texture;

void main(void) {
	out_colour = texture2D(u_texture, pass_uv);
}
#version 140

in vec2 pass_uv;

out vec4 out_colour;

uniform sampler2D t_texture;

void main(void) {
	out_colour = texture2D(t_texture, pass_uv);
	//out_colour.rgb = out_colour.rgb / (out_colour.rgb + 1);
}
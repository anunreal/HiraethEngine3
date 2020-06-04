#vertex
#version 140

in vec2 in_position;
in uint in_colour;

out vec4 pass_colour;

vec4 unpackColour() {
	vec4 v;
	v.a = float((uint(0xFF000000) & in_colour) >> 24);
	v.b = float((uint(0x00FF0000) & in_colour) >> 16);
	v.g = float((uint(0x0000FF00) & in_colour) >> 8);
	v.r = float( uint(0x000000FF) & in_colour);
	return v / 255;
}

void main(void) {
	gl_Position = vec4(in_position, 0.0, 1.0);
	pass_colour = unpackColour();
}

#fragment
#version 140

out vec4 out_colour;

in vec4 pass_colour;

uniform vec4 u_colour;

void main(void) {
	if(u_colour.a == 0.0)
		out_colour = pass_colour;
	else
		out_colour = u_colour;
}
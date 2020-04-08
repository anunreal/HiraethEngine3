#version 330 core

precision highp int;

in vec4  in_position;
in uint  in_colour;
in float in_width;

out VS_OUT {
	vec4  colour;
	float width;
} vs_out;


uniform mat4 u_projMat;
uniform mat4 u_viewMat;

vec4 unpackColour() {
	vec4 v;
	v.a = float((uint(0xFF000000) & in_colour) >> 24);
	v.b = float((uint(0x00FF0000) & in_colour) >> 16);
	v.g = float((uint(0x0000FF00) & in_colour) >> 8);
	v.r = float( uint(0x000000FF) & in_colour);
	return v / 255;
}

void main(void) {
	vec4 pos = vec4(in_position.xy, 0.0, 1.0);

	if(in_position.w == 1)
		pos = (u_projMat * u_viewMat * in_position);

	gl_Position = pos;
	vs_out.colour = unpackColour();
	vs_out.width  = in_width;
}

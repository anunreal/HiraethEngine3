#vertex
#version 140

in vec4  in_vertex;
in uint  in_data;

flat out vec4 pass_colour;
out vec2 pass_uv;

vec4 unpackColour() {
	vec4 v;
	v.a = float((uint(0xFF000000) & in_data) >> 24);
	v.b = float((uint(0x00FF0000) & in_data) >> 16);
	v.g = float((uint(0x0000FF00) & in_data) >> 8);
	v.r = float( uint(0x000000FF) & in_data);
	return v / 255;
}

void main(void) {
	gl_Position = vec4(in_vertex.xy, 0.0, 1.0);
	pass_colour = unpackColour();
	pass_uv 	= in_vertex.zw;
}

#fragment
#version 140

flat in vec4 pass_colour;
in vec2 pass_uv;

out vec4 out_colour;

uniform sampler2D t_atlas;
uniform vec2 	  u_alphaValues;
uniform float 	  u_textSize;

float calculateFontAlpha() {
	float spread = 10;
	float dist = texture(t_atlas, pass_uv).a;
	float smoothing = clamp(0.25 / (spread * u_textSize), 0, 1);
	return smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);
}

void main(void) {
	float alpha = calculateFontAlpha();
	out_colour = vec4(pass_colour.rgb, alpha);
}

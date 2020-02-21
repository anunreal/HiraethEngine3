#version 140

in vec2 pass_uv;

out vec4 out_colour;

uniform sampler2D t_texture;
uniform float u_gamma = 2.2;

void main(void) {
	// tone mapping 
	vec3 hdrRgb = texture2D(t_texture, pass_uv).rgb;
	hdrRgb      = hdrRgb / (hdrRgb + vec3(1.0));
	// gamma correction
	out_colour  = vec4(pow(hdrRgb, vec3(1.0 / u_gamma)), 1.0);
}
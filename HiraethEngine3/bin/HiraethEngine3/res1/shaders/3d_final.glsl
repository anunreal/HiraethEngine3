#version 140

out vec4 out_colour;

in vec2 pass_uv;

uniform sampler2D t_in0;
uniform sampler2D t_in1;
uniform bool  u_hdr;
uniform float u_exposure;
uniform float u_gamma;

void main(void) {
	out_colour = textureLod(t_in0, pass_uv, 0) + textureLod(t_in1, pass_uv, 0) * 0.25;

	if(u_hdr)
		//out_colour.rgb = out_colour.rgb / (out_colour.rgb + vec3(1.0));
		out_colour.rgb = vec3(1.0) - exp(-out_colour.rgb * u_exposure);
	
	if(u_gamma != 1.0)
		out_colour.rgb = pow(out_colour.rgb, vec3(1.0 / u_gamma));
	
	out_colour.a = 1.0;
}
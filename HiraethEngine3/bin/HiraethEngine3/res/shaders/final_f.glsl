#version 140

in vec2 pass_uv;

out vec4 out_colour;

uniform sampler2D t_texture;
uniform float u_gamma = 2.2;
uniform int u_toneMapping = 0;

void main(void) {
	vec4 in_colour = texture2D(t_texture, pass_uv);
	vec3 colour = in_colour.rgb;
	
	// tone mapping 
	if(u_toneMapping == 1)
		colour = colour / (colour + 1.0);
	
	// gamma correction
	out_colour = vec4(pow(colour, vec3(1.0 / u_gamma)), in_colour.a);
}
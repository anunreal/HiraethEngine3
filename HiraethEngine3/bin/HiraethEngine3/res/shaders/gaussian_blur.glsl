#version 140

out vec4 out_colour;

in vec2 pass_uv;

uniform sampler2D t_texture;
uniform vec2 u_direction;
uniform vec2 u_resolution;

vec4 blur13() {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.411764705882353) * u_direction;
	vec2 off2 = vec2(3.2941176470588234) * u_direction;
	vec2 off3 = vec2(5.176470588235294) * u_direction;
	color += texture2D(t_texture, pass_uv) * 0.1964825501511404;
	color += texture2D(t_texture, pass_uv + (off1 / u_resolution)) * 0.2969069646728344;
	color += texture2D(t_texture, pass_uv - (off1 / u_resolution)) * 0.2969069646728344;
	color += texture2D(t_texture, pass_uv + (off2 / u_resolution)) * 0.09447039785044732;
	color += texture2D(t_texture, pass_uv - (off2 / u_resolution)) * 0.09447039785044732;
	color += texture2D(t_texture, pass_uv + (off3 / u_resolution)) * 0.010381362401148057;
	color += texture2D(t_texture, pass_uv - (off3 / u_resolution)) * 0.010381362401148057;
	return color;
}

void main(void) {
	out_colour = blur13();
	out_colour = vec4(1.0, 0.0, 0.0, 1.0);
}
#version 140

out vec4 out_colour;

in vec2 pass_uv;

uniform sampler2D t_in;
uniform vec3 u_threshold;

void main(void) {
	vec4 col = texture(t_in, pass_uv);
	float brightness = dot(col.rgb, u_threshold);
	//brightness = 1.2;
	if(brightness > 1.0) {
        out_colour = vec4(col.rgb, 1.0);
		out_colour.rgb = out_colour.rgb / (out_colour.rgb + vec3(1.0));
	} else {
        out_colour = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
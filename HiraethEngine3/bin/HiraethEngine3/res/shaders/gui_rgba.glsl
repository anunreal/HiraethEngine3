#vertex
#version 140

in vec2 in_position;

void main(void) {
	gl_Position = vec4(in_position, 0.0, 1.0);
}

#fragment
#version 140

out vec4 out_colour;

uniform vec4 u_colour;

void main(void) {
	out_colour = u_colour;
}
#vertex
#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in mat4 in_transMat;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;

void main(void) {
	gl_Position = u_projMat * u_viewMat * in_transMat * vec4(in_position, 0.0, 1.0);
}

#fragment
#version 140

out vec4 out_colour;

void main(void) {
	out_colour = vec4(1.0);
}
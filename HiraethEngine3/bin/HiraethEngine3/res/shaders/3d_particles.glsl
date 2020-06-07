#vertex
#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in mat4 in_transMat;
layout(location = 5) in vec4 in_colour;
layout(location = 6) in vec4 in_uvs;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;

out vec4 pass_colour;
out vec2 pass_uv;

void main(void) {
	gl_Position = u_projMat * u_viewMat * in_transMat * vec4(in_position, 0.0, 1.0);
	pass_colour = in_colour;
	pass_uv = in_uvs.xy + in_uvs.zw * (in_position * 0.5 + 0.5);
	pass_uv.y = 1.0 - pass_uv.y;
}

#fragment
#version 140

out vec4 out_colour;

in vec4 pass_colour;
in vec2 pass_uv;

uniform sampler2D t_atlas;

void main(void) {
	out_colour = pass_colour * texture(t_atlas, pass_uv);
	//out_colour = vec4(pass_uv, 0.0, 1.0);
}
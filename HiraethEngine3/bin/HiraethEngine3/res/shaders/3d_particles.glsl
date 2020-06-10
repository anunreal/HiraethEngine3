#vertex
#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in mat4 in_transMat;
layout(location = 5) in vec4 in_colour;
layout(location = 6) in vec4 in_uvs;

out vec4 pass_colour;
out vec2 pass_uv;
out vec4 pass_shadowCoords;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_shadowSpace;

void main(void) {
	vec4 worldPos = in_transMat * vec4(in_position, 0.0, 1.0);
	gl_Position = u_projMat * u_viewMat * worldPos;
	pass_colour = in_colour;
	pass_uv = in_uvs.xy + in_uvs.zw * (in_position * 0.5 + 0.5);
	pass_uv.y = 1.0 - pass_uv.y;
	pass_shadowCoords = u_shadowSpace * worldPos; 
}

#fragment
#version 330
#include "res/shaders/3d_shader.glh"

out vec4 out_colour;

in vec4 pass_colour;

uniform sampler2D t_atlas;
uniform bool u_receiveShadows;

void main(void) {
	out_colour = pass_colour * texture(t_atlas, pass_uv);
	if(u_receiveShadows)
		out_colour.rgb *= calculateShadows();
	//out_colour = vec4(pass_uv, 0.0, 1.0);
}
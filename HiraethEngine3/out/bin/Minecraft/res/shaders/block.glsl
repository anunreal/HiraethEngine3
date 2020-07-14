#vertex
#version 140

in vec3 in_position;
in vec2 in_uv;

out vec2 pass_uv;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_transMat;

void main(void) {
	gl_Position = u_projMat * u_viewMat * u_transMat * vec4(in_position, 1.0);
	pass_uv = in_uv;
}

#fragment
#version 140

out vec4 out_colour;

in vec2 pass_uv;

uniform sampler2D u_texture;

void main(void) { 
	out_colour = vec4(pass_uv, 0.0, 1.0);
	out_colour = texture(u_texture, pass_uv);
} 
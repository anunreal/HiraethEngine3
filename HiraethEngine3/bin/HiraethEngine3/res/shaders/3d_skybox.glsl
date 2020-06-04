#vertex
#version 330 core

layout(location = 0) in vec3 in_position;

out vec3 pass_pos;
out vec4 pass_uv;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;

void main(void) {
	pass_pos = in_position;
	mat4 rotView = mat4(mat3(u_viewMat)); // remove translation
	vec4 clipPos = u_projMat * rotView * vec4(in_position, 1.0);
	gl_Position = clipPos.xyww; // ensure depth (always behind everything)
	//pass_uv = (gl_Position.xy / gl_Position.w) * 0.5 + 0.5;
	pass_uv = gl_Position;
}

#fragment
#version 330 core

out vec4 out_colour;

in vec3 pass_pos;
in vec4 pass_uv;

uniform samplerCube t_skybox;
uniform sampler2D t_alphaTest;
uniform bool u_realDepthTest;

void main(void) {
	vec2 uv = (pass_uv.xy / pass_uv.w) * 0.5 + 0.5;
	if(!u_realDepthTest && texture(t_alphaTest, uv).a > 0.0)
		discard;

	vec3 envColour = textureLod(t_skybox, pass_pos, 0).rgb;
	if(envColour.r < 0.0)
	out_colour = vec4(1, 0, 0, 1);
	else
	//envColour = pow(envColour, vec3(1.0 / 2.2)); // gamma
	out_colour = vec4(envColour, 1.0);
}

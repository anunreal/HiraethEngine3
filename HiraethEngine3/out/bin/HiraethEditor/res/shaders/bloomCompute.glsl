#version 450
#define TILE_SIZE 8

layout(location = 0) uniform writeonly image2D t_outLod0;
layout(location = 1) uniform sampler2D t_brightPass;
layout(location = 2) uniform sampler2D t_lowerTexture;
uniform vec2 u_direction;
uniform vec2 u_size;
uniform int u_lod;

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

vec4 blur13(vec2 pass_uv) {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.411764705882353) * u_direction;
	vec2 off2 = vec2(3.2941176470588234) * u_direction;
	vec2 off3 = vec2(5.176470588235294) * u_direction;
	color += textureLod(t_brightPass, pass_uv, u_lod) * 0.1964825501511404;
	color += textureLod(t_brightPass, pass_uv + (off1 / u_size), u_lod) * 0.2969069646728344;
	color += textureLod(t_brightPass, pass_uv - (off1 / u_size), u_lod) * 0.2969069646728344;
	color += textureLod(t_brightPass, pass_uv + (off2 / u_size), u_lod) * 0.09447039785044732;
	color += textureLod(t_brightPass, pass_uv - (off2 / u_size), u_lod) * 0.09447039785044732;
	color += textureLod(t_brightPass, pass_uv + (off3 / u_size), u_lod) * 0.010381362401148057;
	color += textureLod(t_brightPass, pass_uv - (off3 / u_size), u_lod) * 0.010381362401148057;
	return color;
}

void main(void) {
	const ivec2 tile_xy   = ivec2(gl_WorkGroupID.xy);
	const ivec2 thread_xy = ivec2(gl_LocalInvocationID.xy);
	const ivec2 samplePos = tile_xy * ivec2(TILE_SIZE) + thread_xy;
	vec2 uv = samplePos / (u_size - vec2(1));

	vec4 add = vec4(0.0);
	if(u_lod < 4)
		add = textureLod(t_lowerTexture, uv, u_lod + 1);

	imageStore(t_outLod0, samplePos, blur13(uv) + add);
}
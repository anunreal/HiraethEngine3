#vertex
#version 330 core

precision highp int;

in vec4  in_position;
in uint  in_colour;
in float in_width;

out VS_OUT {
	vec4  colour;
	float width;
} vs_out;


uniform mat4 u_projMat;
uniform mat4 u_viewMat;

vec4 unpackColour() {
	vec4 v;
	v.a = float((uint(0xFF000000) & in_colour) >> 24);
	v.b = float((uint(0x00FF0000) & in_colour) >> 16);
	v.g = float((uint(0x0000FF00) & in_colour) >> 8);
	v.r = float( uint(0x000000FF) & in_colour);
	return v / 255;
}

void main(void) {
	vec4 pos = vec4(in_position.xy, 0.0, 1.0);

	if(in_position.w == 1)
		pos = (u_projMat * u_viewMat * in_position);

	gl_Position = pos;
	vs_out.colour = unpackColour();
	vs_out.width  = in_width;
}

#geometry
#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT {
	vec4  colour;
	float width;
} gs_in[];

out vec4 fs_colour;

void main(void) {
	
	fs_colour = gs_in[0].colour;
	
	vec2 direction = normalize(gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w - gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w);
	vec2 normal    = vec2(direction.y, -direction.x);
		
	vec2 offset0 = vec2(normal * gl_in[0].gl_Position.w * gs_in[0].width / 2.);
	vec2 offset1 = vec2(normal * gl_in[1].gl_Position.w * gs_in[1].width / 2.);
		
	vec2 p0 = gl_in[0].gl_Position.xy + offset0;
	vec2 p1 = gl_in[0].gl_Position.xy - offset0;
	vec2 p2 = gl_in[1].gl_Position.xy + offset1;
	vec2 p3 = gl_in[1].gl_Position.xy - offset1;
		
		
	gl_Position = vec4(p0, gl_in[0].gl_Position.zw);
	EmitVertex();
	
	gl_Position = vec4(p1, gl_in[0].gl_Position.zw);
	EmitVertex();
	
	gl_Position = vec4(p2, gl_in[1].gl_Position.zw);
	EmitVertex();
	
	gl_Position = vec4(p3, gl_in[1].gl_Position.zw);
	EmitVertex();
	
	EndPrimitive();
} 

#fragment
#version 330 core

precision highp int;

out vec4 out_colour;

in vec4 fs_colour;

void main(void) {
	out_colour = fs_colour;
}

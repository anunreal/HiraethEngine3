#version 330 core

precision highp int;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
//layout(line_strip, max_vertices = 2) out;

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
	
	/*
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();
	EndPrimitive();
	*/
} 
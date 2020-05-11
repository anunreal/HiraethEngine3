#vertex
#version 140

in vec2 in_position;

out vec2 pass_uv;
flat out int pass_cubeMap;

uniform mat4 u_transMat;
uniform bool u_isCubeMap;

void main(void) {
	gl_Position = u_transMat * vec4(in_position, 0.0, 1.0);
	pass_uv = in_position * 0.5 + 0.5;
	//if(!u_isCubeMap)
		//pass_uv.y = 1.0 - pass_uv.y;
	pass_cubeMap = (u_isCubeMap) ? 1 : 0;
}	

#fragment
#version 430 core

in vec2 pass_uv;
flat in int pass_cubeMap;

out vec4 out_colour;

layout(location = 0) uniform sampler2D t_d2Tex;
layout(location = 1) uniform samplerCube t_cubeTex;
uniform bool u_isHdr; // apply tone mapping?wA

vec3 getPosition(vec3 texCoord) {
	vec3 pos = vec3(0.0);
	int i = int(texCoord.z);
	vec2 u_outputSize = vec2(1);
	
	switch(i) {
		case 0: // positive x
		pos = vec3(1.0, 1.0 - texCoord.y / u_outputSize.y * 2.0, 1.0 - texCoord.x / u_outputSize.y * 2.0);
		break;
		
		case 1: // negative x
		pos = vec3(-1.0, 1.0 - texCoord.y / u_outputSize.y * 2.0, texCoord.x / u_outputSize.y * 2.0 - 1.0);
		break;
		
		case 2: // positive y
		pos = vec3(texCoord.x / u_outputSize.x * 2.0 - 1.0, 1.0, texCoord.y / u_outputSize.y * 2.0 - 1.0);
		break;
		
		case 3: // negative y
		pos = vec3(texCoord.x / u_outputSize.x * 2.0 - 1.0, -1.0, 1.0 - texCoord.y / u_outputSize.y * 2.0);
		break;
		
		case 4: // positive z
		pos = vec3(texCoord.x / u_outputSize.x * 2.0 - 1.0, 1.0 - texCoord.y / u_outputSize.y * 2.0, 1.0);
		break;
		
		case 5: // negative z
		pos = vec3(1.0 - texCoord.x / u_outputSize.x * 2.0, 1.0 - texCoord.y / u_outputSize.y * 2.0, -1.0);
		break;
	}
	
	return pos;
}

void main(void) {
	if(pass_cubeMap == 1) {
		// unfold cube map
		vec2 localUv = vec2(1.0) - vec2(mod(pass_uv.x * 4.0, 1.0), mod(pass_uv.y * 3.0, 1.0));
		vec3 position = vec3(localUv, -1);
		if(pass_uv.y > (1.0 / 3.0 * 2.0) && pass_uv.x > 1.0 / 4 && pass_uv.x < (1.0 / 4) * 2.0) {
			// positive y
			position.z = 2;
		} else if(pass_uv.y < 1.0 / 3.0 && pass_uv.x > 1.0 / 4 && pass_uv.x < (1.0 / 4) * 2.0) {
			// negative y
			position.z = 3;
		} else if(pass_uv.y > 1.0 / 3.0 && pass_uv.y < (1.0 / 3.0) * 2.0) {
			if(pass_uv.x < 1.0 / 4.0)
				position.z = 0;
			else if(pass_uv.x < (1.0 / 4.0) * 2.0)
				position.z = 4;
			else if(pass_uv.x < (1.0 / 4.0) * 3.0)
				position.z = 1;
			else
				position.z = 5;
		}
		
		if(position.z > -1.0) {
			out_colour = texture(t_cubeTex, getPosition(position));
			//out_colour.rgb = out_colour.rgb / (out_colour.rgb + vec3(1));
		} else
			discard;
	} else
		out_colour = texture2D(t_d2Tex, pass_uv);	

	if(u_isHdr)
		out_colour.rgb = out_colour.rgb / (out_colour.rgb + vec3(1.0));
}
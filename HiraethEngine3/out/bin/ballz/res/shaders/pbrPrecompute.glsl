#version 450
#define TILE_SIZE 16
#define PI 3.14159265359

const vec2 invAtan = vec2(0.1591, 0.3183);

layout(binding = 0)          uniform writeonly  imageCube t_outEnv;
layout(binding = 1)          uniform writeonly  imageCube t_outIrr;
layout(binding = 2, rgba16f) uniform readonly   image2D   t_inHdr;
uniform sampler2D t_inHdrSamp;
uniform vec2 u_outputSize = vec2(512);
uniform vec2 u_inputSize;
uniform bool u_firstPass;
uniform float u_roughness;

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  

float hypot (vec2 z) {
	float t;
	float x = abs(z.x);
	float y = abs(z.y);
	t = min(x, y);
	x = max(x, y);
	t = t / x;
	return (z.x == 0.0 && z.y == 0.0) ? 0.0 : x * sqrt(1.0 + t * t);
}

vec2 sampleSphericalMap(vec3 v) {
	vec2 uv;
	float theta = atan(v.z, v.x);
	float r = hypot(v.xz);
	float phi = atan(v.y, r);
	uv = vec2(theta, phi);
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

vec3 getPosition(ivec3 texCoord) {
	vec3 pos = vec3(0.0);
	
	switch(texCoord.z) {
		case 0: // positive x
		pos = vec3(1.0, 1.0 - texCoord.y / u_outputSize.x * 2.0, 1.0 - texCoord.x / u_outputSize.y * 2.0);
		break;
		
		case 1: // negative x
		pos = vec3(-1.0, 1.0 - texCoord.y / u_outputSize.x * 2.0, texCoord.x / u_outputSize.y * 2.0 - 1.0);
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

vec3 calculateIrradiance(vec3 normal) {
	vec3 irradiance = vec3(0.0);  

	vec3 up    = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, normal);
	up         = cross(normal, right);

	float phiDelta = 0.03;
	float thetaDelta = 0.05;
	float nrSamples = 0.0; 
	for(float phi = 0.0; phi < 2.0 * PI; phi += phiDelta) {
		for(float theta = 0.0; theta < 0.5 * PI; theta += thetaDelta) {
			// spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 
			
			vec2 uv = sampleSphericalMap(normalize(sampleVec));
			ivec2 final = ivec2(uv * u_inputSize);
			irradiance += imageLoad(t_inHdr, final).rgb * cos(theta) * sin(theta);
			//irradiance += textureLod(t_inHdr, uv, 0).rgb * cos(theta) * sin(theta);
			//irradiance += texture(t_inHdr, uv).rgb * cos(theta) * sin(theta);
			//irradiance += vec3(1, 1, 0);
			nrSamples++;
		}
	}
	
	return PI * irradiance * (1.0 / float(nrSamples));
}

float DistributionGGX(float ndotv, float k) {
	float nom = ndotv;
	float denom = ndotv * (1.0 - k) + k;
	return nom / denom;
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
} 

vec3 calculateEnvironment(vec3 N, vec3 V) {
	const uint SAMPLE_COUNT = 4096u;
	float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);     

    for(uint i = 0u; i < SAMPLE_COUNT; ++i)	{
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, u_roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

		float NdotH = dot(N, H);
		float D   = DistributionGGX(NdotH, u_roughness);
		float pdf = (D * NdotH / (4.0 * dot(H, V))) + 0.0001; 

		float resolution = 512.0; // resolution of source cubemap (per face)
		float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
		float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

		float mipLevel = u_roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0) {
			vec2 uv = sampleSphericalMap(normalize(L));
			prefilteredColor += textureLod(t_inHdrSamp, uv, mipLevel).rgb * NdotL;
			totalWeight      += NdotL;
        }
	}

    prefilteredColor = prefilteredColor / totalWeight;
	return prefilteredColor;
}

void main(void) {
	const ivec3 tile_xy   = ivec3(gl_WorkGroupID.xyz);
	const ivec3 thread_xy = ivec3(gl_LocalInvocationID.xyz);
	const ivec3 samplePos = tile_xy * ivec3(TILE_SIZE, TILE_SIZE, 1) + thread_xy;
	vec3 worldPos = normalize(getPosition(samplePos));
	
	const vec2 uv = sampleSphericalMap(worldPos);
	imageStore(t_outEnv, samplePos, vec4(calculateEnvironment(worldPos, worldPos), 1.0));
	
	if(u_firstPass)
		imageStore(t_outIrr, samplePos, vec4(calculateIrradiance(worldPos), 1.0));
}
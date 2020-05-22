#include "heD3.h"
#include "heCore.h"
#include "heWin32Layer.h"

HeD3Level* heD3Level = nullptr;


HeD3LightSource* heD3LightSourceCreateDirectional(HeD3Level* level, hm::vec3f const& direction, hm::colour const& colour) {
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_DIRECTIONAL;
    light->vector          = direction;
    light->colour          = colour;
    light->update          = true;
    return light;
};


HeD3LightSource* heD3LightSourceCreateSpot(HeD3Level* level, hm::vec3f const& position, hm::vec3f const& direction, float const inAngle, float const outAngle, float const constLightValue, float const linearLightValue, float const quadraticLightValue, hm::colour const& colour) {
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_SPOT;
    light->vector          = position;
    light->colour          = colour;
    light->update          = true;
    light->data[0]         = direction.x;
    light->data[1]         = direction.y;
    light->data[2]         = direction.z;
    light->data[3]         = std::cos(hm::to_radians(inAngle));
    light->data[4]         = std::cos(hm::to_radians(outAngle));
    light->data[5]         = constLightValue;
    light->data[6]         = linearLightValue;
    light->data[7]         = quadraticLightValue;
    return light;
};

HeD3LightSource* heD3LightSourceCreatePoint(HeD3Level* level, hm::vec3f const& position, float const constLightValue, float const linearLightValue, float const quadraticLightValue, hm::colour const& colour) {
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_POINT;
    light->vector          = position;
    light->colour          = colour;
    light->update          = true;
    light->data[0]         = constLightValue;
    light->data[1]         = linearLightValue;
    light->data[2]         = quadraticLightValue;
    return light;
};


void heD3InstanceUpdate(HeD3Instance* instance) {
    // update physics
    if(instance->physics) {
        instance->transformation.position = hePhysicsComponentGetPosition(instance->physics);
        instance->transformation.rotation = hePhysicsComponentGetRotation(instance->physics);
    }
};

void heD3InstanceSetPosition(HeD3Instance* instance, hm::vec3f const& position) {
    instance->transformation.position = position;
    if(instance->physics)
        hePhysicsComponentSetPosition(instance->physics, position);
};


void heD3LevelRemoveInstance(HeD3Level* level, HeD3Instance* instance) {
    uint32_t index = 0;
    for(HeD3Instance& all : level->instances) {
        if(&all == instance)
            break;
        index++;
    }
    
    auto it = level->instances.begin();
    std::advance(it, index);
    level->instances.erase(it);
};

void heD3LevelUpdate(HeD3Level* level) {
    // update all instances
    for(auto& all : level->instances)
        heD3InstanceUpdate(&all);
};

HeD3Instance* heD3LevelGetInstance(HeD3Level* level, uint16_t const index) {
    auto begin = level->instances.begin();
    std::advance(begin, index);
    return &(*begin);
};

HeD3LightSource* heD3LevelGetLightSource(HeD3Level* level, uint16_t const index) {
    auto begin = level->lights.begin();
    std::advance(begin, index);
    return &(*begin);
};

void heD3SkyboxCreate(HeD3Skybox* skybox, std::string const& hdrFile) {
    heWin32TimerStart();

	const hm::vec2i OUT_RES(512);
	const uint8_t MIP_COUNT = 5;

	std::string name = hdrFile.substr(hdrFile.find_last_of('/') + 1, hdrFile.find('.'));
    HeTexture hdr;
    HeShaderProgram shader;
    heShaderCreateCompute(&shader, "res/shaders/pbrPrecompute.glsl");
    
    skybox->specular = &heAssetPool.texturePool[name + "_spec"];
    skybox->specular->format      = HE_COLOUR_FORMAT_RGBA16;
    skybox->specular->size        = OUT_RES;
#ifdef HE_ENABLE_NAMES
	skybox->specular->name        = "outEnvironment";
#endif
	skybox->specular->parameters  = HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_CLAMP_EDGE;
	skybox->specular->mipMapCount = MIP_COUNT;
    heTextureCreateEmptyCubeMap(skybox->specular);
    
    skybox->irradiance = &heAssetPool.texturePool[name + "_irr"];
    skybox->irradiance->format     = HE_COLOUR_FORMAT_RGBA16;
    skybox->irradiance->size       = OUT_RES;
#ifdef HE_ENABLE_NAMES
    skybox->irradiance->name       = "outIrradiance";
#endif
	skybox->irradiance->parameters = HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE;
	heTextureCreateEmptyCubeMap(skybox->irradiance);

	hdr.parameters = HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_CLAMP_EDGE;
    hdr.mipMapCount = MIP_COUNT;
	heTextureLoadHdrFromFile(&hdr, hdrFile);

    heShaderBind(&shader);
	heShaderLoadUniform(&shader, "u_inputSize", hm::vec2f(hdr.size));
    heShaderLoadUniform(&shader, "u_firstPass", true);
	heImageTextureBind(skybox->irradiance, heShaderGetSamplerLocation(&shader, "t_outIrr", 1), 0, -1, HE_ACCESS_WRITE_ONLY);
	heImageTextureBind(&hdr, heShaderGetSamplerLocation(&shader, "t_inHdr", 2), 0, -1, HE_ACCESS_READ_ONLY);
    heTextureBind(&hdr, heShaderGetSamplerLocation(&shader, "t_inHdrSamp", 3));
	
	hm::vec2i mipSize = OUT_RES;
	for(uint8_t i = 0; i < MIP_COUNT; ++i) { 
        float roughness = i / ((float) MIP_COUNT - 1);
		heShaderLoadUniform(&shader, "u_roughness", roughness);
		heShaderLoadUniform(&shader, "u_outputSize", hm::vec2f(mipSize));
        heImageTextureBind(skybox->specular, heShaderGetSamplerLocation(&shader, "t_outEnv", 0), i, -1, HE_ACCESS_WRITE_ONLY);
    	heShaderRunCompute(&shader, mipSize.x / 16, mipSize.y / 16, 6);
		if(i == 0)
			heShaderLoadUniform(&shader, "u_firstPass", false);
        mipSize = mipSize / 2;
	}
	
    //heTextureFilterTrilinear(skybox->specular);
	heShaderUnbind();
    heShaderDestroy(&shader);
	heTextureDestroy(&hdr);
	
    heWin32TimerPrint("SKYBOX CREATION");
};

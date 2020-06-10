#include "hepch.h"
#include "heD3.h"
#include "heCore.h"
#include "heWin32Layer.h"

HeD3Level* heD3Level = nullptr;


// -- instance

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


// -- frustum

void heD3FrustumUpdate(HeD3Frustum* frustum, HeWindow* window, hm::vec3f const& position, hm::vec3f const& rotation) {
    { // calculate corners
        hm::mat4f rotationMat(1.0f);
        rotationMat = hm::rotate(rotationMat, (float) -rotation.y, hm::vec3f(0, 1, 0));
        rotationMat = hm::rotate(rotationMat, (float) -rotation.x, hm::vec3f(1, 0, 0));

        float fov        = (float) std::tanf(frustum->viewInfo.fov * (float) hm::PI / 360.f);
        float farHeight  = frustum->viewInfo.farPlane * fov;
        float farWidth   = farHeight * window->windowInfo.aspectRatio;
        float nearHeight = frustum->viewInfo.nearPlane * fov;
        float nearWidth  = nearHeight * window->windowInfo.aspectRatio;

        hm::vec3f forward    (rotationMat * hm::vec4f(0, 0, -1, 0)); 
        hm::vec3f toFar      (forward * frustum->viewInfo.farPlane);
        hm::vec3f toNear     (forward * frustum->viewInfo.nearPlane);
        hm::vec3f centerFar  (position + toFar);
        hm::vec3f centerNear (position + toNear);

        hm::vec3f up    = hm::vec3f(rotationMat * hm::vec4f(0, 1, 0, 0));
        hm::vec3f right = hm::cross(forward, up);
        
        hm::vec3f farTop     = centerFar  + up * farHeight;
        hm::vec3f farBottom  = centerFar  - up * farHeight;
        hm::vec3f nearTop    = centerNear + up * nearHeight;
        hm::vec3f nearBottom = centerNear - up * nearHeight;
        
        frustum->corners[0] = farTop + right * farWidth;
        frustum->corners[1] = farTop - right * farWidth;
        frustum->corners[2] = farBottom + right * farWidth;
        frustum->corners[3] = farBottom - right * farWidth;
        frustum->corners[4] = nearTop + right * nearWidth;
        frustum->corners[5] = nearTop - right * nearWidth;
        frustum->corners[6] = nearBottom + right * nearWidth;
        frustum->corners[7] = nearBottom - right * nearWidth;        
    }

    { // calculate bounds
        // set bounds to some value
        frustum->bounds[0] = frustum->corners[0];
        frustum->bounds[1] = frustum->corners[0];

        // check for lowest and highest values
        for(uint8_t i = 1; i < 8; ++i) {
            hm::vec3f& p = frustum->corners[i];
            if(p.x < frustum->bounds[0].x)
                frustum->bounds[0].x = p.x;
            else if(p.x > frustum->bounds[1].x)
                frustum->bounds[1].x = p.x;

            if(p.y < frustum->bounds[0].y)
                frustum->bounds[0].y = p.y;
            else if(p.y > frustum->bounds[1].y)
                frustum->bounds[1].y = p.y;
        
            if(p.z < frustum->bounds[0].z)
                frustum->bounds[0].z = p.z;
            else if(p.z > frustum->bounds[1].z)
                frustum->bounds[1].z = p.z;
        }
    }
};

void heD3FrustumUpdateWithMatrix(HeD3Frustum* frustum, HeWindow* window, hm::vec3f const& position, hm::vec3f const& rotation, hm::mat4f const& matrix) {
    { // calculate corners
        hm::mat4f rotationMat(1.0f);
        rotationMat = hm::rotate(rotationMat, (float) -rotation.y, hm::vec3f(0, 1, 0));
        rotationMat = hm::rotate(rotationMat, (float) -rotation.x, hm::vec3f(1, 0, 0));
   
        float fov        = (float) std::tanf(frustum->viewInfo.fov * (float) hm::PI / 360.f);
        float farHeight  = frustum->viewInfo.farPlane * fov;
        float farWidth   = farHeight * window->windowInfo.aspectRatio;
        float nearHeight = frustum->viewInfo.nearPlane * fov;
        float nearWidth  = nearHeight * window->windowInfo.aspectRatio;

        hm::vec3f forward    (rotationMat * hm::vec4f(0, 0, -1, 0)); 
        hm::vec3f toFar      (forward * frustum->viewInfo.farPlane);
        hm::vec3f toNear     (forward * frustum->viewInfo.nearPlane);
        hm::vec3f centerFar  (position + toFar);
        hm::vec3f centerNear (position + toNear);

        hm::vec3f up    = hm::vec3f(rotationMat * hm::vec4f(0, 1, 0, 0));
        hm::vec3f right = hm::cross(forward, up);
        
        hm::vec3f farTop     = centerFar  + up * farHeight;
        hm::vec3f farBottom  = centerFar  - up * farHeight;
        hm::vec3f nearTop    = centerNear + up * nearHeight;
        hm::vec3f nearBottom = centerNear - up * nearHeight;
        
        frustum->corners[0] = hm::vec3f(matrix * hm::vec4f(farTop + right * farWidth));
        frustum->corners[1] = hm::vec3f(matrix * hm::vec4f(farTop - right * farWidth));
        frustum->corners[2] = hm::vec3f(matrix * hm::vec4f(farBottom + right * farWidth));
        frustum->corners[3] = hm::vec3f(matrix * hm::vec4f(farBottom - right * farWidth));
        frustum->corners[4] = hm::vec3f(matrix * hm::vec4f(nearTop + right * nearWidth));
        frustum->corners[5] = hm::vec3f(matrix * hm::vec4f(nearTop - right * nearWidth));
        frustum->corners[6] = hm::vec3f(matrix * hm::vec4f(nearBottom + right * nearWidth));
        frustum->corners[7] = hm::vec3f(matrix * hm::vec4f(nearBottom - right * nearWidth));        
    }

    { // calculate bounds
        // set bounds to some value
        frustum->bounds[0] = frustum->corners[0];
        frustum->bounds[1] = frustum->corners[0];

        // check for lowest and highest values
        for(uint8_t i = 1; i < 8; ++i) {
            hm::vec3f& p = frustum->corners[i];
            if(p.x < frustum->bounds[0].x)
                frustum->bounds[0].x = p.x;
            else if(p.x > frustum->bounds[1].x)
                frustum->bounds[1].x = p.x;

            if(p.y < frustum->bounds[0].y)
                frustum->bounds[0].y = p.y;
            else if(p.y > frustum->bounds[1].y)
                frustum->bounds[1].y = p.y;
        
            if(p.z < frustum->bounds[0].z)
                frustum->bounds[0].z = p.z;
            else if(p.z > frustum->bounds[1].z)
                frustum->bounds[1].z = p.z;
        }
    }
};


// -- camera

void heD3CameraClampRotation(HeD3Camera* camera) {
    if(camera->rotation.x < -90)
        camera->rotation.x = -90;
    else if(camera->rotation.x > 90)
        camera->rotation.x = 90;

    if(camera->rotation.y < 0)
        camera->rotation.y = 360 + camera->rotation.y;
    else if(camera->rotation.y > 360)
        camera->rotation.y = camera->rotation.y - 360;
};


// -- light

HeD3LightSource* heD3LightSourceCreateDirectional(HeD3Level* level, hm::vec3f const& direction, hm::colour const& colour) {
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_DIRECTIONAL;
    light->vector          = direction;
    light->colour          = colour;
    light->update          = true;

    heD3ShadowMapCreate(&light->shadows, light);
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

void heD3ShadowMapCreate(HeD3ShadowMap* shadowMap, HeD3LightSource* source) {
    source->castShadows      = true;
    shadowMap->viewMatrix    = hm::mat4f(1.0f);
    shadowMap->depthFbo.size = shadowMap->resolution;
    heFboCreate(&shadowMap->depthFbo);
    heFboCreateDepthTextureAttachment(&shadowMap->depthFbo);
    heFboDisableColourAttachment(&shadowMap->depthFbo);
    heFboValidate(&shadowMap->depthFbo);
    heFboUnbind();
};


// -- level

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

void heD3LevelUpdate(HeD3Level* level, float const delta) {
    if(level->physics.setup) {
        hePhysicsLevelUpdate(&level->physics, delta);

        if(!level->freeCamera)
            level->camera.position = hePhysicsActorGetEyePosition(level->physics.actor);
    }
        
    // update all instances
    for(auto& all : level->instances)
        heD3InstanceUpdate(&all);

    // update all particle sources
    for(auto& all : level->particles)
        heParticleSourceUpdate(&all, delta, level);
};

void heD3LevelDestroy(HeD3Level* level) {
    hePhysicsLevelDestroy(&level->physics);
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


// -- skybox

void heD3SkyboxCreate(HeD3Skybox* skybox, std::string const& hdrFile) {
    const hm::vec2i OUT_RES(512);
    const uint8_t MIP_COUNT = 5;

    size_t index = hdrFile.find_last_of('/') + 1;
    std::string name = hdrFile.substr(index, hdrFile.find('.') - index);
    HeTexture hdr;
    HeShaderProgram shader;
    heShaderCreateCompute(&shader, "res/shaders/pbrPrecompute.glsl");

    hdr.parameters = HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_CLAMP_EDGE;
    hdr.mipmapCount = MIP_COUNT;
    heTextureLoadFromHdrImageFile(&hdr, hdrFile);
    
    skybox->specular = &heAssetPool.texturePool[name + "_specular"];
    skybox->specular->format      = HE_COLOUR_FORMAT_RGBA16;
    skybox->specular->channels    = 4;
    skybox->specular->size        = OUT_RES;
#ifdef HE_ENABLE_NAMES
    skybox->specular->name        = name + "_specular";
#endif
    skybox->specular->parameters  = HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_CLAMP_EDGE;
    skybox->specular->mipmapCount = MIP_COUNT;
    heTextureCreateEmptyCubeMap(skybox->specular);
    
    skybox->irradiance = &heAssetPool.texturePool[name + "_irradiance"];
    skybox->irradiance->format     = HE_COLOUR_FORMAT_RGBA16;
    skybox->irradiance->channels   = 4;
    skybox->irradiance->size       = OUT_RES;
#ifdef HE_ENABLE_NAMES
    skybox->irradiance->name       = name + "_irradiance";
#endif
    skybox->irradiance->parameters = HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE;
    heTextureCreateEmptyCubeMap(skybox->irradiance);

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
    
    heShaderUnbind();
    heShaderDestroy(&shader);
    heTextureDestroy(&hdr);
};

void heD3SkyboxLoad(HeD3Skybox* skybox, std::string const& fileName) {    
    skybox->specular = &heAssetPool.texturePool["binres/textures/skybox/" + fileName + "_specular.h3asset"];
    skybox->specular->parameters = HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_CLAMP_EDGE;
    heTextureLoadFromHdrCubemapFile(skybox->specular, "binres/textures/skybox/" + fileName + "_specular.h3asset");

    skybox->irradiance = &heAssetPool.texturePool["binres/textures/skybox/" + fileName + "_irradiance.h3asset"];
    skybox->irradiance->parameters = HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE;
    heTextureLoadFromHdrCubemapFile(skybox->irradiance, "binres/textures/skybox/" + fileName + "_irradiance.h3asset");
};


// -- particles

void heParticleSourceCreate(HeParticleSource* source, HeD3Transformation const& transformation, HeSpriteAtlas* atlas, uint32_t const atlasIndex, uint32_t const particleCount) {
    source->atlas         = atlas;
    source->particleCount = particleCount;
    source->atlasIndex    = atlasIndex;
    source->particles     = (HeParticle*) malloc(particleCount * sizeof(HeParticle));
    source->dataBuffer    = (float*) malloc((size_t) particleCount * source->FLOATS_PER_PARTICLE * sizeof(float));
    source->emitter.transformation = transformation;
    heRandomCreate(&source->emitter.random, 0);
    memset(source->particles,  0, sizeof(source->particles));
    memset(source->dataBuffer, 0, sizeof(source->dataBuffer));
};

void heParticleSourceDestroy(HeParticleSource* source) {
    heTextureDestroy(source->atlas->texture);
    free(source->particles);
    free(source->dataBuffer);
};

void heParticleSourceSpawnParticle(HeParticleEmitter* source, HeParticle* particle) {
    particle->transformation.scale    = hm::vec3f(heRandomFloat(&source->random, source->minSize, source->maxSize));

    hm::vec3f position = source->transformation.position; // point emitter
    // find random position in the emitter
    if(source->type == HE_PARTICLE_EMITTER_TYPE_BOX) {
        position.x += heRandomFloat(&source->random, -source->box.x, source->box.x);
        position.y += heRandomFloat(&source->random, -source->box.y, source->box.y);
        position.z += heRandomFloat(&source->random, -source->box.z, source->box.z);
    } else if(source->type == HE_PARTICLE_EMITTER_TYPE_SPHERE) {
        // find random direction into the sphere, and then scale to be maximum the radius
        hm::vec3f velocity;
        velocity.x = heRandomFloat(&source->random, -1, 1);
        velocity.y = heRandomFloat(&source->random, -1, 1);
        velocity.z = heRandomFloat(&source->random, -1, 1);
        position += velocity * heRandomFloat(&source->random, 0, source->sphere); 
    }
    
    particle->transformation.position = position;
    particle->transformation.rotation = hm::quatf();
    particle->velocity = hm::vec3f(heRandomFloat(&source->random, -1, 1), heRandomFloat(&source->random, -1, 1), heRandomFloat(&source->random, -1, 1)) * heRandomFloat(&source->random, source->minSpeed, source->maxSpeed);
    particle->colour                  = hm::interpolateColour(source->minColour, source->maxColour, heRandomFloat(&source->random, 0.f, 1.f));
    particle->remaining               = heRandomFloat(&source->random, source->minTime, source->maxTime);
};

void heParticleSourceUpdate(HeParticleSource* source, float const delta, HeD3Level* level) {
    uint32_t created = 0;

    for(uint32_t i = 0; i < source->particleCount; ++i) {
        HeParticle* particle = &source->particles[i];
        uint32_t offset = i * source->FLOATS_PER_PARTICLE;
        particle->remaining -= delta;
        if(particle->remaining > 0.f) {
            particle->velocity.y -= source->gravity * delta;
            particle->transformation.position += particle->velocity * delta;

        } else {
            heParticleSourceSpawnParticle(&source->emitter, particle);
            created++;
        }

        // build trans mat and add to vbo
        hm::mat4f transMat = hm::translate(hm::mat4f(1.0f), particle->transformation.position);

        // transpose view matrix into trans matrix so that we cancel the rotation
        transMat[0][0] = level->camera.viewMatrix[0][0];
        transMat[1][0] = level->camera.viewMatrix[0][1];
        transMat[2][0] = level->camera.viewMatrix[0][2];
        transMat[0][1] = level->camera.viewMatrix[1][0];
        transMat[1][1] = level->camera.viewMatrix[1][1];
        transMat[2][1] = level->camera.viewMatrix[1][2];
        transMat[0][2] = level->camera.viewMatrix[2][0];
        transMat[1][2] = level->camera.viewMatrix[2][1];
        transMat[2][2] = level->camera.viewMatrix[2][2];
        transMat = hm::scale(transMat, particle->transformation.scale);

        float colour[4];
        colour[0] = hm::getR(&particle->colour);
        colour[1] = hm::getG(&particle->colour);
        colour[2] = hm::getB(&particle->colour);
        colour[3] = hm::getA(&particle->colour);

        hm::vec4f uvs = heSpriteAtlasGetUvs(source->atlas, source->atlasIndex);
            
        memcpy(&source->dataBuffer[offset], &transMat[0][0], 16 * sizeof(float));
        memcpy(&source->dataBuffer[offset + 16], &colour[0], 4 * sizeof(float));
        memcpy(&source->dataBuffer[offset + 20], &uvs.x, 4 * sizeof(float));
    }
};

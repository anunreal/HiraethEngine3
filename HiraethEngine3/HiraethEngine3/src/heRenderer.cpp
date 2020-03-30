#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"

void heRenderEngineCreate(HeRenderEngine* engine, HeWindow* window) {
    
    engine->window = window;
    engine->textureShader = heAssetPoolGetShader("gui_texture");
    engine->d3FinalShader = heAssetPoolGetShader("d3_final", "res/shaders/quad_v.glsl", "res/shaders/d3_final_f.glsl");
    
    engine->quadVao = &heAssetPool.meshPool["quad_vao"];
    heVaoCreate(engine->quadVao);
    heVaoBind(engine->quadVao);
    heVaoAddData(engine->quadVao, { -1, 1,  1, 1,  -1, -1,  -1, -1,  1, 1,  1, -1 }, 2);
    heVaoUnbind(engine->quadVao);
    
    engine->hdrFbo.samples = 4;
    engine->hdrFbo.size    = window->windowInfo.size;
    engine->hdrFbo.flags   = HE_FBO_FLAG_HDR | HE_FBO_FLAG_DEPTH_RENDER_BUFFER; 
    heFboCreate(&engine->hdrFbo);
    
    engine->resolvedFbo.samples = 1;
    engine->resolvedFbo.size = window->windowInfo.size;
    engine->resolvedFbo.flags = HE_FBO_FLAG_HDR | HE_FBO_FLAG_DEPTH_RENDER_BUFFER;
    heFboCreate(&engine->resolvedFbo);
    heEnableDepth(1);
    
};

void heRenderEngineResize(HeRenderEngine* engine) {
    
    if(engine->hdrFbo.size != engine->window->windowInfo.size) {
        heFboResize(&engine->hdrFbo, engine->window->windowInfo.size);
        heFboResize(&engine->resolvedFbo, engine->window->windowInfo.size);
    }
    
};

void heRenderEngineDestroy(HeRenderEngine* engine) {
    
    heShaderDestroy(engine->textureShader);
    heVaoDestroy(engine->quadVao);
    
};

void heShaderLoadMaterial(HeShaderProgram* shader, const HeMaterial* material) {
    
    unsigned int count = 0;
    for (const auto& textures : material->textures)
        heTextureBind(textures.second, heShaderGetSamplerLocation(material->shader, "t_" + textures.first, count++));
    
    for (const auto& uniforms : material->uniforms)
        heShaderLoadUniform(shader, "u_" + uniforms.first, &uniforms.second);
    
};

void heShaderLoadLight(HeShaderProgram* program, const HeD3LightSource* light, const int8_t index) {
    
    std::string uniformName;
    if (index == -1)
        uniformName = "u_light";
    else
        uniformName = "u_lights[" + std::to_string(index) + "]";
    
    heShaderLoadUniform(program, uniformName + ".vector", light->vector);
    heShaderLoadUniform(program, uniformName + ".colour", light->colour);
    heShaderLoadUniform(program, uniformName + ".type",   light->type);
    heShaderLoadUniform(program, uniformName + ".data1",  hm::vec4f(light->data[0], light->data[1], light->data[2], light->data[3]));
    heShaderLoadUniform(program, uniformName + ".data2", hm::vec4f(light->data[4], light->data[5], light->data[6], light->data[7]));
    
};

void heD3InstanceRender(HeD3Instance* instance) {
    
    if (instance->mesh != nullptr) {
        // load instance data
        heShaderLoadUniform(instance->material->shader, "u_transMat", hm::createTransformationMatrix(instance->transformation.position,
                                                                                                     instance->transformation.rotation, instance->transformation.scale));
        
        // load material
        heShaderLoadMaterial(instance->material->shader, instance->material);
        
        heVaoBind(instance->mesh);
        heVaoRender(instance->mesh);
    }
    
};

void heD2TextureRender(HeRenderEngine* engine, const HeTexture* texture, const hm::vec2f& position, const hm::vec2f& size) {
    
    hm::vec2f realPosition = position;
    if (realPosition.x >= 0)
        realPosition.x /= engine->window->windowInfo.size.x;
    else
        realPosition.x = -realPosition.x;
    
    if (realPosition.y >= 0)
        realPosition.y /= engine->window->windowInfo.size.y;
    else
        realPosition.y = -realPosition.y;
    
    realPosition   = realPosition * 2.0f - 1.0f;
    realPosition.y = -realPosition.y;
    
    heShaderBind(engine->textureShader);
    heShaderLoadUniform(engine->textureShader, "u_transMat", hm::createTransformationMatrix(realPosition, size));
    heTextureBind(texture, 0);
    heVaoBind(engine->quadVao);
    heVaoRender(engine->quadVao);
    
};

void heD3LevelRender(HeD3Level* level) {
    
    std::vector<uint32_t> updatedLights;
    std::map<HeShaderProgram*, std::vector<HeD3Instance*>> shaderMap;
    bool printLights = heDebugIsInfoRequested(HE_DEBUG_INFO_LIGHTS);
    if(printLights)
        heDebugPrint("=== LIGHT SOURCES ===");
    
    uint32_t lightIndex = 0;
    for(auto lights = level->lights.begin(); lights != level->lights.end(); ++lights) {
        if(lights->update) {
            //heLoadLightToShader(all.first, &(*lights), lightIndex);
            //updatedLights[lights] = lightIndex;
            updatedLights.emplace_back(lightIndex);
            lights->update = false;
        }
        
        if(printLights) {
            std::string string = "HeD3LightSource {\n";
            string += " type    = " + std::to_string(lights->type) + "\n";
            string += " vector  = " + hm::to_string(lights->vector) + "\n";
            string += " colour  = " + hm::to_string(lights->colour) + "\n";
            for(uint8_t i = 0; i < 8; ++i)
                string += " data[" + std::to_string(i) + "] = " + std::to_string(lights->data[i]) + "\n";
            string += "}\n";
            heDebugPrint(string);
        }
        
        lightIndex++;
    }
    
    if(printLights)
        heDebugPrint("=== LIGHT SOURCES ===");
    
    for(auto it = level->instances.begin(); it != level->instances.end(); ++it) {
        if(it->material != nullptr) 
            shaderMap[it->material->shader].emplace_back(&(*it));
    }
    
    
    for(const auto& all : shaderMap) {
        
        heShaderBind(all.first);
        heShaderLoadUniform(all.first, "u_time", level->time);
        
        // load camera
        heShaderBind(all.first);
        heShaderLoadUniform(all.first, "u_viewMat",   level->camera.viewMatrix);
        heShaderLoadUniform(all.first, "u_projMat",   level->camera.projectionMatrix);
        heShaderLoadUniform(all.first, "u_cameraPos", level->camera.position);
        
        // load lights
        for(uint32_t lights : updatedLights) {
            auto it = level->lights.begin();
            std::advance(it, lights);
            heShaderLoadLight(all.first, &(*it), lights);
        }
        
        for(HeD3Instance* instances : all.second)
            heD3InstanceRender(instances);
        
    }
    
};

void heRenderEnginePrepareD3(HeRenderEngine* engine) {
    
    heRenderEngineResize(engine);
    heFboBind(&engine->hdrFbo);
    heFrameClear(engine->window->windowInfo.backgroundColour, 2);
    
};

void heRenderEngineFinishD3(HeRenderEngine* engine) {
    
    heFboUnbind(engine->window->windowInfo.size);
    heFboRender(&engine->hdrFbo, &engine->resolvedFbo);
    
    heVaoBind(engine->quadVao);
    heShaderBind(engine->d3FinalShader);
    heTextureBind(engine->resolvedFbo.colourTextures[0], 0);
    heVaoRender(engine->quadVao);
    
};

void heRenderEngineSetProperty(HeRenderEngine* engine, const std::string& name, const int32_t value) {
    
    
    
};
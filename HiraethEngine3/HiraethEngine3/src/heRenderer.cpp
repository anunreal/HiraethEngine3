#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"

void heCreateRenderEngine(HeRenderEngine* engine, HeWindow* window) {
    
    engine->window = window;
    engine->textureShader = heGetShader("gui_texture");
    engine->d3FinalShader = heGetShader("d3_final", "res/shaders/quad_v.glsl", "res/shaders/d3_final_f.glsl");
    
    engine->quadVao = &heAssetPool.meshPool["quad_vao"];
    heCreateVao(engine->quadVao);
    heBindVao(engine->quadVao);
    heAddVaoData(engine->quadVao, { -1, 1,  1, 1,  -1, -1,  -1, -1,  1, 1,  1, -1 }, 2);
    heUnbindVao(engine->quadVao);
    
    engine->hdrFbo.samples = 4;
    engine->hdrFbo.size    = window->windowInfo.size;
    engine->hdrFbo.flags   = HE_FBO_FLAG_HDR | HE_FBO_FLAG_DEPTH_RENDER_BUFFER; 
    heCreateFbo(&engine->hdrFbo);
    
    engine->resolvedFbo.samples = 1;
    engine->resolvedFbo.size = window->windowInfo.size;
    engine->resolvedFbo.flags = HE_FBO_FLAG_HDR | HE_FBO_FLAG_DEPTH_RENDER_BUFFER;
    heCreateFbo(&engine->resolvedFbo);
    heEnableDepth(1);
    
};

void heResizeRenderEngine(HeRenderEngine* engine) {
    
    if(engine->hdrFbo.size != engine->window->windowInfo.size) {
        heResizeFbo(&engine->hdrFbo, engine->window->windowInfo.size);
        heResizeFbo(&engine->resolvedFbo, engine->window->windowInfo.size);
    }
    
};

void heDestroyRenderEngine(HeRenderEngine* engine) {
    
    heDestroyShader(engine->textureShader);
    heDestroyVao(engine->quadVao);
    
};

void heLoadMaterialToShader(HeShaderProgram* shader, const HeMaterial* material) {
    
    unsigned int count = 0;
    for (const auto& textures : material->textures)
        heBindTexture(textures.second, heGetShaderSamplerLocation(material->shader, "t_" + textures.first, count++));
    
    for (const auto& uniforms : material->uniforms)
        heLoadShaderUniform(shader, "u_" + uniforms.first, &uniforms.second);
    
};

void heLoadLightToShader(HeShaderProgram* program, const HeD3LightSource* light, const int8_t index) {
    
    std::string uniformName;
    if (index == -1)
        uniformName = "u_light";
    else
        uniformName = "u_lights[" + std::to_string(index) + "]";
    
    heLoadShaderUniform(program, uniformName + ".vector", light->vector);
    heLoadShaderUniform(program, uniformName + ".colour", light->colour);
    heLoadShaderUniform(program, uniformName + ".type",   light->type);
    heLoadShaderUniform(program, uniformName + ".data1",  hm::vec4f(light->data[0], light->data[1], light->data[2], light->data[3]));
    heLoadShaderUniform(program, uniformName + ".data2", hm::vec4f(light->data[4], light->data[5], light->data[6], light->data[7]));
    
};

void heRenderD3Instance(HeD3Instance* instance) {
    
    if (instance->mesh != nullptr) {
        // load instance data
        heLoadShaderUniform(instance->material->shader, "u_transMat", hm::createTransformationMatrix(instance->transformation.position,
                                                                                                     instance->transformation.rotation, instance->transformation.scale));
        
        // load material
        heLoadMaterialToShader(instance->material->shader, instance->material);
        
        heBindVao(instance->mesh);
        heRenderVao(instance->mesh);
    }
    
};

void heRenderD2Texture(HeRenderEngine* engine, const HeTexture* texture, const hm::vec2f& position, const hm::vec2f& size) {
    
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
    
    heBindShader(engine->textureShader);
    heLoadShaderUniform(engine->textureShader, "u_transMat", hm::createTransformationMatrix(realPosition, size));
    heBindTexture(texture, 0);
    heBindVao(engine->quadVao);
    heRenderVao(engine->quadVao);
    
};

void heRenderD3Level(HeD3Level* level) {
    
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
            string += " type    = " + std::to_string(lights->tpye) + "\n";
            string += " vector  = " + hm::to_string(light->vector) + "\n";
            string += " colour  = " + hm::to_string(light->colour) + "\n";
            for(uint8_t i = 0; i < 8; ++i)
                string += " data[" + std::to_string(i) + "] = " + std::to_string(lights->[i]) + "\n";
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
        
        heBindShader(all.first);
        heLoadShaderUniform(all.first, "u_time", level->time);
        
        // load camera
        heBindShader(all.first);
        heLoadShaderUniform(all.first, "u_viewMat",   level->camera.viewMatrix);
        heLoadShaderUniform(all.first, "u_projMat",   level->camera.projectionMatrix);
        heLoadShaderUniform(all.first, "u_cameraPos", level->camera.position);
        
        // load lights
        for(uint32_t lights : updatedLights) {
            auto it = level->lights.begin();
            std::advance(it, lights);
            heLoadLightToShader(all.first, &(*it), lights);
        }
        
        for(HeD3Instance* instances : all.second)
            heRenderD3Instance(instances);
        
    }
    
};

void hePrepareD3RenderEngine(HeRenderEngine* engine) {
    
    heResizeRenderEngine(engine);
    heBindFbo(&engine->hdrFbo);
    heClearFrame(engine->window->windowInfo.backgroundColour, 2);
    
};

void heEndD3RenderEngine(HeRenderEngine* engine) {
    
    heUnbindFbo(engine->window->windowInfo.size);
    heDrawFbo(&engine->hdrFbo, &engine->resolvedFbo);
    
    heBindVao(engine->quadVao);
    heBindShader(engine->d3FinalShader);
    heBindTexture(engine->resolvedFbo.colourTextures[0], 0);
    heRenderVao(engine->quadVao);
    
};

void heSetRenderProperty(HeRenderEngine* engine, const std::string& name, const int32_t value) {
    
    
    
};
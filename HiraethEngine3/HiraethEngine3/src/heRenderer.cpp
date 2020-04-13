#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"
#include "heDebugUtils.h"
#include "heD3.h"

HeRenderEngine* heRenderEngine = nullptr;


void heRenderEngineCreate(HeRenderEngine* engine, HeWindow* window) {
    
    engine->quadVao = &heAssetPool.meshPool["quad_vao"];
    
#ifdef HE_ENABLE_NAMES
    engine->resolvedFbo.name = "resolvedFbo";
    engine->gBufferFbo.name  = "gbufferFbo";
    engine->quadVao->name    = "quadVao";
#endif
    
    // common
    engine->window = window;
    engine->textureShader = heAssetPoolGetShader("gui_texture");
    engine->finalShader = heAssetPoolGetShader("d3_final", "res/shaders/quad_v.glsl", "res/shaders/final_f.glsl");
    heShaderBind(engine->finalShader);
    heShaderLoadUniform(engine->finalShader, "u_gamma", 1.0f);
    
    // quad vao
    heVaoCreate(engine->quadVao);
    heVaoBind(engine->quadVao);
    heVaoAddData(engine->quadVao, { -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1 }, 2);
    heVaoUnbind(engine->quadVao);
    
    heUiQueueCreate(&engine->uiQueue);
    
    // hdr
    engine->resolvedFbo.samples = 1;
    engine->resolvedFbo.size = window->windowInfo.size;
    engine->resolvedFbo.flags = HE_FBO_FLAG_HDR | HE_FBO_FLAG_DEPTH_RENDER_BUFFER;
    heFboCreate(&engine->resolvedFbo);
    
    // d3
    engine->gBufferShader = heAssetPoolGetShader("d3_gbuffer");
    engine->gLightingShader = heAssetPoolGetShader("d3_lighting");
    
    engine->gBufferFbo.samples = 1;
    engine->gBufferFbo.flags = HE_FBO_FLAG_HDR | HE_FBO_FLAG_DEPTH_RENDER_BUFFER;
    engine->gBufferFbo.size = window->windowInfo.size;
    heFboCreate(&engine->gBufferFbo); // creates vertex position texture
    heFboCreateColourAttachment(&engine->gBufferFbo, HE_COLOUR_FORMAT_RGBA16); // creates normal texture
    heFboCreateColourAttachment(&engine->gBufferFbo, HE_COLOUR_FORMAT_RGBA8); // creates diffuse texture
    heFboCreateColourAttachment(&engine->gBufferFbo, HE_COLOUR_FORMAT_RGBA8); // creates arm texture
    
};

void heRenderEngineResize(HeRenderEngine* engine) {
    
    if(engine->gBufferFbo.size != engine->window->windowInfo.size) {
        heFboResize(&engine->gBufferFbo, engine->window->windowInfo.size);
        heFboResize(&engine->resolvedFbo, engine->window->windowInfo.size);
    }
    
};

void heRenderEngineDestroy(HeRenderEngine* engine) {
    
    heShaderDestroy(engine->textureShader);
    heShaderDestroy(engine->finalShader);
    heShaderDestroy(engine->gBufferShader);
    heVaoDestroy(engine->quadVao);
    heFboDestroy(&engine->gBufferFbo);
    heFboDestroy(&engine->resolvedFbo);
    
};

void heRenderEnginePrepare(HeRenderEngine* engine) {
    
    heFrameClear(engine->window->windowInfo.backgroundColour, HE_FRAME_BUFFER_BIT_COLOUR | HE_FRAME_BUFFER_BIT_DEPTH);
    
};

void heRenderEngineFinish(HeRenderEngine* engine) {
    
    heWindowSwapBuffers(engine->window);
    
};

void heShaderLoadMaterial(HeShaderProgram* shader, HeMaterial const* material) {
    
    heShaderClearSamplers(shader);
    uint8_t count = 0;
    for (const auto& textures : material->textures)
        heTextureBind(textures.second, heShaderGetSamplerLocation(shader, "t_" + textures.first, count++));
    
    for (const auto& uniforms : material->uniforms)
        heShaderLoadUniform(shader, "u_" + uniforms.first, &uniforms.second);
    
    heShaderLoadUniform(shader, "u_materialType", material->type);
    
};

void heShaderLoadLight(HeShaderProgram* program, HeD3LightSource const* light, int8_t const index) {
    
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
    
};

void heD3LevelRender(HeRenderEngine* engine, HeD3Level* level) {
    
    // render all objects into the gbuffer
    heShaderBind(engine->gBufferShader);
    heShaderLoadUniform(engine->gBufferShader, "u_viewMat", level->camera.viewMatrix);
    heShaderLoadUniform(engine->gBufferShader, "u_projMat", level->camera.projectionMatrix);
    
    for(auto const& all : level->instances) {
        // render instance into the gbuffer
        hm::mat4f transMat = hm::createTransformationMatrix(all.transformation.position, all.transformation.rotation, all.transformation.scale);
        heShaderLoadUniform(engine->gBufferShader, "u_transMat", transMat);
        heShaderLoadUniform(engine->gBufferShader, "u_normMat",  hm::transpose(hm::inverse(hm::mat3f(transMat))));
        heShaderLoadMaterial(engine->gBufferShader, all.material);
        
        heVaoBind(all.mesh);
        heVaoRender(all.mesh);
    }
    
    
    heShaderBind(engine->gLightingShader);
    heFboUnbind(engine->window->windowInfo.size);
    heCullEnable(false);
    
    // TODO(Victor): avoid for loop here, maybe store changed lights?
    uint8_t lightCount = 0;
    for(auto& all : level->lights) {
        if(all.update) {
            all.update = false;
            heShaderLoadLight(engine->gLightingShader, &all, lightCount);
        }
        
        lightCount++;
    }
    
    if(engine->outputTexture == 0) {
        heTextureBind(engine->gBufferFbo.colourTextures[0], heShaderGetSamplerLocation(engine->gLightingShader, "t_worldSpace", 0));
        heTextureBind(engine->gBufferFbo.colourTextures[1], heShaderGetSamplerLocation(engine->gLightingShader, "t_normals", 1));
        heTextureBind(engine->gBufferFbo.colourTextures[2], heShaderGetSamplerLocation(engine->gLightingShader, "t_diffuse", 2));
        heTextureBind(engine->gBufferFbo.colourTextures[3], heShaderGetSamplerLocation(engine->gLightingShader, "t_arm", 3));
        heShaderLoadUniform(engine->gLightingShader, "u_cameraPos", level->camera.position);
        heVaoBind(engine->quadVao);
        heVaoRender(engine->quadVao);
    } else
        heUiRenderTexture(engine, engine->gBufferFbo.colourTextures[engine->outputTexture - 1], hm::vec2f(-.5f), hm::vec2f(-1.f));
    
};

void heRenderEnginePrepareD3(HeRenderEngine* engine) {
    
    heRenderEngineResize(engine);
    heFboBind(&engine->gBufferFbo);
    heFrameClear(hm::colour(0, 0, 0, 255), HE_FRAME_BUFFER_BIT_COLOUR | HE_FRAME_BUFFER_BIT_DEPTH);
    heDepthEnable(true);
    heBlendMode(-1);
    heCullEnable(true);
    
};

void heRenderEngineFinishD3(HeRenderEngine* engine) {
    
    //heFboUnbind(engine->window->windowInfo.size);
    
    /*
    heVaoBind(engine->quadVao);
    heShaderBind(engine->finalShader);
    heTextureBind(engine->gBufferFbo.colourTextures[2], 0);
    heVaoRender(engine->quadVao);
    */
    
};

void heRenderEnginePrepareUi(HeRenderEngine* engine) {
    
    //heFboBind(&engine->hdrFbo); // we only need the multisampled part of the fbo
    //heFrameClear(hm::colour(0, 0, 0, 255), HE_FRAME_BUFFER_BIT_COLOUR);
    heBlendMode(0);
    
};

void heRenderEngineFinishUi(HeRenderEngine* engine) {
    
    //heFboUnbind(engine->window->windowInfo.size);
    //heFboRender(&engine->hdrFbo, &engine->resolvedFbo);
    
    /*
    heBlendMode(0);
    heVaoBind(engine->quadVao);
    heShaderBind(engine->finalShader);
    heTextureBind(engine->resolvedFbo.colourTextures[0], 0);
    heVaoRender(engine->quadVao);
    */
    
};

void heRenderEngineSetProperty(HeRenderEngine* engine, std::string const& name, int32_t const value) {
    
    
    
};


void heUiQueueCreate(HeUiQueue* queue) {
    
    heVaoCreate(&queue->linesVao, HE_VAO_TYPE_LINES);
    heVaoBind(&queue->linesVao);
    HeVbo vbo;
    heVboAllocate(&vbo, 0, 4, HE_VBO_USAGE_DYNAMIC); // coordinates (2 or 3), w component indicates 2d (0) or 3d (1)
    heVaoAddVbo(&queue->linesVao, &vbo);
    heVboAllocate(&vbo, 0, 1, HE_VBO_USAGE_DYNAMIC, HE_UNIFORM_DATA_TYPE_UINT); // colour
    heVaoAddVbo(&queue->linesVao, &vbo);
    heVboAllocate(&vbo, 0, 1, HE_VBO_USAGE_DYNAMIC); // width
    heVaoAddVbo(&queue->linesVao, &vbo);
    heVaoUnbind(&queue->linesVao);
    queue->linesShader = heAssetPoolGetShader("gui_lines");
    
};

void heUiRenderTexture(HeRenderEngine* engine, HeTexture const* texture, hm::vec2f const& position, hm::vec2f const& size) {
    
    heUiRenderTexture(engine, texture->textureId, position, size);
    
};

void heUiRenderTexture(HeRenderEngine* engine, uint32_t const texture, hm::vec2f const& position, hm::vec2f const& size) {
    
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

void heUiQueueRenderLines(HeRenderEngine* engine) {
    
    size_t size = engine->uiQueue.lines.size();
    if(size == 0)
        return;
    
    heDepthEnable(false);
    
    
    // build lines mesh
    std::vector<float> data;
    data.reserve(size * 8);
    std::vector<uint32_t> colour;
    colour.reserve(size * 2);
    std::vector<float> width;
    width.reserve(size * 2);
    
    for(auto const& lines : engine->uiQueue.lines) {
        if(lines.d3) {
            data.emplace_back(lines.p0.x);
            data.emplace_back(lines.p0.y);
            data.emplace_back(lines.p0.z);
            data.emplace_back(1.f);
            
            data.emplace_back(lines.p1.x);
            data.emplace_back(lines.p1.y);
            data.emplace_back(lines.p1.z);
            data.emplace_back(1.f);
        } else {
            data.emplace_back(lines.p0.x / engine->window->windowInfo.size.x * 2.f - 1.f);
            data.emplace_back(1.f - lines.p0.y / engine->window->windowInfo.size.y * 2.f);
            data.emplace_back(0.f);
            data.emplace_back(0.f);
            
            data.emplace_back(lines.p1.x / engine->window->windowInfo.size.x * 2.f - 1.f);
            data.emplace_back(1.f - lines.p1.y / engine->window->windowInfo.size.y * 2.f);
            data.emplace_back(0.f);
            data.emplace_back(0.f);
        }
        
        uint32_t c = hm::encodeColour(lines.colour);
        colour.emplace_back(c);
        colour.emplace_back(c);
        
        float widthScreenSpace = lines.width / engine->window->windowInfo.size.x;
        //float widthScreenSpace = 0.01f;
        width.emplace_back(widthScreenSpace);
        width.emplace_back(widthScreenSpace);
    }
    
    HeD3Camera* cam = &heD3Level->camera;
    heShaderBind(engine->uiQueue.linesShader);
    heShaderLoadUniform(engine->uiQueue.linesShader, "u_projMat", cam->projectionMatrix);
    heShaderLoadUniform(engine->uiQueue.linesShader, "u_viewMat", cam->viewMatrix);
    heVaoBind(&engine->uiQueue.linesVao);
    heVaoUpdateData(&engine->uiQueue.linesVao, data, 0);
    heVaoUpdateDataUint(&engine->uiQueue.linesVao, colour, 1);
    heVaoUpdateData(&engine->uiQueue.linesVao, width, 2);
    heVaoRender(&engine->uiQueue.linesVao);
    
};

void heUiQueueRender(HeRenderEngine* engine) {
    
    heUiQueueRenderLines(engine);
    // clear queue
    engine->uiQueue.lines.clear();
    
};


void heUiPushLineD2(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::colour const& colour, float const width) {
    
    engine->uiQueue.lines.emplace_back(p0, p1, colour, width);
    
};

void heUiPushLineD3(HeRenderEngine* engine, hm::vec3f const& p0, hm::vec3f const& p1, hm::colour const& colour, float const width) {
    
    engine->uiQueue.lines.emplace_back(p0, p1, colour, width);
    
};


hm::vec2f heSpaceScreenToClip(hm::vec2f const& pixelspace, HeWindow const* window) {
    
    return hm::vec2f((pixelspace.x / window->windowInfo.size.x) * 2.f - 1.f,
                     1.f - (pixelspace.y / window->windowInfo.size.y) * 2.f);
    
};

hm::vec2f heSpaceWorldToScreen(hm::vec3f const& worldSpace, HeD3Camera const * camera, HeWindow const* window) {
    
    hm::vec4f clip = camera->viewMatrix * hm::vec4f(worldSpace, 1.0f);
    clip = camera->projectionMatrix * clip;
    if(clip.w != 0.0f) {
        clip.x /= clip.w;
        clip.y /= clip.w;
        clip.z /= clip.w;
    }
    
    return hm::vec2f(((clip.x + 1.f) / 2.f) * window->windowInfo.size.x,
                     ((1.f - clip.y) / 2.f) * window->windowInfo.size.y);
    
};
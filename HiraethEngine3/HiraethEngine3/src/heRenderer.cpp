#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"
#include "heDebugUtils.h"
#include "heD3.h"

HeRenderEngine* heRenderEngine = nullptr;


void heRenderEngineCreate(HeRenderEngine* engine, HeWindow* window) {
	engine->shapes.quadVao   = &heAssetPool.meshPool["quad_vao"];
	engine->shapes.cubeVao   = &heAssetPool.meshPool["cube_vao"];
    engine->shapes.sphereVao = &heAssetPool.meshPool["sphere_vao"];
	
#ifdef HE_ENABLE_NAMES
	engine->resolvedFbo.name       = "resolvedFbo";
	engine->gBufferFbo.name        = "gbufferFbo";
	engine->shapes.quadVao->name   = "quadVao";
    engine->shapes.cubeVao->name   = "cubeVao";
	engine->shapes.sphereVao->name = "sphereVao";
#endif
	
// common
	engine->window = window;
	engine->textureShader = heAssetPoolGetShader("gui_texture");
	engine->rgbaShader = heAssetPoolGetShader("gui_rgba");
	engine->finalShader = heAssetPoolGetShader("d3_final", "res/shaders/quad_v.glsl", "res/shaders/final_f.glsl");
	heShaderBind(engine->finalShader);
	heShaderLoadUniform(engine->finalShader, "u_gamma", 1.0f);
    
// quad vao
	heVaoCreate(engine->shapes.quadVao);
	heVaoBind(engine->shapes.quadVao);
	heVaoAddData(engine->shapes.quadVao, { -1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1 }, 2);
	heVaoUnbind(engine->shapes.quadVao);
   
// cube vao
	heVaoCreate(engine->shapes.cubeVao);
	heVaoBind(engine->shapes.cubeVao);
	heVaoAddData(engine->shapes.cubeVao, {
			-1.0f,-1.0f,-1.0f, // triangle 1 : begin
			-1.0f,-1.0f, 1.0f,
			-1.0f,1.0f, 1.0f, // triangle 1 : end
			1.0f,1.0f,-1.0f, // triangle 2 : begin
			-1.0f,-1.0f,-1.0f,
			-1.0f, 1.0f,-1.0f, // triangle 2 : end
			1.0f,-1.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,
			1.0f,-1.0f,-1.0f,
			1.0f, 1.0f,-1.0f,
			1.0f,-1.0f,-1.0f,
			-1.0f,-1.0f,-1.0f,
			-1.0f,-1.0f,-1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,
			1.0f,-1.0f, 1.0f,
			-1.0f,-1.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f,-1.0f, 1.0f,
			1.0f,-1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f,-1.0f,-1.0f,
			1.0f, 1.0f,-1.0f,
			1.0f,-1.0f,-1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f,-1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f,-1.0f,
			-1.0f, 1.0f,-1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,
			-1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			1.0f,-1.0f, 1.0f
		}, 3);
	
	heVaoUnbind(engine->shapes.cubeVao);
	
// ui
	heUiQueueCreate(&engine->uiQueue);
	
// d3

	engine->gBufferShader   = heAssetPoolGetShader("d3_gbuffer");
	engine->gLightingShader = heAssetPoolGetShader("d3_lighting");
	engine->skyboxShader    = heAssetPoolGetShader("d3_skybox");
	engine->brdfIntegration = heAssetPoolGetTexture("res/textures/utils/brdf.png", HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE);
	
	engine->gBufferFbo.size = window->windowInfo.size;
	heFboCreate(&engine->gBufferFbo);
	heFboCreateDepthBufferAttachment(&engine->gBufferFbo);
	heFboCreateColourTextureAttachment(&engine->gBufferFbo, HE_COLOUR_FORMAT_RGBA16); // creates worldpos texture
	heFboCreateColourTextureAttachment(&engine->gBufferFbo, HE_COLOUR_FORMAT_RGBA16); // creates normal texture
	heFboCreateColourTextureAttachment(&engine->gBufferFbo, HE_COLOUR_FORMAT_RGBA8); // creates diffuse texture
	heFboCreateColourTextureAttachment(&engine->gBufferFbo, HE_COLOUR_FORMAT_RGBA8); // creates arm texture
};

void heRenderEngineResize(HeRenderEngine* engine) {
    // resize only if the size has changed and its not 0 (minimized window)
    if(engine->gBufferFbo.size != engine->window->windowInfo.size && engine->window->windowInfo.size != hm::vec2i(0)) {
        heFboResize(&engine->gBufferFbo, engine->window->windowInfo.size);
        //heFboResize(&engine->resolvedFbo, engine->window->windowInfo.size);
    }
};

void heRenderEngineDestroy(HeRenderEngine* engine) {
    heShaderDestroy(engine->textureShader);
    heShaderDestroy(engine->finalShader);
    heShaderDestroy(engine->gBufferShader);
    heVaoDestroy(engine->shapes.quadVao);
    heFboDestroy(&engine->gBufferFbo);
    heFboDestroy(&engine->resolvedFbo);
};

void heRenderEnginePrepare(HeRenderEngine* engine) {
    heRenderEngineResize(engine);
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
    // render all cts into the gbuffer
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
        heTextureBind(engine->gBufferFbo.colourAttachments[0].id, heShaderGetSamplerLocation(engine->gLightingShader, "t_worldSpace", 0));
        heTextureBind(engine->gBufferFbo.colourAttachments[1].id, heShaderGetSamplerLocation(engine->gLightingShader, "t_normals", 1));
        heTextureBind(engine->gBufferFbo.colourAttachments[2].id, heShaderGetSamplerLocation(engine->gLightingShader, "t_diffuse", 2));
        heTextureBind(engine->gBufferFbo.colourAttachments[3].id, heShaderGetSamplerLocation(engine->gLightingShader, "t_arm", 3));
        heTextureBind(level->skybox.irradiance, heShaderGetSamplerLocation(engine->gLightingShader, "t_irradiance", 4));
        heTextureBind(level->skybox.specular, heShaderGetSamplerLocation(engine->gLightingShader, "t_environment", 5));
        heTextureBind(engine->brdfIntegration, heShaderGetSamplerLocation(engine->gLightingShader, "t_brdf", 6));
        heShaderLoadUniform(engine->gLightingShader, "u_cameraPos", level->camera.position);
    	heUiSetQuadVao(engine->shapes.quadVao);
        heVaoRender(engine->shapes.quadVao);
    } else
        heUiRenderTexture(engine, engine->gBufferFbo.colourAttachments[engine->outputTexture - 1].id, hm::vec2f(-.5f), hm::vec2f(-1.f));
    
    if(level->skybox.specular) {
        // render sky box
        heDepthEnable(false);
        heVaoBind(engine->shapes.cubeVao);
        heShaderBind(engine->skyboxShader);
        heShaderLoadUniform(engine->skyboxShader, "u_projMat", level->camera.projectionMatrix);
        heShaderLoadUniform(engine->skyboxShader, "u_viewMat", level->camera.viewMatrix);
        heTextureBind(level->skybox.specular, heShaderGetSamplerLocation(engine->skyboxShader, "t_skybox", 0));
        heTextureBind(engine->gBufferFbo.colourAttachments[0].id, heShaderGetSamplerLocation(engine->skyboxShader, "t_alphaTest", 1));
        heVaoRender(engine->shapes.cubeVao);
    }
};

void heRenderEnginePrepareD3(HeRenderEngine* engine) {
    heFboBind(&engine->gBufferFbo);
    heFrameClear(hm::colour(0, 0, 0, 0), HE_FRAME_BUFFER_BIT_COLOUR | HE_FRAME_BUFFER_BIT_DEPTH);
    heDepthEnable(true);
    heBlendMode(-1);
    heCullEnable(true);
};

void heRenderEngineFinishD3(HeRenderEngine* engine) {
    //heFboUnbind(engine->window->windowInfo.size);
    
    /*
    heVaoBind(engine->shapes.quadVao);
    heShaderBind(engine->finalShader);
    heTextureBind(engine->gBufferFbo.colourTextures[2], 0);
    heVaoRender(engine->shapes.quadVao);
    */
};

void heRenderEnginePrepareUi(HeRenderEngine* engine) {
    //heFboBind(&engine->hdrFbo); // we only need the multisampled part of the fbo
    //heFrameClear(hm::colour(0, 0, 0, 255), HE_FRAME_BUFFER_BIT_COLOUR);
	heFboUnbind(engine->window->windowInfo.size);
    heBlendMode(0);
};

void heRenderEngineFinishUi(HeRenderEngine* engine) {
    //heFboUnbind(engine->window->windowInfo.size);
    //heFboRender(&engine->hdrFbo, &engine->resolvedFbo);
    
    /*
    heBlendMode(0);
    heVaoBind(engine->shapes.quadVao);
    heShaderBind(engine->finalShader);
    heTextureBind(engine->resolvedFbo.colourTextures[0], 0);
    heVaoRender(engine->shapes.quadVao);
    */
};

void heRenderEngineSetProperty(HeRenderEngine* engine, std::string const& name, int32_t const value) {
    
};


void heUiQueueCreate(HeUiQueue* queue) {
    // lines
#ifdef HE_ENABLE_NAMES
	queue->linesVao.name = "linesVao";
	queue->textVao.name  = "textVao";
#endif
	
    heVaoCreate(&queue->linesVao, HE_VAO_TYPE_LINES);
    heVaoBind(&queue->linesVao);
    HeVbo vbo;
    heVboAllocate(&vbo, 0, 4, HE_VBO_USAGE_DYNAMIC); // coordinates (2 or 3), w component indicates 2d (0) or 3d (1)
    heVaoAddVbo(&queue->linesVao, &vbo);
    heVboAllocate(&vbo, 0, 1, HE_VBO_USAGE_DYNAMIC, HE_DATA_TYPE_UINT); // colour
    heVaoAddVbo(&queue->linesVao, &vbo);
    heVboAllocate(&vbo, 0, 1, HE_VBO_USAGE_DYNAMIC); // width
    heVaoAddVbo(&queue->linesVao, &vbo);
    heVaoUnbind(&queue->linesVao);

    queue->linesShader = heAssetPoolGetShader("gui_lines");

	// text
	heVaoCreate(&queue->textVao, HE_VAO_TYPE_TRIANGLES);
	heVaoBind(&queue->textVao);
	heVboAllocate(&vbo, 0, 4, HE_VBO_USAGE_DYNAMIC); // vertices and uvs
	heVaoAddVbo(&queue->textVao, &vbo);
	heVboAllocate(&vbo, 0, 1, HE_VBO_USAGE_DYNAMIC, HE_DATA_TYPE_UINT); // colour
	heVaoAddVbo(&queue->textVao, &vbo);
	heVaoUnbind(&queue->textVao);

	queue->textShader = heAssetPoolGetShader("gui_text");
};

void heUiSetQuadVao(HeVao* vao, hm::vec2f const& p0, hm::vec2f const& p1, hm::vec2f const& p2, hm::vec2f const& p3) {
    heVaoBind(vao);
	heVaoUpdateData(vao, {p0.x, p0.y,
						  p1.x, p1.y,
						  p2.x, p2.y,
						  p2.x, p2.y,
						  p3.x, p3.y,
						  p1.x, p1.y
		}, 0);
};


void heUiRenderTexture(HeRenderEngine* engine, HeTexture const* texture, hm::vec2f const& position, hm::vec2f const& size) {
    heUiRenderTexture(engine, texture->textureId, position, size, texture->cubeMap);
};

void heUiRenderTexture(HeRenderEngine* engine, uint32_t const texture, hm::vec2f const& position, hm::vec2f const& size, bool const isCubeMap) {
    hm::vec2f realPosition = position; // in gl coordinate system
    hm::vec2f realSize = size; // in window space system
    if (realPosition.x >= 0)
        realPosition.x /= engine->window->windowInfo.size.x;
    else
        realPosition.x = -realPosition.x;
    
    if (realPosition.y >= 0)
        realPosition.y /= engine->window->windowInfo.size.y;
    else
        realPosition.y = -realPosition.y;
    
    if(realSize.x >= 0)
        realSize.x /= engine->window->windowInfo.size.x;
    else
        realSize.x = -realSize.x;
    
    if(realSize.y >= 0)
        realSize.y /= engine->window->windowInfo.size.y;
    else
        realSize.y = -realSize.y;
    
    realPosition   = realPosition * 2.0f - 1.0f;
    realPosition.y = -realPosition.y;
    
    heShaderBind(engine->textureShader);
    heShaderLoadUniform(engine->textureShader, "u_transMat", hm::createTransformationMatrix(realPosition, realSize));
    heShaderLoadUniform(engine->textureShader, "u_isCubeMap", isCubeMap);
    std::string textureName = (isCubeMap) ? "t_cubeTex" : "t_d2Tex";
    heTextureBind(texture, heShaderGetSamplerLocation(engine->textureShader, textureName, isCubeMap), isCubeMap);
	heUiSetQuadVao(engine->shapes.quadVao);
	heVaoRender(engine->shapes.quadVao);
};

void heUiRenderQuad(HeRenderEngine* engine, hm::vec2i const& p0, hm::vec2i const& p1, hm::vec2i const& p2, hm::vec2i const& p3, hm::colour const& colour) {
	hm::vec2f _p0 = heSpaceScreenToClip(p0, engine->window);
	hm::vec2f _p1 = heSpaceScreenToClip(p1, engine->window);
	hm::vec2f _p2 = heSpaceScreenToClip(p2, engine->window);
	hm::vec2f _p3 = heSpaceScreenToClip(p3, engine->window);

	heShaderBind(engine->rgbaShader);
	heUiSetQuadVao(engine->shapes.quadVao, _p0, _p1, _p2, _p3);
	heShaderLoadUniform(engine->rgbaShader, "u_colour", colour);
	heVaoRender(engine->shapes.quadVao);
	heVaoUnbind(engine->shapes.quadVao);
	heShaderUnbind();
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

void heUiQueueRenderTexts(HeRenderEngine* engine) {
	std::vector<float> vertices;
	std::vector<uint32_t> colours;
	hm::vec2f windowSize(engine->window->windowInfo.size);
	
	heShaderBind(engine->uiQueue.textShader);
	heVaoBind(&engine->uiQueue.textVao);

	for (auto const& all : engine->uiQueue.texts) {
        // prepare font
	    heTextureBind(all.first->atlas, heShaderGetSamplerLocation(engine->uiQueue.textShader, "t_atlas", 0));	
		
	    for (HeUiText const& texts : all.second) {
			float scale = texts.size / (float) all.first->size;
			hm::vec2f cursor = hm::vec2f(all.first->padding.x, all.first->padding.y) * scale;
			cursor += texts.position;
			for (char const chars : texts.text) {
                if (chars == 32) {
                    // space
                    cursor.x += all.first->spaceWidth * scale;
					continue;
				}
				
				auto it = all.first->characters.find((uint32_t) (chars));
				if(it == all.first->characters.end()) {
					HE_DEBUG("Could not find character [" + std::to_string(chars) + "] in font [" + all.first->name + "]");
					continue;
				}

				HeFont::Character const* c = &it->second;
				
				// build quads
				hm::vec2f p0, p1, p2, p3, uv0, uv1, uv2, uv3;
				p0  = hm::vec2f(c->offset) * scale + cursor;
				p1  = hm::vec2f(p0.x + c->size.x * scale, p0.y);
				p2  = hm::vec2f(p0.x                    , p0.y + c->size.y * scale);
				p3  = hm::vec2f(p0.x + c->size.x * scale, p0.y + c->size.y * scale);

				uv0 = hm::vec2f(c->uv.x, c->uv.y);
				uv1 = hm::vec2f(c->uv.z, c->uv.y);
				uv2 = hm::vec2f(c->uv.x, c->uv.w);
				uv3 = hm::vec2f(c->uv.z, c->uv.w);

                p0 = heSpaceScreenToClip(p0, engine->window);
                p1 = heSpaceScreenToClip(p1, engine->window);
                p2 = heSpaceScreenToClip(p2, engine->window);
                p3 = heSpaceScreenToClip(p3, engine->window);

				// push these vertices as triangles
                vertices.emplace_back(p0.x);
                vertices.emplace_back(p0.y);
				vertices.emplace_back(uv0.x);
				vertices.emplace_back(uv0.y);

                vertices.emplace_back(p1.x);
                vertices.emplace_back(p1.y);
                vertices.emplace_back(uv1.x);
				vertices.emplace_back(uv1.y);

				vertices.emplace_back(p2.x);
                vertices.emplace_back(p2.y);
                vertices.emplace_back(uv2.x);
				vertices.emplace_back(uv2.y);
				
                vertices.emplace_back(p2.x);
                vertices.emplace_back(p2.y);
                vertices.emplace_back(uv2.x);
				vertices.emplace_back(uv2.y);

				vertices.emplace_back(p3.x);
                vertices.emplace_back(p3.y);
                vertices.emplace_back(uv3.x);
				vertices.emplace_back(uv3.y);

				vertices.emplace_back(p1.x);
                vertices.emplace_back(p1.y);
                vertices.emplace_back(uv1.x);
				vertices.emplace_back(uv1.y);

				uint32_t col = hm::encodeColour(texts.colour);
				colours.emplace_back(col);
				colours.emplace_back(col);
				colours.emplace_back(col);
				colours.emplace_back(col);
				colours.emplace_back(col);
				colours.emplace_back(col);
				
                cursor.x += c->xadvance * scale;
			}

			float textSize = 1.f;
			float f = 1.f - textSize;
			if (f < 0.f) f = 0.f;
			float boldness = 0.50f + (0.12f * f);
			float softness = 0.01f + (0.22f * f);

			heShaderLoadUniform(engine->uiQueue.textShader, "u_textSize", scale);
			heShaderLoadUniform(engine->uiQueue.textShader, "u_alphaValues", hm::vec2f(boldness, softness));
			heVaoUpdateData(&engine->uiQueue.textVao, vertices, 0);
			heVaoUpdateDataUint(&engine->uiQueue.textVao, colours, 1);
			heVaoRender(&engine->uiQueue.textVao);
		}		
	}
};

void heUiQueueRender(HeRenderEngine* engine) {
	heBlendMode(0);
    heUiQueueRenderLines(engine);
	heUiQueueRenderTexts(engine);
    // clear queue
    engine->uiQueue.lines.clear();
    engine->uiQueue.texts.clear();
};


void heUiPushLineD2(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::colour const& colour, float const width) {
    engine->uiQueue.lines.emplace_back(p0, p1, colour, width);
};

void heUiPushLineD3(HeRenderEngine* engine, hm::vec3f const& p0, hm::vec3f const& p1, hm::colour const& colour, float const width) {
    engine->uiQueue.lines.emplace_back(p0, p1, colour, width);
};

void heUiPushText(HeRenderEngine* engine, HeFont const* font, std::string const& text, hm::vec2i const& position, uint16_t const size, hm::colour const& colour) {
    engine->uiQueue.texts[font].emplace_back(HeUiText(text, position, size, colour));
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

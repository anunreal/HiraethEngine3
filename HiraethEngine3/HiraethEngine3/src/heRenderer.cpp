#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"
#include "heDebugUtils.h"
#include "heD3.h"
#include "heWin32Layer.h" // @temp

struct HeD3RenderInfo {
	float exposure = 1.f;
	float gamma = 1.f;
};

HeUiTextManager heTextManager;
HeD3RenderInfo heD3RenderInfo; // info for the current frame, set when rendering i.e. a d3 level, used in the final pass
HeRenderEngine* heRenderEngine = nullptr;

void hePostProcessEngineCreate(HePostProcessEngine* engine, HeWindow* window) {
#ifdef HE_ENABLE_NAMES
	engine->bloomFbo.name = "bloomFbo";
#endif

	hm::vec2i bloomSize(hm::floorPowerOfTwo(window->windowInfo.size.x), hm::floorPowerOfTwo(window->windowInfo.size.y));
	engine->bloomFbo.size = bloomSize;
	heFboCreate(&engine->bloomFbo);
	heFboCreateColourTextureAttachment(&engine->bloomFbo, HE_COLOUR_FORMAT_RGB16); // the bright pass, blur input
	heFboCreateColourTextureAttachment(&engine->bloomFbo, HE_COLOUR_FORMAT_RGBA16, bloomSize, 4); // blur output -> actual bloom
    heFboUnbind(window->windowInfo.size);

	engine->brightPassShader = heAssetPoolGetShader("bright_pass", "res/shaders/quad_v.glsl", "res/shaders/brightPassFilter.glsl");
	engine->combineShader = heAssetPoolGetShader("combine_shader", "res/shaders/quad_v.glsl", "res/shaders/3d_final.glsl");
	
	engine->gaussianBlurShader = &heAssetPool.shaderPool["bloomShader"];
	heShaderCreateCompute(engine->gaussianBlurShader, "res/shaders/bloomCompute.glsl");
	
	engine->initialized = true;
};

void hePostProcessEngineDestroy(HePostProcessEngine* engine) {
	heFboDestroy(&engine->bloomFbo);
	heShaderDestroy(engine->brightPassShader);
	heShaderDestroy(engine->gaussianBlurShader);
	heShaderDestroy(engine->combineShader);
};

void heRenderEngineCreate(HeRenderEngine* engine, HeWindow* window) {
	engine->shapes.quadVao	 = &heAssetPool.meshPool["quad_vao"];
	engine->shapes.cubeVao	 = &heAssetPool.meshPool["cube_vao"];
	engine->shapes.sphereVao = &heAssetPool.meshPool["sphere_vao"];
	
#ifdef HE_ENABLE_NAMES
	engine->hdrFbo.name	             = "hdrFbo";
	engine->shapes.quadVao->name     = "quadVao";
	engine->shapes.cubeVao->name     = "cubeVao";
	engine->shapes.sphereVao->name   = "sphereVao";
	engine->deferred.gBufferFbo.name = "gbufferFbo";
	engine->forward.forwardFbo.name  = "forwardFbo";
#endif

	// common
	engine->window = window;
	engine->textureShader = heAssetPoolGetShader("gui_texture");
	engine->rgbaShader    = heAssetPoolGetShader("gui_rgba");
	
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
			-1.0f, 1.0f, 1.0f, // triangle 1 : end
			 1.0f, 1.0f,-1.0f, // triangle 2 : begin
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
	
	engine->skyboxShader	= heAssetPoolGetShader("3d_skybox");
	engine->brdfIntegration = heAssetPoolGetTexture("res/textures/utils/brdf.png", HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_EDGE);
	engine->hdrFbo.size = window->windowInfo.size;
	heFboCreate(&engine->hdrFbo);
	heFboCreateColourTextureAttachment(&engine->hdrFbo, HE_COLOUR_FORMAT_RGBA16);
	heFboValidate(&engine->hdrFbo);
	//heFboCreateDepthBufferAttachment(&engine->hdrFbo);
		
	if(engine->renderMode == HE_RENDER_MODE_DEFERRED) {
		engine->deferred.gBufferShader	 = heAssetPoolGetShader("3d_gbuffer");
		engine->deferred.gLightingShader = heAssetPoolGetShader("3d_lighting");
		engine->deferred.gBufferFbo.size = window->windowInfo.size;
		heFboCreate(&engine->deferred.gBufferFbo);
		heFboCreateDepthBufferAttachment(&engine->deferred.gBufferFbo);
		heFboCreateColourTextureAttachment(&engine->deferred.gBufferFbo, HE_COLOUR_FORMAT_RGBA16); // creates worldpos texture
		heFboCreateColourTextureAttachment(&engine->deferred.gBufferFbo, HE_COLOUR_FORMAT_RGB16);  // creates normal texture
		heFboCreateColourTextureAttachment(&engine->deferred.gBufferFbo, HE_COLOUR_FORMAT_RGBA8);  // creates diffuse texture
		heFboCreateColourTextureAttachment(&engine->deferred.gBufferFbo, HE_COLOUR_FORMAT_RGB8);   // creates arm texture
		heFboCreateColourTextureAttachment(&engine->deferred.gBufferFbo, HE_COLOUR_FORMAT_RGB16);  // creates emission texture

	} else if(engine->renderMode == HE_RENDER_MODE_FORWARD) {
		engine->forward.forwardFbo.size = engine->window->windowInfo.size;
		heFboCreate(&engine->forward.forwardFbo);
		heFboCreateColourBufferAttachment(&engine->forward.forwardFbo, HE_COLOUR_FORMAT_RGBA16, 4);
		heFboCreateDepthBufferAttachment(&engine->forward.forwardFbo, 4);
		heFboValidate(&engine->forward.forwardFbo);
		
		heUboAllocate(&engine->forward.lightsUbo, "u_lights", 1 * (16 * sizeof(float)));
		heUboAllocate(&engine->forward.lightsUbo, "numLights", sizeof(int32_t));
		heUboCreate(&engine->forward.lightsUbo);
	}
};

void heRenderEngineResize(HeRenderEngine* engine) {
	// resize only if the size has changed and its not 0 (minimized window)
	if(engine->window->resized && engine->window->windowInfo.size != hm::vec2i(0)) {
		heFboResize(&engine->hdrFbo, engine->window->windowInfo.size);

		if(engine->renderMode == HE_RENDER_MODE_DEFERRED)
			heFboResize(&engine->deferred.gBufferFbo, engine->window->windowInfo.size);
		else if(engine->renderMode == HE_RENDER_MODE_FORWARD)
			heFboResize(&engine->forward.forwardFbo, engine->window->windowInfo.size);
		
		if(engine->postProcess.initialized) {
			hm::vec2i bloomSize(hm::floorPowerOfTwo(engine->window->windowInfo.size.x), hm::floorPowerOfTwo(engine->window->windowInfo.size.y));
			heFboResize(&engine->postProcess.bloomFbo, bloomSize);
		}

		// clear text meshes
		heTextManager.texts.clear();
        heFboUnbind(engine->window->windowInfo.size);
	}
};

void heRenderEngineDestroy(HeRenderEngine* engine) {
	heVaoDestroy(engine->shapes.quadVao);
	heVaoDestroy(engine->shapes.cubeVao);
	heVaoDestroy(engine->shapes.sphereVao);

	heShaderDestroy(engine->textureShader);
	heShaderDestroy(engine->rgbaShader);
	heShaderDestroy(engine->skyboxShader);
	heFboDestroy(&engine->hdrFbo);
	heTextureDestroy(engine->brdfIntegration);

	if (engine->renderMode == HE_RENDER_MODE_DEFERRED) {
		heShaderDestroy(engine->deferred.gBufferShader);
		heShaderDestroy(engine->deferred.gLightingShader);
		heFboDestroy(&engine->deferred.gBufferFbo);
	} else {
		heFboDestroy(&engine->forward.forwardFbo);
		heUboDestroy(&engine->forward.lightsUbo);
	}	

	heUiQueueDestroy(&engine->uiQueue);
	
	// destroy ui and post process
	if(engine->postProcess.initialized)
		hePostProcessEngineDestroy(&engine->postProcess);
};

void heRenderEnginePrepare(HeRenderEngine* engine) {
	heRenderEngineResize(engine);
	heFrameClear(engine->window->windowInfo.backgroundColour, HE_FRAME_BUFFER_BIT_COLOUR | HE_FRAME_BUFFER_BIT_DEPTH);
};

void heRenderEngineFinish(HeRenderEngine* engine) {
	heWindowSwapBuffers(engine->window);
};

void heShaderLoadMaterial(HeRenderEngine* engine, HeShaderProgram* shader, HeMaterial const* material) {
	if(engine->renderMode == HE_RENDER_MODE_DEFERRED)
		heShaderClearSamplers(shader);

	for (const auto& textures : material->textures)
		heTextureBind(textures.second, heShaderGetSamplerLocation(shader, "t_" + textures.first));
	
	for (const auto& uniforms : material->uniforms)
		heShaderLoadUniform(shader, "u_" + uniforms.first, &uniforms.second);

	heShaderLoadUniform(shader, "u_emission", material->emission);
	if(heRenderEngine->renderMode == HE_RENDER_MODE_DEFERRED)
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
	heShaderLoadUniform(program, uniformName + ".type",	  light->type);
	heShaderLoadUniform(program, uniformName + ".data1",  hm::vec4f(light->data[0], light->data[1], light->data[2], light->data[3]));
	heShaderLoadUniform(program, uniformName + ".data2", hm::vec4f(light->data[4], light->data[5], light->data[6], light->data[7]));
};

void heD3LevelRenderDeferred(HeRenderEngine* engine, HeD3Level* level) {
	// render all cts into the gbuffer
	heFboBind(&engine->deferred.gBufferFbo);
	heFrameClear(hm::colour(0, 0, 0, 0), HE_FRAME_BUFFER_BIT_COLOUR | HE_FRAME_BUFFER_BIT_DEPTH);

	heShaderBind(engine->deferred.gBufferShader);
	heShaderLoadUniform(engine->deferred.gBufferShader, "u_viewMat", level->camera.viewMatrix);
	heShaderLoadUniform(engine->deferred.gBufferShader, "u_projMat", level->camera.projectionMatrix);

	for(auto const& all : level->instances) {
		if(all.mesh == nullptr)
			continue;

		// render instance into the gbuffer
		hm::mat4f transMat = hm::createTransformationMatrix(all.transformation.position, all.transformation.rotation, all.transformation.scale);
		heShaderLoadUniform(engine->deferred.gBufferShader, "u_transMat", transMat);
		heShaderLoadUniform(engine->deferred.gBufferShader, "u_normMat",	 hm::transpose(hm::inverse(hm::mat3f(transMat))));
		heShaderLoadMaterial(engine, engine->deferred.gBufferShader, all.material);
		
		heVaoBind(all.mesh);
		heVaoRender(all.mesh);
	}
	
	heShaderBind(engine->deferred.gLightingShader);
	heCullEnable(false);
	
	// TODO(Victor): avoid for loop here, maybe store changed lights?
	uint8_t lightCount = 0;
	for(auto& all : level->lights) {
		if(all.update) {
			all.update = false;
			heShaderLoadLight(engine->deferred.gLightingShader, &all, lightCount);
		}
		
		lightCount++;
	}
	
	if(engine->outputTexture == 0) {
		uint32_t texCount = 0;
		heFboBind(&engine->hdrFbo);
		heFrameClear(hm::colour(0), HE_FRAME_BUFFER_BIT_COLOUR);
		
		heTextureBind(engine->deferred.gBufferFbo.colourAttachments[0].id, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_worldSpace", texCount++));
		heTextureBind(engine->deferred.gBufferFbo.colourAttachments[1].id, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_normals", texCount++));
		heTextureBind(engine->deferred.gBufferFbo.colourAttachments[2].id, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_diffuse", texCount++));
		heTextureBind(engine->deferred.gBufferFbo.colourAttachments[3].id, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_arm", texCount++));
		heTextureBind(engine->deferred.gBufferFbo.colourAttachments[4].id, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_emission", texCount++));
		heTextureBind(level->skybox.irradiance, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_irradiance", texCount++));
		heTextureBind(level->skybox.specular, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_environment", texCount++));
		heTextureBind(engine->brdfIntegration, heShaderGetSamplerLocation(engine->deferred.gLightingShader, "t_brdf", texCount++));
		heShaderLoadUniform(engine->deferred.gLightingShader, "u_cameraPos", level->camera.position);
		heUiSetQuadVao(engine->shapes.quadVao);
		heVaoRender(engine->shapes.quadVao);
	} else {
		heFboUnbind(engine->window->windowInfo.size);
		heUiRenderTexture(engine, engine->deferred.gBufferFbo.colourAttachments[engine->outputTexture - 1].id, hm::vec2f(-.5f), hm::vec2f(-1.f));
	}
	
	if(level->skybox.specular) {
		// render sky box
		heDepthEnable(false);
		heVaoBind(engine->shapes.cubeVao);
		heShaderBind(engine->skyboxShader);
		heShaderLoadUniform(engine->skyboxShader, "u_projMat", level->camera.projectionMatrix);
		heShaderLoadUniform(engine->skyboxShader, "u_viewMat", level->camera.viewMatrix);
		heShaderLoadUniform(engine->skyboxShader, "u_realDepthTest", false);
		heTextureBind(level->skybox.specular, heShaderGetSamplerLocation(engine->skyboxShader, "t_skybox", 0));
		heTextureBind(engine->deferred.gBufferFbo.colourAttachments[0].id, heShaderGetSamplerLocation(engine->skyboxShader, "t_alphaTest", 1));
		heVaoRender(engine->shapes.cubeVao);
	}

	heD3RenderInfo.exposure = level->camera.exposure;
	heD3RenderInfo.gamma    = level->camera.gamma;
};

void heD3InstanceRenderForward(HeRenderEngine* engine, HeD3Instance* instance) {
	hm::mat4f transMat = hm::createTransformationMatrix(instance->transformation.position, instance->transformation.rotation, instance->transformation.scale);
	heShaderLoadUniform(instance->material->shader, "u_transMat", transMat);
	heShaderLoadUniform(instance->material->shader, "u_normMat", hm::transpose(hm::inverse(hm::mat3f(transMat))));
	heShaderLoadMaterial(engine, instance->material->shader, instance->material);
	heVaoBind(instance->mesh);
	heVaoRender(instance->mesh);
};

void heD3LevelRenderForward(HeRenderEngine* engine, HeD3Level* level) {
	heFboBind(&engine->forward.forwardFbo);
	heFrameClear(hm::colour(0, 0, 0, 0), HE_FRAME_BUFFER_BIT_COLOUR | HE_FRAME_BUFFER_BIT_DEPTH);

	// update lights ubo
	uint32_t index   = 0;
	uint32_t offset  = 0;
	b8 bufferChanged = false;

	for (auto lights = level->lights.begin(); lights != level->lights.end(); ++lights) {
		if(lights->update) {
			//heUboUpdateData(&engine->forward.lightsUbo, index * lightSize, lightSize, &(*lights));
			hm::vec4f c(hm::getR(&lights->colour), hm::getG(&lights->colour), hm::getB(&lights->colour), hm::getA(&lights->colour));
			float type = (float) lights->type;

			heUboUpdateData(&engine->forward.lightsUbo, offset, sizeof(float), &type);
			offset += sizeof(float);
			heUboUpdateData(&engine->forward.lightsUbo, offset, 3 * sizeof(float), &lights->vector);
			offset += 3 * sizeof(float);
			heUboUpdateData(&engine->forward.lightsUbo, offset, 4 * sizeof(float), &c);
			offset += 4 * sizeof(float);
			heUboUpdateData(&engine->forward.lightsUbo, offset, 8 * sizeof(float), &lights->data);
			offset += 8 * sizeof(float);
			bufferChanged = true;
			lights->update = false;
		}
		
		index++;
	}

	heUboUpdateVariable(&engine->forward.lightsUbo, "numLights", &index);
	if (bufferChanged)
		heUboUploadData(&engine->forward.lightsUbo);

	// map all instances to their shader
	std::map<HeShaderProgram*, std::vector<HeD3Instance*>> shaderMap;
	for (auto it = level->instances.begin(); it != level->instances.end(); ++it) {
		if (it->material != nullptr && it->mesh != nullptr)
			shaderMap[it->material->shader].emplace_back(&(*it));
	}

	// loop through shaders, load data and then render all instances
	for (auto const& all : shaderMap) {
		heShaderBind(all.first);
		heShaderLoadUniform(all.first, "u_time", level->time);
		heUboBind(&engine->forward.lightsUbo, heShaderGetUboLocation(all.first, "LightInformation"));
		
		// load camera
		heShaderLoadUniform(all.first, "u_viewMat",   level->camera.viewMatrix);
		heShaderLoadUniform(all.first, "u_projMat",   level->camera.projectionMatrix);
		heShaderLoadUniform(all.first, "u_cameraPos", level->camera.position);

		// load pbr shit
		heTextureBind(level->skybox.irradiance, heShaderGetSamplerLocation(all.first, "t_irradiance"));
		heTextureBind(level->skybox.specular, heShaderGetSamplerLocation(all.first, "t_environment"));
		heTextureBind(engine->brdfIntegration, heShaderGetSamplerLocation(all.first, "t_brdf"));
		
		
		for (HeD3Instance* instances : all.second)		
			heD3InstanceRenderForward(engine, instances);
	}

	// render skybox
	if(level->skybox.specular) {
		heDepthFunc(HE_FRAGMENT_TEST_LEQUAL);
		heCullEnable(false);
		heVaoBind(engine->shapes.cubeVao);
		heShaderBind(engine->skyboxShader);
		heShaderLoadUniform(engine->skyboxShader, "u_projMat", level->camera.projectionMatrix);
		heShaderLoadUniform(engine->skyboxShader, "u_viewMat", level->camera.viewMatrix);
		heShaderLoadUniform(engine->skyboxShader, "u_realDepthTest", true);
		heTextureBind(level->skybox.specular, heShaderGetSamplerLocation(engine->skyboxShader, "t_skybox", 0));
		heTextureBind(nullptr, heShaderGetSamplerLocation(engine->skyboxShader, "t_alphaTest", 1));
		heVaoRender(engine->shapes.cubeVao);
	}

	heD3RenderInfo.exposure = level->camera.exposure;
	heD3RenderInfo.gamma    = level->camera.gamma;
	heFboRender(&engine->forward.forwardFbo, &engine->hdrFbo);
};

void heD3LevelRender(HeRenderEngine* engine, HeD3Level* level) {
	if(engine->renderMode == HE_RENDER_MODE_DEFERRED)
		heD3LevelRenderDeferred(engine, level);
	else if(engine->renderMode == HE_RENDER_MODE_FORWARD)
		heD3LevelRenderForward(engine, level);
};

void heRenderEnginePrepareD3(HeRenderEngine* engine) {
	heDepthEnable(true);
	heCullEnable(true);
	heBlendMode(-1);
};

void heRenderEngineFinishD3(HeRenderEngine* engine) {
	heFboUnbind(engine->window->windowInfo.size);
	heCullEnable(false);
	heDepthEnable(false);
	
	// post process hdr buffer
	if(engine->postProcess.initialized && engine->outputTexture == 0) {
		hePostProcessBloomPass(engine);

		heShaderBind(engine->postProcess.combineShader);
		heTextureBind(engine->hdrFbo.colourAttachments[0].id, heShaderGetSamplerLocation(engine->postProcess.combineShader, "t_in0", 0));
		heTextureBind(engine->postProcess.bloomFbo.colourAttachments[1].id, heShaderGetSamplerLocation(engine->postProcess.combineShader, "t_in1", 1));
		heShaderLoadUniform(engine->postProcess.combineShader, "u_hdr",      true);
		heShaderLoadUniform(engine->postProcess.combineShader, "u_exposure", heD3RenderInfo.exposure);
		heShaderLoadUniform(engine->postProcess.combineShader, "u_gamma",    heD3RenderInfo.gamma);
		heUiSetQuadVao(engine->shapes.quadVao);
		heVaoRender(engine->shapes.quadVao);
	}
};

void heRenderEnginePrepareUi(HeRenderEngine* engine) {
	heBlendMode(0);
	heCullEnable(false);
};

void heRenderEngineFinishUi(HeRenderEngine* engine) {

};

void hePostProcessBloomPass(HeRenderEngine* engine) {
	// bright pass filter
	heFboBind(&engine->postProcess.bloomFbo);
	heShaderBind(engine->postProcess.brightPassShader);
	heShaderLoadUniform(engine->postProcess.brightPassShader, "u_threshold", hm::vec3f(0.2126f, 0.7152f, 0.0722f));
	heTextureBind(engine->hdrFbo.colourAttachments[0].id, heShaderGetSamplerLocation(engine->postProcess.brightPassShader, "t_in", 0));
	heUiSetQuadVao(engine->shapes.quadVao);
	heVaoRender(engine->shapes.quadVao);
	heFboUnbind(engine->window->windowInfo.size);

	heTextureBind(engine->postProcess.bloomFbo.colourAttachments[0].id, 0);
	heTextureCreateMipmaps(engine->postProcess.bloomFbo.colourAttachments[0].id, 4); // downscale

	heShaderBind(engine->postProcess.gaussianBlurShader);
	heTextureBind(engine->postProcess.bloomFbo.colourAttachments[1].id, heShaderGetSamplerLocation(engine->postProcess.gaussianBlurShader, "t_lowerTexture", 2));
	
	int8_t lod = 4;
	hm::vec2i size = engine->postProcess.bloomFbo.size;
	for(int8_t i = 0; i < lod; ++i)
		size /= 2;
		
	for(int8_t i = lod; i >= 0; i--) {
		heShaderLoadUniform(engine->postProcess.gaussianBlurShader, "u_lod", i);
		heShaderLoadUniform(engine->postProcess.gaussianBlurShader, "u_size", hm::vec2f(size));
		HeTexture wrapper = heFboCreateColourTextureWrapper(&engine->postProcess.bloomFbo.colourAttachments[1]);
		heImageTextureBind(&wrapper, heShaderGetSamplerLocation(engine->postProcess.gaussianBlurShader, "t_outLod0", 0), i, -1, HE_ACCESS_WRITE_ONLY);

		// horizontal blur
		heTextureBind(engine->postProcess.bloomFbo.colourAttachments[0].id, heShaderGetSamplerLocation(engine->postProcess.gaussianBlurShader, "t_brightPass", 1));
		heShaderLoadUniform(engine->postProcess.gaussianBlurShader, "u_direction", hm::vec2f(1, 0));
		heShaderRunCompute(engine->postProcess.gaussianBlurShader, size.x / 8, size.y / 8, 1);

		// vertical blur
		heTextureBind(engine->postProcess.bloomFbo.colourAttachments[1].id, heShaderGetSamplerLocation(engine->postProcess.gaussianBlurShader, "t_brightPass", 1));
		heShaderLoadUniform(engine->postProcess.gaussianBlurShader, "u_direction", hm::vec2f(0, 1));
		heShaderRunCompute(engine->postProcess.gaussianBlurShader, size.x / 8, size.y / 8, 1);
		size *= 2;
	}
};



void heUiQueueCreate(HeUiQueue* queue) {
	// lines
#ifdef HE_ENABLE_NAMES
	queue->linesVao.name = "linesVao";
	queue->textVao.name	 = "textVao";
	queue->quadsVao.name = "quadsVao";
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

	// quads
	heVaoCreate(&queue->quadsVao, HE_VAO_TYPE_TRIANGLES);
	heVaoBind(&queue->quadsVao);
	heVboAllocate(&vbo, 0, 2, HE_VBO_USAGE_DYNAMIC);
	heVaoAddVbo(&queue->quadsVao, &vbo);
	heVboAllocate(&vbo, 0, 1, HE_VBO_USAGE_DYNAMIC, HE_DATA_TYPE_UINT);
	heVaoAddVbo(&queue->quadsVao, &vbo);
	heVaoUnbind(&queue->quadsVao);
};

void heUiQueueDestroy(HeUiQueue* queue) {
	heVaoDestroy(&queue->linesVao);
	heVaoDestroy(&queue->textVao);
	heVaoDestroy(&queue->quadsVao);
	heShaderDestroy(queue->linesShader);
	heShaderDestroy(queue->textShader);
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


void heUiTextCreateMesh(HeRenderEngine* engine, HeUiTextMesh* mesh, HeScaledFont const* font, std::string const& text, hm::colour const& colour) {
	uint32_t vertexCount = (uint32_t) text.size() * 6;
	hm::vec2f windowSize(engine->window->windowInfo.size);
	mesh->vertices.reserve(vertexCount * 4);
	mesh->colours.reserve(vertexCount);

	hm::vec2f cursor = hm::vec2f(font->padding.x, font->padding.y);
	for (unsigned char chars : text) {
		if (chars == 32) {
			// space
			cursor.x += font->spaceWidth;
			continue;
		}
				
		if(chars == 10) {
			// new line
			cursor.x  = 0.f;
			cursor.y += font->lineHeight;
			continue;	
		}

		if(chars == 9) {
			// tab
			cursor.x += font->spaceWidth * 4;
			continue;
		}
				
		auto it = font->font->characters.find(chars);
		if(it == font->font->characters.end()) {
			HE_DEBUG("Could not find character [" + std::to_string(chars) + "] in font [" + font->font->name + "]");
			continue;
		}
				
		HeFont::Character const* c = &it->second;
				
		// build quads
		hm::vec2f p0, p1, p2, p3, uv0, uv1, uv2, uv3;
		p0	= hm::vec2f(c->offset) * font->scale + cursor;
		p1	= hm::vec2f(p0.x + c->size.x * font->scale, p0.y);
		p2	= hm::vec2f(p0.x                          , p0.y + c->size.y * font->scale);
		p3	= hm::vec2f(p0.x + c->size.x * font->scale, p0.y + c->size.y * font->scale);
				
		uv0 = hm::vec2f(c->uv.x, c->uv.y);
		uv1 = hm::vec2f(c->uv.z, c->uv.y);
		uv2 = hm::vec2f(c->uv.x, c->uv.w);
		uv3 = hm::vec2f(c->uv.z, c->uv.w);
				
		p0 = heSpaceScreenToClip(p0, engine->window);
		p1 = heSpaceScreenToClip(p1, engine->window);
		p2 = heSpaceScreenToClip(p2, engine->window);
		p3 = heSpaceScreenToClip(p3, engine->window);
				
		// push these mesh->vertices as triangles
		mesh->vertices.emplace_back(p0.x);
		mesh->vertices.emplace_back(p0.y);
		mesh->vertices.emplace_back(uv0.x);
		mesh->vertices.emplace_back(uv0.y);
				
		mesh->vertices.emplace_back(p1.x);
		mesh->vertices.emplace_back(p1.y);
		mesh->vertices.emplace_back(uv1.x);
		mesh->vertices.emplace_back(uv1.y);
				
		mesh->vertices.emplace_back(p2.x);
		mesh->vertices.emplace_back(p2.y);
		mesh->vertices.emplace_back(uv2.x);
		mesh->vertices.emplace_back(uv2.y);
				
		mesh->vertices.emplace_back(p2.x);
		mesh->vertices.emplace_back(p2.y);
		mesh->vertices.emplace_back(uv2.x);
		mesh->vertices.emplace_back(uv2.y);
				
		mesh->vertices.emplace_back(p3.x);
		mesh->vertices.emplace_back(p3.y);
		mesh->vertices.emplace_back(uv3.x);
		mesh->vertices.emplace_back(uv3.y);
				
		mesh->vertices.emplace_back(p1.x);
		mesh->vertices.emplace_back(p1.y);
		mesh->vertices.emplace_back(uv1.x);
		mesh->vertices.emplace_back(uv1.y);
				
		uint32_t col = hm::encodeColour(colour);
		mesh->colours.emplace_back(col);
		mesh->colours.emplace_back(col);
		mesh->colours.emplace_back(col);
		mesh->colours.emplace_back(col);
		mesh->colours.emplace_back(col);
		mesh->colours.emplace_back(col);
				
		cursor.x += c->xadvance * font->scale;
	}

	mesh->width = (cursor.x / engine->window->windowInfo.size.x) * 2.f;	
};

HeUiTextMesh* heUiTextFindOrCreateMesh(HeRenderEngine* engine, HeScaledFont const* font, std::string const& text, hm::colour const& colour) {
	auto it = heTextManager.texts.find(text);
	if(it != heTextManager.texts.end()) {
		it->second.lastFrameAccess = heTextManager.currentFrame;
		return &it->second;
	}

	HeUiTextMesh* mesh = &heTextManager.texts[text];
	heUiTextCreateMesh(engine, mesh, font, text, colour);
	mesh->lastFrameAccess = heTextManager.currentFrame;
	return mesh;
};



void heUiRenderTexture(HeRenderEngine* engine, HeTexture const* texture, hm::vec2f const& position, hm::vec2f const& size) {
	HeTextureRenderMode mode = HE_TEXTURE_RENDER_2D;
	if(texture->cubeMap)
		mode = HE_TEXTURE_RENDER_CUBE_MAP;
	if(texture->format == HE_COLOUR_FORMAT_RGBA16 || texture->format == HE_COLOUR_FORMAT_RGB16)
		mode |= HE_TEXTURE_RENDER_HDR;
	
	heUiRenderTexture(engine, texture->textureId, position, size, mode);
};

void heUiRenderTexture(HeRenderEngine* engine, uint32_t const texture, hm::vec2f const& position, hm::vec2f const& size, HeTextureRenderMode const mode) {
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

	b8 isCubeMap = mode & HE_TEXTURE_RENDER_CUBE_MAP;
	heShaderLoadUniform(engine->textureShader, "u_isCubeMap", isCubeMap);
	heShaderLoadUniform(engine->textureShader, "u_isHdr", mode & HE_TEXTURE_RENDER_HDR);
	std::string textureName = (isCubeMap) ? "t_cubeTex" : "t_d2Tex";
	heTextureBind(texture, heShaderGetSamplerLocation(engine->textureShader, textureName, isCubeMap), isCubeMap);
	heUiSetQuadVao(engine->shapes.quadVao);
	heVaoRender(engine->shapes.quadVao);
};

void heUiRenderQuad(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::vec2f const& p2, hm::vec2f const& p3, hm::colour const& colour) {
	hm::vec2f _p0 = heSpaceScreenToClip(p0, engine->window);
	hm::vec2f _p1 = heSpaceScreenToClip(p1, engine->window);
	hm::vec2f _p2 = heSpaceScreenToClip(p2, engine->window);
	hm::vec2f _p3 = heSpaceScreenToClip(p3, engine->window);
	
	heShaderBind(engine->rgbaShader);
	heUiSetQuadVao(engine->shapes.quadVao, _p0, _p1, _p2, _p3);
	heShaderLoadUniform(engine->rgbaShader, "u_colour", colour);
	heVaoRender(engine->shapes.quadVao);
};

void heUiRenderText(HeRenderEngine* engine, HeScaledFont const* font, std::string const& text, hm::vec2f const& position, hm::colour const& colour, HeTextAlignMode align) {
	HeUiTextMesh mesh;
	heUiTextCreateMesh(engine, &mesh, font, text, colour);
	
	heShaderBind(engine->uiQueue.textShader);
	heVaoBind(&engine->uiQueue.textVao);
	heTextureBind(font->font->atlas, heShaderGetSamplerLocation(engine->uiQueue.textShader, "t_atlas", 0));

	hm::vec2f realPosition = position / engine->window->windowInfo.size * 2.f;
	if(align == HE_TEXT_ALIGN_CENTER)
		realPosition.x -= mesh.width / 2.f;

	heShaderLoadUniform(engine->uiQueue.textShader, "u_textSize", font->scale);
	heShaderLoadUniform(engine->uiQueue.textShader, "u_position", realPosition);
	heVaoUpdateData(&engine->uiQueue.textVao, mesh.vertices, 0);
	heVaoUpdateDataUint(&engine->uiQueue.textVao, mesh.colours, 1);
	heVaoRender(&engine->uiQueue.textVao);
};


void heUiQueueRenderLines(HeRenderEngine* engine) {
	size_t size = engine->uiQueue.lines.size();
	if(size == 0)
		return;
	
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
	hm::vec2f windowSize(engine->window->windowInfo.size);
	
	heShaderBind(engine->uiQueue.textShader);
	heVaoBind(&engine->uiQueue.textVao);
	
	for (auto const& all : engine->uiQueue.texts) {
		// prepare font
		heTextureBind(all.first->font->atlas, heShaderGetSamplerLocation(engine->uiQueue.textShader, "t_atlas", 0));	
		
		for (HeUiText const& texts : all.second) {
			HeUiTextMesh* mesh = heUiTextFindOrCreateMesh(engine, all.first, texts.text, texts.colour);

			hm::vec2f position = texts.position / engine->window->windowInfo.size * 2.f;
			if(texts.align == HE_TEXT_ALIGN_CENTER)
				position.x -= mesh->width / 2.f;
			
			heShaderLoadUniform(engine->uiQueue.textShader, "u_textSize", all.first->scale);
			heShaderLoadUniform(engine->uiQueue.textShader, "u_position", position);
			heVaoUpdateData(&engine->uiQueue.textVao, mesh->vertices, 0);
			heVaoUpdateDataUint(&engine->uiQueue.textVao, mesh->colours, 1);
			heVaoRender(&engine->uiQueue.textVao);
		}
	}
};

void heUiQueueRenderQuads(HeRenderEngine* engine) {
	hm::vec2f windowSize(engine->window->windowInfo.size);
	heShaderBind(engine->rgbaShader);
	heVaoBind(&engine->uiQueue.quadsVao);

	uint32_t size = (uint32_t) engine->uiQueue.quads.size() * 6;
	std::vector<float> vertices;
	std::vector<uint32_t> colours;
	vertices.reserve(size * 2);
	colours.reserve(size);
	
	for(HeUiColouredQuad const& all : engine->uiQueue.quads) {
		hm::vec2f _p0 = heSpaceScreenToClip(all.points[0], engine->window);
		hm::vec2f _p1 = heSpaceScreenToClip(all.points[1], engine->window);
		hm::vec2f _p2 = heSpaceScreenToClip(all.points[2], engine->window);
		hm::vec2f _p3 = heSpaceScreenToClip(all.points[3], engine->window);

		vertices.emplace_back(_p0.x);
		vertices.emplace_back(_p0.y);

		vertices.emplace_back(_p1.x);
		vertices.emplace_back(_p1.y);
		
		vertices.emplace_back(_p2.x);
		vertices.emplace_back(_p2.y);

		vertices.emplace_back(_p2.x);
		vertices.emplace_back(_p2.y);

		vertices.emplace_back(_p3.x);
		vertices.emplace_back(_p3.y);

		vertices.emplace_back(_p1.x);
		vertices.emplace_back(_p1.y);

		uint32_t colour = hm::encodeColour(all.colour);
		colours.emplace_back(colour);
		colours.emplace_back(colour);
		colours.emplace_back(colour);
		colours.emplace_back(colour);
		colours.emplace_back(colour);
		colours.emplace_back(colour);
	}

	heShaderLoadUniform(engine->rgbaShader, "u_colour", hm::colour(0, 0, 0, 0));
	heVaoUpdateData(&engine->uiQueue.quadsVao, vertices, 0);
	heVaoUpdateDataUint(&engine->uiQueue.quadsVao, colours, 1);
	heVaoRender(&engine->uiQueue.quadsVao);
};

void heUiQueueRender(HeRenderEngine* engine) {
	heBlendMode(0);
	heDepthEnable(false);
	if(engine->uiQueue.lines.size()) {
		heUiQueueRenderLines(engine);
		engine->uiQueue.lines.clear();
	}
	
	if(engine->uiQueue.quads.size()) {
		heUiQueueRenderQuads(engine);
		engine->uiQueue.quads.clear();
		}
	
	if(engine->uiQueue.texts.size()) {
		heUiQueueRenderTexts(engine);
		engine->uiQueue.texts.clear();
	}

	heTextManager.currentFrame++;
	
	// remove all unused text meshes
	auto it = heTextManager.texts.cbegin();
	while(it != heTextManager.texts.cend()) {
		if(heTextManager.currentFrame > 5 && it->second.lastFrameAccess < heTextManager.currentFrame - 5)
			it = heTextManager.texts.erase(it);
		else
			++it;
	}

	if(heTextManager.currentFrame > 100000)
		heTextManager.currentFrame = 0;
};


void heUiPushLineD2(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::colour const& colour, float const width) {
	engine->uiQueue.lines.emplace_back(p0, p1, colour, width);
};

void heUiPushLineD3(HeRenderEngine* engine, hm::vec3f const& p0, hm::vec3f const& p1, hm::colour const& colour, float const width) {
	engine->uiQueue.lines.emplace_back(p0, p1, colour, width);
};

void heUiPushText(HeRenderEngine* engine, HeScaledFont const* font, std::string const& text, hm::vec2f const& position, hm::colour const& colour, HeTextAlignMode const align) {
	if(text.size() > 0)
		engine->uiQueue.texts[font].emplace_back(HeUiText(text, position, colour, align));
};

void heUiPushQuad(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::vec2f const& p2, hm::vec2f const& p3, hm::colour const& colour) {
	engine->uiQueue.quads.emplace_back(p0, p1, p2, p3, colour);
};

void heUiPushQuad(HeRenderEngine* engine, hm::vec2f const& position, hm::vec2f const& size, hm::colour const& colour) {
	engine->uiQueue.quads.emplace_back(position, position + hm::vec2f(0, size.y), position + hm::vec2f(size.x, 0), position + size, colour);
};


hm::vec2f heSpaceScreenToClip(hm::vec2f const& pixelspace, HeWindow const* window) {
	return hm::vec2f((pixelspace.x / window->windowInfo.size.x) * 2.f - 1.f,
					 1.f - (pixelspace.y / window->windowInfo.size.y) * 2.f);
};

hm::vec3f heSpaceWorldToScreen(hm::vec3f const& worldSpace, HeD3Camera const * camera, HeWindow const* window) {
	hm::vec4f clip = camera->viewMatrix * hm::vec4f(worldSpace, 1.0f);
	clip = camera->projectionMatrix * clip;
	if(clip.w != 0.0f) {
		clip.x /= clip.w;
		clip.y /= clip.w;
		clip.z /= clip.w;
	}
	
	return hm::vec3f(((clip.x + 1.f) / 2.f) * window->windowInfo.size.x, ((1.f - clip.y) / 2.f) * window->windowInfo.size.y, clip.w);
};

#pragma once
#include "heD3.h"

struct HeWindow; // avoid #include "heWindow.h"

struct HeRenderEngine {
    // a 2d quad vao used for 2d rendering
    HeVao* quadVao = nullptr;
    // used for 2d texture rendering
    HeShaderProgram* textureShader = nullptr;
    // the window this engine operates on
    HeWindow* window = nullptr;
    
    // a map of render properties. The name of the property (%name%, without the %) will be replaced
    // in every shader with the value stored (can be an int as string...)
    std::map<std::string, std::string> renderProperties;
    
    /* 3d stuff */
    
    // a 16 bit multisampled fbo used for hdr rendering 
    HeFbo hdrFbo;
    // an 16 bit fbo used for post-process image manipulation 
    HeFbo resolvedFbo;
    // used for rendering the hdr fbo onto the screen with some final colour corrections etc
    HeShaderProgram* d3FinalShader = nullptr;
};


// loads all important data for the render engine
extern HE_API void heCreateRenderEngine(HeRenderEngine* engine, HeWindow* window);
// checks if the fbos of the engine are the size of the window, and (if not so) resizes them
extern HE_API void heResizeRenderEngine(HeRenderEngine* engine);
// destroys all the data of the render engine
extern HE_API void heDestroyRenderEngine(HeRenderEngine* engine);
// loads given material to given shader by uploading all textures and uniforms set in the material
extern HE_API void heLoadMaterialToShader(HeShaderProgram* shader, const HeMaterial* material);
// loads a 3d light source to given shader. If index is -1, the shader is assumed to only have one light as a uniform 
// called u_light. If index is greater or equal to 0, the shader is assumed to have an array of lights, called 
// u_lights[]
extern HE_API void heLoadLightToShader(HeShaderProgram* program, const HeD3LightSource* light, const int8_t index);
// renders a single 3d instance using the instances mesh and material. Make sure that a shader is bound and
// set up before this is called (camera and lights)
extern HE_API void heRenderD3Instance(HeD3Instance* instance);
// renders given texture to the currently bound fbo. The texture will be centered at position. Position and scale can 
// be relativ (negativ) or absolute (positiv). This will load the texture shader if it wasnt already
extern HE_API void heRenderD2Texture(HeRenderEngine* engine, const HeTexture* texture, const hm::vec2f& position, const hm::vec2f& size);
// renders a complete level with all its instances by mapping all instances to their shader and loading all
// important data (camera, lights...) to these shaders
extern HE_API void heRenderD3Level(HeD3Level* level);
// prepares the render engine for 3d rendering by binding the hdr fbo
extern HE_API void hePrepareD3RenderEngine(HeRenderEngine* engine);
// ends the render engine by drawing the fbo onto the screen
extern HE_API void heEndD3RenderEngine(HeRenderEngine* engine);

// updates a (or adds a new) render property for given engine. This should be done before the shaders are loaded.
// If a property is updated after the shaders are loaded, you can reload specific shaders manually
extern HE_API void heSetRenderProperty(HeRenderEngine* engine, const std::string& name, const int32_t value);
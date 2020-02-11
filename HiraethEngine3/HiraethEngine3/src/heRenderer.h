#pragma once
#include "heD3.h"
#include "heWindow.h"

struct HeRenderEngine {
    // a 2d quad vao used for 2d rendering
    HeVao* quadVao = nullptr;
    // used for 2d texture rendering
    HeShaderProgram* textureShader = nullptr;
    // the window this engine operates on
    HeWindow* window = nullptr;
};

// loads all important data for the render engine
extern HE_API void heCreateRenderEngine(HeRenderEngine* engine, HeWindow* window);
// destroys all the data of the render engine
extern HE_API void heDestroyRenderEngine(HeRenderEngine* engine);
// loads given material to given shader by uploading all textures and uniforms set in the material
extern HE_API void heLoadMaterialToShader(HeShaderProgram* shader, const HeMaterial* material);
// loads a 3d light source to given shader. If index is -1, the shader is assumed to only have one light as a uniform called u_light.
// If index is greater or equal to 0, the shader is assumed to have an array of lights, called u_lights[]
extern HE_API void heLoadLightToShader(HeShaderProgram* program, const HeD3Light* light, const int index);
// renders a single 3d instance using the instances mesh and material. Make sure that a shader is bound and
// set up before this is called (camera and lights)
extern HE_API void heRenderD3Instance(HeD3Instance* instance);
// renders given texture to the currently bound fbo. The texture will be centered at position. Position and scale can be relativ (negativ)
// or absolute (positiv). This will load the texture shader if it wasnt already
extern HE_API void heRenderD2Texture(HeRenderEngine* engine, const HeTexture* texture, const hm::vec2f& position, const hm::vec2f& size);
// renders a complete level with all its instances by mapping all instances to their shader and loading all
// important data (camera, lights...) to these shaders
extern HE_API void heRenderD3Level(HeD3Level* level);
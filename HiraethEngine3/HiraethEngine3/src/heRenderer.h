#ifndef HE_RENDERER_H
#define HE_RENDERER_H

//#include "heD3.h"
#include <string>
#include "heGlLayer.h"

struct HeD3Level;
struct HeD3Camera;
struct HeD3Instance;
struct HeD3LightSource;
struct HeMaterial;

/*

TODO(Victor):
Add UI rendering
Add Post Process (Bloom)
Deferred Render Engine
Multiple Lights dynamically

*/

struct HeWindow; // avoid #include "heWindow.h"

struct HeUiLine {
    hm::colour colour;
    // two end points of the line in screen space
    hm::vec3f p0, p1;
    // width of the line in screen space
    float width;
    // if this is true, the line is in 2d (screen space) else this is in 3d (world space)
    b8 d3;
    
    HeUiLine() : p0(0.f), p1(0.f), colour(0), width(0.f), d3(false) {};
    HeUiLine(hm::vec2f const& p0, hm::vec2f const& p1, hm::colour const& col, float const width) :
    p0(p0), p1(p1), colour(col), width(width), d3(false) {};
    HeUiLine(hm::vec3f const& p0, hm::vec3f const& p1, hm::colour const& col, float const width) :
    p0(p0), p1(p1), colour(col), width(width), d3(true) {};
};

struct HeUiQueue {
    std::vector<HeUiLine> lines;
    HeVao linesVao;
    HeShaderProgram* linesShader = nullptr;
};

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
    
    // the ui render queue for batch rendering
    HeUiQueue uiQueue;
    
    /* hdr stuff */
    
    // a 16 bit multisampled fbo used for hdr rendering
    HeFbo hdrFbo;
    // an 16 bit fbo used for post-process image manipulation
    HeFbo resolvedFbo;
    // used for rendering the hdr fbo onto the screen with tone mapping, colour corrections etc
    HeShaderProgram* finalShader = nullptr;
};

// the current active render engine. Can be set and (then) used at any time
extern HeRenderEngine* heRenderEngine;

// loads all important data for the render engine
extern HE_API void heRenderEngineCreate(HeRenderEngine* engine, HeWindow* window);
// checks if the fbos of the engine are the size of the window, and (if not so) resizes them
extern HE_API void heRenderEngineResize(HeRenderEngine* engine);
// destroys all the data of the render engine
extern HE_API void heRenderEngineDestroy(HeRenderEngine* engine);
// loads given material to given shader by uploading all textures and uniforms set in the material
extern HE_API void heShaderLoadMaterial(HeShaderProgram* shader, HeMaterial const* material);
// loads a 3d light source to given shader. If index is -1, the shader is assumed to only have one light as a uniform
// called u_light. If index is greater or equal to 0, the shader is assumed to have an array of lights, called u_lights[]
extern HE_API void heShaderLoadLight(HeShaderProgram* program, HeD3LightSource const* light, int8_t const index);
// renders a single 3d instance using the instances mesh and material. Make sure that a shader is bound and
// set up before this is called (camera and lights)
extern HE_API void heD3InstanceRender(HeD3Instance* instance);
// renders given texture to the currently bound fbo. The texture will be centered at position. Position and scale can
// be relativ (negativ) or absolute (positiv). This will load the texture shader if it wasnt already
extern HE_API void heD2TextureRender(HeRenderEngine* engine, HeTexture const* texture, hm::vec2f const& position, hm::vec2f const& size);
// renders a complete level with all its instances by mapping all instances to their shader and loading all
// important data (camera, lights...) to these shaders
extern HE_API void heD3LevelRender(HeD3Level* level);
// prepares the render engine for 3d rendering by binding the multisampled hdr fbo
extern HE_API void heRenderEnginePrepareD3(HeRenderEngine* engine);
// ends the render engine by drawing the fbo onto the screen
extern HE_API void heRenderEngineFinishD3(HeRenderEngine* engine);
// prepares the render engine for ui rendering by binding the multisampled fbo
extern HE_API void heRenderEnginePrepareUi(HeRenderEngine* engine);
// blits from the multisampled fbo onto the screen
extern HE_API void heRenderEngineFinishUi(HeRenderEngine* engine);
// updates a (or adds a new) render property for given engine. This should be done before the shaders are loaded.
// If a property is updated after the shaders are loaded, you can reload specific shaders manually
extern HE_API void heRenderEngineSetProperty(HeRenderEngine* engine, std::string const& name, int32_t const value);

// sets up this ui queue by creating the necessary vaos and shaders
extern HE_API void heUiQueueCreate(HeUiQueue* queue);
// renders all lines batched together
extern HE_API void heUiQueueRenderLines(HeRenderEngine* queue);
// renders all ui elements pushed
extern HE_API void heUiQueueRender(HeRenderEngine* engine);

// pushes a new line to the render queue. The line will go from p0 to p1 (in pixels), width is in pixels
extern HE_API inline void heUiPushLineD2(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::colour const& colour, float const width);
// pushes a new line to the render queue. The line will be projected onto the screen using the camera of the current active d3 level.
// p0 and p1 must be in world space, width is in pixels
extern HE_API inline void heUiPushLineD3(HeRenderEngine* engine, hm::vec3f const& p0, hm::vec3f const& p1, hm::colour const& colour, float const width);

// transforms given vector from pixel space to clip space ([-1:1])
extern HE_API inline hm::vec2f heSpaceScreenToClip(hm::vec2f const& pixelspace, HeWindow const* window);
// calculates the screen space coordinate (in pixels) of the given world spacce position by transforming it with the matrices of
// the camera
extern HE_API inline hm::vec2f heSpaceWorldToScreen(hm::vec3f const& worldSpace, HeD3Camera const* camera, HeWindow const* window);

#endif
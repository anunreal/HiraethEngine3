#ifndef HE_RENDERER_H
#define HE_RENDERER_H

#include <string>
#include <unordered_map>
#include "heGlLayer.h"

struct HeD3Level;
struct HeD3Camera;
struct HeD3Instance;
struct HeD3LightSource;
struct HeMaterial;
struct HeScaledFont;
struct HeWindow;

/*
  TODO(Victor):	 Add Post Process (Bloom), Forward renderer
*/


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

struct HeUiText {
	std::string text;
	hm::vec2i	position; // in window space
	hm::colour	colour;

	HeUiText() : text(""), position(0), colour(0) {};
	HeUiText(std::string const& text, hm::vec2i const& position, hm::colour const& colour) : text(text), position(position), colour(colour) {};
};

struct HeUiQueue {
	HeVao linesVao;
	HeShaderProgram* linesShader = nullptr;
	std::vector<HeUiLine> lines;
	
	HeVao textVao;
	HeShaderProgram* textShader	 = nullptr;
	std::unordered_map<HeScaledFont const*, std::vector<HeUiText>> texts;
};

struct HeRenderEngine {
	struct {
		// a 2d quad vao used for 2d rendering
		HeVao* quadVao = nullptr;
		// a unit cube
		HeVao* cubeVao = nullptr;
		// a unit sphere
		HeVao* sphereVao = nullptr;
	} shapes;
	
	// the window this engine operates on
	HeWindow* window = nullptr;

	// a simple rgba shader that renders a mesh with one rgba colour
	HeShaderProgram* rgbaShader; 
	// used for 2d texture rendering
	HeShaderProgram* textureShader = nullptr;
	// the ui render queue for batch rendering
	HeUiQueue uiQueue;
	
	// the texture that should be drawn on screen. If this is 0, the final texture (with lightin applied) is drawn.
	// 1: world space texture
	// 2: normal texture
	// 3: diffuse texture
	// 4: arm texture
	uint8_t outputTexture = 0;
	// a 16 bit multisampled fbo used for hdr rendering
	HeFbo gBufferFbo;
	// an 16 bit fbo used for post-process image manipulation
	HeFbo resolvedFbo;
	// used for rendering the hdr fbo onto the screen with tone mapping, colour corrections etc
	HeShaderProgram* finalShader	 = nullptr;
	// the shader used for rendering d3 objects into the gbuffer. This will put information like position, normals... into the
	// gbuffer independant of the instances material
	HeShaderProgram* gBufferShader	 = nullptr;
	// the lighting shader after the gbuffer pass
	HeShaderProgram* gLightingShader = nullptr;
	// renders an hdr skybox of a level (cube map texture) 
	HeShaderProgram* skyboxShader	 = nullptr;
	// the brdf integration texture (rg channels)
	HeTexture* brdfIntegration = nullptr;
};

// the current active render engine. Can be set and (then) used at any time
extern HeRenderEngine* heRenderEngine;


// -- render engine

// loads all important data for the render engine
extern HE_API void heRenderEngineCreate(HeRenderEngine* engine, HeWindow* window);
// checks if the fbos of the engine are the size of the window, and (if not so) resizes them
extern HE_API void heRenderEngineResize(HeRenderEngine* engine);
// destroys all the data of the render engine
extern HE_API void heRenderEngineDestroy(HeRenderEngine* engine);
// prepares the render engine for a new frame
extern HE_API void heRenderEnginePrepare(HeRenderEngine* engine);
// finishes up the frame by swapping the buffers
extern HE_API void heRenderEngineFinish(HeRenderEngine* engine);
// loads given material to given shader by uploading all textures and uniforms set in the material
extern HE_API void heShaderLoadMaterial(HeShaderProgram* shader, HeMaterial const* material);
// loads a 3d light source to given shader. If index is -1, the shader is assumed to only have one light as a uniform
// called u_light. If index is greater or equal to 0, the shader is assumed to have an array of lights, called u_lights[]
extern HE_API void heShaderLoadLight(HeShaderProgram* program, HeD3LightSource const* light, int8_t const index);
// renders a single 3d instance using the instances mesh and material. Make sure that a shader is bound and
// set up before this is called (camera and lights)
extern HE_API void heD3InstanceRender(HeD3Instance* instance);
// renders a complete level with all its instances by mapping all instances to their shader and loading all
// important data (camera, lights...) to these shaders
extern HE_API void heD3LevelRender(HeRenderEngine* engine, HeD3Level* level);
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


// -- ui

// sets up this ui queue by creating the necessary vaos and shaders
extern HE_API void heUiQueueCreate(HeUiQueue* queue);
// updates the vao to hold the given vertices (makes two triangles). Binds the given vao first. The default arguments
// build a quad that covers the whole screen
extern HE_API inline void heUiSetQuadVao(HeVao* vao, hm::vec2f const& p0 = hm::vec2f(-1, 1), hm::vec2f const& p1 = hm::vec2f(1, 1), hm::vec2f const& p2 = hm::vec2f(-1, -1), hm::vec2f const& p3 = hm::vec2f(1, -1));

// renders given texture to the currently bound fbo. This can either be a cube or a 2d texture. The texture will be centered at
// position. Position and scale can be relativ (negativ) or absolute (positiv). This will load the texture shader
// if it wasnt already
extern HE_API inline void heUiRenderTexture(HeRenderEngine* engine, HeTexture const* texture, hm::vec2f const& position, hm::vec2f const& size);
// renders given 2d texture to the currently bound fbo. The texture will be centered at position. Position and scale can
// be relativ (negativ) or absolute (positiv). This will load the texture shader if it wasnt already
extern HE_API void heUiRenderTexture(HeRenderEngine* engine, uint32_t const texture, hm::vec2f const& position, hm::vec2f const& size, bool const isCubeMap = false);
// renders a quad with given vertices in window space with given colour
extern HE_API void heUiRenderQuad(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::vec2f const& p2, hm::vec2f const& p3, hm::colour const& colour);

// renders all lines batched together
extern HE_API void heUiQueueRenderLines(HeRenderEngine* queue);
// renders all texts batched by their font
extern HE_API void heUiQueueRenderTexts(HeRenderEngine* queue);
// renders all ui elements pushed
extern HE_API void heUiQueueRender(HeRenderEngine* engine);
// pushes a new line to the render queue. The line will go from p0 to p1 (in pixels), width is in pixels
extern HE_API inline void heUiPushLineD2(HeRenderEngine* engine, hm::vec2f const& p0, hm::vec2f const& p1, hm::colour const& colour, float const width);
// pushes a new line to the render queue. The line will be projected onto the screen using the camera of the current active d3 level.
// p0 and p1 must be in world space, width is in pixels
extern HE_API inline void heUiPushLineD3(HeRenderEngine* engine, hm::vec3f const& p0, hm::vec3f const& p1, hm::colour const& colour, float const width);
// pushes a new text with position in window space (pixels)
extern HE_API inline void heUiPushText(HeRenderEngine* engine, HeScaledFont const* font, std::string const& text, hm::vec2i const& position, hm::colour const& colour);

// transforms given vector from pixel space to clip space ([-1:1])
extern HE_API inline hm::vec2f heSpaceScreenToClip(hm::vec2f const& pixelspace, HeWindow const* window);
// calculates the screen space coordinate (in pixels) of the given world spacce position by transforming it with the matrices of
// the camera
extern HE_API inline hm::vec2f heSpaceWorldToScreen(hm::vec3f const& worldSpace, HeD3Camera const* camera, HeWindow const* window);

#endif

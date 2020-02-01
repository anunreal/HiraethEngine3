#pragma once
#pragma once
#include <map>
#include <vector>
#include <string>
#include "hm/hm.hpp"
#include "heTypes.h"

enum HeUniformDataType {
	HE_SHADER_DATA_TYPE_INT,
	HE_SHADER_DATA_TYPE_VEC2,
	HE_SHADER_DATA_TYPE_BOOL,
	HE_SHADER_DATA_TYPE_FLOAT,
	HE_SHADER_DATA_TYPE_DOUBLE,
	HE_SHADER_DATA_TYPE_COLOUR,
};

enum HeFboFlags {
	HE_FBO_FLAG_NONE = 0,
	HE_FBO_FLAG_HDR = 0b0001,
	HE_FBO_FLAG_DEPTH_RENDER_BUFFER = 0b0010,
	HE_FBO_FLAG_DEPTH_TEXTURE = 0b0100
};

struct HeShaderData {
	HeUniformDataType type;

	union {
		int _int;
		float _float;
		hm::vec2f _vec2;
		hm::colour _colour;
	};
};

struct HeShaderProgram {
	unsigned int programId = 0;
	// maps uniforms to the locations returned by gl. Every uniform has to be loaded once, then they will
	// be stored in the map for faster lookup.
	std::map<std::string, int> uniforms;
	// maps samplers to the texture slots, i.e. a sampler2D in a shader has a uniform location, but that is not the texture
	// slot. When loading a sampler uniform, the wanted texture slot can be given as a parameter. This slot is saved here so
	// that we dont have to remember it 
	std::map<std::string, int> samplers;
};

struct HeVbo {
	unsigned int vboId = 0;
	unsigned int dimensions = 0;
	unsigned int verticesCount = 0;
};

struct HeVao {
	unsigned int vaoId = 0;
	unsigned int verticesCount = 0;
	std::vector<HeVbo> vbos;
};

struct HeFbo {
	// the gl id of this fbo, used by the engine
	unsigned int fboId = 0;
	// an array of (possible) depth attachments to this fbo. These values may be set or just left at 0 if that attachment
	// was not created
	// 0. depth texture
	// 1. depth buffer
	unsigned int depthAttachments[2] = { 0 };


	/* only one of the two arrays should be used, therefore a fbo should only have either colour buffers or
	 * colour textures */

	 // a vector of (multisampled) colour buffers attached to this array.
	std::vector<unsigned int> colourBuffers;
	// a vector of colour textures attached to this array
	std::vector<unsigned int> colourTextures;

	// the amount of samples in this fbo (multisampling). Must be either 4, 8, 16. More samplers mean smoother
	// results but higher memory usage. Needs to be set before fbo creation
	unsigned int samples = 1;
	// flags of this fbo. More flags can be set, but only one of HE_FBO_FLAG_DEPTH_RENDER_BUFFER and HE_FBO_FLAG_DEPTH_TEXTURE
	// should be set. Needs to be set before fbo creation
	HeFboFlags flags;
	// the size of this fbo (in pixels). Needs to be set before fbo creation
	hm::vec2i size;
};

struct HeTexture {
	// for every time this texture is requested from the asset pool, this reference count goes up.
	// If this texture is destroyed with heDestroyTexture, referenceCount will go down.
	// Only if the referenceCount is 0, the texture will actually be deleted
	unsigned int referenceCount = 0;

	// the opengl texture id
	unsigned int textureId = 0;
	// dimensions of the texture
	int width = 0, height = 0;
	// the number of channels in this texture. Can be 3 (rgb) or 4 (rgba)
	int channels = 0;
};


// maps a pointer to a texture in the texture pool to the content of the file it was loaded from
// this is only used if the texture was loaded from a side-thread (not the thread the window was created in)
typedef std::map<HeTexture*, unsigned char*> HeThreadLoader;

namespace HeAssets {
	extern HeThreadLoader threadLoader;
};


// --- Shaders

// loads a shader from given shader files
extern HE_API void heLoadShader(HeShaderProgram* program, const std::string& vertexShader, const std::string& fragmentShader);
// binds given shader
extern HE_API void heBindShader(const HeShaderProgram* program);
// unbinds the currently bound shader
extern HE_API void heUnbindShader();
// deletes the given shader
extern HE_API void heDestroyShader(HeShaderProgram* program);

// gets the location of given uniform in given shader. Once the uniform is loaded, the location is stored in program
// for faster lookups later. If the uniform wasnt found (spelling mistake or optimized away), -1 is returned
extern HE_API int heGetShaderUniformLocation(HeShaderProgram* program, const std::string& uniform);
// gets the location of given sampler in given shader. Once the sampler is loaded, the location is stored in program
// for faster lookups later. If the sampler wasnt found (spelling mistake or optimized away), -1 is returned.
// If the sampler does exist, it will be bound to given texture slot (glActiveTexture(GL_TEXTURE0 + requestedSlot))
extern HE_API int heGetShaderSamplerLocation(HeShaderProgram* program, const std::string& sampler, const unsigned int requestedSlot);

extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const int value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const float value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const double value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const uint32_t value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::mat4& value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec2f& value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec4f& value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const hm::colour& value);
extern HE_API void heLoadShaderUniform(HeShaderProgram* program, const std::string& uniformName, const HeShaderData* data);


// --- Buffers

// creates a new vbo buffer from given data. Dimensions is the number of floats that build one vertex 
// (i.e. 3 for a normal, 2 for a uv). This does not add the buffer to any vao. A vbo can be used in
// different vaos, though it is not recommended
extern HE_API void heCreateVbo(HeVbo* vbo, const std::vector<float>& data);
// deletes the given vbo and sets its id to 0
extern HE_API void heDestroyVbo(HeVbo* vbo);

// creates a new empty vao
extern HE_API void heCreateVao(HeVao* vao);
// adds an existing vbo to given vao. Both vao and vbo need to be created beforehand, and the vao needs
// to be bound before this call. If this is the first vbo for the given vao, the verticesCount 
// of the vao is calculated 
extern HE_API void heAddVbo(HeVao* vao, HeVbo* vbo);
// adds new data to given vao. The vao needs to be created and bound before this.
extern HE_API void heAddVaoData(HeVao* vao, const std::vector<float>& data, const unsigned int dimensions);
// binds given vao
extern HE_API void heBindVao(const HeVao* vao);
// unbinds the currently bound vao
extern HE_API void heUnbindVao(const HeVao* vao);
// deletes given vao and all associated vbos
extern HE_API void heDestroyVao(HeVao* vao);
// renders given vao. This assumes the vao to be in GL_TRIANGLES mode
extern HE_API void heRenderVao(const HeVao* vao);


// adds a new colour texture attachment to given fbo
extern HE_API void heCreateColourAttachment(HeFbo* fbo);
// adds a new (multisampled) colour buffer attachment to given fbo
extern HE_API void heCreateMultisampledColourAttachment(HeFbo* fbo);
// creates a new fbo. All parameters of the fbo must already be set (see parameter description). This will add
// a (multisampled, if samples is bigger than 1) colour attachment and either a depth texture or a depth buffer
// attachment to the fbo
extern HE_API void heCreateFbo(HeFbo* fbo);
// binds given fbo and resizes the viewport
extern HE_API void heBindFbo(HeFbo* fbo);
// unbinds the current fbo and restores the viewport to given windowSize
extern HE_API void heUnbindFbo(const hm::vec2i& windowSize);
// unbinds the current fbo
extern HE_API void heUnbindFbo();
// destroys given fbo
extern HE_API void heDestroyFbo(HeFbo* fbo);
// resizes given fbo by destroying it and creating a new version with new size
extern HE_API void heResizeFbo(HeFbo* fbo, const hm::vec2i& newSize);


// --- Textures

// loads a new texture from given file. If given file does not exist, an error is printed and no gl texture
// will be generated
extern HE_API void heLoadTexture(HeTexture* texture, const std::string& fileName);
// creates an opengl texture from given information. Id will be set to the gl texture id created. This will
// also delete the buffer
extern HE_API void heCreateTexture(unsigned char* buffer, unsigned int* id, const int width, const int height,
	const int channels);
// loads a new texture from given stream. If the stream is invalid, an error is printed and no gl texture
// will be generated
extern HE_API void heLoadTexture(HeTexture* texture, FILE* stream);
// binds given texture to given gl slot
extern HE_API void heBindTexture(const HeTexture* texture, const int slot);
// binds given texture id to given gl slot
extern HE_API void heBindTexture(const unsigned int texture, const int slot);
// unbinds the currently bound texture from given gl slot
extern HE_API void heUnbindTexture(int slot);
// deletes given texture if its reference count is currently 1
extern HE_API void heDestroyTexture(HeTexture* texture);


// --- Utils

// clears the buffers of current fbo to given colour. Type is the different buffers to be cleared
// type = 0: The colour buffer is cleared
// type = 1: The depth buffer is cleared
// type = 2: Both the colour and the depth colour are cleared
extern HE_API void heClearFrame(const hm::colour& colour, const int type);
// sets the gl blend mode to given mode. Possible modes:
// -1 = disable gl blending
// 0  = normal blending (one minus source alpha)
// 1  = additive blending (add two colours)
// 2  = disable blending by completely overwriting previous colour with new one
extern HE_API void heBlendMode(const int mode);
// applies a gl blend mode to only given colour attachment of a fbo. Same modes as above apply
extern HE_API void heBufferBlendMode(const int attachmentIndex, const int mode);
// en- or disables depth testing
extern HE_API void heEnableDepth(const bool depth);


// --- ThreadLoader

// loads all resources in the thread loader into gl and clears the loader afterwards. This must be called from the 
// thread the window was created in (the main thread)
extern HE_API void heUpdateThreadLoader();
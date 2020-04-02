#ifndef HE_GL_LAYER_H
#define HE_GL_LAYER_H

#include <map>
#include <vector>
#include <string>
#include "hm/hm.hpp"
#include "heTypes.h"

struct HeShaderData {
    HeUniformDataType type;
    
    union {
        float _float;
        int32_t _int;
        hm::vec2f _vec2;
        hm::vec3f _vec3;
        hm::colour _colour;
    };
};

struct HeShaderProgram {
    uint32_t programId = 0;
    // a list of all files loaded for this shader. Everytime the shader is bound and hotswapping is enabled,
    // all shader files will be checked for modification and then the shader might be reloaded
    std::vector<std::string> files;
    // maps uniforms to the locations returned by gl. Every uniform has to be loaded once, then they will
    // be stored in the map for faster lookup.
    std::map<std::string, int16_t> uniforms;
    // maps samplers to the texture slots, i.e. a sampler2D in a shader has a uniform location, but that is not the
    // texture slot. When loading a sampler uniform, the wanted texture slot can be given as a parameter. This slot is
    // saved here so that we dont have to remember it
    std::map<std::string, int16_t> samplers;
};

struct HeVbo {
    uint32_t vboId = 0;
    uint32_t verticesCount = 0;
    uint8_t dimensions = 0;
    std::vector<float> data;
};

struct HeVao {
    uint32_t vaoId = 0;
    uint32_t verticesCount = 0;
    std::vector<HeVbo> vbos;
};

struct HeFbo {
    // the gl id of this fbo, used by the engine
    uint32_t fboId = 0;
    // an array of (possible) depth attachments to this fbo. These values may be set or just left at 0 if that attachment
    // was not created
    // 0. depth texture
    // 1. depth buffer
    uint32_t depthAttachments[2] = { 0 };
    
    /* only one of the two arrays should be used, therefore a fbo should only have either colour buffers or
     * colour textures */
    
    // a vector of (multisampled) colour buffers attached to this array.
    std::vector<uint32_t> colourBuffers;
    // a vector of colour textures attached to this array
    std::vector<uint32_t> colourTextures;
    
    // the amount of samples in this fbo (multisampling). Must be either 4, 8, 16. More samplers mean smoother
    // results but higher memory usage. Needs to be set before fbo creation
    uint8_t samples = 1;
    // flags of this fbo. More flags can be set, but only one of HE_FBO_FLAG_DEPTH_RENDER_BUFFER and
    // HE_FBO_FLAG_DEPTH_TEXTURE
    // should be set. Needs to be set before fbo creation
    HeFboFlags flags;
    // the size of this fbo (in pixels). Needs to be set before fbo creation
    hm::vec2i size;
};

struct HeTexture {
    // for every time this texture is requested from the asset pool, this reference count goes up.
    // If this texture is destroyed with heDestroyTexture, referenceCount will go down.
    // Only if the referenceCount is 0, the texture will actually be deleted
    uint32_t referenceCount = 0;
    // the opengl texture id
    uint32_t textureId = 0;
    // the internal format used for storing the pixels. Can be:
    // 0: RGBA (8bit / channel)
    // 1: RGBA (16bit / channel)
    HeColourFormat format = HE_COLOUR_FORMAT_NONE;
    // dimensions of the texture
    int32_t width = 0;
    int32_t height = 0;
    // the number of channels in this texture. Can be 3 (rgb) or 4 (rgba)
    int32_t channels = 0;
};


// --- Shaders

// loads a compute shader from given file
extern HE_API void heShaderLoadCompute(HeShaderProgram* program, const std::string& computeShader);
// loads a shader from given shader files
extern HE_API void heShaderLoadProgram(HeShaderProgram* program, const std::string& vertexShader, const std::string& fragmentShader);
// creates a new shader by loading it from given files. This should only be used once (first creation) as it will store
// important data for now. This should be used when the user loads shaders
extern HE_API void heShaderCreateProgram(HeShaderProgram* program, const std::string& vertexShader, const std::string& framentShader);
// binds given shader. If shader hotswapping is reloaded and this shader was modified it will also reload it
extern HE_API void heShaderBind(HeShaderProgram* program);
// unbinds the currently bound shader
extern HE_API void heShaderUnbind();
// deletes the given shader
extern HE_API void heShaderDestroy(HeShaderProgram* program);
// runs given compute shader. The number of groups in each dimension can be specified here, according to the texture this shader is operating on
extern HE_API void heShaderRunCompute(HeShaderProgram* program, const uint32_t groupsX, const uint32_t groupsY, const uint32_t groupsZ);
// reloads given shader
extern HE_API void heShaderReload(HeShaderProgram* program);
// checks if any of the shaders files were modified (and shader hotswapping is enabled), and if so it
// reloads the program
extern HE_API void heShaderCheckReload(HeShaderProgram* program);

// gets the location of given uniform in given shader. Once the uniform is loaded, the location is stored in program
// for faster lookups later. If the uniform wasnt found (spelling mistake or optimized away), -1 is returned
extern HE_API int heShaderGetUniformLocation(HeShaderProgram* program, const std::string& uniform);
// gets the location of given sampler in given shader. Once the sampler is loaded, the location is stored in program
// for faster lookups later. If the sampler wasnt found (spelling mistake or optimized away), -1 is returned.
// If the sampler does exist, it will be bound to given texture slot (glActiveTexture(GL_TEXTURE0 + requestedSlot))
extern HE_API int heShaderGetSamplerLocation(HeShaderProgram* program, const std::string& sampler, const uint8_t requestedSlot);

extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const void* data, const HeUniformDataType dataType);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const int value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const float value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const double value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const uint32_t value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::mat3f& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::mat4f& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec2f& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec3f& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::vec4f& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const hm::colour& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, const std::string& uniformName, const HeShaderData* data);



// --- Buffers

// creates a new vbo buffer from given data. Dimensions is the number of floats that build one vertex
// (i.e. 3 for a normal, 2 for a uv). This does not add the buffer to any vao. A vbo can be used in
// different vaos, though it is not recommended
extern HE_API void heVboCreate(HeVbo* vbo, const std::vector<float>& data);
// deletes the given vbo and sets its id to 0
extern HE_API void heVboDestroy(HeVbo* vbo);

// creates a new empty vao
extern HE_API void heVaoCreate(HeVao* vao);
// simply uploads the data stored in the vbo onto the currently bound vao. This will clear the data stored in the vbo
// attributeIndex is the index of this vbo in the vao
extern HE_API void heVaoAddVboData(HeVbo* vbo, const int8_t attributeIndex);
// adds an existing vbo to given vao. Both vao and vbo need to be created beforehand, and the vao needs
// to be bound before this call. If this is the first vbo for the given vao, the verticesCount
// of the vao is calculated
extern HE_API void heVaoAddVbo(HeVao* vao, HeVbo* vbo);
// adds new data to given vao. The vao needs to be created and bound before this.
extern HE_API void heVaoAddData(HeVao* vao, const std::vector<float>& data, const uint8_t dimensions);
// binds given vao
extern HE_API void heVaoBind(const HeVao* vao);
// unbinds the currently bound vao
extern HE_API void heVaoUnbind(const HeVao* vao);
// deletes given vao and all associated vbos
extern HE_API void heVaoDestroy(HeVao* vao);
// renders given vao. This assumes the vao to be in GL_TRIANGLES mode
extern HE_API void heVaoRender(const HeVao* vao);


// adds a new colour texture attachment to given fbo
extern HE_API void heFboCreateColourAttachment(HeFbo* fbo);
// adds a new (multisampled) colour buffer attachment to given fbo
extern HE_API void heFboCreateMultisampledColourAttachment(HeFbo* fbo);
// creates a new fbo. All parameters of the fbo must already be set (see parameter description). This will add
// a (multisampled, if samples is bigger than 1) colour attachment and either a depth texture or a depth buffer
// attachment to the fbo
extern HE_API void heFboCreate(HeFbo* fbo);
// binds given fbo and resizes the viewport
extern HE_API void heFboBind(HeFbo* fbo);
// unbinds the current fbo and restores the viewport to given windowSize
extern HE_API void heFboUnbind(const hm::vec2i& windowSize);
// unbinds the current fbo
extern HE_API void heFboUnbind();
// destroys given fbo
extern HE_API void heFboDestroy(HeFbo* fbo);
// resizes given fbo by destroying it and creating a new version with new size
extern HE_API void heFboResize(HeFbo* fbo, const hm::vec2i& newSize);
// draws the source fbo onto the target fbo. If the source should be drawn directly onto the screen, then a "fake" fbo
// must be created with an id of 0 and the size of the window
extern HE_API void heFboRender(HeFbo* sourceFbo, HeFbo* targetFbo);
// renders the source fbo directly onto the window with given size
extern HE_API void heFboRender(HeFbo* sourceFbo, const hm::vec2i& windowSize);


// --- Textures

// creates an empty texture. The details of the texture (width, height...) must already be set
extern inline HE_API void heTextureCreateEmpty(HeTexture* texture);
// loads a new texture from given file. If given file does not exist, an error is printed and no gl texture
// will be generated. If gammaCorrect is true, the texture will be saved as srgb, which is useful for
extern HE_API void heTextureLoadFromFile(HeTexture* texture, const std::string& fileName);
// creates an opengl texture from given information. Id will be set to the gl texture id created. This will
// also delete the buffer
extern HE_API void heTextureCreateFromBuffer(unsigned char* buffer, uint32_t* id, const int16_t width, const int16_t height, const int8_t channels,
                                             const HeColourFormat format);
// loads a new texture from given stream. If the stream is invalid, an error is printed and no gl texture
// will be generated
extern HE_API void heTextureLoadFromFile(HeTexture* texture, FILE* stream);
// binds given texture to given gl slot
extern HE_API void heTextureBind(const HeTexture* texture, const int8_t slot);
// binds given texture id to given gl slot
extern HE_API void heTextureBind(const uint32_t texture, const int8_t slot);
// binds a given texture to a shader for reading / writing, mostly used for compute shaders
// slot is the uniform slot this shader should be bound to
// if layer is -1, a new layered texture binding is established, else the given layer is used (layer >= 0)
// access is the permissions required for this image, can be one of the following:
//  0: read only
//  1: write only
//  2: read and write
extern HE_API void heImageTextureBindImage(const HeTexture* texture, const int8_t slot, const int8_t layer, const HeAccessType access);
// unbinds the currently bound texture from given gl slot
extern HE_API void heTextureUnbind(const int8_t slot);
// deletes given texture if its reference count is currently 1
extern HE_API void heTextureDestroy(HeTexture* texture);

// before calling any of the following functions, a texture must be bound

// pixelated filter
extern HE_API void heTextureFilterNearest();
// smooth filter
extern HE_API void heTextureFilterBilinear();
// uses mipmaps
extern HE_API void heTextureFilterTrilinear();
// anisotropic filtering
extern HE_API void heTextureFilterAnisotropic();

// clamps to the edge (border pixels will be stretched)
extern HE_API void heTextureClampEdge();
// clamps to a black border around the image
extern HE_API void heTextureClampBorder();
// tiles the image
extern HE_API void heTextureClampRepeat();


// --- Utils

// clears the buffers of current fbo to given colour. Type is the different buffers to be cleared
// type = 0: The colour buffer is cleared
// type = 1: The depth buffer is cleared
// type = 2: Both the colour and the depth colour are cleared
extern HE_API void heFrameClear(const hm::colour& colour, const int8_t type);
// sets the gl blend mode to given mode. Possible modes:
// -1 = disable gl blending
//  0 = normal blending (one minus source alpha)
//  1 = additive blending (add two colours)
//  2 = disable blending by completely overwriting previous colour with new one
extern HE_API void heBlendMode(const int8_t mode);
// applies a gl blend mode to only given colour attachment of a fbo. Same modes as above apply
extern HE_API void heBufferBlendMode(const int8_t attachmentIndex, const int8_t mode);
// en- or disables depth testing
extern HE_API void heEnableDepth(const b8 depth);
// sets the view port. lowerleft is the lower left corner of the new view port (in pixels, (0|0) is default
// size is the viewport size in pixels (width, height)
extern HE_API void heViewport(const hm::vec2i& lowerleft, const hm::vec2i& size);

#endif
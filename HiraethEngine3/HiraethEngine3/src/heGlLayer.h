#ifndef HE_GL_LAYER_H
#define HE_GL_LAYER_H

#include <unordered_map>
#include <vector>
#include <string>
#include "hm/hm.hpp"
#include "heTypes.h"

struct HeShaderData {
    HeDataType type;
    
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
    std::unordered_map<std::string, int16_t> uniforms;
    // maps samplers to the texture slots, i.e. a sampler2D in a shader has a uniform location, but that is not the
    // texture slot. When loading a sampler uniform, the wanted texture slot can be given as a parameter. This slot is
    // saved here so that we dont have to remember it
    std::unordered_map<std::string, int8_t> samplers;
	// whether this is a compute shader or a normal pipeline shader
    b8 computeShader = false;
	// the memory used by this shader, in bytes
	uint32_t memory = 0;
	
#ifdef HE_ENABLE_NAMES
    std::string name = "";
#endif
};

struct HeVbo {
    // the gl id
    uint32_t              vboId         = 0;
    // the amount of vertices (independant of dimensions) in this buffer
    uint32_t              verticesCount = 0;
    // the dimensions of this vbo (2 for vec2, 3 for vec3...)
    uint8_t               dimensions    = 0;
    // the usage of this vbo. Usually static (uploaded once, used many times)
    HeVboUsage            usage         = HE_VBO_USAGE_STATIC;
    // the type of this vbo. For vectors this should be float.
    // the only other accepted type is int
    HeDataType     type          = HE_DATA_TYPE_FLOAT;

	// the memory used by this texture, in bytes
	uint32_t memory = 0;
    
    // filled if the vbo was loaded from a different thread and type is FLOAT
    std::vector<float>    dataf;
    // filled if the vbo was loaded from a different thread and type is INT
    std::vector<int32_t>  datai;
    // filled if the vbo was loaded from a different thread and type is UNSIGNED INT
    std::vector<uint32_t> dataui;
};

struct HeVao {
    HeVaoType type = HE_VAO_TYPE_NONE;
    uint32_t vaoId = 0;
    uint32_t verticesCount = 0;
    std::vector<HeVbo> vbos;
    
#ifdef HE_ENABLE_NAMES
    std::string name = "";
#endif
};


struct HeFboAttachment {
    // texture or buffer?
    b8             texture = true;
    // if true this attachment is resized (relative to the fbo size change). If false this attachment does not get resized
    b8             resize  = true;
    // the number of samples for multisampling. Only works if this is a buffer
    uint8_t        samples = 1;
    // the gl id of the texture or buffer of this attachment
    uint32_t       id      = 0;
    // the colour format of this attachment
    HeColourFormat format  = HE_COLOUR_FORMAT_RGBA8;
    // the size of this attachment in pixels
    hm::vec2i      size;

	// the memory used by this texture, in bytes
	uint32_t memory = 0;
};

struct HeFbo {
    // the gl id of this fbo, used by the engine
    uint32_t fboId = 0;
    // the default size of this fbo
    hm::vec2i size;
    // set to true if a multisampled colour attachment or depth attachment was created for this fbo.
    b8 useMultisampling = false;
    // the depth attachment of this fbo
	HeFboAttachment depthAttachment;
    // a list of colour attachments on this fbo
    std::vector<HeFboAttachment> colourAttachments;
    
#ifdef HE_ENABLE_NAMES
    std::string name = "";
#endif
};

struct HeTexture {
    // for every time this texture is requested from the asset pool, this reference count goes up.
    // If this texture is destroyed with heDestroyTexture, referenceCount will go down.
    // Only if the referenceCount is 0, the texture will actually be deleted
	uint32_t referenceCount = 0;
    // the opengl texture id
	uint32_t textureId = 0;
    // the internal format used for storing the pixels. Can be:
    // 0: RGBA (8bit / channel) (unsigned chars)
    // 1: RGBA (16bit / channel) (floats)
    HeColourFormat format = HE_COLOUR_FORMAT_NONE;
	// the parameters of this texture (filtering and clamping)
	HeTextureParameter parameters = HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_REPEAT;
    // dimensions of the texture
	hm::vec2i size;
    // the number of channels in this texture. Can be 3 (rgb) or 4 (rgba)
    int32_t channels = 0;
	// if the FILTER_TRILINEAR flag is set, this is the number of mipmaps to create
    uint16_t mipMapCount = 1000;
    // whether this is a cube map or a normal 2d texture
	b8 cubeMap = false;
    
    // one of the following buffers is set if the texture was loaded from a different thread. The float buffer is only used if
    // this is an hdr texture, else the char buffer is set. This pointer is freed and set to nullptr when the texture is loaded
    // in the thread loader
	
    float*         bufferf = nullptr;
    unsigned char* bufferc = nullptr;

	// the memory used by this texture, in bytes
	uint32_t memory = 0;
	
#ifdef HE_ENABLE_NAMES
    std::string name = "";
#endif
};


// -- Shaders

// loads a compute shader from given file
extern HE_API void heShaderLoadCompute(HeShaderProgram* program, std::string const& computeShader);
// loads a vertex/fragment shader from given shader files. The code must be seperated into the different files for this to work.
// This will bind the shader.
extern HE_API void heShaderLoadProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& fragmentShader);
// loads a vertex/geometry/fragment shader program from given shader files. The code must be seperated into the different files
// for this to work. This will bind the shader
extern HE_API void heShaderLoadProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& geometryShader, std::string const& fragmentShader);
// loads a shader from given file. That file must containt all shader code, split up by definitions. The geometry stage can be added
// but doesnt have to be. The different shader sources are split by a # and then the name of the stage (i.e. #vertex at the beginning
// of the file, then #geometry and #fragment. This will bind the shader. When error checking, keep in mind that the file lines
// printed are relative to the shader source, not the file beginning
extern HE_API void heShaderLoadProgram(HeShaderProgram* program, std::string const& shaderFile);
// loads a compute shader from given file. This will set up the program once and should therefore be called by the user
extern HE_API void heShaderCreateCompute(HeShaderProgram* program, std::string const& shaderFile);
// creates a new shader by loading it from given files. This should only be used once (first creation) as it will store
// important data for now. This should be used when the user loads shaders
extern HE_API void heShaderCreateProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& framentShader);
// creates a new shader by loading it from given files. This should only be used once (first creation) as it will store
// important data for now. This should be used when the user loads shaders
extern HE_API void heShaderCreateProgram(HeShaderProgram* program, std::string const& vertexShader, std::string const& geometryShader, std::string const& framentShader);
// creates a new shader from given file. This should only be used once (first creation) as it will store
// important data for now. This should be used when the user loads shaders
extern HE_API void heShaderCreateProgram(HeShaderProgram* program, std::string const& file);
// binds given shader. If shader hotswapping is reloaded and this shader was modified it will also reload it
extern HE_API void heShaderBind(HeShaderProgram* program);
// unbinds the currently bound shader
extern HE_API void heShaderUnbind();
// deletes the given shader
extern HE_API void heShaderDestroy(HeShaderProgram* program);
// runs given compute shader. The number of groups in each dimension can be specified here, according to the texture this shader
// is operating on
extern HE_API void heShaderRunCompute(HeShaderProgram* program, uint32_t const groupsX, uint32_t const groupsY, uint32_t const groupsZ);
// reloads given shader
extern HE_API void heShaderReload(HeShaderProgram* program);
// checks if any of the shaders files were modified (and shader hotswapping is enabled), and if so it
// reloads the program
extern HE_API void heShaderCheckReload(HeShaderProgram* program);

// gets the location of given uniform in given shader. Once the uniform is loaded, the location is stored in program
// for faster lookups later. If the uniform wasnt found (spelling mistake or optimized away), -1 is returned
extern HE_API int heShaderGetUniformLocation(HeShaderProgram* program, std::string const& uniform);
// gets the location of given sampler in given shader. Once the sampler is loaded, the location is stored in program
// for faster lookups later. If the sampler wasnt found (spelling mistake or optimized away), -1 is returned.
// If the sampler does exist, it will be bound to given texture slot (glActiveTexture(GL_TEXTURE0 + requestedSlot))
extern HE_API int heShaderGetSamplerLocation(HeShaderProgram* program, std::string const& sampler, uint8_t const requestedSlot);

// sets all samplers (loaded in the shader) to 0
extern HE_API void heShaderClearSamplers(HeShaderProgram const* program);

extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, void const* data, HeDataType const dataType);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, const float value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, const double value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, int32_t const value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, uint32_t const value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::mat3f const& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::mat4f const& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::vec2f const& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::vec3f const& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::vec4f const& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, hm::colour const& value);
extern HE_API void heShaderLoadUniform(HeShaderProgram* program, std::string const& uniformName, HeShaderData const* data);


// -- Buffers

// creates a new vbo buffer from given data. Dimensions is the number of floats that build one vertex
// (i.e. 3 for a normal, 2 for a uv). This does not add the buffer to any vao. A vbo can be used in
// different vaos, though it is not recommended
extern HE_API void heVboCreate(HeVbo* vbo, std::vector<float> const& data, uint8_t const dimensions, HeVboUsage const usage = HE_VBO_USAGE_STATIC);
// creates a new vbo buffer from given data. See comments above for more info. This buffer exists for ints only. This should only be used if the buffer
// explicitely uses ints
extern HE_API void heVboCreateInt(HeVbo* vbo, std::vector<int32_t> const& data, uint8_t const dimensions, HeVboUsage const usage = HE_VBO_USAGE_STATIC);
// creates a new vbo buffer from given data. See comments above for more info. This buffer exists for ints only. This should only be used if the buffer
// explicitely uses unsigned ints
extern HE_API void heVboCreateUint(HeVbo* vbo, std::vector<uint32_t> const& data, uint8_t const dimensions, HeVboUsage const usage = HE_VBO_USAGE_STATIC);
// allocates a new empty vbo of given size.
extern HE_API void heVboAllocate(HeVbo* vbo, uint32_t const size, uint8_t const dimensions, HeVboUsage const usage = HE_VBO_USAGE_STATIC, HeDataType const type = HE_DATA_TYPE_FLOAT);
// deletes the given vbo and sets its id to 0
extern HE_API void heVboDestroy(HeVbo* vbo);
// creates a new empty vao
extern HE_API void heVaoCreate(HeVao* vao, HeVaoType const type = HE_VAO_TYPE_TRIANGLES);
// simply uploads the data stored in the vbo onto the currently bound vao. This will clear the data stored in the vbo
// attributeIndex is the index of this vbo in the vao. The vbo must have already set all attributes (data buffer, dimensions, usage)
extern HE_API void heVaoAddVboData(HeVbo* vbo, int8_t const attributeIndex);
// adds an existing vbo to given vao. Both vao and vbo need to be created beforehand, and the vao needs
// to be bound before this call. If this is the first vbo for the given vao, the verticesCount
// of the vao is calculated
extern HE_API void heVaoAddVbo(HeVao* vao, HeVbo* vbo);
// adds new data to given vao. The vao needs to be created and bound before this.
extern HE_API void heVaoAddData(HeVao* vao, std::vector<float> const& data, uint8_t const dimensions, HeVboUsage const usage = HE_VBO_USAGE_STATIC);
// adds new data to given vao. The vao needs to be created and bound before this
extern HE_API void heVaoAddDataInt(HeVao* vao, std::vector<int32_t> const& data, uint8_t const dimensions, HeVboUsage const usage = HE_VBO_USAGE_STATIC);
// adds new data to given vao. The vao needs to be created and bound before this
extern HE_API void heVaoAddDataUint(HeVao* vao, std::vector<uint32_t> const& data, uint8_t const dimensions, HeVboUsage const usage = HE_VBO_USAGE_STATIC);
// updates the data of given vbo (vboIndex). Make sure the vao is bound before this call.
extern HE_API void heVaoUpdateData(HeVao* vao, std::vector<float> const& data, uint8_t const vboIndex);
// updates the data of given vbo (vboIndex). Make sure the vao is bound before this call.
extern HE_API void heVaoUpdateDataInt(HeVao* vao, std::vector<int32_t> const& data, uint8_t const vboIndex);
// updates the data of given vbo (vboIndex). Make sure the vao is bound before this call.
extern HE_API void heVaoUpdateDataUint(HeVao* vao, std::vector<uint32_t> const& data, uint8_t const vboIndex);
// binds given vao
extern HE_API void heVaoBind(HeVao const* vao);
// unbinds the currently bound vao
extern HE_API void heVaoUnbind(HeVao const* vao);
// deletes given vao and all associated vbos
extern HE_API void heVaoDestroy(HeVao* vao);
// renders given vao. This assumes the vao to be in GL_TRIANGLES mode
extern HE_API void heVaoRender(HeVao const* vao);

// creates a new fbo. All parameters of the fbo must already be set (see parameter description). This will add
// a (multisampled, if samples is bigger than 1) colour attachment and either a depth texture or a depth buffer
// attachment to the fbo
extern HE_API void heFboCreate(HeFbo* fbo);
// adds a new colour texture attachment to given fbo. If size is -1, the
extern HE_API void heFboCreateColourTextureAttachment(HeFbo* fbo, HeColourFormat const format = HE_COLOUR_FORMAT_RGBA8, hm::vec2i const& size = hm::vec2i(-1));
// adds a new (multisampled) colour buffer attachment to given fbo
extern HE_API void heFboCreateColourBufferAttachment(HeFbo* fbo, HeColourFormat const format = HE_COLOUR_FORMAT_RGBA8, uint8_t const samples = 1, hm::vec2i const& size = hm::vec2i(-1));
// creates a new depth buffer attachment for given fbo. The buffer will be the size of the fbo. The fbo must already be bound
// before this call.
extern HE_API void heFboCreateDepthBufferAttachment(HeFbo* fbo, uint8_t const samples = 1);
// creates a new depth texture attachment for given fbo. This texture will be the size of the fbo. The fbo must already be bound
// before this call
extern HE_API void heFboCreateDepthTextureAttachment(HeFbo* fbo);
// binds given fbo and resizes the viewport
extern HE_API void heFboBind(HeFbo* fbo);
// unbinds the current fbo and restores the viewport to given windowSize
extern HE_API void heFboUnbind(hm::vec2i const& windowSize);
// unbinds the current fbo
extern HE_API void heFboUnbind();
// destroys given fbo
extern HE_API void heFboDestroy(HeFbo* fbo);
// resizes given fbo by destroying it and creating a new version with new size
extern HE_API void heFboResize(HeFbo* fbo, hm::vec2i const& newSize);
// draws the source fbo onto the target fbo. If the source should be drawn directly onto the screen, then a "fake" fbo
// must be created with an id of 0 and the size of the window
extern HE_API void heFboRender(HeFbo* sourceFbo, HeFbo* targetFbo);
// renders the source fbo directly onto the window with given size
extern HE_API void heFboRender(HeFbo* sourceFbo, hm::vec2i const& windowSize);
// checks if the currently bound fbo was correctly set up.
extern HE_API void heFboValidate();


// -- Textures

// creates a new empty cube map texture. The details of the texture (width, height, format, channels) must already be set
extern HE_API void heTextureCreateEmptyCubeMap(HeTexture* texture);
// creates an empty texture. The details of the texture (width, height, format, channels) must already be set
extern inline HE_API void heTextureCreateEmpty(HeTexture* texture);
// loads a new texture from given file. If given file does not exist, an error is printed and no gl texture
// will be generated.
extern HE_API void heTextureLoadFromFile(HeTexture* texture, std::string const& fileName);
// loads a new hdr texture from given file. If given file does not exist, an error is printed and no gl texture will be
// generated. This must be a .hdr file to work correctly
extern HE_API void heTextureLoadHdrFromFile(HeTexture* texture, std::string const& fileName);
// loads a new texture from given stream. If the stream is invalid, an error is printed and no gl texture
// will be generated
extern HE_API void heTextureLoadFromFile(HeTexture* texture, FILE* stream);
// creates an opengl texture from given information. Id will be set to the gl texture id created. This will
// also delete the buffer
//extern HE_API void heTextureCreateFromBuffer(unsigned char* buffer, uint32_t* id, int16_t const width, int16_t const height, int8_t const channels, HeColourFormat const format);
// creates the opengl texture for given texture. This HeTexture must already have either the char or the float buffer and all its
// information (width, height, format, channels) set. This will free the buffer used and (if enabled) set the gl objects name
extern HE_API void heTextureCreateFromBuffer(HeTexture* texture);
// binds given texture to given gl slot
extern HE_API void heTextureBind(HeTexture const* texture, int8_t const slot);
// binds given texture id to given gl slot
extern HE_API void heTextureBind(uint32_t const texture, int8_t const slot, b8 const cubeMap = false);
// binds a given texture to a shader for reading / writing, mostly used for compute shaders
// slot is the uniform slot this shader should be bound to
// level is the mipmap level which to bind
// if layer is -1, a new layered texture binding is established, else the given layer is used (layer >= 0)
// access is the permissions required for this image, can be one of the following:
//  0: read only
//  1: write only
//  2: read and write
extern HE_API void heImageTextureBind(HeTexture const* texture, int8_t const slot, int8_t const level, int8_t const layer, HeAccessType const access);
// unbinds the currently bound texture from given gl slot
extern HE_API void heTextureUnbind(int8_t const slot, b8 const cubeMap = false);
// deletes given texture if its reference count is currently 1
extern HE_API void heTextureDestroy(HeTexture* texture);

// before calling any of the following functions, a texture must be bound

// pixelated filter
extern HE_API void heTextureFilterLinear(HeTexture const* texture);
// smooth filter
extern HE_API void heTextureFilterBilinear(HeTexture const* texture);
// uses mipmaps (creates a max of count mipmaps for the current texture)
extern HE_API void heTextureFilterTrilinear(HeTexture const* texture, uint16_t const count = 1000);
// anisotropic filtering
extern HE_API void heTextureFilterAnisotropic(HeTexture const* texture);

// clamps to the edge (border pixels will be stretched)
extern HE_API void heTextureClampEdge(HeTexture const* texture);
// clamps to a black border around the image
extern HE_API void heTextureClampBorder(HeTexture const* texture);
// tiles the image
extern HE_API void heTextureClampRepeat(HeTexture const* texture);


// -- Utils

// clears the buffers of current fbo to given colour. Type is the different buffers to be cleared
extern HE_API void heFrameClear(hm::colour const& colour, HeFrameBufferBits const type);
// sets the gl blend mode to given mode. Possible modes:
// -1 = disable gl blending
//  0 = normal blending (one minus source alpha)
//  1 = additive blending (add two colours)
//  2 = disable blending by completely overwriting previous colour with new one
extern HE_API void heBlendMode(int8_t const mode);
// applies a gl blend mode to only given colour attachment of a fbo. Same modes as above apply
extern HE_API void heBufferBlendMode(int8_t const attachmentIndex, int8_t const mode);
// en- or disables depth testing
extern HE_API void heDepthEnable(b8 const depth);
// sets the depth function. Only works when depth testing is enabled
extern HE_API void heDepthFunc(HeFragmentTestFunction const function);
// enables back face culling (good for performance)
extern HE_API void heCullEnable(b8 const culling);
// enables or disables the stencil buffer
extern HE_API void heStencilEnable(b8 const enable);
// sets the stencil mask
extern HE_API void heStencilMask(uint32_t const mask);
// describes how stencil testing works. Possible values for now are
extern HE_API void heStencilFunc(HeFragmentTestFunction const function, uint32_t const threshold, uint32_t const mask);
// sets the view port. lowerleft is the lower left corner of the new view port (in pixels, (0|0) is default
// size is the viewport size in pixels (width, height)
extern HE_API void heViewport(hm::vec2i const& lowerleft, hm::vec2i const& size);
// returns the memory usage as queried from opengl. CURRENTLY DOESNT WORK
extern HE_API int32_t heMemoryGetUsage();
// prints information about the graphics card and renderer version this engine is running on
extern HE_API void heGlPrintInfo();


// -- Errors (only functional if HE_ENABLE_ERROR_CHECKING is defined)

// clears all set error flags
extern HE_API void heGlErrorClear();
// saves all errors that occured during the last frame in a vector that can be read from using heGlErrorGetLast()
extern HE_API void heGlErrorSaveAll();
// returns the oldest error saved (newest after last error clear). If this returns 0, no more gl error was found. Loop through this
// to get all errors that happened during the last frame.
extern HE_API uint32_t heGlErrorGet();
// checks if an error has occured just now (since the last check)
extern HE_API uint32_t heGlErrorCheck();

#endif

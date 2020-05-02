#ifndef HE_ASSETS_H
#define HE_ASSETS_H

#include "heGlLayer.h"

struct HeMaterial {
    // a type id, dependant of the shader. All materials with the same shader have the same type.
    uint32_t type = 0;
    // pointer to a shader in the asset pool
    HeShaderProgram* shader = nullptr;
    // name of the sampler (without the t_ prefix) and a pointer to the asset pool
    std::unordered_map<std::string, HeTexture*> textures;
    // name of the uniform and some data
    std::unordered_map<std::string, HeShaderData> uniforms;
};

struct HeFont {
    struct Character {
        uint32_t id;             // ascii id
		hm::vec4f uv;            // in texture space
		hm::vec2<uint32_t> size;  // in pixel space
		hm::vec2<int32_t> offset; // in pixel space
		uint8_t xadvance;        // in pixel space
	};

	std::unordered_map<uint32_t, Character> characters;
	hm::vec4<uint8_t> padding;
	uint8_t size, lineHeight, baseLine;
	uint8_t spaceWidth;
	
	HeTexture* atlas = nullptr;
	
#ifdef HE_ENABLE_NAMES
	std::string name;
#endif
};

// maps the name of the mesh (usually a file) to its vao
typedef std::unordered_map<std::string, HeVao> HeMeshPool;
// maps the name of the file to the texture
typedef std::unordered_map<std::string, HeTexture> HeTexturePool;
// maps the name of a shader to the program
typedef std::unordered_map<std::string, HeShaderProgram> HeShaderPool;
// maps any type of materials
typedef std::unordered_map<std::string, HeMaterial> HeMaterialPool;
// maps fonts to their (file) name
typedef std::unordered_map<std::string, HeFont> HeFontPool;

// maps a pointer to a texture in the texture pool to the content of the file it was loaded from
// this is only used if the texture was loaded from a side-thread (not the thread the window was created in)
typedef std::vector<HeTexture*> HeTextureRequests;
// a list of pointers in the mesh pool of vaos that need to be loaded. The vao can be set up normally (via
// heAddVaoData...), the data will be stored in the HeVbos of the vao. Once the thread loader loads this vao,
// the data will be uploaded to gl and deleted
typedef std::vector<HeVao*> HeVaoRequests;
// maps different types of objects to their memory usage in bytes
typedef std::unordered_map<HeMemoryType, uint64_t> HeMemoryTracker;

// pools for the different assets
struct HeAssetPool {
	HeMeshPool     meshPool;
    HeShaderPool   shaderPool;
    HeTexturePool  texturePool;
    HeMaterialPool materialPool;
    HeFontPool     fontPool;
};

struct HeThreadLoader {
    HeTextureRequests textures;
    HeVaoRequests vaos;
    
    b8 updateRequested = false;
};

extern HeAssetPool heAssetPool;
extern HeThreadLoader heThreadLoader;
extern HeMemoryTracker heMemoryTracker;


// -- Materials

// creates a new pbr material if it doesnt already exists from given textures. This material will be stored in
// the material pool.
// If a material with given name was already created before, that material will be returned
extern HE_API HeMaterial* heMaterialCreatePbr(std::string const& name, HeTexture* diffuseTexture, HeTexture* normalTexture, HeTexture* armTexture);
// returns the type id for given shader. This is used for grouping materials by their shader.
extern HE_API uint32_t heMaterialGetType(std::string const& shaderName);


// -- fonts

// tries to load a font with given name.
extern HE_API void heFontLoad(HeFont* font, std::string const& name);
// returns true if the given font can display the given ascii code
extern HE_API inline b8 heFontHasCharacter(HeFont const* font, uint32_t const asciiCode);
// returns the size of given ascii character in pixels or a zero vector if that character is not in the given font
extern HE_API inline hm::vec2i heFontGetCharacterSize(HeFont const* font, char const asciiCode, uint32_t const size);
// returns the width of given string in pixels with this font, with given size applied (in pixels)
extern HE_API uint32_t heFontGetStringWidthInPixels(HeFont const* font, std::string const& string, uint32_t const size);

// -- Assets

// checks if given mesh was already loaded and returns it if so. Else this mesh will be loaded now from given file
extern HE_API HeVao* heAssetPoolGetMesh(std::string const& file);
// checks if given texture was already loaded and returns it if so. Else this texture will be loaded now from
// given file
extern HE_API HeTexture* heAssetPoolGetTexture(std::string const& file, HeTextureParameter const parameters = HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_REPEAT);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded from given file
// (res/shaders/[name].glsl)
extern HE_API HeShaderProgram* heAssetPoolGetShader(std::string const& name);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded from the two
// given files.
// The file names must be the full relative path (res/shaders/...) including the file ending
extern HE_API HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& fShader);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded from the three given files
// (vertex, geometry and fragment). The file names must be the full relative path (res/shaders/...) including the file ending
extern HE_API HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& gShader, std::string const& fShader);
// returns the material with given name from the material pool or a pointer to an empty material if it wasnt created before
extern HE_API HeMaterial* heAssetPoolGetMaterial(const std::string& name);
// returns the material with given name if it was created before or a new one (with an assigned id) if no material with that name
// could be found
extern HE_API HeMaterial* heAssetPoolGetNewMaterial(std::string const& name);
// returns a font with given name. The font file and its atlas must be places in res/fonts/. This name is without
// the parent folder and without file extensions.
extern HE_API HeFont* heAssetPoolGetFont(std::string const& name);


// -- memory tracker

// returns the amount of bytes allocated per pixel with the given format. This will return the amount of channels
// times the amount of bytes per channel.
extern HE_API inline uint8_t heMemoryGetBytesPerPixel(HeColourFormat const format);
// rounds the usage to the next bigger page size (power of 2) since opengl will occupy that much space for textures...
extern HE_API uint32_t heMemoryRoundUsage(uint32_t const usage);


// -- ThreadLoader

// creates a request for loading given texture into the gl context. This is called from a non-context thread.
// texture should point to a texture in the texture pool, with either the char or float buffer set. This will simply put the
// texture in the thread loader and request an update for the heThreadLoader
extern HE_API void heThreadLoaderRequestTexture(HeTexture* texture);
// creates a new request for given vao. If this is called from the main thread (with a valid gl context), this function
// does nothing. Else a new request will be created and the requestUpdate of the thread loader is set to true. All
// data should already uploaded to the vao at this point
extern HE_API void heThreadLoaderRequestVao(HeVao* vao);
// loads all resources in the thread loader into gl and clears the loader afterwards. This must be called from the
// thread the window was created in (the main thread)
extern HE_API void heThreadLoaderUpdate();

#endif

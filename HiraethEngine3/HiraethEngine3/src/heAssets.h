#ifndef HE_ASSETS_H
#define HE_ASSETS_H

#include "heGlLayer.h"
#include <fstream>

// This struct is used for reading plain ascii files.
struct HeTextFile {
    std::string   name;     // without extension
    std::string   fullPath; // full (relative) path of this file, used to open the file
    std::string   fullName; // with extension
    uint32_t      lineNumber        = 0;
    uint32_t      maxBufferSize     = 0;
    uint32_t      currentBufferSize = 0;
    b8 skipEmptyLines               = true;
    
    std::ifstream stream;
    b8            open         = false;
    char*         buffer       = nullptr;
    uint32_t      bufferOffset = 0;     
    uint32_t      version      = 0;
};

struct HeMaterial {
    // a type id, dependant of the shader. All materials with the same shader have the same type.
    uint32_t type = 0;
    // pointer to a shader in the asset pool
    HeShaderProgram* shader = nullptr;
    // name of the sampler (without the t_ prefix) and a pointer to the asset pool
    std::unordered_map<std::string, HeTexture*> textures;
    // name of the uniform and some data
    std::unordered_map<std::string, HeShaderData> uniforms;

    hm::colour emission = hm::colour(0);
};

struct HeFont {
    struct Character {
        int32_t id = 0;             // ascii id
        hm::vec4f uv;            // in texture space
        hm::vec<2, uint32_t> size;  // in pixel space
        hm::vec<2, int32_t> offset; // in pixel space
        uint8_t xadvance;         // in pixel space
    };

    //  std::unordered_map<uint32_t, Character> characters;
    std::unordered_map<int32_t, Character> characters;
    hm::vec<4, uint8_t> padding;
    uint8_t size = 0, lineHeight = 0, baseLine = 0, spaceWidth = 0;
    
    HeTexture* atlas = nullptr;
    
#ifdef HE_ENABLE_NAMES
    std::string name;
#endif
};

struct HeScaledFont {
    HeFont const* font  = nullptr;
    uint8_t       size  = 0; // the size of this scaled font in pixels
    float         scale = 1.f; // relative scale from the native font, calculated by this size / the font size 
    
    // all of these are in pixels, scaled up or down from the original values in font
    
    float lineHeight = 0.f, baseLine = 0.f, spaceWidth = 0.f; 
    hm::vec4f padding;  
};

struct HeSpriteAtlas {
    HeTexture* texture = nullptr;
    uint32_t   rows    = 0;
    uint32_t   columns = 0;
    uint32_t   count   = 0;
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
typedef std::unordered_map<std::string, HeSpriteAtlas> HeSpriteAtlasPool;

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
    HeFontPool        fontPool;
    HeMeshPool        meshPool;
    HeShaderPool      shaderPool;
    HeTexturePool     texturePool;
    HeMaterialPool    materialPool;
    HeSpriteAtlasPool spriteAtlasPool;
};

struct HeThreadLoader {
    HeTextureRequests textures;
    HeVaoRequests vaos;
    
    b8 updateRequested = false;
};

extern HeAssetPool heAssetPool;
extern HeThreadLoader heThreadLoader;
extern HeMemoryTracker heMemoryTracker;


// -- Text files

// tries to open given file. Sets the open flag of the file to true on success. If the file could not be found,
// an error message is printed and the open flag is set to false. If bufferSize is greater than 0, this file
// is read using a buffer rather than std::getline
// This will also try to read the files version number if loadVersionTag is set to true (default)
extern HE_API void heTextFileOpen(HeTextFile* file, std::string const& path, uint32_t const bufferSize, b8 const loadVersionTag = true);
// closes the file and sets all data to 0
extern HE_API void heTextFileClose(HeTextFile* file);
// gets the next line from the given file and increases the line number
extern HE_API b8 heTextFileGetLine(HeTextFile* file, std::string* result);
// returns the next char from the stream
extern HE_API b8 heTextFileGetChar(HeTextFile* file, char* result);
// parses a float of the following format by reading characters from the given stream (and stores the result in
// the float pointer): +ffff.fff (or -ffff.fff). This function returns true if the int could be parsed or false
// if an error occurs (reached end of file or new line)
extern HE_API b8 heTextFileGetFloat(HeTextFile* file, float* result);
// parses an int of the following format by reading characters from the given stream (and stores the result in the
// int pointer): +iiii (or -iiii). The int can be of any size (8, 16, 32 or 64 bit). This function returns true if
// the int could be parsed or false if an error occurs (reached end of file or new line)
template<typename T>
extern HE_API b8 heTextFileGetInt(HeTextFile* file, T* result);
// parses count amount of floats directly written back to back from the given stream. This simply calls the
// heTextFileGetFloat function count times
extern HE_API b8 heTextFileGetFloats(HeTextFile* file, uint8_t const count, void* ptr);
// parses count amount of ints directly written back to back from the given stream. This simply calls the
// heTextFileGetInt function count times
template<typename T>
extern HE_API b8 heTextFileGetInts(HeTextFile* file, uint8_t const count, void* ptr);
// returns the whole content of the file as string
extern HE_API b8 heTextFileGetContent(HeTextFile* file, std::string* result);
// returns the next char that wasnt read yet
extern HE_API char heTextFilePeek(HeTextFile* file);
// skips behind the next new-line char
extern HE_API void heTextFileSkipLine(HeTextFile* file);


// -- Sprite Atlas

// loads a texture from given file and stores the information in the atlas 
extern HE_API void heSpriteAtlasLoad(HeSpriteAtlas* atlas, std::string const& textureFile, uint32_t const rows, uint32_t const columns, uint32_t const totalCount);
// returns the uv coordinates (xy) and width and height (zw) for that index in the sprite atlas, in the range of
// 0 to 1
extern HE_API hm::vec4f heSpriteAtlasGetUvs(HeSpriteAtlas* atlas, uint32_t const index);

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
// creates a scaled version of the font with given size in pixels.
extern HE_API void heFontCreateScaled(HeFont const* font, HeScaledFont* scaled, uint32_t const size);
// returns true if the given font can display the given ascii code
extern HE_API inline b8 heFontHasCharacter(HeFont const* font, int32_t const asciiCode);
// returns true if the given font can display the given ascii code
extern HE_API inline b8 heScaledFontHasCharacter(HeScaledFont const* font, int32_t const asciiCode);
// returns the size of given ascii character in pixels or a zero vector if that character is not in the given font
extern HE_API inline hm::vec2i heFontGetCharacterSize(HeFont const* font, int32_t const asciiCode);
// returns the size of given ascii character in pixels or a zero vector if that character is not in the given font
extern HE_API inline hm::vec2f heScaledFontGetCharacterSize(HeScaledFont const* font, int32_t const asciiCode);
// returns the width of given string in pixels with this font, with given size applied (in pixels)
extern HE_API int32_t heFontGetStringWidthInPixels(HeFont const* font, std::string const& string);
// returns the width of given string in pixels with this font, with given size applied (in pixels)
extern HE_API float heScaledFontGetStringWidthInPixels(HeScaledFont const* font, std::string const& string);


// -- Assets

// checks if given mesh was already loaded and returns it if so. Else this mesh will be loaded now from given file
extern HE_API HeVao* heAssetPoolGetMesh(std::string const& file);
// checks if given texture was already loaded and returns it if so. Else this texture will be loaded now from
// given file
extern HE_API HeTexture* heAssetPoolGetImageTexture(std::string const& file, HeTextureParameter const parameters = HE_TEXTURE_FILTER_BILINEAR | HE_TEXTURE_CLAMP_REPEAT);
// checks if given texture was already loaded and returns it if so. Else this texture will be loaded now from
// given compressed file
extern HE_API HeTexture* heAssetPoolGetCompressedTexture(std::string const& file, HeTextureParameter const parameters = HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_CLAMP_REPEAT);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded from given file
// (res/shaders/[name].glsl)
extern HE_API HeShaderProgram* heAssetPoolGetShader(std::string const& name);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded from the two
// given files.
// The file names must be the full relative path (res/shaders/...) including the file ending
extern HE_API HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& fShader);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded from the three
// given files (vertex, geometry and fragment). The file names must be the full relative path (res/shaders/...)
// including the file ending
extern HE_API HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& gShader, std::string const& fShader);
// returns the material with given name from the material pool or a pointer to an empty material if it wasnt
// created before
extern HE_API HeMaterial* heAssetPoolGetMaterial(const std::string& name);
// returns the material with given name if it was created before or a new one (with an assigned id) if no material
// with that name could be found
extern HE_API HeMaterial* heAssetPoolGetNewMaterial(std::string const& name);
// returns a font with given name. The font file and its atlas must be places in res/fonts/. This name is without
// the parent folder and without file extensions.
extern HE_API HeFont* heAssetPoolGetFont(std::string const& name);
// returns a sprite atlas with given name. If a sprite atlas with that name wasnt already loaded, the texture with
// given name will be requested from the asset pool and the data will be set 
extern HE_API HeSpriteAtlas* heAssetPoolGetSpriteAtlas(std::string const& name, uint32_t const rows, uint32_t const columns, uint32_t const totalCount);
// returns a sprite atlas with given name. If a sprite atlas with that name wasnt already loaded with the function above, nullptr is returned because we need data to be set 
extern HE_API HeSpriteAtlas* heAssetPoolGetSpriteAtlas(std::string const& name);

// -- ThreadLoader

// creates a request for loading given texture into the gl context. This is called from a non-context thread.
// texture should point to a texture in the texture pool, with either the char or float buffer set. This will
// simply put the texture in the thread loader and request an update for the heThreadLoader
extern HE_API void heThreadLoaderRequestTexture(HeTexture* texture);
// creates a new request for given vao. If this is called from the main thread (with a valid gl context),
// this function does nothing. Else a new request will be created and the requestUpdate of the thread loader is
// set to true. All data should already uploaded to the vao at this point
extern HE_API void heThreadLoaderRequestVao(HeVao* vao);
// loads all resources in the thread loader into gl and clears the loader afterwards. This must be called from the
// thread the window was created in (the main thread)
extern HE_API void heThreadLoaderUpdate();

#endif

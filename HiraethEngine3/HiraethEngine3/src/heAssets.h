#pragma once
#include "heGlLayer.h"

struct HeMaterial {
    // pointer to a shader in the asset pool
    HeShaderProgram* shader = nullptr;
    // name of the sampler (without the t_ prefix) and a pointer to the asset pool
    std::map<std::string, HeTexture*> textures;
    // name of the uniform and some data
    std::map<std::string, HeShaderData> uniforms;
};

struct HeThreadLoaderVao {
    std::vector<unsigned int> dimensions;
    std::vector<std::vector<float>> data;
};

// maps the name of the mesh (usually a file) to its vao
typedef std::map<std::string, HeVao> HeMeshPool;
// maps the name of the file to the texture
typedef std::map<std::string, HeTexture> HeTexturePool;
// maps the name of a shader to the program
typedef std::map<std::string, HeShaderProgram> HeShaderPool;
// maps any type of materials
typedef std::map<std::string, HeMaterial> HeMaterialPool;
// maps a pointer to a texture in the texture pool to the content of the file it was loaded from
// this is only used if the texture was loaded from a side-thread (not the thread the window was created in)
typedef std::map<HeTexture*, unsigned char*> HeTextureRequests;
// a list of pointers in the mesh pool of vaos that need to be loaded. The vao can be set up normally (via
// heAddVaoData...), the data will be stored in the HeVbos of the vao. Once the thread loader loads this vao,
// the data will be uploaded to gl and deleted
//typedef std::map<HeVao*, HeThreadLoaderVao> HeVaoRequests;
typedef std::vector<HeVao*> HeVaoRequests;

struct HeAssetPool {
    HeMeshPool meshPool;
    HeShaderPool shaderPool;
    HeTexturePool texturePool;
    HeMaterialPool materialPool;
};

struct HeThreadLoader {
    HeTextureRequests textures;
    HeVaoRequests vaos;
    
    bool updateRequested = false;
};

extern HeAssetPool heAssetPool;
extern HeThreadLoader heThreadLoader;

// --- Materials
// creates a new pbr material if it doesnt already exists from given textures. This material will be stored in 
// the material pool.
// If a material with given name was already created before, that material will be returned
extern HE_API HeMaterial* heCreatePbrMaterial(const std::string& name, HeTexture* diffuseTexture, HeTexture* normalTexture, HeTexture* armTexture);


// --- Assets

// checks if given mesh was already loaded and returns it if so. Else this mesh will be loaded now from given file
extern HE_API HeVao* heGetMesh(const std::string& file);
// checks if given texture was already loaded and returns it if so. Else this texture will be loaded now from 
// given file
extern HE_API HeTexture* heGetTexture(const std::string& file);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded, the filenames 
// will be expected
// to be name + "_v" / "_f" depending on the type and the files should be in the res/shaders/ folder
extern HE_API HeShaderProgram* heGetShader(const std::string& name);
// checks if given shader was already loaded and returns it if so. Else this shader will be loaded from the two 
// given files.
// The file names must be the full relative path (res/shaders/...) including the file ending
extern HE_API HeShaderProgram* heGetShader(const std::string& name, const std::string& vShader, const std::string& fShader);
// returns the material with given name from the material pool or nullptr if no material with that name was found
extern HE_API HeMaterial* heGetMaterial(const std::string& name);


// --- ThreadLoader

// creates a request for loading given texture into the gl context. This is called from a non-context thread. 
// texture should point to
// a texture in the texture pool, buffer is the texture buffer read from a file. This will simply put the entry in 
// the thread loader
// and request an update for the heThreadLoader
extern HE_API void heRequestTexture(HeTexture* texture, unsigned char* buffer);
// creates a new request for given vao. If this is called from the main thread (with a valid gl context), this function
// does nothing. Else a new request will be created and the requestUpdate of the thread loader is set to true. All
// data should already uploaded to the vao at this point
extern HE_API void heRequestVao(HeVao* vao);
// loads all resources in the thread loader into gl and clears the loader afterwards. This must be called from the 
// thread the window was created in (the main thread)
extern HE_API void heUpdateThreadLoader();
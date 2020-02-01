#pragma once
#include "heGlLayer.h"


// maps the name of the mesh (usually a file) to its vao
typedef std::map<std::string, HeVao> HeMeshPool;
// maps the name of the file to the texture
typedef std::map<std::string, HeTexture> HeTexturePool;
// maps the name of a shader to the program
typedef std::map<std::string, HeShaderProgram> HeShaderPool;
// maps a pointer to a texture in the texture pool to the content of the file it was loaded from
// this is only used if the texture was loaded from a side-thread (not the thread the window was created in)
typedef std::map<HeTexture*, unsigned char*> HeTextureRequests;

struct HeAssetPool {
	HeMeshPool meshPool;
	HeShaderPool shaderPool;
	HeTexturePool texturePool;
};

struct HeThreadLoader {
	HeTextureRequests textures;

	bool updateRequested = false;
};

extern HeAssetPool heAssetPool;
extern HeThreadLoader heThreadLoader;


// --- Assets

// checks if given mesh was already loaded and returns it if so. Else this mesh will be loaded now from given file
extern HE_API HeVao* heGetMesh(const std::string& file);
// checks if given texture was already loaded and returns it if so. Else this texture will be loaded now from given file
extern HE_API HeTexture* heGetTexture(const std::string& file);
// checks if given shader was already loaded and return it if so. Else this shader will be loaded, the filenames will be expected
// to be name + "_v" / "_f" depending on the type and the files should be in the res/shaders/ folder
extern HE_API HeShaderProgram* heGetShader(const std::string& name);


// --- ThreadLoader

// creates a request for loading given texture into the gl context. This is called from a non-context thread. texture should point to
// a texture in the texture pool, buffer is the texture buffer read from a file. This will simply put the entry in the thread loader
// and request an update for the heThreadLoader
extern HE_API void heRequestTexture(HeTexture* texture, unsigned char* buffer);
// loads all resources in the thread loader into gl and clears the loader afterwards. This must be called from the 
// thread the window was created in (the main thread)
extern HE_API void heUpdateThreadLoader();
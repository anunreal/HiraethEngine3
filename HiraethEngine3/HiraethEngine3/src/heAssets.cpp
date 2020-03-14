#include "heAssets.h"
#include "heLoader.h"
#include "heWindow.h"

HeAssetPool heAssetPool;
HeThreadLoader heThreadLoader;


// --- Materials
HeMaterial* heCreatePbrMaterial(const std::string& name, HeTexture* diffuseTexture, HeTexture* normalTexture, HeTexture* armTexture) {
    
    auto it = heAssetPool.materialPool.find(name);
    if (it != heAssetPool.materialPool.end())
        return &it->second;
    
    HeMaterial* mat = &heAssetPool.materialPool[name];
    mat->shader = heGetShader("3d_pbr");
    mat->textures["diffuse"] = diffuseTexture;
    mat->textures["normal"] = normalTexture;
    mat->textures["arm"] = armTexture;
    return mat;
    
}


// --- Assets

HeVao* heGetMesh(const std::string& file) {
    
    auto it = heAssetPool.meshPool.find(file);
    if (it != heAssetPool.meshPool.end())
        return &it->second;
    
    // load model
    return heLoadD3Obj(file);
    
};

HeTexture* heGetTexture(const std::string& file) {
    
    auto it = heAssetPool.texturePool.find(file);
    if (it != heAssetPool.texturePool.end()) {
        it->second.referenceCount++;
        return &it->second;
    }
    
    HeTexture* t = &heAssetPool.texturePool[file];
    t->referenceCount = 1;
    heLoadTexture(t, file);
    return t;
    
};

HeShaderProgram* heGetShader(const std::string& name) {
    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    heCreateShader(s, "res/shaders/" + name + "_v.glsl", "res/shaders/" + name + "_f.glsl");
    return s;
    
};

HeShaderProgram* heGetShader(const std::string& name, const std::string& vShader, const std::string& fShader) {
    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    heCreateShader(s, vShader, fShader);
    return s;
    
};

HeMaterial* heGetMaterial(const std::string& name) {
    
    auto it = heAssetPool.materialPool.find(name);
    if (it != heAssetPool.materialPool.end())
        return &it->second;
    
    return nullptr;
    
}


// --- ThreadLoader

void heRequestTexture(HeTexture* texture, unsigned char* buffer) {
    
    if(!heIsMainThread()) {
        heThreadLoader.textures[texture] = buffer;
        heThreadLoader.updateRequested = true;
    }
    
};

void heRequestVao(HeVao* vao) {
    
    if(!heIsMainThread()) {
        heThreadLoader.vaos.emplace_back(vao);
        heThreadLoader.updateRequested = true;
    }
    
};

void heUpdateThreadLoader() {
    
    for (auto& all : heThreadLoader.textures)
        heCreateGlTextureFromBuffer(all.second, &all.first->textureId, all.first->width, all.first->height, all.first->channels, 
                                    all.first->format);
    
    for(auto all : heThreadLoader.vaos) {
        heCreateVao(all);
        heBindVao(all);
        all->verticesCount = (unsigned int) (all->vbos[0].data.size()) / all->vbos[0].dimensions;
        
        unsigned int counter = 0;
        for(auto& vbos : all->vbos)
            heAddVboData(&vbos, counter++);
        
        heUnbindVao(all);
    }
    
    heThreadLoader.textures.clear();
    heThreadLoader.vaos.clear();
    heThreadLoader.updateRequested = false;
    
};
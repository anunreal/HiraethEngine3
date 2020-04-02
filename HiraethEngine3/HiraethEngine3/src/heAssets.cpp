#include "heAssets.h"
#include "heLoader.h"
#include "heWindow.h"

HeAssetPool heAssetPool;
HeThreadLoader heThreadLoader;


// --- Materials
HeMaterial* heMaterialCreatePbr(const std::string& name, HeTexture* diffuseTexture, HeTexture* normalTexture, HeTexture* armTexture) {
    
    auto it = heAssetPool.materialPool.find(name);
    if (it != heAssetPool.materialPool.end())
        return &it->second;
    
    HeMaterial* mat = &heAssetPool.materialPool[name];
    mat->shader = heAssetPoolGetShader("3d_pbr");
    mat->textures["diffuse"] = diffuseTexture;
    mat->textures["normal"] = normalTexture;
    mat->textures["arm"] = armTexture;
    return mat;
    
};


// --- Assets

HeVao* heAssetPoolGetMesh(const std::string& file) {
    
    auto it = heAssetPool.meshPool.find(file);
    if (it != heAssetPool.meshPool.end())
        return &it->second;
    
    // load model
    HeVao* vao = &heAssetPool.meshPool[file];
    heMeshLoad(file, vao);
    
#ifdef HE_ENABLE_NAMES
    vao->name = file;
#endif
    
    return vao;
    
};

HeTexture* heAssetPoolGetTexture(const std::string& file) {
    
    auto it = heAssetPool.texturePool.find(file);
    if (it != heAssetPool.texturePool.end()) {
        it->second.referenceCount++;
        return &it->second;
    }
    
    HeTexture* t = &heAssetPool.texturePool[file];
    t->referenceCount = 1;
    heTextureLoadFromFile(t, file);
    return t;
    
};

HeShaderProgram* heAssetPoolGetShader(const std::string& name) {
    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    heShaderCreateProgram(s, "res/shaders/" + name + "_v.glsl", "res/shaders/" + name + "_f.glsl");
    
#ifdef HE_ENABLE_NAMES
    s->name = name;
#endif
    
    return s;
    
};

HeShaderProgram* heAssetPoolGetShader(const std::string& name, const std::string& vShader, const std::string& fShader) {
    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    heShaderCreateProgram(s, vShader, fShader);
    return s;
    
};

HeMaterial* heAssetPoolGetMaterial(const std::string& name) {
    
    auto it = heAssetPool.materialPool.find(name);
    if (it != heAssetPool.materialPool.end())
        return &it->second;
    
    return nullptr;
    
}


// --- ThreadLoader

void heThreadLoaderRequestTexture(HeTexture* texture, unsigned char* buffer) {
    
    if(!heIsMainThread()) {
        heThreadLoader.textures[texture] = buffer;
        heThreadLoader.updateRequested = true;
    }
    
};

void heThreadLoaderRequestVao(HeVao* vao) {
    
    if(!heIsMainThread()) {
        heThreadLoader.vaos.emplace_back(vao);
        heThreadLoader.updateRequested = true;
    }
    
};

void heThreadLoaderUpdate() {
    
    // textures
    for (auto& all : heThreadLoader.textures)
        heTextureCreateFromBuffer(all.second, &all.first->textureId, all.first->width, all.first->height, all.first->channels,
                                  all.first->format);
    
    // vaos
    for(auto all : heThreadLoader.vaos) {
        heVaoCreate(all);
        heVaoBind(all);
        all->verticesCount = (unsigned int) (all->vbos[0].data.size()) / all->vbos[0].dimensions;
        
        unsigned int counter = 0;
        for(auto& vbos : all->vbos)
            heVaoAddVboData(&vbos, counter++);
        
        heVaoUnbind(all);
    }
    
    // clear
    heThreadLoader.textures.clear();
    heThreadLoader.vaos.clear();
    heThreadLoader.updateRequested = false;
    
};
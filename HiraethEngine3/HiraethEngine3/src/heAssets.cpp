#include "heAssets.h"
#include "heLoader.h"
#include "heWindow.h"


HeAssetPool heAssetPool;
HeThreadLoader heThreadLoader;

std::unordered_map<std::string, uint32_t> shaderTypeIds;
uint32_t shaderTypeCounter = 0;

// --- Materials
HeMaterial* heMaterialCreatePbr(std::string const& name, HeTexture* diffuseTexture, HeTexture* normalTexture, HeTexture* armTexture) {
    
    HeMaterial* mat = heAssetPoolGetNewMaterial(name);
    mat->shader = heAssetPoolGetShader("3d_pbr");
    mat->textures["diffuse"] = diffuseTexture;
    mat->textures["normal"]  = normalTexture;
    mat->textures["arm"]     = armTexture;
    mat->type                = heMaterialGetType("3d_pbr");
    return mat;
    
};

uint32_t heMaterialGetType(std::string const& shaderName) {
    
    auto it = shaderTypeIds.find(shaderName);
    if(it != shaderTypeIds.end())
        return it->second;
    
    uint32_t id = ++shaderTypeCounter;
    shaderTypeIds[shaderName] = id;
    return id;
    
};


// --- Assets

HeVao* heAssetPoolGetMesh(std::string const& file) {
    
    auto it = heAssetPool.meshPool.find(file);
    if (it != heAssetPool.meshPool.end())
        return &it->second;
    
    // load model
    HeVao* vao = &heAssetPool.meshPool[file];
    
#ifdef HE_ENABLE_NAMES
    vao->name = file;
#endif
    
    heMeshLoad(file, vao);
    
    return vao;
    
};

HeTexture* heAssetPoolGetTexture(std::string const& file) {
    
    auto it = heAssetPool.texturePool.find(file);
    if (it != heAssetPool.texturePool.end()) {
        it->second.referenceCount++;
        return &it->second;
    }
    
    HeTexture* t = &heAssetPool.texturePool[file];
    t->referenceCount = 1;
    
#ifdef HE_ENABLE_NAMES
    t->name = file;
#endif
    
    heTextureLoadFromFile(t, file);
    return t;
    
};

HeShaderProgram* heAssetPoolGetShader(std::string const& name) {
    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    
#ifdef HE_ENABLE_NAMES
    s->name = name;
#endif
    
    heShaderCreateProgram(s, "res/shaders/" + name + ".glsl");
    
    return s;
    
};

HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& fShader) {
    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    
#ifdef HE_ENABLE_NAMES
    s->name = name;
#endif
    
    heShaderCreateProgram(s, vShader, fShader);
    return s;
    
};

HeShaderProgram* heAssetPoolGetShader(std::string const& name, std::string const& vShader, std::string const& gShader, std::string const& fShader) {
    
    auto it = heAssetPool.shaderPool.find(name);
    if (it != heAssetPool.shaderPool.end())
        return &it->second;
    
    HeShaderProgram* s = &heAssetPool.shaderPool[name];
    
#ifdef HE_ENABLE_NAMES
    s->name = name;
#endif
    
    heShaderCreateProgram(s, vShader, gShader, fShader);
    return s;
    
};

HeMaterial* heAssetPoolGetMaterial(std::string const& name) {
    
    auto it = heAssetPool.materialPool.find(name);
    if (it != heAssetPool.materialPool.end())
        return &it->second;
    
    return nullptr;
    
}

HeMaterial* heAssetPoolGetNewMaterial(std::string const& name) {
    
    auto it = heAssetPool.materialPool.find(name);
    if(it != heAssetPool.materialPool.end())
        return &it->second;
    
    HeMaterial* m = &heAssetPool.materialPool[name];
    return m;
    
};


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
    for (auto& all : heThreadLoader.textures) {
        heTextureCreateFromBuffer(all.second, &all.first->textureId, all.first->width, all.first->height, all.first->channels,
                                  all.first->format);
        
#ifdef HE_ENABLE_NAMES
        heTextureLabel(all.first, all.first->name);
#endif
    }
    
    // vaos
    for(auto all : heThreadLoader.vaos) {
        heVaoCreate(all);
        heVaoBind(all);
        
        HeVbo* vbo = &all->vbos[0];
        uint32_t size = 0;
        switch(vbo->type) {
            case HE_UNIFORM_DATA_TYPE_FLOAT:
            size = (uint32_t) vbo->dataf.size();
            break;
            
            case HE_UNIFORM_DATA_TYPE_INT:
            size = (uint32_t) vbo->datai.size();
            break;
            
            case HE_UNIFORM_DATA_TYPE_UINT:
            size = (uint32_t) vbo->dataui.size();
            break;
        }
        
        all->verticesCount = size / all->vbos[0].dimensions;
        
        uint32_t counter = 0;
        for(auto& vbos : all->vbos)
            heVaoAddVboData(&vbos, counter++);
        
        heVaoUnbind(all);
    }
    
    // clear
    heThreadLoader.textures.clear();
    heThreadLoader.vaos.clear();
    heThreadLoader.updateRequested = false;
    
};
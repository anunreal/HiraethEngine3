#include "heLoader.h"
#include "heAssets.h"
#include "heUtils.h"
#include "heWindow.h"
#include "heCore.h"
#include <fstream>


void parseObjVertex(const int ids[3], HeD3MeshBuilder& mesh) {
    
    const hm::vec3f& vertex = mesh.vertices[ids[0]];
    const hm::vec2f& uv = mesh.uvs[ids[1]];
    const hm::vec3f& normal = mesh.normals[ids[2]];
    
    mesh.verticesArray.emplace_back(vertex.x);
    mesh.verticesArray.emplace_back(vertex.y);
    mesh.verticesArray.emplace_back(vertex.z);
    
    mesh.uvArray.emplace_back(uv.x);
    mesh.uvArray.emplace_back(uv.y);
    
    mesh.normalArray.emplace_back(normal.x);
    mesh.normalArray.emplace_back(normal.y);
    mesh.normalArray.emplace_back(normal.z);
    
};

void parseObjTangents(const hm::vec3f (&vertices)[3], const hm::vec2f (&uvs)[3], HeD3MeshBuilder& mesh) {
    
    hm::vec3f edge1 = vertices[1] - vertices[0];
    hm::vec3f edge2 = vertices[2] - vertices[0];
    
    hm::vec2f deltaUv1 = uvs[1] - uvs[0];
    hm::vec2f deltaUv2 = uvs[2] - uvs[0];
    
    const float f = 1.0f / (deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y);
    
    float x = f * (deltaUv2.y * edge1.x - deltaUv1.y * edge2.x);
    float y = f * (deltaUv2.y * edge1.y - deltaUv1.y * edge2.y);
    float z = f * (deltaUv2.y * edge1.z - deltaUv1.y * edge2.z);
    
    hm::vec3f tang(x, y, z);
    tang = hm::normalize(tang);
    
    for (uint8_t i = 0; i < 3; ++i) {
        mesh.tangentArray.emplace_back(tang.x);
        mesh.tangentArray.emplace_back(tang.y);
        mesh.tangentArray.emplace_back(tang.z);
    }
    
};

// parses a float of given format:
// +ffff.fff (or -ffff.fff)
b8 parseFloatFixedWidth(std::ifstream& stream, float* result) {
    
    char c;
    stream.get(c);
    if(c == '\n')
        return false;
    
    int sign = (c == '-') ? -1 : 1;
    *result = 0;
    
    // 0 = 3;
    // 1 = 2;
    // 2 = 1;
    // 3 = 0
    // 4 = -1
    // 5 = -2
    // 6 = -3
    
    for(uint8_t i = 0; i < 7; ++i) {
        stream.get(c);
        int exp = -(i - 4) - 1;
        int ch = (int) (c - '0');
        *result = *result + (float) (ch * std::pow(10, exp));
        
        if(i == 3)
            stream.get(c); // skip .
    }
    
    *result *= sign;
    return true;
    
};

// parses an int of given format:
// +iiii (or -iiii)
template<typename T>
b8 parseIntFixedWidth(std::ifstream& stream, T* result) {
    
    char c;
    stream.get(c);
    if(c == '\n')
        return false;
    
    uint8_t sign = (c == '-') ? -1 : 1;
    *result = 0;
    
    for(uint8_t i = 0; i < 4; ++i) {
        stream.get(c);
        uint16_t exp = (3 - i);
        T ch = (T) (c - '0');
        *result = *result + ch * (T) std::pow(10, exp);
    }
    
    *result *= sign;
    return true;
    
};

// parses n floats and stores them in ptr
void parseFloats(std::ifstream& stream, int n, void* ptr) {
    
    for(uint8_t i = 0; i < n; ++i)
        parseFloatFixedWidth(stream, &((float*)ptr)[i]);
    
};

// parses n ints and stores them in ptr
template<typename T>
void parseInts(std::ifstream& stream, const uint8_t n, void* ptr) {
    
    for(uint8_t i = 0; i < n; ++i)
        parseIntFixedWidth(stream, &((T*)ptr)[i]);
    
};

void heMeshLoad(const std::string& fileName, HeVao* vao) {
    
    std::ifstream stream(fileName);
    if (!stream.good()) {
        HE_ERROR("Could not load model file [" + fileName + "]");
        return;
    }
    
    std::string string;
    HeD3MeshBuilder mesh;
    
    while (std::getline(stream, string)) {
        if (string.size() == 0 || string[0] == '#')
            continue;
        
        std::vector<std::string> args = heStringSplit(string, ' ');
        if (heStringStartsWith(string, "v ")) {
            // parse vertex
            mesh.vertices.emplace_back(hm::vec3f(std::stof(args[1]), std::stof(args[2]), std::stof(args[3])));
        } else if (heStringStartsWith(string, "vt ")) {
            // parse uv
            mesh.uvs.emplace_back(hm::vec2f(std::stof(args[1]), std::stof(args[2])));
        } else if (heStringStartsWith(string, "vn ")) {
            // parse normal
            mesh.normals.emplace_back(hm::vec3f(std::stof(args[1]), std::stof(args[2]), std::stof(args[3])));
        } else if (heStringStartsWith(string, "f ")) {
            // parse face
            const std::vector<std::string> vertex0 = heStringSplit(args[1], '/');
            const std::vector<std::string> vertex1 = heStringSplit(args[2], '/');
            const std::vector<std::string> vertex2 = heStringSplit(args[3], '/');
            
            int32_t vertexId0[3] = { std::stoi(vertex0[0]) - 1, std::stoi(vertex0[1]) - 1, std::stoi(vertex0[2]) - 1 };
            int32_t vertexId1[3] = { std::stoi(vertex1[0]) - 1, std::stoi(vertex1[1]) - 1, std::stoi(vertex1[2]) - 1 };
            int32_t vertexId2[3] = { std::stoi(vertex2[0]) - 1, std::stoi(vertex2[1]) - 1, std::stoi(vertex2[2]) - 1 };
            
            parseObjVertex(vertexId0, mesh);
            parseObjVertex(vertexId1, mesh);
            parseObjVertex(vertexId2, mesh);
            
            // we can probably optimize this
            
            hm::vec3f tvertices[3];
            hm::vec2f tuvs[3];
            tvertices[0] = mesh.vertices[vertexId0[0]];
            tvertices[1] = mesh.vertices[vertexId1[0]];
            tvertices[2] = mesh.vertices[vertexId2[0]];
            tuvs[0] = mesh.uvs[vertexId0[1]];
            tuvs[1] = mesh.uvs[vertexId1[1]];
            tuvs[2] = mesh.uvs[vertexId2[1]];
            
            parseObjTangents(tvertices, tuvs, mesh);
        }
    }
    
    stream.close();
    
    b8 isMainThread = heIsMainThread();
    
    if(isMainThread) {
        heVaoCreate(vao);
        heVaoBind(vao);
    }
    
    heVaoAddData(vao, mesh.verticesArray, 3);
    heVaoAddData(vao, mesh.uvArray, 2);
    heVaoAddData(vao, mesh.normalArray, 3);
    heVaoAddData(vao, mesh.tangentArray, 3);
    
    if(!isMainThread)
        heThreadLoaderRequestVao(vao);
    
};


void heD3InstanceLoad(const std::string& fileName, HeD3Instance* instance) {
    
    std::ifstream stream(fileName);
    if(!stream) {
        HE_ERROR("Could not find asset file [" + fileName + "]");
        return;
    }
    
    
    std::string line;
    std::getline(stream, line);
    
    std::string assetName = fileName.substr(fileName.find_last_of('/'), fileName.find('.'));
    
#ifdef HE_ENABLE_NAMES
    instance->name = assetName;
#endif
    
    if(instance->material == nullptr)
        instance->material = &heAssetPool.materialPool[assetName];
    
    HeMaterial* material = instance->material;
    material->shader = heAssetPoolGetShader(line);
    
    std::getline(stream, line);
    while(!line.empty()) {
        // parse texture to material
        size_t pos = line.find('=');
        std::string name = line.substr(0, pos);
        std::string tex = line.substr(pos + 1);
        material->textures[name] = heAssetPoolGetTexture("res/textures/assets/" + tex);
        std::getline(stream, line);
    }
    
    HeD3MeshBuilder builder;
    
    // parse vertices
    {
        float result = 0.f;
        while(parseFloatFixedWidth(stream, &result))
            builder.verticesArray.emplace_back(result);
    }
    
    // parse uvs
    {
        float result = 0.f;
        while(parseFloatFixedWidth(stream, &result))
            builder.uvArray.emplace_back(result);
    }
    
    // parse normals
    {
        float result = 0.f;
        while(parseFloatFixedWidth(stream, &result))
            builder.normalArray.emplace_back(result);
    }
    
    // parse tangents
    {
        float result = 0.f;
        while(parseFloatFixedWidth(stream, &result))
            builder.tangentArray.emplace_back(result);
    }
    
    stream.close();
    
    
    b8 isMainThread = heIsMainThread();
    
    HeVao* vao = &heAssetPool.meshPool[fileName];
    
#ifdef HE_ENABLE_NAMES
    vao->name = fileName;
#endif
    
    if(isMainThread) {
        heVaoCreate(vao);
        heVaoBind(vao);
    }
    
    heVaoAddData(vao, builder.verticesArray, 3);
    heVaoAddData(vao, builder.uvArray, 2);
    heVaoAddData(vao, builder.normalArray, 3);
    heVaoAddData(vao, builder.tangentArray, 3);
    
    if(!isMainThread)
        heThreadLoaderRequestVao(vao);
    
    instance->mesh = vao;
    
};

void heD3LevelLoad(const std::string& fileName, HeD3Level* level) {
    
    std::ifstream stream(fileName);
    if(!stream) {
        HE_ERROR("Could not find level file [" + fileName + "]");
        return;
    }
    
    char type;
    char c;
    while(stream.get(type)) {
        stream.get(c); // skip colon
        if(type == 'i') {
            // instance
            std::string name = "";
            stream.get(c);
            while(c != ',') {
                name += c;
                stream.get(c);
            }
            
            HeD3Instance* instance = &level->instances.emplace_back();
            hm::vec3f rotation;
            
            parseFloats(stream, 3, &instance->transformation.position);
            parseFloats(stream, 3, &rotation);
            parseFloats(stream, 3, &instance->transformation.scale);
            
            instance->transformation.rotation = hm::fromEulerRadians(rotation);
            heD3InstanceLoad("res/assets/" + name, instance);
            stream.get(c); // skip next line
        } else if(type == 'l') {
            // light
            HeD3LightSource* light = &level->lights.emplace_back();
            stream.get(c);
            light->type     = (HeLightSourceType) (c - '0');
            light->colour.a = 255; // unneccesary for lights
            light->update   = true;
            parseFloats(stream, 3, &light->vector);
            parseInts<uint8_t>(stream, 3, &light->colour);
            parseFloatFixedWidth(stream, &light->colour.i);
            parseFloats(stream, 8, &light->data);
        }
    }
    
    stream.close();
    heD3LevelSetActive(level);
    
};
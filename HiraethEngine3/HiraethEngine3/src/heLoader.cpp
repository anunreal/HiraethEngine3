#include "heLoader.h"
#include "heAssets.h"
#include "heUtils.h"
#include "heWindow.h"
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
    
    for (unsigned int i = 0; i < 3; ++i) {
        mesh.tangentArray.emplace_back(tang.x);
        mesh.tangentArray.emplace_back(tang.y);
        mesh.tangentArray.emplace_back(tang.z);
    }
    
};

HeVao* heLoadD3Obj(const std::string& fileName) {
    
    std::ifstream stream(fileName);
    if (!stream.good()) {
        std::cout << "Error: Could not load model file [" << fileName << "]" << std::endl;
        return nullptr;
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
            
            int vertexId0[3] = { std::stoi(vertex0[0]) - 1, std::stoi(vertex0[1]) - 1, std::stoi(vertex0[2]) - 1 };
            int vertexId1[3] = { std::stoi(vertex1[0]) - 1, std::stoi(vertex1[1]) - 1, std::stoi(vertex1[2]) - 1 };
            int vertexId2[3] = { std::stoi(vertex2[0]) - 1, std::stoi(vertex2[1]) - 1, std::stoi(vertex2[2]) - 1 };
            
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
    
    std::cout << "Finished obj loading " << mesh.verticesArray.size() << std::endl;
    
    stream.close();
    
    bool isMainThread = heIsMainThread();
    
    HeVao* vao = &heAssetPool.meshPool[fileName];
    if(isMainThread) {
        heCreateVao(vao);
        heBindVao(vao);
    }
    
    heAddVaoData(vao, mesh.verticesArray, 3);
    heAddVaoData(vao, mesh.uvArray, 2);
    heAddVaoData(vao, mesh.normalArray, 3);
    heAddVaoData(vao, mesh.tangentArray, 3);
    
    if(!isMainThread)
        heRequestVao(vao);
    
    return vao;
    
};

void heLoadD3Level(HeD3Level* level, const std::string& fileName) {
    
    std::ifstream stream(fileName);
    std::string line;
    
    std::vector<HeMaterial*> materials;
    
    while(std::getline(stream, line)) {
        std::vector<std::string> args = heStringSplit(line.substr(2), ':');
        if(line[0] == 'm') {
            // material
            std::string name = fileName + "_" + std::to_string(materials.size());
            HeMaterial* mat = heCreatePbrMaterial(name, heGetTexture(args[0]), heGetTexture(args[1]), heGetTexture(args[2]));
            materials.emplace_back(mat);
        } else if(line[0] == 'i') {
            // instance
            HeD3Instance* ins            = &level->instances.emplace_back();
            ins->mesh                    = heGetMesh(args[0]);
            ins->material                = materials[std::stoi(args[1])];
            ins->transformation.position = hm::parseVec3f(args[2]);
            ins->transformation.rotation = hm::fromEuler(hm::parseVec3f(args[3]));
            ins->transformation.scale    = hm::parseVec3f(args[4]);
        } else if(line[0] == 'l') {
            
        }
    }
    
    stream.close();
    
};
#pragma once
#include "heD3.h"

struct HeD3MeshBuilder {
    /* a list of all  */
    
    std::vector<hm::vec3f> vertices;
    std::vector<hm::vec2f> uvs;
    std::vector<hm::vec3f> normals;
    std::vector<hm::vec3f> tangents;
    
    
    /* the actual data buffers in the correct order */
    
    std::vector<float> verticesArray;
    std::vector<float> uvArray;
    std::vector<float> normalArray;
    std::vector<float> tangentArray;
};


// loads a 3d object from given file and stores the data in a vao from the asset pool. The name of the mesh in the 
// asset pool will be the file name. This loads the vertices, uvs, normals and tangents of the model
extern HE_API HeVao* heLoadD3Obj(const std::string& fileName);
// loads all instances (and their materials) and lights from given file
extern HE_API void heLoadD3Level(HeD3Level* level, const std::string& fileName);
// loads an asset from given file. This asset must be a valid h3asset file. This will load a material and a mesh and
// simply replace the existing ones in instance (should be nullptr)
extern HE_API void heLoadAsset(const std::string& fileName, HeD3Instance* instance);
// loads a level from given file. This file must be a valid h3level file. This will load all assets and lights in
// that level.
extern HE_API void heLoadLevel(const std::string& fileName, HeD3Level* level);
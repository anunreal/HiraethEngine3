#ifndef HE_LOADER_H
#define HE_LOADER_H

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
extern HE_API void heMeshLoad(const std::string& fileName, HeVao* vao);
// loads an asset from given file. This asset must be a valid h3asset file. This will load a material and a mesh and
// simply replace the existing ones in instance (should be nullptr). If physics is not nullptr and there is physics info
// in the asset file, the data will be parsed and stored in that pointer
extern HE_API void heD3InstanceLoad(const std::string& fileName, HeD3Instance* instance, HePhysicsInfo* physics);
// loads a level from given file. This file must be a valid h3level file. This will load all assets and lights in
// that level. Specification for the level file format:
// Instances:
// i:[asset_file],[position rotation scale]
//   the asset must be placed in res/assets (that folder is not in name), subfolders are allowed
//   position, rotation and scale are 9 floats with fixed size without any borders between them for faster parsing
//
// Lights:
// l:[type][vector][colour][data]
//   type is the type of the light as int (see HeLightSourceType)
//   vector is a fixed width vector3f (see HeD3LightSource#vector)
//   colour are 3 fixed width ints and one fixed width float (see hm::colour)
//   data are up to 8 fixed width floats (see HeD3LightSource#data)
//   because we are only parsing numbers here, the parser does not expect any border character between the values
extern HE_API void heD3LevelLoad(const std::string& fileName, HeD3Level* level);

#endif
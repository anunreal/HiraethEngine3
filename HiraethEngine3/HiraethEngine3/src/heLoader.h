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
extern HE_API void heMeshLoad(std::string const& fileName, HeVao* vao);
// loads an asset from given file. This asset must be a valid h3asset file. This will load a material and a mesh and
// simply replace the existing ones in instance (should be nullptr). If physics is not nullptr and there is physics info
// in the asset file, the data will be parsed and stored in that pointer
extern HE_API void heD3InstanceLoad(std::string const& fileName, HeD3Instance* instance, HePhysicsShapeInfo* physics);
extern HE_API void heD3InstanceLoadBinary(std::string const& fileName, HeD3Instance* instance, HePhysicsShapeInfo* physics);
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
extern HE_API void heD3LevelLoad(std::string const& fileName, HeD3Level* level, b8 const loadPhysics);
// loads a binary file level
extern HE_API void heD3LevelLoadBinary(std::string const& fileName, HeD3Level* level, b8 const loadPhysics);

// parses a float of the following format by reading characters from the given stream (and stores the result in the float pointer):
// +ffff.fff (or -ffff.fff). This function returns true if the int could be parsed or false if an error occurs (reached end of file
// or new line)
extern HE_API b8 heAsciiStreamParseFloatFixedWidth(std::ifstream& stream, float* result);
// parses count amount of floats directly written back to back from the given stream. This simply calls the heParseFloatFixedWidth
// function count times
extern HE_API b8 heAsciiStreamParseFloats(std::ifstream& stream, uint8_t const count, void* ptr);
// parses an int of the following format by reading characters from the given stream (and stores the result in the int pointer):
// +iiii (or -iiii). The int can be of any size (8, 16, 32 or 64 bit). This function returns true if the int could be parsed or false
// if an error occurs (reached end of file or new line)
template<typename T>
extern HE_API b8 heAsciiStreamParseIntFixedWidth(std::ifstream& stream, T* result);
// parses count amount of ints directly written back to back from the given stream. This simply calls the heParseIntFixedWidth
// function count times
template<typename T>
extern HE_API b8 heAsciiStreamParseInts(std::ifstream& stream, uint8_t const count, void* ptr);

#endif
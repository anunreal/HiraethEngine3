#include "heLoader.h"
#include "heAssets.h"
#include "heUtils.h"
#include "heWindow.h"
#include "heCore.h"
#include "heBinary.h"
#include "heWin32Layer.h"
#include <fstream>
#include <limits>

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

void heMeshLoad(std::string const& fileName, HeVao* vao) {
	HeTextFile file;
	heTextFileOpen(&file, fileName, 0);
	if(!file.open)
		return;
	
    std::string string;
    HeD3MeshBuilder mesh;
    
    while (heTextFileGetLine(&file, &string)) {
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

	heTextFileClose(&file);
	
    b8 isMainThread = heIsMainThread();
    
    if(isMainThread) {
        heVaoCreate(vao);
        heVaoBind(vao);
    }
    
    heVaoAddData(vao, mesh.verticesArray, 3, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, mesh.uvArray,       2, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, mesh.normalArray,   3, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, mesh.tangentArray,  3, HE_VBO_USAGE_STATIC);
    
    if(!isMainThread)
        heThreadLoaderRequestVao(vao);
};


void heD3InstanceLoad(std::string const& fileName, HeD3Instance* instance, HePhysicsShapeInfo* physics) {
	HeTextFile file;
	heTextFileOpen(&file, fileName, 0);
	if(!file.open)
		return;
	
    HE_LOG("Loading instance " + fileName);
    
    std::string line;
	heTextFileGetLine(&file, &line);
	
    std::string assetName = fileName.substr(fileName.find_last_of('/'), fileName.find('.'));
    
#ifdef HE_ENABLE_NAMES
    instance->name = assetName;
#endif
    
    
    // MATERIAL
    instance->material = heAssetPoolGetNewMaterial(assetName);
    //instance->material->shader = heAssetPoolGetShader(line);
    instance->material->type = heMaterialGetType(line);
    
	heTextFileGetLine(&file, &line);
	while(!line.empty()) {
        // parse texture to material
        size_t pos = line.find('=');
        std::string name = line.substr(0, pos);
        std::string tex = line.substr(pos + 1);
        //heTimerStart();
		HeTexture* texture = heAssetPoolGetTexture("res/textures/assets/" + tex, HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_FILTER_ANISOTROPIC | HE_TEXTURE_CLAMP_REPEAT);
		instance->material->textures[name] = texture;

		//heTimerPrint(tex);
		heTextFileGetLine(&file, &line);
	}

	heTextureUnbind(0);
    
    
    // MESH
    HeD3MeshBuilder builder;
    
    // parse vertices
    {
        float result = 0.f;
        while(heTextFileGetFloat(&file, &result))
            builder.verticesArray.emplace_back(result);
    }
    
    // parse uvs
    {
        float result = 0.f;
        while(heTextFileGetFloat(&file, &result))
            builder.uvArray.emplace_back(result);
    }
    
    // parse normals
    {
        float result = 0.f;
        while(heTextFileGetFloat(&file, &result))
            builder.normalArray.emplace_back(result);
    }
    
    
    // parse tangents
    {
        float result = 0.f;
        while(heTextFileGetFloat(&file, &result))
            builder.tangentArray.emplace_back(result);
    }
    
    HeVao* vao = &heAssetPool.meshPool[fileName];
    
#ifdef HE_ENABLE_NAMES
    vao->name = fileName;
#endif
    
    b8 isMainThread = heIsMainThread();
    if(isMainThread) {
        heVaoCreate(vao);
        heVaoBind(vao);
    }
    
    heVaoAddData(vao, builder.verticesArray, 3, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, builder.uvArray,       2, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, builder.normalArray,   3, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, builder.tangentArray,  3, HE_VBO_USAGE_STATIC);
    
    if(!isMainThread)
        heThreadLoaderRequestVao(vao);
    
    instance->mesh = vao;
    
    // PHYSICS
    char c = heTextFilePeek(&file);
    // check if file still has content and physics is not nullptr
    if(c != '\n' && physics) {
        char type;
        heTextFileGetChar(&file, &type);
        physics->type = (HePhysicsShapeType) int(type - '0');
        
        heTextFileGetFloat(&file, &physics->mass);
        heTextFileGetFloat(&file, &physics->friction);
        heTextFileGetFloat(&file, &physics->restitution);
        
        switch(physics->type) {
            case HE_PHYSICS_SHAPE_CONCAVE_MESH:
            case HE_PHYSICS_SHAPE_CONVEX_MESH: {
                hm::vec3f vec;
                while(heTextFileGetFloats(&file, 3, &vec))
                    physics->mesh.emplace_back(vec);
                break;
            };
            
            case HE_PHYSICS_SHAPE_BOX:
            heTextFileGetFloats(&file, 3, &physics->box);
            break;
            
            case HE_PHYSICS_SHAPE_SPHERE:
            heTextFileGetFloat(&file, &physics->sphere);
            break;
            
            case HE_PHYSICS_SHAPE_CAPSULE:
            heTextFileGetFloats(&file, 2, &physics->capsule);
            break;
        }
    }
    
	heTextFileClose(&file);
};

void heD3InstanceLoadBinary(std::string const& fileName, HeD3Instance* instance, HePhysicsShapeInfo* physics) {
    HeBinaryBuffer buffer;
    
    if(!heBinaryBufferOpenFile(&buffer, fileName, 4096, HE_ACCESS_READ_ONLY)) {
        HE_ERROR("Could not find asset file [" + fileName + "]");
        return;
    }
    
    HE_LOG("Loading instance " + fileName);
    
    //std::string assetName = fileName.substr(fileName.find_last_of('/'), fileName.find('.'));
	std::string assetName = buffer.name;
	
#ifdef HE_ENABLE_NAMES
    instance->name = assetName;
#endif
    
    std::string line;
    heBinaryBufferGetString(&buffer, &line);
    
    instance->material = heAssetPoolGetNewMaterial(assetName);
    instance->material->type = heMaterialGetType(line);
    
    // -- material
    
    while(heBinaryBufferPeek(&buffer) != '\n') {
        // parse texture to material
        heBinaryBufferGetString(&buffer, &line);
        size_t pos = line.find('=');
        std::string name = line.substr(0, pos);
        std::string tex = line.substr(pos + 1);

        //heTimerStart();
		HeTexture* texture = heAssetPoolGetTexture("res/textures/assets/bin/" + tex, HE_TEXTURE_FILTER_TRILINEAR | HE_TEXTURE_FILTER_ANISOTROPIC | HE_TEXTURE_CLAMP_REPEAT);
        instance->material->textures[name] = texture;
		//heTimerPrint(tex + " (" + std::to_string(instance->material->textures[name]->size.x) + "x" + std::to_string(instance->material->textures[name]->size.y) + ")");
	}
	
	heTextureUnbind(0);    
    
    // -- mesh
    
    buffer.offset += 1; // skip \r\n
    
    HeD3MeshBuilder builder;
    heBinaryBufferGetFloatBuffer(&buffer, &builder.verticesArray);
    heBinaryBufferGetFloatBuffer(&buffer, &builder.uvArray);
    heBinaryBufferGetFloatBuffer(&buffer, &builder.normalArray);
    heBinaryBufferGetFloatBuffer(&buffer, &builder.tangentArray);
    HeVao* vao = &heAssetPool.meshPool[fileName];
    
#ifdef HE_ENABLE_NAMES
    vao->name = assetName;
#endif
    
    b8 isMainThread = heIsMainThread();
    if(isMainThread) {
        heVaoCreate(vao);
        heVaoBind(vao);
    }


    heVaoAddData(vao, builder.verticesArray, 3, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, builder.uvArray,       2, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, builder.normalArray,   3, HE_VBO_USAGE_STATIC);
    heVaoAddData(vao, builder.tangentArray,  3, HE_VBO_USAGE_STATIC);

    instance->mesh = vao;
    
    if(!isMainThread)
        heThreadLoaderRequestVao(vao);
    
    
    // -- physics
    
    if(heBinaryBufferAvailable(&buffer)) { // we still have data here
        int32_t type;
        heBinaryBufferGetInt(&buffer, &type);
        physics->type = (HePhysicsShapeType) type;
        heBinaryBufferGetFloat(&buffer, &physics->mass);
        heBinaryBufferGetFloat(&buffer, &physics->friction);
        heBinaryBufferGetFloat(&buffer, &physics->restitution);
        std::vector<float> data;
        heBinaryBufferGetFloatBuffer(&buffer, &data);
        
        switch(physics->type) {
            case HE_PHYSICS_SHAPE_CONCAVE_MESH:
            case HE_PHYSICS_SHAPE_CONVEX_MESH: {
                for(uint16_t i = 0; i < (uint16_t) data.size(); i+=3)
                    physics->mesh.emplace_back(hm::vec3f(data[(size_t) i], data[(size_t) (i + 1)], data[(size_t) (i + 2)]));
                break;
            }
            
            case HE_PHYSICS_SHAPE_BOX:
            physics->box.x = data[0];
            physics->box.y = data[1];
            physics->box.z = data[2];
            break;
            
            case HE_PHYSICS_SHAPE_SPHERE:
            physics->sphere = data[0];
            break;
            
            case HE_PHYSICS_SHAPE_CAPSULE:
            physics->capsule.x = data[0];
            physics->capsule.y = data[1];
            break;
        }
    }
    
    
    heBinaryBufferCloseFile(&buffer);
};

void heD3LevelLoad(std::string const& fileName, HeD3Level* level, b8 const loadPhysics) {
	HeTextFile file;
	heTextFileOpen(&file, fileName, 2048);
	
    // some kind of physics info
    if(loadPhysics && !hePhysicsLevelIsSetup(&level->physics))
        hePhysicsLevelCreate(&level->physics, HePhysicsLevelInfo(hm::vec3f(0, -10, 0)));
    
    const b8 BINARY = true;
    
    struct HeD3InstancePrefab {
        std::string        name;
        HeVao*             vao;
        HeMaterial*        material;
        HePhysicsShapeInfo physics;
    } prefab;

    char type;
    char c;
    while(heTextFileGetChar(&file, &type)) {
        //stream.get(c); // skip colon
		heTextFileGetChar(&file, &c);
		if(type == 'i') {
            // instance
            std::string name = "";
            //stream.get(c);
            heTextFileGetChar(&file, &c);
			while(c != ',') {
                name += c;
                heTextFileGetChar(&file, &c);
            }
            
            HeD3Instance* instance = &level->instances.emplace_back();
            
            heTextFileGetFloats(&file, 3, &instance->transformation.position);
            heTextFileGetFloats(&file, 4, &instance->transformation.rotation);
            heTextFileGetFloats(&file, 3, &instance->transformation.scale);
            
            HePhysicsShapeInfo physics;
            // for debugging purposes
            if(name.compare(name.size() - 4, 4, ".obj") == 0) {
                instance->mesh = &heAssetPool.meshPool[name];
                heMeshLoad("res/assets/" + name, instance->mesh);
            } else {
                if(name == prefab.name) {
                    instance->mesh = prefab.vao;
                    instance->material = prefab.material;
                    physics = prefab.physics;
                } else {
                    if(BINARY)
                        heD3InstanceLoadBinary("res/assets/bin/" + name, instance, &physics);
                    else
                        heD3InstanceLoad("res/assets/" + name, instance, &physics);
                    prefab.material = instance->material;
                    prefab.physics  = physics;
                    prefab.vao      = instance->mesh;
                    prefab.name     = name;
                }
            }
            
            if(loadPhysics && physics.type != HE_PHYSICS_SHAPE_NONE) {
                instance->physics = &level->physics.components.emplace_back();
                hePhysicsComponentCreate(instance->physics, physics);
                hePhysicsComponentSetTransform(instance->physics, instance->transformation);
                hePhysicsLevelAddComponent(&level->physics, instance->physics);
            }
            
            heTextFileGetChar(&file, &c); // skip next line
        } else if(type == 'l') {
            // light
            HeD3LightSource* light = &level->lights.emplace_back();
            heTextFileGetChar(&file, &c);
            light->type     = (HeLightSourceType) (c - '0');
            light->colour.a = 255; // not needed for lights
            light->update   = true;
            heTextFileGetFloats(&file, 3, &light->vector);
            heTextFileGetInts<uint8_t>(&file, 3, &light->colour);
            heTextFileGetFloat(&file, &light->colour.i);
            heTextFileGetFloats(&file, 8, &light->data);
        }
    }
    
    heTextFileClose(&file);
};

void heD3LevelLoadBinary(std::string const& fileName, HeD3Level* level, b8 const loadPhysics) {
    
};

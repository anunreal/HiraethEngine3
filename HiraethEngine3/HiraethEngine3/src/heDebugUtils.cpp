#include "heDebugUtils.h"
#include "heD3.h"
#include "heCore.h"
#include "heUtils.h"
#include "heRenderer.h"
#include "heBinary.h"
#include "heWin32Layer.h"
#include "heConsole.h"
#include <sstream> // for he_float_to_string
#include <iomanip> // for std::setprecision 

HeDebugInfo heDebugInfo;

inline void heFlagStringBuilder(std::string& output, std::string const& in) {
    if(output.size() == 0)
        output = in;
    else
        output += ", " + in;
};

void heDebugRequestInfo(const HeDebugInfoFlags flags) {
    // print debug information
    if(flags == HE_DEBUG_INFO_LIGHTS) {
        heDebugPrint("=== LIGHTS ===\n");
        for(const auto& all : heD3Level->lights) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint(string);
        }
        
        heDebugPrint("=== LIGHTS ===");
    }
    
    if(flags == HE_DEBUG_INFO_INSTANCES) {
        heDebugPrint("=== INSTANCES ===\n");
        uint16_t counter = 0;
        for(const auto& all : heD3Level->instances) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint('[' + std::to_string(counter++) + "] = " + string);
        }
        
        heDebugPrint("=== INSTANCES ===");
    }
    
    if(flags & HE_DEBUG_INFO_CAMERA) {
        heDebugPrint("=== CAMERA ===\n");
        std::string string;
        he_to_string(&heD3Level->camera, string);
        heDebugPrint(string);
        heDebugPrint("=== CAMERA ===");
    }
    
    if(flags & HE_DEBUG_INFO_ENGINE) {
        heDebugPrint("=== ENGINE ===\n");
        std::string string;
        he_to_string(&heRenderEngine->gBufferFbo, string);
        heDebugPrint("gBuffer = " + string);
        heDebugPrint("=== ENGINE ===");
    }

	if(flags & HE_DEBUG_INFO_MEMORY) {
		heDebugPrint("=== MEMORY ===");
		uint64_t total = heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] +
			heMemoryTracker[HE_MEMORY_TYPE_VAO] + 
			heMemoryTracker[HE_MEMORY_TYPE_FBO] +
			heMemoryTracker[HE_MEMORY_TYPE_SHADER] + 
			heMemoryTracker[HE_MEMORY_TYPE_CONTEXT];
		
		heDebugPrint("textures = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_TEXTURE]));
		heDebugPrint("vaos     = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_VAO]));
		heDebugPrint("fbos     = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_FBO]));
		heDebugPrint("context  = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_CONTEXT]));
		heDebugPrint("shaders  = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_SHADER]));
		heDebugPrint("total    = " + he_bytes_to_string(total));
		heDebugPrint("total    = " + he_bytes_to_string(heMemoryGetUsage()));
		heDebugPrint("=== MEMORY ===");
	}
};

b8 heDebugIsInfoRequested(const HeDebugInfoFlags flag) {
    b8 set = heDebugInfo.flags & flag;
    heDebugInfo.flags = heDebugInfo.flags & ~flag;
    return set;
};

void heDebugSetOutput(std::ostream* stream) {
    heDebugInfo.stream = stream;
};

void heDebugPrint(const std::string& message) {
    std::string output;
    output.append(message);
    output.push_back('\n');
    
    if (heDebugInfo.stream == nullptr) {
		heConsolePrint(output);
        std::cout << output;
        std::cout.flush();
    } else {
        heConsolePrint(output);
        *heDebugInfo.stream << output;
        heDebugInfo.stream->flush();
    }
};


std::string he_to_string(HeLightSourceType const& type) {
    std::string s;
    
    switch(type) {
        case HE_LIGHT_SOURCE_TYPE_DIRECTIONAL:
        s = "directional";
        break;
        
        case HE_LIGHT_SOURCE_TYPE_POINT:
        s = "point";
        break;
        
        case HE_LIGHT_SOURCE_TYPE_SPOT:
        s = "spot";
        break;
    }
    
    return s;
};

std::string he_to_string(HePhysicsShape const& type) {
    std::string s;
    
    switch(type) {
        case HE_PHYSICS_SHAPE_CONVEX_MESH:
        s = "convex";
        break;
        
        case HE_PHYSICS_SHAPE_CONCAVE_MESH:
        s = "concave";
        break;
        
        case HE_PHYSICS_SHAPE_BOX:
        s = "box";
        break;
        
        case HE_PHYSICS_SHAPE_SPHERE:
        s = "sphere";
        break;
        
        case HE_PHYSICS_SHAPE_CAPSULE:
        s = "capsule";
        break;
    };
    
    return s;
};

std::string he_to_string(HeColourFormat const& type) {
    std::string s;
    
    switch(type) {
        case HE_COLOUR_FORMAT_RGB8:
        s = "rgb8";
        break;
        
        case HE_COLOUR_FORMAT_RGBA8:
        s = "rgba8";
        break;
        
        case HE_COLOUR_FORMAT_RGB16:
        s = "rgb16";
        break;
        
        case HE_COLOUR_FORMAT_RGBA16:
        s = "rgba16";
        break;
    };
    
    return s;
};

std::string he_to_string(HeMemoryType const& type) {
	std::string s;

	switch(type) {
	case HE_MEMORY_TYPE_TEXTURE:
		s = "textures";
		break;

	case HE_MEMORY_TYPE_VAO:
		s = "vaos";
		break;

	case HE_MEMORY_TYPE_FBO:
		s = "fbos";
		break;
	}

	return s;
	
};


std::string he_to_string(HeShaderData const* ptr) {
    std::string data;
    
    switch(ptr->type) {
        case HE_DATA_TYPE_INT:
        case HE_DATA_TYPE_UINT:
        case HE_DATA_TYPE_BOOL:
        data = std::to_string(ptr->_int);
        break;
        
        case HE_DATA_TYPE_FLOAT:
        data = std::to_string(ptr->_float);
        break;
        
        case HE_DATA_TYPE_VEC2:
        data = hm::to_string(ptr->_vec2);
        break;
        
        case HE_DATA_TYPE_VEC3:
        data = hm::to_string(ptr->_vec3);
        break;
        
        case HE_DATA_TYPE_COLOUR:
        data = hm::to_string(ptr->_colour);
        break;
    }
    
    return data;
};


void he_to_string(HeTexture const* ptr, std::string& output, std::string const& prefix) {
	output += prefix + "HeTexture {\n";
#ifdef HE_ENABLE_NAMES
	output += prefix + "\tname = " + ptr->name + '\n';
#endif	
	output += prefix + "\treferences = " + std::to_string(ptr->referenceCount) + '\n';
	output += prefix + "\tid         = " + std::to_string(ptr->textureId)      + '\n';
	output += prefix + "\tformat     = " + he_to_string(ptr->format)           + '\n';
	output += prefix + "\tsize       = " + hm::to_string(ptr->size)            + '\n';
	output += prefix + "\tchannels   = " + std::to_string(ptr->channels)       + '\n';
	output += prefix + "\tcubeMap    = " + std::to_string(ptr->cubeMap)        + '\n';
	output += prefix + "\tmemory     = " + he_bytes_to_string(ptr->memory)     + '\n';
	output += prefix + "};\n";
};

void he_to_string(HeFboAttachment const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeFboAttachment {\n";
    if(ptr->texture)
        output += prefix + "\ttexture = " + std::to_string(ptr->id)      + '\n';
    else {
        output += prefix + "\tbuffer  = " + std::to_string(ptr->id)      + '\n';
        output += prefix + "\tsamples = " + std::to_string(ptr->samples) + '\n';
    }
    output += prefix + "\tformat  = " + he_to_string(ptr->format)        + '\n';
    output += prefix + "\tsize    = " + hm::to_string(ptr->size)         + '\n';
	output += prefix + "\tmemory  = " + he_bytes_to_string(ptr->memory)  + '\n';
    output += prefix + "};\n";
};

void he_to_string(HeFbo const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeFbo {\n";
#ifdef HE_ENABLE_NAMES
    output += prefix + "\tname    = " + ptr->name + '\n';
#endif
    output += prefix + "\tid      = " + std::to_string(ptr->fboId) + '\n';
    output += prefix + "\tsize    = " + hm::to_string(ptr->size) + '\n';
	std::string depth;
	he_to_string(&ptr->depthAttachment, depth, prefix + '\t');
	output += prefix + "\tdepth   = " + depth + '\n';
    //output += prefix + "\tmemory  = " + std::to_string(ptr->memory) + '\n';

	output += prefix + "\tattachments:\n";
    for(auto const& all : ptr->colourAttachments)
        he_to_string(&all, output, prefix + "\t\t");
    
    output += prefix + "};\n";
};

void he_to_string(HeD3LightSource const* ptr, std::string& output, std::string const& prefix) {
    std::string type = he_to_string(ptr->type);;
    std::string vectorName = (ptr->type == HE_LIGHT_SOURCE_TYPE_DIRECTIONAL) ? "direction   " : "position    ";
    
    std::string dataString;
    if(ptr->type == HE_LIGHT_SOURCE_TYPE_POINT)
        dataString += "\tlight_values = " + std::to_string(ptr->data[0]) + "/" + std::to_string(ptr->data[1]) + "/" + std::to_string(ptr->data[2]);
    else if(ptr->type == HE_LIGHT_SOURCE_TYPE_SPOT) {
        dataString += "\tdirection    = " + hm::to_string(hm::vec3f(ptr->data[0], ptr->data[1], ptr->data[2])) + "\n";
        dataString += "\tlight circle = " + std::to_string(ptr->data[3]) + " / " + std::to_string(ptr->data[4]) + "\n";
        dataString += "\tlight_values = " + std::to_string(ptr->data[5]) + "/" + std::to_string(ptr->data[6]) + "/" + std::to_string(ptr->data[7]);
    }
    
    std::string active = ((ptr->active) ? "true" : "false");
    output += prefix + "HeD3LightSource {\n";
    output += prefix + "\tactive       = " + active + "\n";
    output += prefix + "\ttype         = " + type + "\n";
    output += prefix + "\tcolour       = " + hm::to_string(ptr->colour) + "\n";
    output += prefix + "\t" + vectorName + " = " + hm::to_string(ptr->vector) + "\n";
    output += prefix + dataString + "\n";
    output += prefix + "};\n";
};

void he_to_string(HeD3Instance const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeD3Instance {\n";
    
#ifdef HE_ENABLE_NAMES
    output += prefix + "\tname     = " + ptr->name + "\n";
    output += prefix + "\tmesh     = " + ptr->mesh->name + "\n";
#endif
    
    if(ptr->material != nullptr)
        he_to_string(ptr->material, output, prefix + '\t');
    output += prefix + "\tposition = " + hm::to_string(ptr->transformation.position) + "\n";
    output += prefix + "\trotation = " + hm::to_string(ptr->transformation.rotation) + "\n";
    output += prefix + "\tscale    = " + hm::to_string(ptr->transformation.scale) + "\n";
    
    if(ptr->physics)
        he_to_string(ptr->physics, output, prefix + '\t');
    
    output += prefix + "};\n";
};

void he_to_string(HeD3Camera const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeD3Camera {\n";
    output += prefix + "\tposition = " + hm::to_string(ptr->position) + "\n";
    output += prefix + "\trotation = " + hm::to_string(ptr->rotation) + "\n";
    output += prefix + "};\n";
};

void he_to_string(HePhysicsShapeInfo const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsShapeInfo {\n";
    output += prefix + "\tshape       = " + he_to_string(ptr->type) + "\n";
    
    if(ptr->type == HE_PHYSICS_SHAPE_BOX)
        output += prefix + "\thalf-axis   = " + hm::to_string(ptr->box) + '\n';
    else if(ptr->type == HE_PHYSICS_SHAPE_SPHERE)
        output += prefix + "\tradius      = " + std::to_string(ptr->sphere) + '\n';
    else if(ptr->type == HE_PHYSICS_SHAPE_CAPSULE)
        output += prefix + "\tradii       = " + hm::to_string(ptr->capsule) + '\n';
    
    output += prefix + "\tmass        = " + std::to_string(ptr->mass) + '\n';
    output += prefix + "\tfriction    = " + std::to_string(ptr->friction) + '\n';
    output += prefix + "\trestitution = " + std::to_string(ptr->restitution) + '\n';
    output += prefix + "};\n";
};

void he_to_string(HePhysicsActorInfo const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsActorInfo {\n";
    output += prefix + "\tstepHeight = " + std::to_string(ptr->stepHeight) + '\n';
    output += prefix + "\tjumpHeight = " + std::to_string(ptr->jumpHeight) + '\n';
    output += prefix + "\teyeOffset  = " + std::to_string(ptr->eyeOffset) + '\n';
    output += prefix + "};\n";
};

void he_to_string(HePhysicsComponent const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsComponent {\n";
    he_to_string(&ptr->shapeInfo, output, prefix + '\t');
    output += prefix + "};\n";
};

void he_to_string(HePhysicsActor const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsActor {\n";
    he_to_string(&ptr->shapeInfo, output, prefix + '\t');
    he_to_string(&ptr->actorInfo, output, prefix + '\t');
    output += prefix + "};\n";
};

void he_to_string(HeMaterial const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeMaterial {\n";
    output += prefix + "\ttype   = " + std::to_string(ptr->type) + '\n';
#ifdef HE_ENABLE_NAMES
    //output += prefix + "\tshader = " + ptr->shader->name + '\n';
    output += prefix + "\ttextures:\n";
    for(auto const& all : ptr->textures)
        output += prefix + "\t\t" + all.first + " = " + all.second->name + '\n';
#endif
    
    output += prefix + "\tuniforms:\n";
    for(auto const& all : ptr->uniforms)
        output += prefix + "\t\t" + all.first + " = " + he_to_string(&all.second) + '\n';
    
    output += prefix + "};\n";
};


std::string he_bytes_to_string(uint64_t const bytes) {
	std::string s;

	if(bytes < 1000)
		s = std::to_string(bytes) + "b";
	else if(bytes < 1000000)
		s = he_float_to_string(bytes / 1000.f, 1) + "kb";
	else if(bytes < 1000000000)
		s = he_float_to_string(bytes / 1000000.f, 1) + "mb";
	
	return s;
};

std::string he_float_to_string(float const _float, uint8_t const precision) {
	std::stringstream stream;
	stream << std::fixed << std::setprecision(precision) << _float;
	return stream.str();
};


b8 heCommandRun(std::string const& message) {
    b8 found = false;
    
    std::vector<std::string> args = heStringSplit(message, ' ');
    if(args[0] == "print") {
        
        // print debugging information
        if(args[1] == "lights") {
            heDebugRequestInfo(HE_DEBUG_INFO_LIGHTS);
            found = true;
        } else if(args[1] == "instances") {
            heDebugRequestInfo(HE_DEBUG_INFO_INSTANCES);
            found = true;
        } else if(args[1] == "camera") {
            heDebugRequestInfo(HE_DEBUG_INFO_CAMERA);
            found = true;
        } else if(args[1] == "errors") {
            uint32_t error;
            heDebugPrint("=== ERRORS ===");
            while((error = heGlErrorGet()) != 0)
                heDebugPrint("Error: " + std::to_string(error));
            heDebugPrint("=== ERRORS ===");
            found = true;
        } else if(args[1] == "engine") {
            heDebugRequestInfo(HE_DEBUG_INFO_ENGINE);
            found = true;
        } else if(args[1] == "memory") {
			heDebugRequestInfo(HE_DEBUG_INFO_MEMORY);
			found = true;
		} else if(args[1] == "textures") {
			heDebugPrint("=== TEXTURES ===");
			std::string string;
			for(auto const& all : heAssetPool.texturePool)
				he_to_string(&all.second, string);

			heDebugPrint(string);	
			heDebugPrint("=== TEXTURES ===");
			found = true;
		} else if(args[1] == "window") {
			heDebugPrint("=== WINDOW ===");

			heDebugPrint("Size: " + hm::to_string(heRenderEngine->window->windowInfo.size));
			
			heDebugPrint("=== WINDOW ===");
		}
		
    } else if(args[0] == "set") {
        // set some information
        if(args[1].compare(0, 9, "instances") == 0) {
            
            // array index
            const uint16_t id = std::stoi(args[1].substr(10, args[1].find(']')));
            HeD3Instance* instance = heD3LevelGetInstance(heD3Level, id);
            
            if(args[2] == "position") {
                //instance->transformation.position = hm::parseVec3f(args[3]);
                heD3InstanceSetPosition(instance, hm::parseVec3f(args[3]));
                found = true;
            } else if(args[2] == "rotation") {
                // check if euler (degerees) or quaternion
                size_t i = std::count(args[3].begin(), args[3].end(), '/');
                if(i == 2)
                    // euler angles
                    instance->transformation.rotation = hm::fromEulerDegrees(hm::parseVec3f(args[3]));
                else
                    // quaternion
                    instance->transformation.rotation = hm::parseQuatf(args[3]);
                if(instance->physics)
                    hePhysicsComponentSetTransform(instance->physics, instance->transformation);
                found = true;
            } else if(args[2] == "scale") {
                instance->transformation.scale = hm::parseVec3f(args[3]);
                found = true;
            }
            
        } else if(args[1].compare(0, 6, "lights") == 0) {
            
            // array index
            const uint16_t id = std::stoi(args[1].substr(7, args[1].find(']')));
            HeD3LightSource* light = heD3LevelGetLightSource(heD3Level, id);
            
            if(args[2] == "intensity") {
                light->colour.i = std::stof(args[3]);
                light->update = true;
                found = true;
            } else if(args[2] == "colour") {
                std::vector<std::string> colour = heStringSplit(args[3], '/');
                light->colour.r = std::stoi(colour[0]);
                light->colour.g = std::stoi(colour[1]);
                light->colour.b = std::stoi(colour[2]);
                light->update = true;
                found = true;
            }
            
        } else if(args[1] == "engine") {
            
            if(args[2] == "output") {
                // update output texture
                uint8_t tex = std::stoi(args[3]);
                heRenderEngine->outputTexture = tex;
                found = true;
            }
            
        } else if(args[1] == "actor") {
            // update actors settings
            HePhysicsActor* actor = heD3Level->physics.actor;
            if(args[2] == "position") {
                hePhysicsActorSetEyePosition(actor, hm::parseVec3f(args[3]));
                found = true;
            } else if(args[2] == "jumpHeight") {
                hePhysicsActorSetJumpHeight(actor, std::stof(args[3]));
                found = true;
            }
        }
        
    } else if(args[0] == "enable") {
        
        // enable something in the engine
        if(args[1] == "physics") {
            if(args[2] == "draw") {
                // enable debug draw
                hePhysicsLevelEnableDebugDraw(&heD3Level->physics, heRenderEngine);
                found = true;
            }
            
        }
    } else if(args[0] == "disable") {
        
        if(args[1] == "physics") {
            if(args[2] == "draw") {
                // disbale debug draw
                hePhysicsLevelDisableDebugDraw(&heD3Level->physics);
                found = true;
            }
        }
        
    } else if(args[0] == "binary") {
        
        if(args[1] == "instance") {
            // convert file to binary
            heBinaryConvertD3InstanceFile(args[2], args[3]);
            found = true;
        }
        
        if(args[1] == "instances") {
            // convert folder
            std::string folder = args[2];
            std::vector<std::string> files;
            heWin32FolderGetFiles(folder, files, false);
            heWin32FolderCreate(folder + "/bin/");
            for(auto const& all : files) {
                HE_LOG("File: " + all);
                size_t pathEnd = all.find_last_of('/');
                heBinaryConvertD3InstanceFile(all, all.substr(0, pathEnd) + "/bin/" + all.substr(pathEnd + 1));
            }
            found = true;
        }
        
        if(args[1] == "texture") {
            // convert texture to binary
            heBinaryConvertTexture(args[2], args[3]);
            found = true;
        }
        
        if(args[1] == "textures") {
            // convert folder
            std::string folder = args[2];
            size_t folderLength = folder.size();
            std::vector<std::string> files;
            heWin32FolderGetFiles(folder, files, true);
            heWin32FolderCreate(folder + "/bin/");
            for(auto& all : files) {
                HE_LOG("File: " + all);
                std::string out = folder + "/bin/" + all.substr(folderLength);
                size_t pathEnd = out.find_last_of('/');
                heWin32FolderCreate(out.substr(0, pathEnd));
                heBinaryConvertTexture(all, out);
            }
            found = true;
        }
    }
    
    return found;
};

void heCommandThread(b8* running) {
    std::string line;
    while((running == nullptr || *running) && std::getline(std::cin, line))
		if(!line.empty())
			if(!heCommandRun(line))
				HE_DEBUG("Invalid command!");
};

void heErrorsPrint() {
    uint32_t error;
    while((error = heGlErrorGet()) != 0)
        HE_WARNING("GL Error: " + std::to_string(error));
};

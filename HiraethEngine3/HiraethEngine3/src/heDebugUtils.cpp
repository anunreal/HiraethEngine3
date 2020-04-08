#include "heDebugUtils.h"
#include "heD3.h"
#include "heCore.h"
#include "heUtils.h"
#include "heRenderer.h"

HeDebugInfo heDebugInfo;

void heDebugRequestInfo(const HeDebugInfoFlags flags) {
    
    //heDebugInfo.flags |= flags;
    // print debug information
    
    if(flags == HE_DEBUG_INFO_LIGHTS) {
        heDebugPrint("=== LIGHTS === ");
        for(const auto& all : heD3Level->lights) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint(string);
        }
        
        heDebugPrint("=== LIGHTS ===");
    }
    
    if(flags == HE_DEBUG_INFO_INSTANCES) {
        heDebugPrint("=== INSTANCES ===");
        for(const auto& all : heD3Level->instances) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint(string);
        }
        
        heDebugPrint("=== INSTANCES ===");
    }
    
    if(flags & HE_DEBUG_INFO_CAMERA) {
        heDebugPrint("=== CAMERA ===");
        std::string string;
        he_to_string(&heD3Level->camera, string);
        heDebugPrint(string);
        heDebugPrint("=== CAMERA ===");
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
        std::cout << output;
        std::cout.flush();
    } else {
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
    output += prefix + "\tshader   = " + ptr->material->shader->name + "\n";
#endif
    
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

void he_to_string(HePhysicsComponent const* ptr, std::string& output, std::string const& prefix) {
    
    HePhysicsInfo const& info = ptr->info;
    
    output += prefix + "HePhysicsComponent {\n";
    output += prefix + "\tshape       = " + he_to_string(info.type) + "\n";
    
    if(info.type == HE_PHYSICS_SHAPE_BOX)
        output += prefix + "\thalf-axis   = " + hm::to_string(info.box) + '\n';
    else if(info.type == HE_PHYSICS_SHAPE_SPHERE)
        output += prefix + "\tradius      = " + std::to_string(info.sphere) + '\n';
    else if(info.type == HE_PHYSICS_SHAPE_CAPSULE)
        output += prefix + "\tradii       = " + hm::to_string(info.capsule) + '\n';
    
    output += prefix + "\tmass        = " + std::to_string(info.mass) + '\n';
    output += prefix + "\tfriction    = " + std::to_string(info.friction) + '\n';
    output += prefix + "\trestitution = " + std::to_string(info.restitution) + '\n';
    output += prefix + "};\n";
    
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
                found = true;
            } else if(args[2] == "scale") {
                instance->transformation.scale = hm::parseVec3f(args[3]);
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
    }
    
    return found;
    
};

void heCommandThread(b8* running) {
    
    std::string line;
    while((running == nullptr || *running) && std::getline(std::cin, line))
        if(!heCommandRun(line))
        HE_DEBUG("Invalid command!");
    
};
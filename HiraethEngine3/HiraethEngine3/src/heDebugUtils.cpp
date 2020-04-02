#include "heDebugUtils.h"
#include "heD3.h"
#include "heCore.h"
#include "heUtils.h"

void he_to_string(const HeD3LightSource* ptr, std::string& output) {
    
    std::string type;
    switch(ptr->type) {
        case HE_LIGHT_SOURCE_TYPE_DIRECTIONAL:
        type = "directional";
        break;
        
        case HE_LIGHT_SOURCE_TYPE_POINT:
        type = "point";
        break;
        
        case HE_LIGHT_SOURCE_TYPE_SPOT:
        type = "spot";
        break;
    }
    
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
    output += "HeD3LightSource {\n";
    output += "\tactive       = " + active + "\n";
    output += "\ttype         = " + type + "\n";
    output += "\tcolour       = " + hm::to_string(ptr->colour) + "\n";
    output += "\t" + vectorName + " = " + hm::to_string(ptr->vector) + "\n";
    output += dataString + "\n";
    output += "};\n";
    
};

void he_to_string(const HeD3Instance* ptr, std::string& output) {
    output += "HeD3Instance {\n";
    
#ifdef HE_ENABLE_NAMES
    output += "\tname     = " + ptr->name + "\n";
    output += "\tmesh     = " + ptr->mesh->name + "\n";
    output +=" \tshader   = " + ptr->material->shader->name + "\n";
#endif
    
    output += "\tposition = " + hm::to_string(ptr->transformation.position) + "\n";
    output += "\trotation = " + hm::to_string(ptr->transformation.rotation) + "\n";
    output += "\tscale    = " + hm::to_string(ptr->transformation.scale) + "\n";
    output += "};\n";
    
};

void he_to_string(const HeD3Camera* ptr, std::string& output) {
    
    output += "HeD3Camera {\n";
    output += "\tposition = " + hm::to_string(ptr->position) + "\n";
    output += "\trotation = " + hm::to_string(ptr->rotation) + "\n";
    output += "};\n";
    
};


b8 heCommandRun(const std::string& message) {
    
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
        }
    } else if(args[0] == "set") {
        // set some information
        if(args[1].compare(0, 9, "instances") == 0) {
            // array index
            const uint16_t id = std::stoi(args[1].substr(10, args[1].find(']')));
            auto it = heD3LevelGetActive()->instances.begin();
            std::advance(it, id);
            HeD3Instance* instance = &(*it); // check for crash
            
            if(args[2] == "position") {
                instance->transformation.position = hm::parseVec3f(args[3]);
                found = true;
            } else if(args[2] == "rotation") {
                // check if euler (degerees) or quaternion
                //size_t i = args[3].count('/');
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
    }
    
    return found;
    
};

void heCommandThread(b8* running) {
    
    std::string line;
    while((running == nullptr || *running) && std::getline(std::cin, line))
        if(!heCommandRun(line))
        HE_DEBUG("Invalid command!");
    
};
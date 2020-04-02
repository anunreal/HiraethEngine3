#include "heDebugUtils.h"
#include "heD3.h"

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
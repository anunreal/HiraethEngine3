#include "heD3.h"


HeD3LightSource* heCreateDirectionalLight(HeD3Level* level, const hm::vec3f& direction, const hm::colour& colour) {
    
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_DIRECTIONAL;
    light->vector          = direction;
    light->colour          = colour;
    light->update          = true;
    return light;
    
};


HeD3LightSource* heCreateSpotLight(HeD3Level* level, const hm::vec3f& position, const hm::vec3f& direction, const float inAngle, const float outAngle, const hm::colour& colour) {
    
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_SPOT;
    light->vector          = position;
    light->colour          = colour;
    light->update          = true;
    light->data[0]         = direction.x;
    light->data[1]         = direction.y;
    light->data[2]         = direction.z;
    light->data[3]         = std::cos(hm::to_radians(inAngle));
    light->data[4]         = std::cos(hm::to_radians(outAngle));
    return light;
    
};

HeD3LightSource* heCreatePointLight(HeD3Level* level, const hm::vec3f& position, const float constLightValue, const float linearLightValue, const float quadraticLightValue, const hm::colour& colour) {
    
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_POINT;
    light->vector          = position;
    light->colour          = colour;
    light->update          = true;
    light->data[0]         = constLightValue;
    light->data[1]         = linearLightValue;
    light->data[2]         = quadraticLightValue;
    return light;
    
};

void heRemoveD3Instance(HeD3Level* level, HeD3Instance* instance) {
    
    unsigned int index = 0;
    for(HeD3Instance& all : level->instances) {
        if(&all == instance)
            break;
        index++;
    }
    
    auto it = level->instances.begin();
    std::advance(it, index);
    level->instances.erase(it);
    
};
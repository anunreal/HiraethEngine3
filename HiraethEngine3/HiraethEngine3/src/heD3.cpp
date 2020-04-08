#include "heD3.h"

HeD3Level* heD3Level = nullptr;


HeD3LightSource* heD3LightSourceCreateDirectional(HeD3Level* level, hm::vec3f const& direction, hm::colour const& colour) {
    
    HeD3LightSource* light = &level->lights.emplace_back();
    light->type            = HE_LIGHT_SOURCE_TYPE_DIRECTIONAL;
    light->vector          = direction;
    light->colour          = colour;
    light->update          = true;
    return light;
    
};


HeD3LightSource* heD3LightSourceCreateSpot(HeD3Level* level, hm::vec3f const& position, hm::vec3f const& direction, float const inAngle, float const outAngle, float const constLightValue, float const linearLightValue, float const quadraticLightValue, hm::colour const& colour) {
    
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
    light->data[5]         = constLightValue;
    light->data[6]         = linearLightValue;
    light->data[7]         = quadraticLightValue;
    return light;
    
};

HeD3LightSource* heD3LightSourceCreatePoint(HeD3Level* level, hm::vec3f const& position, float const constLightValue, float const linearLightValue, float const quadraticLightValue, hm::colour const& colour) {
    
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


void heD3InstanceUpdate(HeD3Instance* instance) {
    
    // update physics
    if(instance->physics) {
        instance->transformation.position = hePhysicsComponentGetPosition(instance->physics);
        instance->transformation.rotation = hePhysicsComponentGetRotation(instance->physics);
    }
    
};

void heD3InstanceSetPosition(HeD3Instance* instance, hm::vec3f const& position) {
    
    instance->transformation.position = position;
    if(instance->physics)
        hePhysicsComponentSetPosition(instance->physics, position);
    
};


void heD3LevelRemoveInstance(HeD3Level* level, HeD3Instance* instance) {
    
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

void heD3LevelUpdate(HeD3Level* level) {
    
    // update all instances
    for(auto& all : level->instances)
        heD3InstanceUpdate(&all);
    
};

HeD3Instance* heD3LevelGetInstance(HeD3Level* level, uint16_t const index) {
    
    auto begin = level->instances.begin();
    std::advance(begin, index);
    return &(*begin);
    
};

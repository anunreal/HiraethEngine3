#ifndef HE_D3_H
#define HE_D3_H

#include "heAssets.h"
#include <list>

struct HeD3Transformation {
    hm::vec3f position;
    hm::vec3f scale;
    hm::quatf rotation;
    
    HeD3Transformation() : position(0.f), scale(1.f), rotation(0.f, 0.f, 0.f, 1.f) {};
};

struct HeD3Instance {
    // pointer to a vao in the asset pool
    HeVao* mesh = nullptr;
    // pointer to a material in the asset pool
    HeMaterial* material = nullptr;
    HeD3Transformation transformation;
    // a list of indices from the levels light list that apply to this instance.
    // This list should be updated whenever a light or this instance is moved. The size of this list is
    // determined by the light count in the render engine
    std::vector<unsigned int> lightIndices;
};

struct HeD3Camera {
    // world space position
    hm::vec3f position;
    // rotation in degree euler angles
    hm::vec3f rotation;
    // the view matrix of this camera. Must be updated manually
    hm::mat4f viewMatrix;
    // the projection matrix of this camera created when the camera is set up
    hm::mat4f projectionMatrix;
};

struct HeD3LightSource {
    HeLightSourceType type = HE_LIGHT_SOURCE_TYPE_NONE;
    // the vector of this light. For point/spot lights this is the position in world space,
    // for directional light this is the direction
    hm::vec3f vector;
    // the colour of this light in rgb range (0:255)
    hm::colour colour;
    // additional data for this light, dependant on the type
    // PointLight: stores information about how the light acts in the distance
    //  0: the constant light value
    //  1: the linear light value
    //  2: the quadractic light value
    // SpotLight:
    //  0 - 2: a vector representing the direction of the light (normalized)
    //  3: the radius of the inner light circle, as cosinus
    //  4: the radius of the outer light circle, as cosinus
    //  5: the constant light value
    //  6: the linear light value
    //  7: the quadractic light value
    float data[8];
    // this should be true whenever this light is manipulated by the client. When update is true, the light
    // will be uploaded to the shader in the next frame
    b8 update = false;
    // whether this light is active at all. This should only be set to false if a light is off for a larger amount
    // of time
    b8 active = true;
};

struct HeD3Level {
    HeD3Camera camera;
    std::list<HeD3Instance> instances;
    std::list<HeD3LightSource> lights;
    double time = 0.0;
};


// sets all important information for a directional light for a new light source in level.
// direction should be normalized and in world space
extern HeD3LightSource* heD3LightSourceCreateDirectional(HeD3Level* level, const hm::vec3f& direction, const hm::colour& colour);
// sets all important information for a spot light for a new light source in level.
// position is in world space
// direction of the light is in world space
// inAngle and outAngle are the angles in degrees that light is spreaded. In the inner circle, everything will
// be lit with full light, and then its interpolated to zero to outAngle.
extern HeD3LightSource* heD3LightSourceCreateSpot(HeD3Level* level, const hm::vec3f& position, const hm::vec3f& direction, const float inAngle, const float outAngle,  const hm::colour& colour);
// sets all important information for a point light for a new source in level.
// position is in world space
// the three light values control how the light acts over distance.
// the constLightValue should always be 1
// the linear value controls the normal decay, can be around 0.045
// the quadratic value controls the light in far distance, can be around 0.0075
extern HeD3LightSource* heD3LightSourceCreatePoint(HeD3Level* level, const hm::vec3f& position, const float constLightValue, const float linearLightValue, const float quadraticLightValue, const hm::colour& colour);
// removes the HeD3Instance that instance points to from the level, if it does exist there and instance is a
// valid pointer.
extern void heD3LevelRemoveInstance(HeD3Level* level, HeD3Instance* instance);

#endif
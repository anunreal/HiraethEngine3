#ifndef HE_D3_H
#define HE_D3_H

#include "heAssets.h"
#include "hePhysics.h"
#include <list>

struct HeD3Transformation {
    hm::vec3f position;
    hm::vec3f scale;
    hm::quatf rotation;
    
    HeD3Transformation() : position(0.f), scale(1.f), rotation(0.f, 0.f, 0.f, 1.f) {};
};

struct HeD3Instance {
    // pointer to a vao in the asset pool
    HeVao* mesh                 = nullptr;
    // pointer to a material in the asset pool
    HeMaterial* material        = nullptr;
    // (possibly) a pointer to a member of the component list in the HeD3Level's physics level
    HePhysicsComponent* physics = nullptr;
    // world space transformation of this instance
    HeD3Transformation transformation;
    // a list of indices from the levels light list that apply to this instance.
    // This list should be updated whenever a light or this instance is moved. The size of this list is
    // determined by the light count in the render engine
    std::vector<uint32_t> lightIndices;
    
#ifdef HE_ENABLE_NAMES
    std::string name;
#endif
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
    float data[8] = { 0 };
    // this should be true whenever this light is manipulated by the client. When update is true, the light
    // will be uploaded to the shader in the next frame
    b8 update = false;
    // whether this light is active at all. This should only be set to false if a light is off for a larger amount
    // of time
    b8 active = true;
};

struct HeD3Skybox {
    HeTexture* specular = nullptr;
    HeTexture* irradiance = nullptr;
};

struct HeD3Level {
    HeD3Camera camera;
    HeD3Skybox skybox;
    HePhysicsLevel physics;
    std::list<HeD3Instance> instances;
    std::list<HeD3LightSource> lights;
    double time = 0.0;
};

// the current active d3 level. Can be set and (then) used at any time
extern HeD3Level* heD3Level;

// sets all important information for a directional light for a new light source in level.
// direction should be normalized and in world space
extern HE_API HeD3LightSource* heD3LightSourceCreateDirectional(HeD3Level* level, hm::vec3f const& direction, hm::colour const& colour);
// sets all important information for a spot light for a new light source in level.
// position is in world space
// direction of the light is in world space
// inAngle and outAngle are the angles in degrees that light is spreaded. In the inner circle, everything will
// be lit with full light, and then its interpolated to zero to outAngle.
extern HE_API HeD3LightSource* heD3LightSourceCreateSpot(HeD3Level* level, hm::vec3f const& position, hm::vec3f const& direction, float const inAngle, float const outAngle,  float const constLightValue, float const linearLightValue, float const quadraticLightValue, hm::colour const& colour);
// sets all important information for a point light for a new source in level.
// position is in world space
// the three light values control how the light acts over distance.
// the constLightValue should always be 1
// the linear value controls the normal decay, can be around 0.045
// the quadratic value controls the light in far distance, can be around 0.0075
extern HE_API HeD3LightSource* heD3LightSourceCreatePoint(HeD3Level* level, hm::vec3f const& position, float const constLightValue, float const linearLightValue, float const quadraticLightValue, hm::colour const& colour);
// removes the HeD3Instance that instance points to from the level, if it does exist there and instance is a
// valid pointer.
extern HE_API void heD3LevelRemoveInstance(HeD3Level* level, HeD3Instance* instance);
// updates all instances in given level (components). This should be called after the physics update (positions will be updated)
// but before rendering
extern HE_API void heD3LevelUpdate(HeD3Level* level);
// returns the instance with given index from the list of instances in the level
extern HE_API inline HeD3Instance* heD3LevelGetInstance(HeD3Level* level, uint16_t const index);
// returns the light source with given index from the list of lights in the level
extern HE_API inline HeD3LightSource* heD3LevelGetLightSource(HeD3Level* level, uint16_t const index);

// updates all components of this instance
extern HE_API void heD3InstanceUpdate(HeD3Instance* instance);
// sets the new position of the entity. This should always be used over directly setting the instances position as this will
// also update the components
extern HE_API void heD3InstanceSetPosition(HeD3Instance* instance, hm::vec3f const& position);

// loads a skybox from given hdr image. This will unmap the equirectangular image onto a cube map and also create a blurred
// irradiance map. This will load, run and destroy a compute shader and also create the cube maps
extern HE_API void heD3SkyboxCreate(HeD3Skybox* skybox, std::string const& hdrFile);

#endif
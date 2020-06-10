#ifndef HE_D3_H
#define HE_D3_H

#include "heAssets.h"
#include "hePhysics.h"
#include "heUtils.h"
#include <list>

struct HeWindow; // forward declare to avoid include

struct HeD3Transformation {
    hm::vec3f position;
    hm::vec3f scale;
    hm::quatf rotation;
    
    HeD3Transformation() : position(0.f), scale(1.f), rotation(0.f, 0.f, 0.f, 1.f) {};
    HeD3Transformation(hm::vec3f const& pos) : position(pos), scale(1.f), rotation(0.f, 0.f, 0.f, 1.f) {};
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

struct HeD3ViewInformation {
    float nearPlane = 0.1f;
    float farPlane  = 1000.f;
    float fov       = 70.f;
};

struct HeD3Frustum {
    // the corners of this frustum, in the following order:
    // 0: top far right
    // 1: top far left
    // 2: bottom far right
    // 3: bottom far left
    // 4: top near right
    // 5: top near left
    // 6: bottom near right
    // 7: bottom near left
    hm::vec3f corners[8];
    // contains the two bounding points.
    // index 0 are the lowest coordinates in each direction (of the corners)
    // index 1 are the highest coordinates in each direction (of the corners)
    hm::vec3f bounds[2];
    HeD3ViewInformation viewInfo;
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
    // information about the view frustum
    HeD3Frustum frustum;

    float exposure  = 1.f; // default exposure (brightness of the scene)
    float gamma     = 1.f; // disable gamma correction
};

struct HeD3ShadowMap {
    HeFbo depthFbo;
    HeD3Frustum frustum; // frustum of the shadow map box
    hm::mat4f projectionMatrix; // the orthographic projection matrix used for rendering the shadow map
    hm::mat4f viewMatrix; // the view matrix used for rendering the shadow map
    hm::mat4f offsetMatrix; // maps coordinates in range [-1:1] (transform output) to uv range
    hm::mat4f shadowSpaceMatrix; // matrix to convert from world space to the texture map
    
    float     shadowDistance = 40.f; // the length of the shadow map. Objects farther away from the camera do not cast shadows
    float     offset         = 10.f;
    hm::vec2i resolution     = 4096;
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
    
    b8 castShadows = false;
    HeD3ShadowMap shadows;
};

struct HeD3Skybox {
    HeTexture* specular = nullptr;
    HeTexture* irradiance = nullptr;
};

struct HeParticle {
    HeD3Transformation transformation; // xy rotation is ignored for particles, only z works (axis into the display). The position is relative to the particle source
    hm::colour colour; // hdr colour
    float remaining = 0.f; // as long as this value is above 0, this particle is alive
    hm::vec3f velocity; // velocity of this particle, set at spawn
};

struct HeParticleEmitter {
    HeParticleEmitterType type;
    HeD3Transformation    transformation;
    HeRandom              random;
    
    float minSize  = 0.01f, maxSize  = 0.1f;
    float minTime  = 0.00f, maxTime  = 1.0f;
    float minSpeed = 0.50f, maxSpeed = 1.5f;
    hm::colour minColour = hm::colour(0), maxColour = hm::colour(255);
    
    union {
        float     sphere; // radius of the sphere 
        hm::vec3f box;   // half dimensions of the box in each xyz direction
    };

HeParticleEmitter() : box(0.f), type(HE_PARTICLE_EMITTER_TYPE_NONE) {};
};

struct HeParticleSource {
    HeSpriteAtlas* atlas         = nullptr;
    uint32_t       atlasIndex    = 0;
    uint32_t       particleCount = 0;

    HeParticleEmitter emitter;
    HeParticle*       particles  = nullptr; // a pointer to an array of particles, the size of which is specified in the particles create function
    float*            dataBuffer = nullptr; // a float pointer that contains all necessary instanced data for the particles, that is the transformation matrix, uvs and colours

    // gravity that pulls down all particles per update
    float    gravity                  = 9.81f;
    // how many new particles to spawn per update (or respawn dead ones). If weve already spawned that many particles in an update, the other dead ones remaing dead
    uint32_t maxNewParticlesPerUpdate = 1;
    // whether to use additive blending. Useful for when many particles are overlapping and these parts should be brighter, for example fire
    b8 additive = false;
    // whether these particles cast and receive shadows
    b8 enableShadows = true;
    
    // 4x4 transMat = 16
    // hdr colour   =  4
    // uvs          =  4
    static const uint32_t FLOATS_PER_PARTICLE = 24; // constant so its easier to change
};

struct HeD3Level {
    HeD3Camera camera;
    HeD3Skybox skybox;
    HePhysicsLevel physics;

    std::list<HeD3Instance> instances;
    std::list<HeD3LightSource> lights;
    std::list<HeParticleSource> particles;
    
    double time   = 0.0;   // time of the level, increased everytime the frame is rendered. 
    b8 freeCamera = false; // only important when physics are used. If this is true (with physics), the cameras position will not be updated from the physics actor but can be moved around freely by simply modifying its position

    std::string name;
};

// the current active d3 level. Can be set and (then) used at any time
extern HeD3Level* heD3Level;


// -- instance

// updates all components of this instance
extern HE_API void heD3InstanceUpdate(HeD3Instance* instance);
// sets the new position of the entity. This should always be used over directly setting the instances position
// as this will also update the components
extern HE_API void heD3InstanceSetPosition(HeD3Instance* instance, hm::vec3f const& position);


// -- frustum

// updates the corners of the view frustum from the given view matrix. nearPlane and farPlane should be the values
// used when creating the projection matrix (clip / render distance)
extern HE_API void heD3FrustumUpdate(HeD3Frustum* frustum, HeWindow* window, hm::vec3f const& position, hm::vec3f const& rotation);
// updates the corners of the view frustum from the given view matrix. nearPlane and farPlane should be the values
// used when creating the projection matrix (clip / render distance). This will transform all corners with given
// matrix which is useful if youre not working in world space (i.e. shadow box)
extern HE_API void heD3FrustumUpdateWithMatrix(HeD3Frustum* frustum, HeWindow* window, hm::vec3f const& position, hm::vec3f const& rotation, hm::mat4f const& matrix);


// -- camera

// loops the yaw of the camera to be in between 0 and 360 (degrees) and the pitch to not go over 90 (or -90)
extern HE_API void heD3CameraClampRotation(HeD3Camera* camera);


// -- light

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
// sets up a shadow map. The resolution must already be set before this call
extern HE_API void heD3ShadowMapCreate(HeD3ShadowMap* shadowMap, HeD3LightSource* source);


// -- level

// removes the HeD3Instance that instance points to from the level, if it does exist there and instance is a
// valid pointer.
extern HE_API void heD3LevelRemoveInstance(HeD3Level* level, HeD3Instance* instance);
// updates all instances in given level (components). This should be called after the physics update (positions
// will be updated) but before rendering
extern HE_API void heD3LevelUpdate(HeD3Level* level, float const delta);
// cleans up all instances and (if used) the physics of this level
extern HE_API void heD3LevelDestroy(HeD3Level* level);
// returns the instance with given index from the list of instances in the level
extern HE_API inline HeD3Instance* heD3LevelGetInstance(HeD3Level* level, uint16_t const index);
// returns the light source with given index from the list of lights in the level
extern HE_API inline HeD3LightSource* heD3LevelGetLightSource(HeD3Level* level, uint16_t const index);


// -- skybox

// loads a skybox from given hdr image. This will unmap the equirectangular image onto a cube map and also create
// a blurred irradiance map. This will load, run and destroy a compute shader and also create the cube maps
extern HE_API void heD3SkyboxCreate(HeD3Skybox* skybox, std::string const& hdrFile);
// loads the precomputed specular and diffuse textures for that skybox. The files must be in binres/textures/skybox
// and be named fileName_specular and fileName_irradiance respectively. 
extern HE_API void heD3SkyboxLoad(HeD3Skybox* skybox, std::string const& fileName);


// -- particles

// sets up a new particle source. The transformation is the sources location, the particles will be relative around
// that. The atlasIndex is the index of the sprite in the given atlas. particleCount is the size of the array
// storing all current particles.
extern HE_API void heParticleSourceCreate(HeParticleSource* source, HeD3Transformation const& transformation, HeSpriteAtlas* atlas, uint32_t const atlasIndex, uint32_t const particleCount);
// destroys the given particle source and frees all associated data
extern HE_API void heParticleSourceDestroy(HeParticleSource* source);
// assigns new data to the particle. The position will be in the emitter (bounding box dependant of type), the
// velocity will be set and a random colour is generated
extern HE_API void heParticleSourceSpawnParticle(HeParticleEmitter* source, HeParticle* particle);
// updates the particle source by updating the position of the particles (velocity, gravity) and spawning new
// particles for the dead ones
extern HE_API void heParticleSourceUpdate(HeParticleSource* source, float const delta, HeD3Level* level);

#endif

#ifndef HE_PHYSICS_H
#define HE_PHYSICS_H

#include "heTypes.h"
#include "hm/hm.hpp"

// forward declare all necessary bullet classes to avoid including the files in this header
class btCollisionShape;
class btRigidBody;
class btDefaultMotionState;
class btKinematicCharacterController;
class btPairCachingGhostObject;
class btConvexShape;
class btTransform;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btTriangleMesh;

struct HeD3Transformation;
struct HeRenderEngine;

struct HePhysicsShapeInfo {
    // the primitive type of this shape
    HePhysicsShape type;
    // a mass of 0 makes this object static (immovable)
    float mass        = 0.f;
    // friction to the ground. The higher, the less it moves on other surfaces
    float friction    = 0.f;
    // bounciness of this shape on other surfaces. The higher the more it will bounce
    float restitution = 0.f;
    
    // additional information, depending on the type
    union {
        float sphere;      // radius of the sphere
        hm::vec3f box;     // half axis of the box (dimensions)
        hm::vec2f capsule; // total dimensions (width and height) of the capsule
    };
    
    // only set if this is a custom shape
    // if type is convex, all the points will be connected to a surface. That surface will be wrapped over the
    // given points if type is concave, the points will be connected to triangles. This is slower but useful if
    // there is a "hole" in the mesh that objects should be able to go through
    std::vector<hm::vec3f> mesh;
HePhysicsShapeInfo() : box(0.f), type(HE_PHYSICS_SHAPE_NONE) {};
};

struct HePhysicsActorInfo {
    // how high a character can step (in units)
    float stepHeight = 0.2f;
    // the force with which the character jumps. The actual jump height depents on the gravity
    float jumpHeight = 4.0f;
    // the offset of the camera from the bottom of the actors shape
    float eyeOffset  = 1.8f;
};

struct HePhysicsLevelInfo {
    hm::vec3f gravity;
    
HePhysicsLevelInfo() : gravity(0, -10, 0) {};
HePhysicsLevelInfo(hm::vec3f const& gravity) : gravity(gravity) {};
};

struct HePhysicsComponent {
    btCollisionShape*     shape  = nullptr;
    btRigidBody*          body   = nullptr;
    btDefaultMotionState* motion = nullptr;
    HePhysicsShapeInfo    shapeInfo;
};

struct HePhysicsActor {
    btKinematicCharacterController* controller = nullptr;
    btPairCachingGhostObject*       ghost      = nullptr;
    btConvexShape*                  shape      = nullptr;
    HePhysicsShapeInfo              shapeInfo;
    HePhysicsActorInfo              actorInfo;

    // -- read only
    
    hm::vec3f position;     // position of the shape (center)
    hm::vec3f feetPosition; // position of the feet (position - height / 2) 
    hm::vec3f velocity;     // the current velocity of the actor
    hm::quatf rotation;     // the current rotation of the actor
};

struct HePhysicsLevel {
    btDefaultCollisionConfiguration*     config     = nullptr;
    btCollisionDispatcher*               dispatcher = nullptr;
    btBroadphaseInterface*               broadphase = nullptr;
    btSequentialImpulseConstraintSolver* solver     = nullptr;
    btDiscreteDynamicsWorld*             world      = nullptr;
    b8 ghostPairCallbackSet = false;
    b8 enableDebugDraw      = false;    
    b8 setup                = false;
    
    HePhysicsLevelInfo info;
    HePhysicsActor* actor   = nullptr;
    std::list<HePhysicsComponent> components;
};

// -- shape

// checks the type of the info and tries to figure out the height of that shape. This only works if the shape
// is of primitive type
extern HE_API float hePhysicsShapeGetHeight(HePhysicsShapeInfo const* info);


// -- level

// creates a new empty physics level with default settings (gravity)
extern HE_API void hePhysicsLevelCreate(HePhysicsLevel* level, HePhysicsLevelInfo const& info);
// destroys a physics level (cleans up the pointers)
extern HE_API void hePhysicsLevelDestroy(HePhysicsLevel* level);
// adds a physics component to the level. The component must already be set up before this
extern HE_API void hePhysicsLevelAddComponent(HePhysicsLevel* level, HePhysicsComponent const* component);
// removes given component from the given level
extern HE_API void hePhysicsLevelRemoveComponent(HePhysicsLevel* level, HePhysicsComponent const* component);
// sets the actor for given level. One level can only have one actor (for now?). If actor is a nullptr, the
// current actor will be removed from the level
extern HE_API void hePhysicsLevelSetActor(HePhysicsLevel* level, HePhysicsActor* actor);
// updates given level. Delta is the time passed since the last update in seconds
extern HE_API void hePhysicsLevelUpdate(HePhysicsLevel* level, float const delta);
// enables a debug drawer for the collision shapes
extern HE_API void hePhysicsLevelEnableDebugDraw(HePhysicsLevel* level, HeRenderEngine* engine);
extern HE_API void hePhysicsLevelDisableDebugDraw(HePhysicsLevel* level);
// renders the debug information of this physics level if debug drawing is enabled. Should be called after the d3
// render
extern HE_API void hePhysicsLevelDebugDraw(HePhysicsLevel const* level);


// -- component

// creates a new physics component from a shape
extern HE_API void hePhysicsComponentCreate(HePhysicsComponent* component, HePhysicsShapeInfo const& shape);
// destroys given physics component. The component should be removed from all levels before this. This function
// simply deletes all pointers and other data allocated
extern HE_API void hePhysicsComponentDestroy(HePhysicsComponent* component);
// updates the position of given component
extern HE_API void hePhysicsComponentSetPosition(HePhysicsComponent* component, hm::vec3f const& position);
// sets the position, rotation and scale for given component
extern HE_API void hePhysicsComponentSetTransform(HePhysicsComponent* component, HeD3Transformation const& transform);
// returns the current position of the component
extern HE_API hm::vec3f hePhysicsComponentGetPosition(HePhysicsComponent const* component);
// returns the current rotation of the component
extern HE_API hm::quatf hePhysicsComponentGetRotation(HePhysicsComponent const* component);


// -- actor

// creates a new physics component from given shape
extern HE_API void hePhysicsActorCreate(HePhysicsActor* actor, HePhysicsShapeInfo const& shape, HePhysicsActorInfo const& actorInfo);
// destroys given actor. The actor should be removed from all levels before this. This function simply deletes
// all pointers and other data allocated
extern HE_API void hePhysicsActorDestroy(HePhysicsActor* actor);
// updates the position of given actor
extern HE_API void hePhysicsActorSetPosition(HePhysicsActor* actor, hm::vec3f const& position);
// sets the eye position of this actor. This will simply subtract the eye offset from the position and then set
// the new position
extern HE_API inline void hePhysicsActorSetEyePosition(HePhysicsActor* actor, hm::vec3f const& eyePosition);
// sets the velocity (walk direction) of this actor
extern HE_API inline void hePhysicsActorSetVelocity(HePhysicsActor* actor, hm::vec3f const& velocity);
// sets the rotation of this actor
extern HE_API inline void hePhysicsActorSetRotation(HePhysicsActor* actor, hm::quatf const& rotation);
// makes the given actor jump. The more force, the higher the actor will jump. If force is -1, the default force
// will be used
extern HE_API inline void hePhysicsActorJump(HePhysicsActor* actor);
// returns the current position of the actor
extern HE_API inline hm::vec3f hePhysicsActorGetPosition(HePhysicsActor const* actor);
// returns the position of the eyes of this actor. The eye position depends on the eye offset in the actor
// information. The eye position is usually where the camera should be placed
extern HE_API inline hm::vec3f hePhysicsActorGetEyePosition(HePhysicsActor const* actor);
// returns the rotation of that actor('s shape)1
extern HE_API inline hm::quatf hePhysicsActorGetRotation(HePhysicsActor const* actor);
// returns true if the actor currently stands on solid ground
extern HE_API inline b8 hePhysicsActorOnGround(HePhysicsActor const* actor);


// -- these functions simply update the actors settings

extern HE_API inline void hePhysicsActorSetJumpHeight(HePhysicsActor* actor, float const height);

#endif

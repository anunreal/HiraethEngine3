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
class btGhostPairCallback;

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
    float* mesh = nullptr;
    // the number of floats that represent the mesh (only used if mesh is set, convex or concave mesh)
    uint32_t floatCount = 0;

    // this can be used instead of the float pointer, if we want to store the vertices here.
    std::vector<hm::vec3f> meshVertices;
    
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

struct HePhysicsActorSimple {
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

struct HePhysicsActorCustom {
	btPairCachingGhostObject* ghostObject = nullptr;
	btConvexShape* shape = nullptr;  //is also in m_ghostObject, but it needs to be convex, so we store it here to avoid upcast
    btRigidBody* rigidBody = nullptr;
    btDefaultMotionState* motion = nullptr;
    hm::vec3f previousPosition;
    hm::vec3f manualVelocity;
    
    float verticalVelocity = 0.f;
    float verticalOffset   = 0.f;
    float fallSpeed        = 0.f;    
    float jumpSpeed        = 0.f;
    float deceleration     = .1f;

    b8 onGround = false;
    b8 hittingWall = false;
    
    HePhysicsShapeInfo shapeInfo;
    HePhysicsActorInfo actorInfo;

    // -- read only
    
    hm::vec3f position; // position of the shape (center)
    hm::vec3f velocity; // the current velocity of the actor
    hm::quatf rotation; // the current rotation of the actor
};

struct HePhysicsLevel {
    btDefaultCollisionConfiguration*     config     = nullptr;
    btCollisionDispatcher*               dispatcher = nullptr;
    btBroadphaseInterface*               broadphase = nullptr;
    btSequentialImpulseConstraintSolver* solver     = nullptr;
    btDiscreteDynamicsWorld*             world      = nullptr;
    btGhostPairCallback*                 ghostPair  = nullptr;
    b8 ghostPairCallbackSet = false;
    b8 enableDebugDraw      = false;    
    b8 setup                = false;
    
    HePhysicsLevelInfo info;
    HePhysicsActorSimple* actor   = nullptr;
    std::list<HePhysicsComponent> components;
};


// -- shape

// checks the type of the info and tries to figure out the height of that shape. This only works if the shape
// is of primitive type
extern HE_API float hePhysicsShapeGetHeight(HePhysicsShapeInfo const* info);


// -- component

// creates a new physics component from a shape
extern HE_API void hePhysicsComponentCreate(HePhysicsComponent* component, HePhysicsShapeInfo& shape);
// destroys given physics component. The component should be removed from all levels before this. This function
// simply deletes all pointers and other data allocated
extern HE_API void hePhysicsComponentDestroy(HePhysicsComponent* component);
// enables (simulated) rotation of this component only for the axis that the value is not zero. Rotation can still
// be set manually for that component, but wont be changed by collision
extern HE_API void hePhysicsComponentEnableRotation(HePhysicsComponent const* component, hm::vec3f const& axis);
// updates the position of given component
extern HE_API void hePhysicsComponentSetPosition(HePhysicsComponent* component, hm::vec3f const& position);
extern HE_API void hePhysicsComponentSetRotation(HePhysicsComponent* component, hm::vec3f const& rotation);
// sets the position, rotation and scale for given component
extern HE_API void hePhysicsComponentSetTransform(HePhysicsComponent* component, HeD3Transformation const& transform);
// sets the velocity for this physics component
extern HE_API void hePhysicsComponentSetVelocity(HePhysicsComponent* component, hm::vec3f const& velocity);
// returns the current position of the component
extern HE_API hm::vec3f hePhysicsComponentGetPosition(HePhysicsComponent const* component);
// returns the current rotation of the component
extern HE_API hm::quatf hePhysicsComponentGetRotation(HePhysicsComponent const* component);


// -- actor

// creates a new physics component from given shape
extern HE_API void hePhysicsActorSimpleCreate(HePhysicsActorSimple* actor, HePhysicsShapeInfo& shape, HePhysicsActorInfo const& actorInfo);
// destroys given actor. The actor should be removed from all levels before this. This function simply deletes
// all pointers and other data allocated
extern HE_API void hePhysicsActorSimpleDestroy(HePhysicsActorSimple* actor);
// updates the position of given actor
extern HE_API void hePhysicsActorSimpleSetPosition(HePhysicsActorSimple* actor, hm::vec3f const& position);
// sets the eye position of this actor. This will simply subtract the eye offset from the position and then set
// the new position
extern HE_API inline void hePhysicsActorSimpleSetEyePosition(HePhysicsActorSimple* actor, hm::vec3f const& eyePosition);
// sets the velocity (walk direction) of this actor
extern HE_API inline void hePhysicsActorSimpleSetVelocity(HePhysicsActorSimple* actor, hm::vec3f const& velocity);
// sets the rotation of this actor
extern HE_API inline void hePhysicsActorSimpleSetRotation(HePhysicsActorSimple* actor, hm::quatf const& rotation);
// makes the given actor jump. The more force, the higher the actor will jump. If force is -1, the default force
// will be used
extern HE_API inline void hePhysicsActorSimpleJump(HePhysicsActorSimple* actor);
// returns the current position of the actor
extern HE_API inline hm::vec3f hePhysicsActorSimpleGetPosition(HePhysicsActorSimple const* actor);
// returns the position of the eyes of this actor. The eye position depends on the eye offset in the actor
// information. The eye position is usually where the camera should be placed
extern HE_API inline hm::vec3f hePhysicsActorSimpleGetEyePosition(HePhysicsActorSimple const* actor);
// returns the rotation of that actor('s shape)1
extern HE_API inline hm::quatf hePhysicsActorSimpleGetRotation(HePhysicsActorSimple const* actor);
// returns true if the actor currently stands on solid ground
extern HE_API inline b8 hePhysicsActorSimpleOnGround(HePhysicsActorSimple const* actor);
// updates the actors jump height
extern HE_API inline void hePhysicsActorSimpleSetJumpHeight(HePhysicsActorSimple* actor, float const height);


// sets up this physics actor with given information.
extern HE_API void hePhysicsActorCustomCreate(HePhysicsActorCustom* actor, HePhysicsShapeInfo& shapeInfo, HePhysicsActorInfo const& actorInfo);
// updates the velocity and position of this actor. Should be called after the physics level was updated 
extern HE_API void hePhysicsActorCustomUpdate(HePhysicsActorCustom* actor, HePhysicsLevel* level, float const delta);
// updates the position of given actor
extern HE_API void hePhysicsActorCustomSetPosition(HePhysicsActorCustom* actor, hm::vec3f const& position);
extern HE_API void hePhysicsActorCustomSetVelocity(HePhysicsActorCustom* actor, hm::vec3f const& velocity);

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
extern HE_API void hePhysicsLevelSetActor(HePhysicsLevel* level, HePhysicsActorSimple* actor);
extern HE_API void hePhysicsLevelSetActor(HePhysicsLevel* level, HePhysicsActorCustom* actor);
// updates given level. Delta is the time passed since the last update in seconds
extern HE_API void hePhysicsLevelUpdate(HePhysicsLevel* level, float const delta);
// enables a debug drawer for the collision shapes
extern HE_API void hePhysicsEnableDebugDraw(HeRenderEngine* engine);
// disables debug drawing for all levels
extern HE_API void hePhysicsDisableDebugDraw();
// returns true if debug drawing is currently enabled
extern HE_API b8 hePhysicsDebugDrawingEnabled();
// renders the debug information of this physics level if debug drawing is enabled. Should be called after the d3
// render
extern HE_API void hePhysicsLevelDebugDraw(HePhysicsLevel* level);


#endif

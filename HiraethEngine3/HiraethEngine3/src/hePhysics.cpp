#include "hePhysics.h"
#include "GLEW/glew.h"
#include "GLEW/wglew.h"
#include "heD3.h"
#include "heCore.h"
#include "heRenderer.h"

#pragma warning(push, 0)
#include "btBulletDynamicsCommon.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "LinearMath/btDefaultMotionState.h"
#pragma warning(pop)


class HePhysicsDebugDrawer : public btIDebugDraw {
    public:
    int debugMode = 0;
    HeRenderEngine* engine = nullptr;
    
    void drawLine(btVector3 const& from, btVector3 const& to, btVector3 const& color) {
        heUiPushLineD3(engine, hm::vec3f(from.getX(), from.getY(), from.getZ()), hm::vec3f(to.getX(), to.getY(), to.getZ()), hm::colour(uint8_t(color.getX() * 255), uint8_t(color.getY() * 255), uint8_t(color.getZ() * 255)), 4);
    };
    
    virtual void drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
    	btVector3 to = pointOnB + normalOnB * distance;
        const btVector3& from = pointOnB;
        glColor4f(color.getX(), color.getY(), color.getZ(), 1.0f);
        
        drawLine(from, to, color);
    };
	
    virtual void reportErrorWarning(const char* warningString) {
        printf(warningString);
    };
	
    virtual void draw3dText(const btVector3& location, const char* textString) {};
    virtual void setDebugMode(int m) { debugMode = m; };
    virtual int getDebugMode() const { return debugMode; };
};

HePhysicsDebugDrawer hePhysicsDebugDrawer;

btCollisionShape* heCreateShape(HePhysicsInfo const& shape) {
    
    btCollisionShape* bt = nullptr;
    btTriangleMesh* mesh; // only used if concave mesh
    btConvexHullShape* s; // only used if convex mesh
    
    switch(shape.type) {
        case HE_PHYSICS_SHAPE_CONVEX_MESH:
        
        s = new btConvexHullShape();
        for(size_t i = 0; i < shape.mesh.size(); ++i) {
            hm::vec3f const& v = shape.mesh[i];
            s->addPoint(btVector3(v.x, v.y, v.z), true);
        }
        
        //s->addPoint(btVector3((shape->mesh.end() - 1)->x, (shape->mesh.end() - 1)->y, (shape->mesh.end() - 1)->z), true);
        bt = s;
        
        break;
        
        case HE_PHYSICS_SHAPE_CONCAVE_MESH:
        mesh = new btTriangleMesh();
        for(size_t i = 0; i < shape.mesh.size(); i += 3) {
            hm::vec3f const& v0 = shape.mesh[i];
            hm::vec3f const& v1 = shape.mesh[i + 1];
            hm::vec3f const& v2 = shape.mesh[i + 2];
            mesh->addTriangle(btVector3(v0.x, v0.y, v0.z), btVector3(v1.x, v1.y, v1.z), btVector3(v2.x, v2.y, v2.z));
        }
        
        bt = new btBvhTriangleMeshShape(mesh, false);
        break;
        
        case HE_PHYSICS_SHAPE_BOX:
        bt = new btBoxShape(btVector3(shape.box.x, shape.box.y, shape.box.z));
        break;
        
        case HE_PHYSICS_SHAPE_SPHERE:
        bt = new btSphereShape(shape.sphere);
        break;
        
        case HE_PHYSICS_SHAPE_CAPSULE:
        btScalar width(shape.capsule.x / 2.f);
        bt = new btCapsuleShape(width, shape.capsule.y - shape.capsule.x);
        break;
    }
    
    return bt;
    
};



void hePhysicsLevelCreate(HePhysicsLevel* level) {
    
    level->config     = new btDefaultCollisionConfiguration();
    level->dispatcher = new btCollisionDispatcher(level->config);
	btVector3 worldMin(-1000, -1000, -1000);
	btVector3 worldMax(1000, 1000, 1000);
	btAxisSweep3* sweepBP = new btAxisSweep3(worldMin, worldMax);
    level->broadphase = sweepBP;
    //level->broadphase = new btDbvtBroadphase();
    level->solver     = new btSequentialImpulseConstraintSolver();
    level->world      = new btDiscreteDynamicsWorld(level->dispatcher, level->broadphase, level->solver, level->config);
    level->world->setGravity(btVector3(0, -10, 0));
    level->world->getDispatchInfo().m_allowedCcdPenetration = 0.0001f;
    
};

void hePhysicsLevelDestroy(HePhysicsLevel* level) {
    
    delete level->world;
    level->world = nullptr;
    delete level->solver;
    level->solver = nullptr;
    delete level->broadphase;
    level->broadphase = nullptr;
    delete level->dispatcher;
    level->dispatcher = nullptr;
    delete level->config;
    level->config = nullptr;
    
};

void hePhysicsLevelAddComponent(HePhysicsLevel* level, const HePhysicsComponent* component) {
    
    level->world->addRigidBody(component->body);
    
};

void hePhysicsLevelRemoveComponent(HePhysicsLevel* level, const HePhysicsComponent* component) {
    
    level->world->removeRigidBody(component->body);
    
};

void hePhysicsLevelSetActor(HePhysicsLevel* level, HePhysicsActor* actor) {
    
    if(actor == nullptr) {
        // remove current actor
        level->world->removeCollisionObject(level->actor->ghost);
        level->world->removeAction(level->actor->controller);
        level->actor = nullptr;
    } else {
        if(!level->ghostPairCallbackSet) {
            level->broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
            level->ghostPairCallbackSet = true;
        }
        
        level->world->addCollisionObject(actor->ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
        level->world->addAction(actor->controller);
        level->actor = actor;
    }
    
};

void hePhysicsLevelUpdate(HePhysicsLevel* level, const float delta) {
    
    // seconds to ms
    level->world->stepSimulation(delta, 10);
    
};

void hePhysicsLevelEnableDebugDraw(HePhysicsLevel* level, HeRenderEngine* engine) {
    
    hePhysicsDebugDrawer.setDebugMode(btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawWireframe);
    hePhysicsDebugDrawer.engine = engine;
    level->world->setDebugDrawer(&hePhysicsDebugDrawer);
    level->enableDebugDraw = true;
    
};

void hePhysicsLevelDisableDebugDraw(HePhysicsLevel* level) {
    
    hePhysicsDebugDrawer.setDebugMode(0);
    level->world->setDebugDrawer(nullptr);
    level->enableDebugDraw = false;
    
};

void hePhysicsLevelDebugDraw(HePhysicsLevel const* level) {
    
    if(level->enableDebugDraw) {
        heEnableDepth(false);
        level->world->debugDrawWorld();
        heEnableDepth(true);
        glFinish();
    }
    
};

b8 hePhysicsLevelIsSetup(HePhysicsLevel const* level) { return level->world != nullptr; };


void hePhysicsComponentCreate(HePhysicsComponent* component, HePhysicsInfo const& info) {
    
    component->shape = heCreateShape(info);
    
    // create rigid body
    btScalar mass(info.mass);
    bool dynamic = (mass != 0.f);
    btVector3 localInertia(0, 0, 0);
    if(dynamic)
        component->shape->calculateLocalInertia(mass, localInertia);
    
    component->motion    = new btDefaultMotionState();
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, component->motion, component->shape, localInertia);
    rbInfo.m_friction    = info.friction;
    rbInfo.m_restitution = info.restitution;
    component->body      = new btRigidBody(rbInfo);
    component->info      = info;
    component->info.mesh.clear(); // save memory
    
};

void hePhysicsComponentDestroy(HePhysicsComponent* component) {
    
    delete component->motion;
    component->motion = nullptr;
    delete component->shape;
    component->shape = nullptr;
    delete component->body;
    component->body = nullptr;
    
};

void hePhysicsComponentSetPosition(HePhysicsComponent* component, hm::vec3f const& position) {
    
    btTransform transform = component->body->getWorldTransform();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    component->body->setWorldTransform(transform);
    component->motion->setWorldTransform(transform);
    component->body->forceActivationState(ACTIVE_TAG);
    component->body->activate();
    
};

void hePhysicsComponentSetTransform(HePhysicsComponent* component, HeD3Transformation const& transform) {
    
    btTransform bttransform = component->body->getWorldTransform();
    bttransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
    bttransform.setRotation(btQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w));
    component->shape->setLocalScaling(btVector3(transform.scale.x, transform.scale.y, transform.scale.z));
    component->body->setWorldTransform(bttransform);
    component->motion->setWorldTransform(bttransform);
    
};

hm::vec3f hePhysicsComponentGetPosition(HePhysicsComponent const* component) {
    
    btVector3 const& transform = component->body->getWorldTransform().getOrigin();
    return hm::vec3f(transform.getX(), transform.getY(), transform.getZ());
    
};

hm::quatf hePhysicsComponentGetRotation(HePhysicsComponent const* component) {
    
    btQuaternion const& transform = component->body->getWorldTransform().getRotation();
    return hm::quat(transform.getX(), transform.getY(), transform.getZ(), transform.getW());
    
};


void hePhysicsActorCreate(HePhysicsActor* actor, HePhysicsInfo const& info) {
    
    actor->ghost      = new btPairCachingGhostObject();
    actor->ghost->setWorldTransform(btTransform::getIdentity());
    actor->shape      = (btConvexShape*) heCreateShape(info);
    actor->ghost->setCollisionShape(actor->shape);
    actor->ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    actor->controller = new btKinematicCharacterController(actor->ghost, actor->shape, btScalar(.35f), btVector3(0, 1, 0));
    actor->info       = info;
    actor->info.mesh.clear(); // save memory
    
};

void hePhysicsActorDestroy(HePhysicsActor* actor) {
    
    delete actor->controller;
    actor->controller = nullptr;
    delete actor->shape;
    actor->shape = nullptr;
    delete actor->ghost;
    actor->ghost = nullptr;
    
};

void hePhysicsActorSetPosition(HePhysicsActor* actor, hm::vec3f const& position) {
    
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    actor->controller->warp(btVector3(position.x, position.y, position.z));
    actor->ghost->setWorldTransform(transform);
    
};

void hePhysicsActorSetVelocity(HePhysicsActor* actor, hm::vec3f const& velocity) {
    
    actor->controller->setWalkDirection(btVector3(velocity.x, velocity.y, velocity.z));
    
};

void hePhysicsActorJump(HePhysicsActor* actor, float const force) {
    
    actor->controller->jump();
    
};

hm::vec3f hePhysicsActorGetPosition(HePhysicsActor const* actor) {
    
    btVector3 origin = actor->ghost->getWorldTransform().getOrigin();
    return hm::vec3f(origin.getX(), origin.getY(), origin.getZ());
    
};

b8 hePhysicsActorOnGround(HePhysicsActor const* actor) {
    
    return actor->controller->onGround();
    
};
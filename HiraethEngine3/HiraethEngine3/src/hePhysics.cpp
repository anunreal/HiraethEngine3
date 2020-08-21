#include "hepch.h"
#include "hePhysics.h"
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
    
    virtual void drawContactPoint(btVector3 const& pointOnB, btVector3 const& normalOnB, btScalar distance, int lifeTime, btVector3 const& color) {
        btVector3 to = pointOnB + normalOnB * distance;
        const btVector3& from = pointOnB;
        drawLine(from, to, color);
    };
    
    virtual void reportErrorWarning(char const* warningString) {
        printf(warningString);
    };
    
    virtual void draw3dText(btVector3 const& location, char const* textString) {};
    virtual void setDebugMode(int m) { debugMode = m; };
    virtual int getDebugMode() const { return debugMode; };
};

class IgnoreBodyAndGhostCast : public btCollisionWorld::ClosestRayResultCallback {
private:
	btRigidBody* m_pBody;
	btPairCachingGhostObject* m_pGhostObject;

public:
	IgnoreBodyAndGhostCast(btRigidBody* pBody, btPairCachingGhostObject* pGhostObject) 
		: btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
          m_pBody(pBody), m_pGhostObject(pGhostObject) {}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) {
		if(rayResult.m_collisionObject == m_pBody || rayResult.m_collisionObject == m_pGhostObject)
			return 1.0f;

		return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
	}
};

HePhysicsDebugDrawer hePhysicsDebugDrawer;


btCollisionShape* heCreateShape(HePhysicsShapeInfo& shape) {
    btCollisionShape* bt = nullptr;

    if(shape.meshVertices.size()) {
        shape.mesh = &shape.meshVertices.data()[0].x;
        shape.floatCount = (uint32_t) shape.meshVertices.size() * 3;
    }
    
    switch(shape.type) {
    case HE_PHYSICS_SHAPE_CONVEX_MESH: {        
        btConvexHullShape* s = new btConvexHullShape();
        for(size_t i = 0; i < shape.floatCount; i += 3) {
            hm::vec3f const v = hm::vec3f(shape.mesh[i], shape.mesh[i + 1], shape.mesh[i + 2]);
            s->addPoint(btVector3(v.x, v.y, v.z), true);
        }
        
        bt = s;        
        break;
    }
        
    case HE_PHYSICS_SHAPE_CONCAVE_MESH: {
        btTriangleMesh* mesh = new btTriangleMesh();
        for(size_t i = 0; i < shape.floatCount; i += 9) {
            hm::vec3f const v0 = hm::vec3f(shape.mesh[i + 0], shape.mesh[i + 1], shape.mesh[i + 2]);
            hm::vec3f const v1 = hm::vec3f(shape.mesh[i + 3], shape.mesh[i + 4], shape.mesh[i + 5]);
            hm::vec3f const v2 = hm::vec3f(shape.mesh[i + 6], shape.mesh[i + 7], shape.mesh[i + 8]);
            mesh->addTriangle(btVector3(v0.x, v0.y, v0.z), btVector3(v1.x, v1.y, v1.z), btVector3(v2.x, v2.y, v2.z));
        }
        
        bt = new btBvhTriangleMeshShape(mesh, false);
        bt->setUserPointer(mesh);
        break;
    }
        
    case HE_PHYSICS_SHAPE_BOX: {
        bt = new btBoxShape(btVector3(shape.box.x, shape.box.y, shape.box.z));
        break;
    }
        
    case HE_PHYSICS_SHAPE_SPHERE: {
        bt = new btSphereShape(shape.sphere);
        break;
    }
        
    case HE_PHYSICS_SHAPE_CAPSULE: {
        bt = new btCapsuleShape(shape.capsule.x / 2.f, shape.capsule.y - shape.capsule.x);
        break;
    }
        
    default: {
        HE_WARNING("Unknown physics shape type: " + std::to_string(shape.type));
        break;
    }
    }

    if(shape.meshVertices.size()) {
        shape.meshVertices.clear(); // save memory
        std::vector<hm::vec3f>().swap(shape.meshVertices); // save memory
    }
    
    return bt;
};


float hePhysicsShapeGetHeight(HePhysicsShapeInfo const* info) {
    float height = 0.f;

    switch(info->type) {
    case HE_PHYSICS_SHAPE_SPHERE:
        height = info->sphere * 2.f;
        break;
        
    case HE_PHYSICS_SHAPE_BOX:
        height = info->box.y * 2.f;
        break;

    case HE_PHYSICS_SHAPE_CAPSULE:
        height = info->capsule.y;
        break;
    }
    
    return height;
};


void hePhysicsComponentCreate(HePhysicsComponent* component, HePhysicsShapeInfo& info) {
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
    component->shapeInfo = info;
    component->shapeInfo.mesh = nullptr;
};

void hePhysicsComponentDestroy(HePhysicsComponent* component) {
    delete component->body;
    component->body = nullptr;
    delete component->motion;
    component->motion = nullptr;

    if(component->shapeInfo.type == HE_PHYSICS_SHAPE_CONCAVE_MESH) {
        btTriangleMesh* mesh = (btTriangleMesh*) component->shape->getUserPointer();
        delete mesh;
    }

    delete component->shape;
    component->shape = nullptr;
};

void hePhysicsComponentEnableRotation(HePhysicsComponent const* component, hm::vec3f const& axis) {
    component->body->setAngularFactor(btVector3(axis.x, axis.y, axis.z));
};

void hePhysicsComponentSetPosition(HePhysicsComponent* component, hm::vec3f const& position) {
    btTransform transform = component->body->getWorldTransform();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    component->body->setWorldTransform(transform);
    component->motion->setWorldTransform(transform);
    component->body->forceActivationState(ACTIVE_TAG);
    component->body->activate();
};

void hePhysicsComponentSetRotation(HePhysicsComponent* component, hm::vec3f const& rotation) {
    hm::quatf quat = hm::fromEulerDegrees(rotation);
    btTransform transform = component->body->getWorldTransform();
    transform.setRotation(btQuaternion(quat.x, quat.y, quat.z, quat.w));
    component->body->setWorldTransform(transform);
    component->motion->setWorldTransform(transform);
};

void hePhysicsComponentSetTransform(HePhysicsComponent* component, HeD3Transformation const& transform) {
    btTransform bttransform = component->body->getWorldTransform();
    bttransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
    bttransform.setRotation(btQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w));
    component->shape->setLocalScaling(btVector3(transform.scale.x, transform.scale.y, transform.scale.z));
    component->body->setWorldTransform(bttransform);
    component->motion->setWorldTransform(bttransform);
};

void hePhysicsComponentSetVelocity(HePhysicsComponent* component, hm::vec3f const& velocity) {
    component->body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    component->body->activate(true);
};

hm::vec3f hePhysicsComponentGetPosition(HePhysicsComponent const* component) {
    btVector3 const& transform = component->body->getWorldTransform().getOrigin();
    return hm::vec3f(transform.getX(), transform.getY(), transform.getZ());
};

hm::quatf hePhysicsComponentGetRotation(HePhysicsComponent const* component) {
    btQuaternion const& transform = component->body->getWorldTransform().getRotation();
    return hm::quat(transform.getX(), transform.getY(), transform.getZ(), transform.getW());
};


void hePhysicsActorSimpleCreate(HePhysicsActorSimple* actor, HePhysicsShapeInfo& shapeInfo, HePhysicsActorInfo const& actorInfo) {
    actor->ghost      = new btPairCachingGhostObject();
    actor->ghost->setWorldTransform(btTransform::getIdentity());
    actor->shape      = (btConvexShape*) heCreateShape(shapeInfo);
    actor->ghost->setCollisionShape(actor->shape);
    actor->ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    actor->controller = new btKinematicCharacterController(actor->ghost, actor->shape, btScalar(actorInfo.stepHeight), btVector3(0, 1, 0));
    actor->controller->setJumpSpeed(actorInfo.jumpHeight);
    actor->shapeInfo  = shapeInfo;
    actor->actorInfo  = actorInfo;
};

void hePhysicsActorSimpleDestroy(HePhysicsActorSimple* actor) {
    delete actor->controller;
    actor->controller = nullptr;
    delete actor->shape;
    actor->shape = nullptr;
    delete actor->ghost;
    actor->ghost = nullptr;
};

void hePhysicsActorSimpleSetPosition(HePhysicsActorSimple* actor, hm::vec3f const& position) {
    if(!actor->controller)
        return;
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    actor->controller->warp(btVector3(position.x, position.y, position.z));
    actor->ghost->setWorldTransform(transform);
};

void hePhysicsActorSimpleSetEyePosition(HePhysicsActorSimple* actor, hm::vec3f const& eyePosition) {
    if(!actor->controller)
        return;
    hm::vec3f realPosition = eyePosition;
    realPosition.y = eyePosition.y - (actor->shapeInfo.capsule.y / 2.f);
    hePhysicsActorSimpleSetPosition(actor, realPosition);
};

void hePhysicsActorSimpleSetVelocity(HePhysicsActorSimple* actor, hm::vec3f const& velocity) {
    if(!actor->controller)
        return;
    actor->controller->setWalkDirection(btVector3(velocity.x, velocity.y, velocity.z));
};

void hePhysicsActorSimpleSetRotation(HePhysicsActorSimple* actor, hm::quatf const& rotation) {
    if(!actor->controller)
        return;
    actor->ghost->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
};

void hePhysicsActorSimpleJump(HePhysicsActorSimple* actor) {
    if(!actor->controller)
        return;
    actor->controller->setJumpSpeed(actor->actorInfo.jumpHeight);
    actor->controller->jump();
};

hm::vec3f hePhysicsActorSimpleGetPosition(HePhysicsActorSimple const* actor) {
    if(!actor)
        return hm::vec3f(0);
    btVector3 origin = actor->ghost->getWorldTransform().getOrigin();
    return hm::vec3f(origin.getX(), origin.getY(), origin.getZ());
};

hm::vec3f hePhysicsActorSimpleGetEyePosition(HePhysicsActorSimple const* actor) {
    if(!actor)
        return hm::vec3f(0);
    hm::vec3f position = hePhysicsActorSimpleGetPosition(actor);
    position.y += (actor->actorInfo.eyeOffset - actor->shapeInfo.capsule.y / 2.f);
    return position;
};

hm::quatf hePhysicsActorSimpleGetRotation(HePhysicsActorSimple const* actor) {
    if(!actor)
        return hm::quatf();
    btQuaternion rot = actor->ghost->getWorldTransform().getRotation();
    return hm::quatf(rot.getX(), rot.getY(), rot.getZ(), rot.getW());
};

b8 hePhysicsActorSimpleOnGround(HePhysicsActorSimple const* actor) {
    if(!actor->controller)
        return true;
    return actor->controller->onGround();
};

void hePhysicsActorSimpleSetJumpHeight(HePhysicsActorSimple* actor, float const height) {
    if(actor->controller) {
        actor->actorInfo.jumpHeight = height;
        actor->controller->setJumpSpeed(height);
    }
};


void parseGhostContacts(HePhysicsActorCustom* actor, HePhysicsLevel* level, std::vector<hm::vec3f>& surfaceHitNormals) {
    btManifoldArray manifoldArray;
    btBroadphasePairArray& pairArray = actor->ghostObject->getOverlappingPairCache()->getOverlappingPairArray();
    uint32_t numPairs = pairArray.size();

    for(uint32_t i = 0; i < numPairs; ++i) {
        manifoldArray.clear();
        btBroadphasePair const& pair = pairArray[i];
        btBroadphasePair* collisionPair = level->world->getPairCache()->findPair(pair.m_pProxy0, pair.m_pProxy1);

        if(collisionPair == nullptr)
            continue;

        if(collisionPair->m_algorithm != nullptr)
            collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

        for(uint32_t j = 0; j < manifoldArray.size(); ++j) {
            btPersistentManifold* manifold = manifoldArray[j];

            for(uint32_t p = 0; p < manifold->getNumContacts(); ++p) {
                btManifoldPoint const& point = manifold->getContactPoint(p);
                if(point.getDistance() < 0.f) {
                    btVector3 const& ptB = point.getPositionWorldOnB();

                    if(ptB.getY() < actor->position.y - actor->verticalOffset)
                        actor->onGround = true;
                    else {
                        actor->hittingWall = true;
                        surfaceHitNormals.emplace_back(hm::vec3f(point.m_normalWorldOnB.getX(), point.m_normalWorldOnB.getY(), point.m_normalWorldOnB.getZ()));
                    }
                }
            }
        }
    }
};

void updatePosition(HePhysicsActorCustom* actor, HePhysicsLevel* level) {
    IgnoreBodyAndGhostCast rayCallbackBottom(actor->rigidBody, actor->ghostObject);
    level->world->rayTest(actor->rigidBody->getWorldTransform().getOrigin(), actor->rigidBody->getWorldTransform().getOrigin() - btVector3(0, actor->verticalOffset + actor->actorInfo.stepHeight, 0), rayCallbackBottom); 

    if(rayCallbackBottom.hasHit()) {
        float prevY = actor->rigidBody->getWorldTransform().getOrigin().getY();
        actor->rigidBody->getWorldTransform().getOrigin().setY(prevY + (actor->verticalOffset + actor->actorInfo.stepHeight) * (1.f - rayCallbackBottom.m_closestHitFraction));
        btVector3 vel(actor->rigidBody->getLinearVelocity());
        vel.setY(0.f);
        actor->rigidBody->setLinearVelocity(vel);
        actor->onGround = true;
    }

    IgnoreBodyAndGhostCast rayCallbackTop(actor->rigidBody, actor->ghostObject);
    level->world->rayTest(actor->rigidBody->getWorldTransform().getOrigin(), actor->rigidBody->getWorldTransform().getOrigin() + btVector3(0, actor->verticalOffset + actor->actorInfo.stepHeight, 0), rayCallbackTop); 

    if(rayCallbackTop.hasHit()) {
        actor->rigidBody->getWorldTransform().setOrigin(btVector3(actor->previousPosition.x, actor->previousPosition.y, actor->previousPosition.z));
        btVector3 vel(actor->rigidBody->getLinearVelocity());
        vel.setY(0.f);
        actor->rigidBody->setLinearVelocity(vel);
    }

    actor->previousPosition = hm::vec3f(actor->rigidBody->getWorldTransform().getOrigin().getX(), actor->rigidBody->getWorldTransform().getOrigin().getY(), actor->rigidBody->getWorldTransform().getOrigin().getZ());
};

void hePhysicsActorCustomCreate(HePhysicsActorCustom* actor, HePhysicsShapeInfo& shapeInfo, HePhysicsActorInfo const& actorInfo) {
    actor->shape = (btConvexShape*) heCreateShape(shapeInfo);
    actor->motion = new btDefaultMotionState(btTransform::getIdentity());
    actor->ghostObject = new btPairCachingGhostObject();
    actor->ghostObject->setWorldTransform(btTransform::getIdentity());
    actor->ghostObject->setCollisionShape(actor->shape);
    actor->ghostObject->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
    actor->ghostObject->setUserPointer(actor);

    btVector3 inertia;
    actor->shape->calculateLocalInertia(shapeInfo.mass, inertia);

    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(shapeInfo.mass, actor->motion, actor->shape, inertia);
    rigidBodyCI.m_friction      = 0.f;
    rigidBodyCI.m_restitution   = 0.f;
    rigidBodyCI.m_linearDamping = 0.f;
    actor->rigidBody = new btRigidBody(rigidBodyCI);
    actor->rigidBody->setAngularFactor(0.f);
    actor->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    
    actor->verticalOffset = hePhysicsShapeGetHeight(&shapeInfo) / 2.f;
    actor->shapeInfo = shapeInfo;
    actor->actorInfo = actorInfo;
};

void hePhysicsActorCustomUpdate(HePhysicsActorCustom* actor, HePhysicsLevel* level, float const delta) {
    actor->ghostObject->setWorldTransform(actor->rigidBody->getWorldTransform());

    std::vector<hm::vec3f> surfaceHitNormals;
    parseGhostContacts(actor, level, surfaceHitNormals);
    updatePosition(actor, level);
    //updateVelocity(actor);
    
    btTransform trans = actor->rigidBody->getWorldTransform();
    actor->position = hm::vec3f(trans.getOrigin().getX(), trans.getOrigin().getY(), trans.getOrigin().getZ()); 
};

void hePhysicsActorCustomSetPosition(HePhysicsActorCustom* actor, hm::vec3f const& position) {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    actor->rigidBody->setWorldTransform(transform);
    actor->ghostObject->setWorldTransform(transform);
};


void hePhysicsLevelCreate(HePhysicsLevel* level, HePhysicsLevelInfo const& info) {
    level->config     = new btDefaultCollisionConfiguration();
    level->dispatcher = new btCollisionDispatcher(level->config);
    btVector3 worldMin(-1000, -1000, -1000);
    btVector3 worldMax(1000, 1000, 1000);
    level->broadphase = new btDbvtBroadphase();
    level->solver     = new btSequentialImpulseConstraintSolver();
    level->world      = new btDiscreteDynamicsWorld(level->dispatcher, level->broadphase, level->solver, level->config);
    level->world->setGravity(btVector3(info.gravity.x, info.gravity.y, info.gravity.z));
    level->world->getDispatchInfo().m_allowedCcdPenetration = 0.01f;
    level->info = info;
    level->setup = true;
};

void hePhysicsLevelDestroy(HePhysicsLevel* level) {
    for(HePhysicsComponent& all : level->components) {
        level->world->removeRigidBody(all.body);
        hePhysicsComponentDestroy(&all);
    }

    if(level->actor) {
        level->world->removeCollisionObject(level->actor->ghost);
        level->world->removeAction(level->actor->controller);
        hePhysicsActorSimpleDestroy(level->actor);
        delete level->ghostPair;
        level->ghostPair = nullptr;
    }
    
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

void hePhysicsLevelAddComponent(HePhysicsLevel* level, HePhysicsComponent const* component) {
    level->world->addRigidBody(component->body);
};

void hePhysicsLevelRemoveComponent(HePhysicsLevel* level, HePhysicsComponent const* component) {
    level->world->removeRigidBody(component->body);
};

void hePhysicsLevelSetActor(HePhysicsLevel* level, HePhysicsActorSimple* actor) {
    if(actor == nullptr) {
        // remove current actor
        level->world->removeCollisionObject(level->actor->ghost);
        level->world->removeAction(level->actor->controller);
        level->actor = nullptr;
    } else {
        if(!level->ghostPair) {
            level->ghostPair = new btGhostPairCallback();
            level->broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(level->ghostPair);
        }
        
        level->world->addCollisionObject(actor->ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
        level->world->addAction(actor->controller);
        level->actor = actor;
        level->actor->controller->setGravity(btVector3(level->info.gravity.x, level->info.gravity.y, level->info.gravity.z));
    }
};

void hePhysicsLevelSetActor(HePhysicsLevel* level, HePhysicsActorCustom* actor) {
    if(actor == nullptr) {
        // remove current actor
        //level->world->removeCollisionObject(level->actor->ghostObject);
        //level->actor = nullptr;
    } else {
        if(!level->ghostPair) {
            level->ghostPair = new btGhostPairCallback();
            level->broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(level->ghostPair);
        }
        
        level->world->addCollisionObject(actor->ghostObject, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
        level->world->addRigidBody(actor->rigidBody);
        //level->world->addAction(actor->controller);
        //level->actor = actor;
        //level->actor->controller->setGravity(btVector3(level->info.gravity.x, level->info.gravity.y, level->info.gravity.z));
    }
};

void hePhysicsLevelUpdate(HePhysicsLevel* level, float const delta) {
    level->world->stepSimulation(delta, 10);
    
    if(level->actor) {
        hm::vec3f prev = level->actor->position;
        level->actor->position        = hePhysicsActorSimpleGetPosition(level->actor);
        level->actor->feetPosition    = level->actor->position;
        level->actor->feetPosition.y -= hePhysicsShapeGetHeight(&level->actor->shapeInfo) / 2.f;
        level->actor->velocity        = level->actor->position - prev;
        level->actor->rotation        = hePhysicsActorSimpleGetRotation(level->actor);
    }
};

void hePhysicsEnableDebugDraw(HeRenderEngine* engine) {
    hePhysicsDebugDrawer.setDebugMode(btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawWireframe);
    hePhysicsDebugDrawer.engine = engine;
};

void hePhysicsDisableDebugDraw() {
    hePhysicsDebugDrawer.setDebugMode(0);
};

b8 hePhysicsDebugDrawingEnabled() {
    return hePhysicsDebugDrawer.debugMode != 0;
};

void hePhysicsLevelDebugDraw(HePhysicsLevel* level) {
    if(hePhysicsDebugDrawer.debugMode != 0) {
        level->world->setDebugDrawer(&hePhysicsDebugDrawer);
        level->enableDebugDraw = true;
    } else {
        level->world->setDebugDrawer(nullptr);
        level->enableDebugDraw = false;
    }

    if(level->enableDebugDraw)
        level->world->debugDrawWorld();
};

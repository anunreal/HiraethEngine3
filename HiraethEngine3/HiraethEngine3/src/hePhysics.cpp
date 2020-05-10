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

btCollisionShape* heCreateShape(HePhysicsShapeInfo const& shape) {
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
		bt = new btCapsuleShape(shape.capsule.x / 2.f, shape.capsule.y - shape.capsule.x);
		break;
		
	default:
		HE_WARNING("Unknown physics shape type: " + std::to_string(shape.type));
		break;
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


void hePhysicsLevelCreate(HePhysicsLevel* level, HePhysicsLevelInfo const& info) {
	level->config	  = new btDefaultCollisionConfiguration();
	level->dispatcher = new btCollisionDispatcher(level->config);
	btVector3 worldMin(-1000, -1000, -1000);
	btVector3 worldMax(1000, 1000, 1000);
	level->broadphase = new btDbvtBroadphase();
	level->solver	  = new btSequentialImpulseConstraintSolver();
	level->world	  = new btDiscreteDynamicsWorld(level->dispatcher, level->broadphase, level->solver, level->config);
	level->world->setGravity(btVector3(info.gravity.x, info.gravity.y, info.gravity.z));
	level->world->getDispatchInfo().m_allowedCcdPenetration = 0.0001f;
	level->info = info;
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
		level->actor->controller->setGravity(btVector3(level->info.gravity.x, level->info.gravity.y, level->info.gravity.z));
	}
};

void hePhysicsLevelUpdate(HePhysicsLevel* level, const float delta) {
	level->world->stepSimulation(delta, 10);

	if(level->actor) {
		hm::vec3f prev = level->actor->position;
		level->actor->position        = hePhysicsActorGetPosition(level->actor);
		level->actor->feetPosition    = level->actor->position;
		level->actor->feetPosition.y -= hePhysicsShapeGetHeight(&level->actor->shapeInfo) / 2.f;
		level->actor->velocity        = level->actor->position - prev;
	}
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
	if(level->enableDebugDraw)
		level->world->debugDrawWorld();
};

b8 hePhysicsLevelIsSetup(HePhysicsLevel const* level) { return level->world != nullptr; };


void hePhysicsComponentCreate(HePhysicsComponent* component, HePhysicsShapeInfo const& info) {
	component->shape = heCreateShape(info);
	
	// create rigid body
	btScalar mass(info.mass);
	bool dynamic = (mass != 0.f);
	btVector3 localInertia(0, 0, 0);
	if(dynamic)
		component->shape->calculateLocalInertia(mass, localInertia);
	
	component->motion	 = new btDefaultMotionState();
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, component->motion, component->shape, localInertia);
	rbInfo.m_friction	 = info.friction;
	rbInfo.m_restitution = info.restitution;
	component->body		 = new btRigidBody(rbInfo);
	component->shapeInfo = info;
	component->shapeInfo.mesh.clear(); // save memory
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


void hePhysicsActorCreate(HePhysicsActor* actor, HePhysicsShapeInfo const& shapeInfo, HePhysicsActorInfo const& actorInfo) {
	actor->ghost	  = new btPairCachingGhostObject();
	actor->ghost->setWorldTransform(btTransform::getIdentity());
	actor->shape	  = (btConvexShape*) heCreateShape(shapeInfo);
	actor->ghost->setCollisionShape(actor->shape);
	actor->ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
	actor->controller = new btKinematicCharacterController(actor->ghost, actor->shape, btScalar(actorInfo.stepHeight), btVector3(0, 1, 0));
	actor->controller->setJumpSpeed(actorInfo.jumpHeight);
	actor->shapeInfo  = shapeInfo;
	actor->shapeInfo.mesh.clear(); // save memory
	actor->actorInfo  = actorInfo;
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
	if(!actor->controller)
		return;
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(position.x, position.y, position.z));
	actor->controller->warp(btVector3(position.x, position.y, position.z));
	actor->ghost->setWorldTransform(transform);
};

void hePhysicsActorSetEyePosition(HePhysicsActor* actor, hm::vec3f const& eyePosition) {
	if(!actor->controller)
		return;
	hm::vec3f realPosition = eyePosition;
	realPosition.y -= eyePosition.y - (actor->shapeInfo.capsule.y / 2.f);
	hePhysicsActorSetPosition(actor, realPosition);
};

void hePhysicsActorSetVelocity(HePhysicsActor* actor, hm::vec3f const& velocity) {
	if(!actor->controller)
		return;
	actor->controller->setWalkDirection(btVector3(velocity.x, velocity.y, velocity.z));
};

void hePhysicsActorJump(HePhysicsActor* actor) {
	if(!actor->controller)
		return;
	actor->controller->setJumpSpeed(actor->actorInfo.jumpHeight);
	actor->controller->jump();
};

hm::vec3f hePhysicsActorGetPosition(HePhysicsActor const* actor) {
	if(!actor->controller)
		return hm::vec3f(0);
	btVector3 origin = actor->ghost->getWorldTransform().getOrigin();
	return hm::vec3f(origin.getX(), origin.getY(), origin.getZ());
};

hm::vec3f hePhysicsActorGetEyePosition(HePhysicsActor const* actor) {
	if(!actor->controller)
		return hm::vec3f(0);
	hm::vec3f position = hePhysicsActorGetPosition(actor);
	position.y += (actor->actorInfo.eyeOffset - actor->shapeInfo.capsule.y / 2.f);
	return position;
};

b8 hePhysicsActorOnGround(HePhysicsActor const* actor) {
	if(!actor->controller)
		return true;
	return actor->controller->onGround();
};

void hePhysicsActorSetJumpHeight(HePhysicsActor* actor, float const height) {
	if(actor->controller) {
		actor->actorInfo.jumpHeight = height;
		actor->controller->setJumpSpeed(height);
	}
};

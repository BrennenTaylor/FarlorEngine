#include "PhysicsSystem.h"

#include "../ECS/TransformManager.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseInterface.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btConcaveShape.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btDefaultMotionState.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"

#include <btBulletDynamicsCommon.h>

#include <memory>

namespace Farlor {
extern TransformManager g_TransformManager;

PhysicsSystem::PhysicsSystem()
    : m_heightfieldData(1024 * 1024, 0.0f)
    , m_components {}
    , m_collisionPlanes {}
    , m_collisions {}
    , m_gravity { 0.0f, -9.81f, 0.0f }
{
    upDefaultCollisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    upCollisionDispatcher = std::make_unique<btCollisionDispatcher>(upDefaultCollisionConfig.get());
    upOverlappingPairCache = std::make_unique<btDbvtBroadphase>();
    upSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    upDynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(upCollisionDispatcher.get(),
          upOverlappingPairCache.get(), upSolver.get(), upDefaultCollisionConfig.get());
    upDynamicsWorld->setGravity(btVector3(0.0f, -9.81f, 0.0f));


    float minHeight = 0.0f;
    float maxHeight = 500.0f;
    upTerrainShape
          = std::make_unique<btHeightfieldTerrainShape>(1024, 1024, m_heightfieldData.data(), 1.0f,
                minHeight, maxHeight, 1, PHY_ScalarType::PHY_FLOAT, false);

    btTransform terrainTransform;
    terrainTransform.setIdentity();
    terrainTransform.setOrigin(btVector3(0.0f, (maxHeight - minHeight) / 2.0f, 0.0f));

    upTerrainCollisionObject = std::make_unique<btCollisionObject>();
    upTerrainCollisionObject->setWorldTransform(terrainTransform);
    upTerrainCollisionObject->setCollisionShape(upTerrainShape.get());
    upDynamicsWorld->addCollisionObject(upTerrainCollisionObject.get());

    // Add ball
    // upBallCollisionShape = std::make_unique<btSphereShape>(1.0f);
    // btTransform ballTransform;
    // ballTransform.setIdentity();
    // ballTransform.setOrigin(btVector3(0.0f, 10.0f, 0.0f));

    // btVector3 localInertia(0.0f, 0.0f, 0.0f);
    // upBallCollisionShape->calculateLocalInertia(1.0f, localInertia);

    // upBallMotionState = std::make_unique<btDefaultMotionState>(ballTransform);
    // btRigidBody::btRigidBodyConstructionInfo ballRBInfo(
    //       1.0f, upBallMotionState.get(), upBallCollisionShape.get());
    // upBallRigidBody = std::make_unique<btRigidBody>(ballRBInfo);
    // upDynamicsWorld->addRigidBody(upBallRigidBody.get());
}

PhysicsSystem::~PhysicsSystem()
{
    upDynamicsWorld->removeCollisionObject(upTerrainCollisionObject.get());
}

void PhysicsSystem::Tick()
{
    // We want a consistant physics update
    const float stepSize = 1.0f / 60.0f;
    const int maxSubsteps = 10;
    upDynamicsWorld->stepSimulation(stepSize, maxSubsteps);

    // // print positions of all objects
    // for (int j = upDynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
    //     btCollisionObject *obj = upDynamicsWorld->getCollisionObjectArray()[j];
    //     btRigidBody *body = btRigidBody::upcast(obj);
    //     btTransform trans;
    //     if (body && body->getMotionState()) {
    //         body->getMotionState()->getWorldTransform(trans);
    //     } else {
    //         trans = obj->getWorldTransform();
    //     }
    //     printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()),
    //           float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
    // }
}

void PhysicsSystem::AddComponent(PhysicsComponent component) { m_components.push_back(component); }

void PhysicsSystem::AddCollisionPlane(Plane &&plane) { m_collisionPlanes.push_back(plane); }

void PhysicsSystem::Reset() { m_components.clear(); }

void PhysicsSystem::CalculateAcceleration() { }

void PhysicsSystem::ApplyIntegration(float dt) { }

bool PhysicsSystem::CheckPlaneCollisions() { return false; }

void PhysicsSystem::SetNewValues() { }

void PhysicsSystem::HandleCollision(PlaneCollision &planeCollision) { }
}

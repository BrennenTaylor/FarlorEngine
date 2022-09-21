#pragma once

#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "LinearMath/btDefaultMotionState.h"
#include "LinearMath/btMotionState.h"
#include "LinearMath/btVector3.h"
#include "PhysicsComponent.h"
#include "Plane.h"
#include "PlaneCollision.h"

#include "BulletCollision/BroadphaseCollision/btBroadphaseInterface.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"

#include <btBulletDynamicsCommon.h>

#include <vector>

namespace Farlor {
class PhysicsSystem {
   public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Tick();

    void AddComponent(PhysicsComponent component);

    void AddCollisionPlane(Plane &&plane);

    void Reset();

    void UpdateTerrainValues(const std::vector<float> &terrainValues)
    {
        if (terrainValues.size() != m_heightfieldData.size()) {
            std::cout << "Error: incorrect terrain size durring update" << std::endl;
            return;
        }

        upDynamicsWorld->removeCollisionObject(upTerrainCollisionObject.get());
        upTerrainCollisionObject->setCollisionShape(nullptr);

        for (int i = 0; i < terrainValues.size(); i++) {
            m_heightfieldData[i] = terrainValues[i];
        }
        upTerrainCollisionObject->setCollisionShape(upTerrainShape.get());
        upDynamicsWorld->addCollisionObject(upTerrainCollisionObject.get());
    }

    bool CastRay(const Farlor::Vector3 &from, const Farlor::Vector3 &dir, const float checkDistance,
          Farlor::Vector3 &result) const
    {
        btVector3 btFrom(from.x, from.y, from.z);
        btVector3 btDir(dir.x, dir.y, dir.z);
        btVector3 btTo = btFrom + btDir * checkDistance;
        btCollisionWorld::ClosestRayResultCallback res(btFrom, btTo);
        upDynamicsWorld->rayTest(btFrom, btTo, res);

        if (res.hasHit()) {
            result.x = res.m_hitPointWorld.x();
            result.y = res.m_hitPointWorld.y();
            result.z = res.m_hitPointWorld.z();
            return true;
        }
        return false;
    }

   private:
    void CalculateAcceleration();
    void ApplyIntegration(float deltaTime);
    bool CheckPlaneCollisions();
    void SetNewValues();
    void HandleCollision(PlaneCollision &PlaneCollision);

   private:
    std::vector<float> m_heightfieldData;


    std::unique_ptr<btDefaultCollisionConfiguration> upDefaultCollisionConfig = nullptr;
    std::unique_ptr<btCollisionDispatcher> upCollisionDispatcher = nullptr;
    std::unique_ptr<btBroadphaseInterface> upOverlappingPairCache = nullptr;
    std::unique_ptr<btSequentialImpulseConstraintSolver> upSolver = nullptr;
    std::unique_ptr<btDiscreteDynamicsWorld> upDynamicsWorld = nullptr;

    std::unique_ptr<btHeightfieldTerrainShape> upTerrainShape = nullptr;
    std::unique_ptr<btCollisionObject> upTerrainCollisionObject = nullptr;
    std::unique_ptr<btDefaultMotionState> upTerrainMotionState = nullptr;

    std::unique_ptr<btCollisionShape> upBallCollisionShape = nullptr;
    std::unique_ptr<btRigidBody> upBallRigidBody = nullptr;
    std::unique_ptr<btDefaultMotionState> upBallMotionState = nullptr;

    std::vector<PhysicsComponent> m_components;
    std::vector<Plane> m_collisionPlanes;
    std::vector<PlaneCollision> m_collisions;
    Vector3 m_gravity;
};
}

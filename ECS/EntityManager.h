#pragma once

#include "Entity.h"
#include "Component.h"

#include "TransformComponent.h"

#include <vector>
#include <map>
#include <queue>

namespace Farlor {
class ECSManager {
   public:
    static const uint32_t MAX_NUM_COMPONENTS;

    ECSManager();

    Entity CreateEntity();
    void FreeEntity(Entity entity);

   private:
    uint32_t m_createId;

    std::vector<Entity> m_entities;
    std::queue<uint32_t> m_freeEntities;

    std::vector<Entity> m_transformComponents;
};
}

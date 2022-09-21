#pragma once

#include "TransformComponent.h"

#include "Entity.h"

#include <unordered_map>

namespace Farlor {
class TransformManager {
   public:
    TransformComponent *RegisterEntity(Entity entity);
    TransformComponent *RegisterEntity(Entity entity, TransformComponent component);

    // Access the transform component or nullptr if it doesnt exist
    TransformComponent *GetComponent(Entity entity);

    void SetComponent(TransformComponent component, Entity entity);

   public:
    std::unordered_map<Entity, TransformComponent> m_components;
};
}

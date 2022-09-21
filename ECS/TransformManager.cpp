#include "TransformManager.h"

namespace Farlor {
TransformComponent *TransformManager::RegisterEntity(Entity entity)
{
    TransformComponent newComponent;
    newComponent.m_entity = entity;
    m_components.emplace(entity, newComponent);
    return &m_components[entity];
}

TransformComponent *TransformManager::RegisterEntity(Entity entity, TransformComponent component)
{
    component.m_entity = entity;
    m_components.emplace(entity, component);
    return &m_components[entity];
}

TransformComponent *TransformManager::GetComponent(Entity entity)
{
    auto entry = m_components.find(entity);
    if (entry == m_components.end()) {
        return nullptr;
    }
    return &entry->second;
}
}

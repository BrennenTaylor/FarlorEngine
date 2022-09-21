#pragma once

#include "../ECS/Entity.h"
#include <FMath/FMath.h>

#include <string>

namespace Farlor {
struct LightComponent {
    Entity m_entity;

    Vector3 m_color;
    Vector3 m_direction;
    Vector4 m_lightRange;
};
}

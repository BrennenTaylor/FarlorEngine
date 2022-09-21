#pragma once

#include <cstdint>

namespace Farlor
{
    enum class WindowMouseButtons : uint32_t
    {
        Left = 0u,
        Right = 1u,
        Center = 2u
    };

    enum class WindowMouseButtonState : uint32_t
    {
        Down = 0u,
        Up = 1u
    };
}
#pragma once

#include <cstdint>

namespace Farlor
{
    // These are indices indexing into the input state for the mouse
    namespace MouseButtons
    {
        const uint32_t Left = 0;
        const uint32_t Right = Left + 1;
        const uint32_t Center = Right + 1;
        const uint32_t NumMouseButtons = Center + 1;
    }
}
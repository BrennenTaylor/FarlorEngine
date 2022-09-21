#pragma once

#include <cstdint>

namespace Farlor {

/*
To n-byte align a memory address x, mask off log2n(n) lsb of x
Masking bits returns first n-byte aligned address before x
So to find after, mask (x + (alignment - 1))
*/

inline void* AlignForward(void* pAddress, uint8_t alignment)
{
    return (void*)((reinterpret_cast<uintptr_t>(pAddress)
        + static_cast<uintptr_t>(alignment - 1))
        & static_cast<uintptr_t>(~(alignment - 1)));
}

// How much alignment is needed?
inline uint8_t AlignForwardAdjustment(const void* pAddress, uint8_t alignment)
{
    uint8_t adjustment = alignment - (reinterpret_cast<uintptr_t>(pAddress)
        & static_cast<uintptr_t>(alignment - 1));
    
    if(adjustment == alignment)
        return 0;

    return adjustment;
}

// Store a header before each allocation, use adjustment
// space to reduce memory overhead
inline uint8_t AlignForwardAdjustmentWithHeader(const void* pAddress,
    uint8_t alignment, uint8_t headerSize)
{
    uint8_t adjustment = AlignForwardAdjustment(pAddress, alignment);
    uint8_t neededSpace = headerSize;
    
    if(adjustment < neededSpace)
    {
        neededSpace -= adjustment;
        adjustment += adjustment * (neededSpace / alignment);

        if(neededSpace % alignment > 0)
            adjustment += alignment;
    }

    return adjustment;
}


}
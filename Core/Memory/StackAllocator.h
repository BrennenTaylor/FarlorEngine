#pragma once

#include "BaseAllocator.h"

#include <cstdint>

namespace Farlor {

class StackAllocator : public Allocator
{
    StackAllocator(size_t size, void* pStart);
    ~StackAllocator();

    virtual void* Allocate(size_t size, uint8_t alignment) override;
    virtual void Deallocate(void* pMemory) override;

private:
    StackAllocator(const StackAllocator&); // Prevent copies
    StackAllocator& operator=(const StackAllocator&);

    struct StackAllocationHeader
    {
        #if _DEBUG
        void* pPrevAddress;
        #endif
        uint8_t adjustment;
    };

    #if _DEBUG
    void* m_pPrevPosition;
    #endif

    void* m_pCurrentPosition;
};

}
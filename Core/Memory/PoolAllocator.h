#pragma once

#include "BaseAllocator.h"

namespace Farlor {

class PoolAllocator : public Allocator
{
public:
    PoolAllocator(size_t objectSize, uint8_t objectAlignment, size_t size, void* pMemory);
    ~PoolAllocator();

    virtual void* Allocate(size_t size, uint8_t alignment) override;
    virtual void Deallocate(void* pMemory) override;

private:
    PoolAllocator(const PoolAllocator&); //Prevent copies because it might cause errors
    PoolAllocator& operator=(const PoolAllocator&);

    size_t m_objectSize;
    uint8_t m_objectAlignment;

    void** m_ppFreeList;
};

}
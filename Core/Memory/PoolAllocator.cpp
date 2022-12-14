#include "PoolAllocator.h"

#include <cassert>

namespace Farlor {

PoolAllocator::PoolAllocator(size_t objectSize, uint8_t objectAlignment, size_t size, void* pMemory)
    : Allocator(size, pMemory)
    , m_objectSize(objectSize)
    , m_objectAlignment(objectAlignment)
{
    assert(objectSize >= sizeof(void*));

    uint8_t adjustment = AlignForwardAdjustment(pMemory, objectAlignment);
    
    // Where does the free list start?
    m_ppFreeList = (void**)((uintptr_t)pMemory + adjustment);

    // Number of objects we can store
    size_t numObjects = (size - adjustment) / objectSize;


    void** p = m_ppFreeList;
    for(size_t i = 0; i < numObjects - 1; i++)
    {
        // Initialize every "block" to be a pointer pointing to the next one
        *p = (void*)((uintptr_t)p + objectSize);
        p = (void**)*p;
    }

    // The last block points to a nullptr
    *p = nullptr;
}

PoolAllocator::~PoolAllocator()
{
    // Set the free list to not point to anything
    m_ppFreeList = nullptr;
}

void* PoolAllocator::Allocate(size_t size, uint8_t alignment)
{
    // Make sure object is correct size
    assert(size == m_objectSize && alignment == m_objectAlignment);
    
    // If at end, return
    if (m_ppFreeList == nullptr)
        return nullptr;

    // Get pointer to free block
    void* freeBlock = m_ppFreeList;

    // Move free block to next free block
    m_ppFreeList = (void**)(*m_ppFreeList);
    m_usedMemory += size;
    m_numAllocations++;

    return freeBlock;
}

void PoolAllocator::Deallocate(void* pMemory)
{
    // Set that memory block to point to the "next" free block of memory
    *((void**)pMemory) = m_ppFreeList;
    // The free list now points to the freed block of memory
    m_ppFreeList = (void**)pMemory;
    m_usedMemory -= m_objectSize;
    m_numAllocations--;
}

}
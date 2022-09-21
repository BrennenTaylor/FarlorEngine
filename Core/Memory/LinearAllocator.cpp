#include "LinearAllocator.h"

#include <cassert>

namespace Farlor {

LinearAllocator::LinearAllocator(size_t size, void* pStart)
    : Allocator(size, pStart)
    , m_pCurrentPosition(pStart)
{
    assert(size > 0);    
}

LinearAllocator::~LinearAllocator()
{
    m_pCurrentPosition = nullptr;
}

void* LinearAllocator::Allocate(size_t size, uint8_t alignment)
{
    assert(size != 0);
    uint8_t adjustment = AlignForwardAdjustment(m_pCurrentPosition, alignment);
    if(m_usedMemory + adjustment + size > m_size)
    {
        return nullptr;
    }

    uintptr_t pAlignedAddress = (uintptr_t)m_pCurrentPosition + adjustment;
    m_pCurrentPosition = (void*)(pAlignedAddress + size);
    m_usedMemory += size + adjustment;
    m_numAllocations++;
    return (void*) pAlignedAddress;
}

void LinearAllocator::Deallocate(void* pMemory)
{
    assert(false && "Use clear() instead");
}

void LinearAllocator::Clear()
{
    m_numAllocations = 0;
    m_usedMemory = 0;
    m_pCurrentPosition = m_pStart;
}

}
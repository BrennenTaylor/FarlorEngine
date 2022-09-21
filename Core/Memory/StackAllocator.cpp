#include "StackAllocator.h"

#include <cassert>

namespace Farlor {

StackAllocator::StackAllocator(size_t size, void* pStart)
    : Allocator(size, pStart)
    , m_pCurrentPosition(pStart)
{
    assert(size > 0);
    #if _DEBUG
    m_pPrevPosition = nullptr;
    #endif
}

StackAllocator::~StackAllocator()
{
    #if _DEBUG
    m_pPrevPosition = nullptr;
    #endif

    m_pCurrentPosition = nullptr;
}

void* StackAllocator::Allocate(size_t size, uint8_t alignment)
{
    assert(size != 0);

    uint8_t adjustment = AlignForwardAdjustmentWithHeader(m_pCurrentPosition, alignment, sizeof(StackAllocationHeader));
    if (m_usedMemory + adjustment + size > m_size)
        return nullptr;

    void* pAlignedAddress = (void*)((uintptr_t)m_pCurrentPosition + adjustment);

    StackAllocationHeader* header = (StackAllocationHeader*)((uintptr_t)pAlignedAddress - sizeof(StackAllocationHeader));
    header->adjustment = adjustment;
    #if _DEBUG
    header->pPrevAddress = m_pPrevPosition;
    m_pPrevPosition = pAlignedAddress;
    #endif

    m_pCurrentPosition = (void*)((uintptr_t)pAlignedAddress + size);
    m_usedMemory += size + adjustment;
    m_numAllocations++;
    return pAlignedAddress;
}

void StackAllocator::Deallocate(void* pMemory)
{
    assert(pMemory == m_pPrevPosition);

    StackAllocationHeader* header = (StackAllocationHeader*)((uintptr_t)pMemory - sizeof(StackAllocationHeader));
    m_usedMemory -= (uintptr_t)m_pCurrentPosition - (uintptr_t)pMemory + header->adjustment;
    m_pCurrentPosition = (void*)((uintptr_t)pMemory - header->adjustment);

    #if _DEBUG
    m_pPrevPosition = header->pPrevAddress;
    #endif

    m_numAllocations--;
}

}